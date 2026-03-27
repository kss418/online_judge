#include "http_core/testcase_uploader.hpp"

#include "common/file_util.hpp"

#include <cctype>
#include <string_view>

namespace testcase_uploader{
    std::string format_testcase_order(std::int32_t testcase_order);
    std::int32_t make_testcase_order(std::string_view entry_name);
    std::expected<void, std::string> set_archive_entry_name(
        archive_entry& archive_entry,
        std::int32_t testcase_order,
        bool entry_is_input_file,
        const std::string& entry_name
    );
    std::expected<void, std::string> validate_continuous_archive_entries(
        const archive_entry_map& archive_entries
    );
    bool is_input_file(std::string_view entry_name);
    bool is_output_file(std::string_view entry_name);
    bool has_valid_digits(std::string_view entry_name);
}

std::string testcase_uploader::format_testcase_order(std::int32_t testcase_order){
    std::string formatted = std::to_string(testcase_order);
    while(formatted.size() < 3){
        formatted.insert(formatted.begin(), '0');
    }
    return formatted;
}

std::int32_t testcase_uploader::make_testcase_order(std::string_view entry_name){
    return
        (entry_name[0] - '0') * 100 +
        (entry_name[1] - '0') * 10 +
        (entry_name[2] - '0');
}

std::expected<void, std::string> testcase_uploader::set_archive_entry_name(
    archive_entry& archive_entry,
    std::int32_t testcase_order,
    bool entry_is_input_file,
    const std::string& entry_name
){
    if(entry_is_input_file){
        if(!archive_entry.input_name.empty()){
            return std::unexpected(
                "invalid testcase zip: duplicate testcase input for " +
                format_testcase_order(testcase_order)
            );
        }
        archive_entry.input_name = entry_name;
        return {};
    }

    if(!archive_entry.output_name.empty()){
        return std::unexpected(
            "invalid testcase zip: duplicate testcase output for " +
            format_testcase_order(testcase_order)
        );
    }

    archive_entry.output_name = entry_name;
    return {};
}

std::expected<void, std::string> testcase_uploader::validate_continuous_archive_entries(
    const archive_entry_map& archive_entries
){
    std::int32_t expected_order = 1;
    for(const auto& [testcase_order, archive_entry] : archive_entries){
        if(archive_entry.input_name.empty() || archive_entry.output_name.empty()){
            return std::unexpected(
                "invalid testcase zip: missing .in or .out pair for " +
                format_testcase_order(testcase_order)
            );
        }
        if(testcase_order != expected_order){
            return std::unexpected(
                "invalid testcase zip: testcase files must be continuous from 001"
            );
        }

        ++expected_order;
    }

    return {};
}

bool testcase_uploader::is_input_file(std::string_view entry_name){
    return
        entry_name.size() == 6 &&
        entry_name[3] == '.' &&
        entry_name.substr(4) == "in";
}

bool testcase_uploader::is_output_file(std::string_view entry_name){
    return
        entry_name.size() == 7 &&
        entry_name[3] == '.' &&
        entry_name.substr(4) == "out";
}

bool testcase_uploader::has_valid_digits(std::string_view entry_name){
    return
        std::isdigit(static_cast<unsigned char>(entry_name[0])) &&
        std::isdigit(static_cast<unsigned char>(entry_name[1])) &&
        std::isdigit(static_cast<unsigned char>(entry_name[2]));
}

std::expected<testcase_uploader::archive_entry_map, std::string>
testcase_uploader::parse_testcase_archive_entries(
    const std::vector<std::string>& entry_names
){
    archive_entry_map archive_entries;
    for(const auto& entry_name : entry_names){
        const bool entry_is_input_file = is_input_file(entry_name);
        const bool entry_is_output_file = is_output_file(entry_name);
        if(!entry_is_input_file && !entry_is_output_file){
            return std::unexpected(
                "invalid testcase zip: only 001.in and 001.out style files are allowed"
            );
        }

        if(!has_valid_digits(entry_name)){
            return std::unexpected(
                "invalid testcase zip: only 001.in and 001.out style files are allowed"
            );
        }

        const std::int32_t testcase_order = make_testcase_order(entry_name);
        if(testcase_order <= 0 || testcase_order > 999){
            return std::unexpected(
                "invalid testcase zip: testcase order must be between 001 and 999"
            );
        }

        auto& archive_entry = archive_entries[testcase_order];
        const auto set_archive_entry_name_exp = set_archive_entry_name(
            archive_entry,
            testcase_order,
            entry_is_input_file,
            entry_name
        );
        if(!set_archive_entry_name_exp){
            return std::unexpected(set_archive_entry_name_exp.error());
        }
    }

    const auto validate_archive_entries_exp = validate_continuous_archive_entries(
        archive_entries
    );
    if(!validate_archive_entries_exp){
        return std::unexpected(validate_archive_entries_exp.error());
    }

    return archive_entries;
}

std::expected<std::vector<problem_dto::testcase>, error_code>
testcase_uploader::load_testcases_from_directory(
    const std::filesystem::path& directory_path,
    const testcase_uploader::archive_entry_map& archive_entries
){
    std::vector<problem_dto::testcase> testcase_values;
    testcase_values.reserve(archive_entries.size());

    for(const auto& [testcase_order, archive_entry] : archive_entries){
        (void)testcase_order;

        const auto input_exp = file_util::read_file_content(
            directory_path / archive_entry.input_name
        );
        if(!input_exp){
            return std::unexpected(input_exp.error());
        }

        const auto output_exp = file_util::read_file_content(
            directory_path / archive_entry.output_name
        );
        if(!output_exp){
            return std::unexpected(output_exp.error());
        }

        testcase_values.push_back(problem_dto::testcase{
            .input = std::move(*input_exp),
            .output = std::move(*output_exp)
        });
    }

    return testcase_values;
}
