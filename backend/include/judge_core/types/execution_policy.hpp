#pragma once

enum class execution_stop_rule{
    run_all,
    stop_on_first_execution_failure
};

enum class stderr_capture_policy{
    keep_all,
    keep_first_failure_only
};

struct execution_policy{
    execution_stop_rule stop_rule = execution_stop_rule::run_all;
    stderr_capture_policy stderr_policy = stderr_capture_policy::keep_all;
};
