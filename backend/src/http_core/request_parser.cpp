#include "http_core/request_parser.hpp"

std::optional<boost::json::object> request_parser::parse_json_object(
    const request_type& request
){
    boost::system::error_code ec;
    auto request_value = boost::json::parse(request.body(), ec);
    if(ec || !request_value.is_object()){
        return std::nullopt;
    }

    return std::move(request_value.as_object());
}

std::optional<std::vector<std::string_view>> request_parser::parse_path(
    std::string_view prefix,
    std::string_view path
){
    if(!path.starts_with(prefix)){
        return std::nullopt;
    }

    path.remove_prefix(prefix.size());
    if(!path.empty() && path.front() != '/'){
        return std::nullopt;
    }
    if(path.starts_with("/")){
        path.remove_prefix(1);
    }

    std::vector<std::string_view> path_segments;
    while(!path.empty()){
        const std::size_t slash_position = path.find('/');
        if(slash_position == std::string_view::npos){
            path_segments.push_back(path);
            break;
        }

        path_segments.push_back(path.substr(0, slash_position));
        path.remove_prefix(slash_position + 1);
    }

    return path_segments;
}

std::string_view request_parser::get_target_path(std::string_view target){
    const auto query_position = target.find('?');
    if(query_position == std::string_view::npos){
        return target;
    }

    return target.substr(0, query_position);
}

std::optional<std::string_view> request_parser::get_target_query(std::string_view target){
    const auto query_position = target.find('?');
    if(query_position == std::string_view::npos){
        return std::nullopt;
    }

    return target.substr(query_position + 1);
}

std::optional<std::vector<request_parser::query_param>> request_parser::parse_query_params(
    std::string_view query
){
    std::vector<query_param> query_params;
    if(query.empty()){
        return query_params;
    }

    while(true){
        const auto ampersand_position = query.find('&');
        const auto query_segment =
            ampersand_position == std::string_view::npos
                ? query
                : query.substr(0, ampersand_position);

        const auto equal_position = query_segment.find('=');
        if(
            query_segment.empty() ||
            equal_position == std::string_view::npos ||
            equal_position == 0 ||
            equal_position + 1 >= query_segment.size()
        ){
            return std::nullopt;
        }

        query_params.push_back(query_param{
            .key = query_segment.substr(0, equal_position),
            .value = query_segment.substr(equal_position + 1)
        });

        if(ampersand_position == std::string_view::npos){
            break;
        }

        query.remove_prefix(ampersand_position + 1);
    }

    return query_params;
}
