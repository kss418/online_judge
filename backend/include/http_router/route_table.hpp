#pragma once

#include "common/string_util.hpp"
#include "http_core/http_response_util.hpp"
#include "http_core/request_parser.hpp"
#include "http_router/operation_kind.hpp"

#include <boost/beast/http/verb.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace http_route{
    enum class path_segment_kind{
        literal,
        positive_int64,
        positive_int32,
        percent_decoded_string
    };

    struct path_segment_matcher{
        path_segment_kind kind;
        std::string_view value;

        static constexpr path_segment_matcher literal(std::string_view literal_value){
            return path_segment_matcher{
                .kind = path_segment_kind::literal,
                .value = literal_value
            };
        }

        static constexpr path_segment_matcher positive_int64(std::string_view param_name){
            return path_segment_matcher{
                .kind = path_segment_kind::positive_int64,
                .value = param_name
            };
        }

        static constexpr path_segment_matcher positive_int32(std::string_view param_name){
            return path_segment_matcher{
                .kind = path_segment_kind::positive_int32,
                .value = param_name
            };
        }

        static constexpr path_segment_matcher percent_decoded_string(
            std::string_view param_name
        ){
            return path_segment_matcher{
                .kind = path_segment_kind::percent_decoded_string,
                .value = param_name
            };
        }
    };

    inline constexpr std::array<path_segment_matcher, 0> empty_path_pattern{};

    struct route_param{
        using value_type = std::variant<std::int64_t, std::int32_t, std::string>;

        std::string_view name;
        value_type value;
    };

    class route_match{
    public:
        void clear(){
            params_.clear();
        }

        template <typename value_type>
        void add_param(std::string_view name, value_type&& value){
            params_.push_back(route_param{
                .name = name,
                .value = route_param::value_type{
                    std::forward<value_type>(value)
                }
            });
        }

        std::int64_t int64_param(std::string_view name) const{
            return std::get<std::int64_t>(find_param_or_throw(name).value);
        }

        std::int32_t int32_param(std::string_view name) const{
            return std::get<std::int32_t>(find_param_or_throw(name).value);
        }

        const std::string& string_param(std::string_view name) const{
            return std::get<std::string>(find_param_or_throw(name).value);
        }

    private:
        const route_param& find_param_or_throw(std::string_view name) const{
            for(const auto& param_value : params_){
                if(param_value.name == name){
                    return param_value;
                }
            }

            throw std::out_of_range("route param not found");
        }

        std::vector<route_param> params_;
    };

    inline bool try_match_path_pattern(
        std::span<const path_segment_matcher> pattern,
        std::span<const std::string_view> path_segments,
        route_match& route_match_value
    ){
        if(pattern.size() != path_segments.size()){
            return false;
        }

        route_match_value.clear();
        for(std::size_t segment_index = 0; segment_index < pattern.size(); ++segment_index){
            const auto& matcher = pattern[segment_index];
            const auto segment = path_segments[segment_index];

            if(matcher.kind == path_segment_kind::literal){
                if(segment != matcher.value){
                    return false;
                }

                continue;
            }

            if(matcher.kind == path_segment_kind::positive_int64){
                const auto int64_value_opt =
                    string_util::parse_positive_int64(segment);
                if(!int64_value_opt){
                    return false;
                }

                route_match_value.add_param(matcher.value, *int64_value_opt);
                continue;
            }

            if(matcher.kind == path_segment_kind::positive_int32){
                const auto int32_value_opt =
                    string_util::parse_positive_int32(segment);
                if(!int32_value_opt){
                    return false;
                }

                route_match_value.add_param(matcher.value, *int32_value_opt);
                continue;
            }

            const auto decoded_string_opt =
                string_util::decode_percent_encoded(segment);
            if(!decoded_string_opt || decoded_string_opt->empty()){
                return false;
            }

            route_match_value.add_param(
                matcher.value,
                std::move(*decoded_string_opt)
            );
        }

        return true;
    }

    template <typename context_type, typename response_type>
    struct endpoint_descriptor{
        using invoke_type = response_type (*)(context_type& context, const route_match&);

        std::string_view name;
        boost::beast::http::verb method;
        http_route::operation_kind kind;
        std::span<const path_segment_matcher> pattern;
        invoke_type invoke;
    };

    template <typename context_type, typename response_type>
    response_type dispatch_route_table(
        context_type& context,
        std::string_view path,
        std::span<const endpoint_descriptor<context_type, response_type>>
            endpoint_descriptors
    ){
        const auto path_segments_opt = request_parser::parse_path("", path);
        if(!path_segments_opt){
            return http_response_util::create_not_found(context.request);
        }

        context.clear_matched_endpoint();
        bool matched_path = false;
        for(const auto& endpoint_descriptor_value : endpoint_descriptors){
            route_match route_match_value;
            if(!try_match_path_pattern(
                endpoint_descriptor_value.pattern,
                *path_segments_opt,
                route_match_value
            )){
                continue;
            }

            matched_path = true;
            if(context.request.method() != endpoint_descriptor_value.method){
                continue;
            }

            context.set_matched_endpoint(http_route::endpoint_metadata{
                .name = endpoint_descriptor_value.name,
                .kind = endpoint_descriptor_value.kind
            });
            return endpoint_descriptor_value.invoke(
                context,
                route_match_value
            );
        }

        if(matched_path){
            return http_response_util::create_method_not_allowed(context.request);
        }

        return http_response_util::create_not_found(context.request);
    }

    template <typename context_type, typename response_type, std::size_t endpoint_count>
    response_type dispatch_route_table(
        context_type& context,
        std::string_view path,
        const std::array<
            endpoint_descriptor<context_type, response_type>,
            endpoint_count
        >& endpoint_descriptors
    ){
        return dispatch_route_table(
            context,
            path,
            std::span<const endpoint_descriptor<context_type, response_type>>{
                endpoint_descriptors
            }
        );
    }
}
