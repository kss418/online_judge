#include "judge_server/testcase_downloader.hpp"

#include "common/file_util.hpp"
#include "db/problem_core_service.hpp"
#include "db/testcase_service.hpp"
#include "judge_server/tc_util.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

tc_downloader::tc_downloader(
    db_connection connection
) :
    connection_(std::move(connection)){}

std::expected<tc_downloader, error_code> tc_downloader::create(db_connection connection){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return tc_downloader(std::move(connection));
}

std::expected<std::int32_t, error_code> tc_downloader::read_version_file(std::int64_t problem_id) const{
    const auto version_file_path_exp = tc_util::instance().make_tc_version_file_path(
        problem_id
    );
    if(!version_file_path_exp){
        return std::unexpected(version_file_path_exp.error());
    }

    return file_util::read_int32_file(*version_file_path_exp);
}

std::expected<std::pair<std::int32_t, std::int32_t>, error_code> tc_downloader::read_limit_file(
    std::int64_t problem_id
) const{
    const auto time_limit_file_path_exp = tc_util::instance().make_tc_time_limit_file_path(
        problem_id
    );
    if(!time_limit_file_path_exp){
        return std::unexpected(time_limit_file_path_exp.error());
    }

    const auto memory_limit_file_path_exp = tc_util::instance().make_tc_memory_limit_file_path(
        problem_id
    );
    if(!memory_limit_file_path_exp){
        return std::unexpected(memory_limit_file_path_exp.error());
    }

    const auto time_limit_exp = file_util::read_int32_file(*time_limit_file_path_exp);
    if(!time_limit_exp){
        return std::unexpected(time_limit_exp.error());
    }

    const auto memory_limit_exp = file_util::read_int32_file(
        *memory_limit_file_path_exp
    );
    if(!memory_limit_exp){
        return std::unexpected(memory_limit_exp.error());
    }

    return std::pair{*time_limit_exp, *memory_limit_exp};
}

std::expected<bool, error_code> tc_downloader::is_latest(std::int64_t problem_id){
    const auto version_exp = problem_core_service::get_version(connection_, problem_id);
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    const auto local_version_exp = read_version_file(problem_id);
    if(!local_version_exp){
        const error_code local_version_error = local_version_exp.error();
        if(
            local_version_error.type_ == error_type::errno_type &&
            (
                local_version_error.code_ == static_cast<int>(errno_error::file_not_found) ||
                local_version_error.code_ == static_cast<int>(errno_error::invalid_argument)
            )
        ){
            return false;
        }

        return std::unexpected(local_version_error);
    }

    return local_version_exp.value() == version_exp.value();
}

std::expected<void, error_code> tc_downloader::sync_version_file(std::int64_t problem_id){
    const auto version_exp = problem_core_service::get_version(connection_, problem_id);
    if(!version_exp){
        return std::unexpected(version_exp.error());
    }

    const auto local_version_exp = read_version_file(problem_id);
    if(local_version_exp){
        if(local_version_exp.value() == version_exp.value()){
            return {};
        }
    }
    else{
        const error_code local_version_error = local_version_exp.error();
        if(
            local_version_error.type_ != error_type::errno_type ||
            (
                local_version_error.code_ != static_cast<int>(errno_error::file_not_found) &&
                local_version_error.code_ != static_cast<int>(errno_error::invalid_argument)
            )
        ){
            return std::unexpected(local_version_error);
        }
    }

    const auto version_file_path_exp = tc_util::instance().make_tc_version_file_path(
        problem_id
    );
    if(!version_file_path_exp){
        return std::unexpected(version_file_path_exp.error());
    }
    const std::filesystem::path version_file_path = *version_file_path_exp;
    const auto create_directories_exp = file_util::create_directories(
        version_file_path.parent_path()
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto create_version_file_exp = file_util::create_file(
        version_file_path,
        std::to_string(version_exp.value())
    );
    
    if(!create_version_file_exp){
        return std::unexpected(create_version_file_exp.error());
    }

    return {};
}

std::expected<void, error_code> tc_downloader::sync_limit_file(std::int64_t problem_id){
    const auto limits_exp = problem_core_service::get_limits(connection_, problem_id);
    if(!limits_exp){
        return std::unexpected(limits_exp.error());
    }

    const auto local_limits_exp = read_limit_file(problem_id);
    if(local_limits_exp){
        if(
            local_limits_exp->first == limits_exp->time_limit_ms &&
            local_limits_exp->second == limits_exp->memory_limit_mb
        ){
            return {};
        }
    }
    else{
        const error_code local_limits_error = local_limits_exp.error();
        if(
            local_limits_error.type_ != error_type::errno_type ||
            (
                local_limits_error.code_ != static_cast<int>(errno_error::file_not_found) &&
                local_limits_error.code_ != static_cast<int>(errno_error::invalid_argument)
            )
        ){
            return std::unexpected(local_limits_error);
        }
    }

    const auto memory_limit_file_path_exp = tc_util::instance().make_tc_memory_limit_file_path(
        problem_id
    );
    if(!memory_limit_file_path_exp){
        return std::unexpected(memory_limit_file_path_exp.error());
    }

    const auto time_limit_file_path_exp = tc_util::instance().make_tc_time_limit_file_path(
        problem_id
    );
    if(!time_limit_file_path_exp){
        return std::unexpected(time_limit_file_path_exp.error());
    }

    const auto create_directories_exp = file_util::create_directories(
        memory_limit_file_path_exp->parent_path()
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto create_memory_limit_file_exp = file_util::create_file(
        *memory_limit_file_path_exp,
        std::to_string(limits_exp->memory_limit_mb)
    );
    if(!create_memory_limit_file_exp){
        return std::unexpected(create_memory_limit_file_exp.error());
    }

    const auto create_time_limit_file_exp = file_util::create_file(
        *time_limit_file_path_exp,
        std::to_string(limits_exp->time_limit_ms)
    );
    if(!create_time_limit_file_exp){
        return std::unexpected(create_time_limit_file_exp.error());
    }

    return {};
}

std::expected<void, error_code> tc_downloader::sync_tc(std::int64_t problem_id){
    const auto is_latest_exp = is_latest(problem_id);
    if(!is_latest_exp){
        return std::unexpected(is_latest_exp.error());
    }

    if(is_latest_exp.value()){
        return sync_limit_file(problem_id);
    }

    const auto download_all_exp = download_all(problem_id);
    if(!download_all_exp){
        return std::unexpected(download_all_exp.error());
    }

    const auto delete_outdated_exp = delete_outdated(problem_id);
    if(!delete_outdated_exp){
        return std::unexpected(delete_outdated_exp.error());
    }

    const auto sync_limit_file_exp = sync_limit_file(problem_id);
    if(!sync_limit_file_exp){
        return std::unexpected(sync_limit_file_exp.error());
    }

    const auto sync_version_file_exp = sync_version_file(problem_id);
    if(!sync_version_file_exp){
        return std::unexpected(sync_version_file_exp.error());
    }

    return {};
}

std::expected<void, error_code> tc_downloader::download_all(std::int64_t problem_id){
    const auto tc_count_exp = tc_service::get_tc_count(connection_, problem_id);
    if(!tc_count_exp){
        return std::unexpected(tc_count_exp.error());
    }

    for(std::int32_t order = 1; order <= tc_count_exp.value(); ++order){
        const auto download_one_exp = download_one(problem_id, order);
        if(!download_one_exp){
            return std::unexpected(download_one_exp.error());
        }
    }

    return {};
}

std::expected<void, error_code> tc_downloader::delete_outdated(std::int64_t problem_id){
    const auto tc_count_exp = tc_service::get_tc_count(connection_, problem_id);
    if(!tc_count_exp){
        return std::unexpected(tc_count_exp.error());
    }

    const auto problem_directory_path_exp = tc_util::instance()
        .make_tc_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }
    const std::filesystem::path problem_directory_path = *problem_directory_path_exp;
    const auto problem_directory_exists_exp = file_util::exists(
        problem_directory_path
    );
    if(!problem_directory_exists_exp){
        return std::unexpected(problem_directory_exists_exp.error());
    }
    if(!problem_directory_exists_exp.value()){
        return {};
    }

    std::error_code iterator_ec;
    std::filesystem::directory_iterator directory_iterator(problem_directory_path, iterator_ec);
    if(iterator_ec){
        return std::unexpected(error_code::create(error_code::map_errno(iterator_ec.value())));
    }

    for(std::filesystem::directory_iterator end; directory_iterator != end;){
        const std::filesystem::path entry_path = directory_iterator->path();
        const std::string extension = entry_path.extension().string();
        if(extension != ".in" && extension != ".out"){
            directory_iterator.increment(iterator_ec);
            if(iterator_ec){
                return std::unexpected(
                    error_code::create(error_code::map_errno(iterator_ec.value()))
                );
            }
            continue;
        }

        const std::string stem = entry_path.stem().string();
        std::int32_t order = 0;
        const auto [parse_end, parse_ec] = std::from_chars(
            stem.data(),
            stem.data() + stem.size(),
            order
        );

        if(parse_ec != std::errc{} || parse_end != stem.data() + stem.size()){
            directory_iterator.increment(iterator_ec);
            if(iterator_ec){
                return std::unexpected(
                    error_code::create(error_code::map_errno(iterator_ec.value()))
                );
            }
            continue;
        }
        
        if(order <= tc_count_exp.value()){
            directory_iterator.increment(iterator_ec);
            if(iterator_ec){
                return std::unexpected(
                    error_code::create(error_code::map_errno(iterator_ec.value()))
                );
            }
            continue;
        }

        const auto remove_file_exp = file_util::remove_file(entry_path);
        if(!remove_file_exp){
            return std::unexpected(remove_file_exp.error());
        }

        directory_iterator.increment(iterator_ec);
        if(iterator_ec){
            return std::unexpected(error_code::create(error_code::map_errno(iterator_ec.value())));
        }
    }

    return {};
}

std::expected<void, error_code> tc_downloader::delete_one(
    std::int64_t problem_id, std::int32_t order
){
    const auto input_path_exp = tc_util::instance().make_tc_input_path(
        problem_id,
        order
    );
    if(!input_path_exp){
        return std::unexpected(input_path_exp.error());
    }
    const auto remove_input_exp = file_util::remove_file(*input_path_exp);
    if(!remove_input_exp){
        return std::unexpected(remove_input_exp.error());
    }

    const auto output_path_exp = tc_util::instance().make_tc_output_path(
        problem_id,
        order
    );
    if(!output_path_exp){
        return std::unexpected(output_path_exp.error());
    }
    const auto remove_output_exp = file_util::remove_file(*output_path_exp);
    if(!remove_output_exp){
        return std::unexpected(remove_output_exp.error());
    }

    const auto version_file_path_exp = tc_util::instance().make_tc_version_file_path(
        problem_id
    );
    if(!version_file_path_exp){
        return std::unexpected(version_file_path_exp.error());
    }
    const auto remove_version_exp = file_util::remove_file(*version_file_path_exp);
    if(!remove_version_exp){
        return std::unexpected(remove_version_exp.error());
    }

    return {};
}

std::expected<void, error_code> tc_downloader::download_one(
    std::int64_t problem_id, std::int32_t order
){
    const auto tc_exp = tc_service::get_tc(connection_, problem_id, order);
    if(!tc_exp){
        return std::unexpected(tc_exp.error());
    }

    const auto input_path_exp = tc_util::instance().make_tc_input_path(
        problem_id,
        order
    );
    if(!input_path_exp){
        return std::unexpected(input_path_exp.error());
    }
    const auto output_path_exp = tc_util::instance().make_tc_output_path(
        problem_id,
        order
    );
    if(!output_path_exp){
        return std::unexpected(output_path_exp.error());
    }
    const std::filesystem::path input_path = *input_path_exp;
    const std::filesystem::path output_path = *output_path_exp;
    const auto create_directories_exp = file_util::create_directories(
        input_path.parent_path()
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto create_input_exp = file_util::create_file(
        input_path,
        tc_exp->tc_input
    );
    
    if(!create_input_exp){
        return std::unexpected(create_input_exp.error());
    }

    const auto create_output_exp = file_util::create_file(
        output_path,
        tc_exp->tc_output
    );
    
    if(!create_output_exp){
        return std::unexpected(create_output_exp.error());
    }

    return {};
}
