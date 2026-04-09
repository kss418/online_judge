#pragma once

#include "dto/problem_dto.hpp"
#include "http_core/http_response_util.hpp"
#include "http_guard/guard_runner.hpp"

#include <expected>
#include <vector>

namespace testcase_upload_guard{
    using request_type = http_response_util::request_type;
    using response_type = http_response_util::response_type;

    std::expected<std::vector<problem_dto::testcase>, response_type> require_testcase_zip_upload(
        const request_type& request
    );

    inline auto make_testcase_zip_guard(){
        return [](const http_guard::guard_context& context){
            return require_testcase_zip_upload(context.request());
        };
    }
}
