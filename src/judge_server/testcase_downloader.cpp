#include "judge_server/testcase_downloader.hpp"

#include "common/file_util.hpp"
#include "db/problem_core_service.hpp"
#include "db/testcase_service.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

testcase_downloader::testcase_downloader(
    db_connection connection
) :
    connection_(std::move(connection)){}

std::expected<testcase_downloader, error_code> testcase_downloader::create(db_connection connection){
    if(!connection.is_connected()){
        return std::unexpected(error_code::create(errno_error::invalid_file_descriptor));
    }

    return testcase_downloader(std::move(connection));
}

std::expected<std::int32_t, error_code> testcase_downloader::read_version_file(std::int64_t problem_id) const{
    const auto version_file_path_exp = file_utility::instance().make_testcase_version_file_path(
        problem_id
    );
    if(!version_file_path_exp){
        return std::unexpected(version_file_path_exp.error());
    }
    const std::filesystem::path version_file_path = *version_file_path_exp;
    const auto version_file_exists_exp = file_utility::instance().exists(version_file_path);
    if(!version_file_exists_exp){
        return std::unexpected(version_file_exists_exp.error());
    }

    if(!version_file_exists_exp.value()){
        return std::unexpected(error_code::create(errno_error::file_not_found));
    }

    std::ifstream version_file(version_file_path);
    if(!version_file.is_open()){
        return std::unexpected(error_code::create(errno_error::io_error));
    }

    std::int32_t local_version = 0;
    version_file >> local_version;
    if(version_file.bad()){
        return std::unexpected(error_code::create(errno_error::io_error));
    }
    
    if(!version_file){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    version_file >> std::ws;
    if(!version_file.eof()){
        return std::unexpected(error_code::create(errno_error::invalid_argument));
    }

    return local_version;
}

std::expected<bool, error_code> testcase_downloader::is_latest(std::int64_t problem_id){
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

std::expected<void, error_code> testcase_downloader::sync_version_file(std::int64_t problem_id){
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

    const auto version_file_path_exp = file_utility::instance().make_testcase_version_file_path(
        problem_id
    );
    if(!version_file_path_exp){
        return std::unexpected(version_file_path_exp.error());
    }
    const std::filesystem::path version_file_path = *version_file_path_exp;
    const auto create_directories_exp = file_utility::instance().create_directories(
        version_file_path.parent_path()
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto create_version_file_exp = file_utility::instance().create_file(
        version_file_path,
        std::to_string(version_exp.value())
    );
    
    if(!create_version_file_exp){
        return std::unexpected(create_version_file_exp.error());
    }

    return {};
}

std::expected<void, error_code> testcase_downloader::sync_testcase(std::int64_t problem_id){
    const auto is_latest_exp = is_latest(problem_id);
    if(!is_latest_exp){
        return std::unexpected(is_latest_exp.error());
    }

    if(is_latest_exp.value()){
        return {};
    }

    const auto download_all_exp = download_all(problem_id);
    if(!download_all_exp){
        return std::unexpected(download_all_exp.error());
    }

    const auto delete_outdated_exp = delete_outdated(problem_id);
    if(!delete_outdated_exp){
        return std::unexpected(delete_outdated_exp.error());
    }

    const auto sync_version_file_exp = sync_version_file(problem_id);
    if(!sync_version_file_exp){
        return std::unexpected(sync_version_file_exp.error());
    }

    return {};
}

std::expected<void, error_code> testcase_downloader::download_all(std::int64_t problem_id){
    const auto testcase_count_exp = testcase_service::get_testcase_count(connection_, problem_id);
    if(!testcase_count_exp){
        return std::unexpected(testcase_count_exp.error());
    }

    for(std::int32_t order = 1; order <= testcase_count_exp.value(); ++order){
        const auto download_one_exp = download_one(problem_id, order);
        if(!download_one_exp){
            return std::unexpected(download_one_exp.error());
        }
    }

    return {};
}

std::expected<void, error_code> testcase_downloader::delete_outdated(std::int64_t problem_id){
    const auto testcase_count_exp = testcase_service::get_testcase_count(connection_, problem_id);
    if(!testcase_count_exp){
        return std::unexpected(testcase_count_exp.error());
    }

    const auto problem_directory_path_exp = file_utility::instance()
        .make_testcase_problem_directory_path(problem_id);
    if(!problem_directory_path_exp){
        return std::unexpected(problem_directory_path_exp.error());
    }
    const std::filesystem::path problem_directory_path = *problem_directory_path_exp;
    const auto problem_directory_exists_exp = file_utility::instance().exists(
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
        
        if(order <= testcase_count_exp.value()){
            directory_iterator.increment(iterator_ec);
            if(iterator_ec){
                return std::unexpected(
                    error_code::create(error_code::map_errno(iterator_ec.value()))
                );
            }
            continue;
        }

        const auto remove_file_exp = file_utility::instance().remove_file(entry_path);
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

std::expected<void, error_code> testcase_downloader::delete_one(
    std::int64_t problem_id, std::int32_t order
){
    const auto input_path_exp = file_utility::instance().make_testcase_input_path(
        problem_id,
        order
    );
    if(!input_path_exp){
        return std::unexpected(input_path_exp.error());
    }
    const auto remove_input_exp = file_utility::instance().remove_file(*input_path_exp);
    if(!remove_input_exp){
        return std::unexpected(remove_input_exp.error());
    }

    const auto output_path_exp = file_utility::instance().make_testcase_output_path(
        problem_id,
        order
    );
    if(!output_path_exp){
        return std::unexpected(output_path_exp.error());
    }
    const auto remove_output_exp = file_utility::instance().remove_file(*output_path_exp);
    if(!remove_output_exp){
        return std::unexpected(remove_output_exp.error());
    }

    const auto version_file_path_exp = file_utility::instance().make_testcase_version_file_path(
        problem_id
    );
    if(!version_file_path_exp){
        return std::unexpected(version_file_path_exp.error());
    }
    const auto remove_version_exp = file_utility::instance().remove_file(*version_file_path_exp);
    if(!remove_version_exp){
        return std::unexpected(remove_version_exp.error());
    }

    return {};
}

std::expected<void, error_code> testcase_downloader::download_one(
    std::int64_t problem_id, std::int32_t order
){
    const auto testcase_exp = testcase_service::get_testcase(connection_, problem_id, order);
    if(!testcase_exp){
        return std::unexpected(testcase_exp.error());
    }

    const auto input_path_exp = file_utility::instance().make_testcase_input_path(
        problem_id,
        order
    );
    if(!input_path_exp){
        return std::unexpected(input_path_exp.error());
    }
    const auto output_path_exp = file_utility::instance().make_testcase_output_path(
        problem_id,
        order
    );
    if(!output_path_exp){
        return std::unexpected(output_path_exp.error());
    }
    const std::filesystem::path input_path = *input_path_exp;
    const std::filesystem::path output_path = *output_path_exp;
    const auto create_directories_exp = file_utility::instance().create_directories(
        input_path.parent_path()
    );
    if(!create_directories_exp){
        return std::unexpected(create_directories_exp.error());
    }

    const auto create_input_exp = file_utility::instance().create_file(
        input_path,
        testcase_exp->testcase_input
    );
    
    if(!create_input_exp){
        return std::unexpected(create_input_exp.error());
    }

    const auto create_output_exp = file_utility::instance().create_file(
        output_path,
        testcase_exp->testcase_output
    );
    
    if(!create_output_exp){
        return std::unexpected(create_output_exp.error());
    }

    return {};
}
