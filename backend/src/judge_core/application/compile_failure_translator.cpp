#include "judge_core/application/compile_failure_translator.hpp"

namespace{
    execution_report::testcase_execution make_testcase_execution(
        const compile_failure& compile_failure_value
    ){
        execution_report::testcase_execution testcase_execution_value;
        testcase_execution_value.exit_code = compile_failure_value.exit_code;
        testcase_execution_value.stderr_text = compile_failure_value.stderr_text;
        testcase_execution_value.stderr_bytes =
            static_cast<std::int64_t>(compile_failure_value.stderr_text.size());
        return testcase_execution_value;
    }
}

execution_report::batch compile_failure_translator::translate(
    const compile_failure& compile_failure_value
) const{
    execution_report::batch execution_report_value;
    execution_report_value.compile_failed = true;
    execution_report_value.executions.push_back(
        make_testcase_execution(compile_failure_value)
    );
    return execution_report_value;
}
