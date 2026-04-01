#include "common/testcase_zip_service.hpp"

#include "common/blocking_io.hpp"
#include "common/temp_dir.hpp"
#include "common/temp_file.hpp"
#include "common/testcase_uploader.hpp"
#include "common/zip_util.hpp"

#include <utility>

namespace{
    using service_error = testcase_zip_service::error;
    using error_kind = testcase_zip_service::error_kind;

    service_error make_error(error_kind kind, std::string detail = {}){
        return service_error{
            .kind = kind,
            .detail = std::move(detail)
        };
    }
}

std::expected<std::vector<problem_dto::testcase>, testcase_zip_service::error>
testcase_zip_service::load_testcases_from_zip_body(std::string_view zip_body){
    auto temp_zip_file_exp = temp_file::create("/tmp/oj_testcase_zip_XXXXXX");
    if(!temp_zip_file_exp){
        return std::unexpected(make_error(
            error_kind::create_temp_file_failed,
            to_string(temp_zip_file_exp.error())
        ));
    }

    temp_file temp_zip_file = std::move(*temp_zip_file_exp);
    const auto write_body_exp = blocking_io::write_all(
        temp_zip_file.get_fd(),
        std::string{zip_body}
    );
    if(!write_body_exp){
        return std::unexpected(make_error(
            error_kind::write_temp_file_failed,
            to_string(write_body_exp.error())
        ));
    }

    const auto close_zip_file_exp = temp_zip_file.close_fd_checked();
    if(!close_zip_file_exp){
        return std::unexpected(make_error(
            error_kind::finalize_temp_file_failed,
            to_string(close_zip_file_exp.error())
        ));
    }

    const auto list_zip_entries_exp = zip_util::list_entry_names(
        temp_zip_file.get_path()
    );
    if(!list_zip_entries_exp){
        if(list_zip_entries_exp.error() == errno_error::file_not_found){
            return std::unexpected(make_error(error_kind::inspect_zip_unavailable));
        }
        if(list_zip_entries_exp.error().is_bad_request_error()){
            return std::unexpected(make_error(error_kind::inspect_zip_invalid));
        }
        return std::unexpected(make_error(
            error_kind::inspect_zip_failed,
            to_string(list_zip_entries_exp.error())
        ));
    }

    const auto archive_entries_exp = testcase_uploader::parse_testcase_archive_entries(
        *list_zip_entries_exp
    );
    if(!archive_entries_exp){
        return std::unexpected(make_error(
            error_kind::invalid_archive_entries,
            archive_entries_exp.error()
        ));
    }

    auto extraction_directory_exp = temp_dir::create(
        "/tmp/oj_testcase_zip_extract_XXXXXX"
    );
    if(!extraction_directory_exp){
        return std::unexpected(make_error(
            error_kind::create_extract_directory_failed,
            to_string(extraction_directory_exp.error())
        ));
    }

    temp_dir extraction_directory = std::move(*extraction_directory_exp);
    const auto unzip_archive_exp = zip_util::extract_to_directory(
        temp_zip_file.get_path(),
        extraction_directory.get_path()
    );
    if(!unzip_archive_exp){
        if(unzip_archive_exp.error() == errno_error::file_not_found){
            return std::unexpected(make_error(error_kind::extract_zip_unavailable));
        }
        if(unzip_archive_exp.error().is_bad_request_error()){
            return std::unexpected(make_error(error_kind::extract_zip_invalid));
        }
        return std::unexpected(make_error(
            error_kind::extract_zip_failed,
            to_string(unzip_archive_exp.error())
        ));
    }

    const auto testcase_values_exp = testcase_uploader::load_testcases_from_directory(
        extraction_directory.get_path(),
        *archive_entries_exp
    );
    if(!testcase_values_exp){
        return std::unexpected(make_error(
            error_kind::load_testcases_failed,
            to_string(testcase_values_exp.error())
        ));
    }

    return std::move(*testcase_values_exp);
}
