#pragma once

#include "judge_core/types/execution_report.hpp"

#include <optional>
#include <utility>

class execution_bundle{
public:
    static execution_bundle executed(execution_report::batch execution_report_value){
        return execution_bundle(std::move(execution_report_value));
    }

    static execution_bundle skipped(){
        return execution_bundle();
    }

    bool did_execute() const noexcept{
        return execution_report_opt_.has_value();
    }

    bool was_skipped() const noexcept{
        return !did_execute();
    }

    execution_report::batch* execution_report_opt() noexcept{
        return execution_report_opt_ ? &*execution_report_opt_ : nullptr;
    }

    const execution_report::batch* execution_report_opt() const noexcept{
        return execution_report_opt_ ? &*execution_report_opt_ : nullptr;
    }

    execution_report::batch& report(){
        return *execution_report_opt_;
    }

    const execution_report::batch& report() const{
        return *execution_report_opt_;
    }

private:
    execution_bundle() = default;

    explicit execution_bundle(execution_report::batch execution_report_value) :
        execution_report_opt_(std::move(execution_report_value)){}

    std::optional<execution_report::batch> execution_report_opt_;
};
