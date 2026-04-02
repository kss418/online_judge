#include "http_guard/testcase_upload_guard.hpp"

#include "common/testcase_zip_service.hpp"
#include "error/testcase_upload_error.hpp"

std::expected<std::vector<problem_dto::testcase>, testcase_upload_guard::response_type>
testcase_upload_guard::require_testcase_zip_upload(const request_type& request){
    if(request.body().empty()){
        return std::unexpected(http_response_util::create_error(
            request,
            testcase_upload_error::empty_zip_body()
        ));
    }

    const auto testcase_values_exp = testcase_zip_service::load_testcases_from_zip_body(
        request.body()
    );
    if(!testcase_values_exp){
        return std::unexpected(testcase_upload_error::create_response(
            request,
            testcase_values_exp.error()
        ));
    }

    return std::move(*testcase_values_exp);
}
