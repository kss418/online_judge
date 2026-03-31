#pragma once

#include "dto/submission_dto.hpp"

#include <boost/json.hpp>

#include <cstdint>
#include <vector>

namespace submission_json_serializer{
    boost::json::object make_created_object(
        const submission_dto::created& created_value
    );

    boost::json::object make_history_list_object(
        std::int64_t submission_id,
        const submission_dto::history_list& history_values
    );

    boost::json::object make_source_object(
        const submission_dto::source_detail& source_detail_value
    );

    boost::json::object make_detail_object(
        const submission_dto::detail& detail_value
    );

    boost::json::object make_status_snapshot_batch_object(
        const std::vector<submission_dto::status_snapshot>& snapshot_values
    );

    boost::json::object make_list_object(const submission_dto::summary_page& summary_page_value);
}
