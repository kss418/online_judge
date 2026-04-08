#pragma once

#include "common/language_util.hpp"
#include "error/io_error.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

class workspace_layout{
public:
    static std::expected<workspace_layout, io_error> create(
        std::filesystem::path host_root_path
    ){
        if(host_root_path.empty()){
            return std::unexpected(
                io_error{
                    io_error_code::invalid_argument,
                    "workspace root path is empty"
                }
            );
        }

        return workspace_layout(std::move(host_root_path));
    }

    const std::filesystem::path& host_root_path() const noexcept{
        return host_root_path_;
    }

    static std::filesystem::path sandbox_root_path(){
        return "/workspace";
    }

    std::filesystem::path sandbox_root() const{
        return sandbox_root_path();
    }

    std::expected<std::filesystem::path, io_error> source_path_for(
        std::string_view language
    ) const{
        const auto supported_language_opt =
            language_util::find_supported_language(language);
        if(!supported_language_opt){
            return std::unexpected(
                io_error{
                    io_error_code::invalid_argument,
                    "unsupported language"
                }
            );
        }

        if(supported_language_opt->language == "java"){
            return host_root_path_ / "Main.java";
        }

        return host_root_path_ /
               ("main" + std::string(supported_language_opt->source_extension));
    }

    std::expected<std::filesystem::path, io_error> sandbox_path_for(
        const std::filesystem::path& host_path
    ) const{
        if(host_path.empty()){
            return std::unexpected(
                io_error{
                    io_error_code::invalid_argument,
                    "workspace host path is empty"
                }
            );
        }

        const std::filesystem::path relative_path =
            host_path.lexically_relative(host_root_path_);
        if(
            relative_path.empty() ||
            relative_path.native().starts_with("..")
        ){
            return std::unexpected(
                io_error{
                    io_error_code::invalid_argument,
                    "path is outside workspace"
                }
            );
        }

        return sandbox_root() / relative_path;
    }

private:
    explicit workspace_layout(std::filesystem::path host_root_path) :
        host_root_path_(std::move(host_root_path)){}

    std::filesystem::path host_root_path_;
};
