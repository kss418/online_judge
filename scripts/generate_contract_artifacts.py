#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
STATUS_MANIFEST_PATH = ROOT / "contracts" / "submission_statuses.json"
HTTP_ERROR_MANIFEST_PATH = ROOT / "contracts" / "http_errors.json"
SUBMISSION_STATUS_HEADER_PATH = ROOT / "backend" / "include" / "common" / "submission_status.hpp"
HTTP_ERROR_HEADER_PATH = ROOT / "backend" / "include" / "error" / "http_error.hpp"


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def extract_enum_members(path: Path, enum_name: str) -> list[str]:
    source = path.read_text(encoding="utf-8")
    match = re.search(
        rf"enum\s+class\s+{re.escape(enum_name)}\s*\{{(?P<body>.*?)\}};",
        source,
        re.S,
    )
    if not match:
        raise SystemExit(f"failed to find enum {enum_name} in {path}")

    members: list[str] = []
    for raw_line in match.group("body").split(","):
        member = raw_line.strip()
        if not member:
            continue
        member = member.split("=", 1)[0].strip()
        members.append(member)
    return members


def validate_unique_codes(entries: list[dict], key: str, label: str) -> None:
    codes = [entry[key] for entry in entries]
    if len(codes) != len(set(codes)):
        raise SystemExit(f"duplicate {label} codes in manifest")


def require_fields(entry: dict, fields: list[str], label: str) -> None:
    missing = [field for field in fields if field not in entry]
    if missing:
        raise SystemExit(f"{label} missing required fields: {', '.join(missing)}")


def validate_status_manifest(statuses: list[dict], enum_members: list[str]) -> list[dict]:
    validate_unique_codes(statuses, "code", "submission status")
    expected_codes = set(enum_members)
    manifest_codes = {entry["code"] for entry in statuses}
    if manifest_codes != expected_codes:
        missing = sorted(expected_codes - manifest_codes)
        extra = sorted(manifest_codes - expected_codes)
        problems = []
        if missing:
            problems.append(f"missing: {', '.join(missing)}")
        if extra:
            problems.append(f"extra: {', '.join(extra)}")
        raise SystemExit(f"submission status manifest mismatch ({'; '.join(problems)})")

    status_by_code = {entry["code"]: entry for entry in statuses}
    ordered_statuses: list[dict] = []
    for code in enum_members:
        entry = status_by_code[code]
        require_fields(
            entry,
            [
                "code",
                "db_enum_value",
                "public_label_ko",
                "badge_tone",
                "statistics_bucket",
                "is_terminal",
                "is_failure",
                "filter_visible",
                "statistics_visible",
            ],
            f"submission status {code}",
        )
        ordered_statuses.append(entry)

    return ordered_statuses


def validate_http_error_manifest(errors: list[dict], enum_members: list[str]) -> list[dict]:
    validate_unique_codes(errors, "code", "http error")
    expected_codes = set(enum_members)
    manifest_codes = {entry["code"] for entry in errors}
    if manifest_codes != expected_codes:
        missing = sorted(expected_codes - manifest_codes)
        extra = sorted(manifest_codes - expected_codes)
        problems = []
        if missing:
            problems.append(f"missing: {', '.join(missing)}")
        if extra:
            problems.append(f"extra: {', '.join(extra)}")
        raise SystemExit(f"http error manifest mismatch ({'; '.join(problems)})")

    error_by_code = {entry["code"]: entry for entry in errors}
    ordered_errors: list[dict] = []
    for code in enum_members:
        entry = error_by_code[code]
        require_fields(
            entry,
            [
                "code",
                "http_status",
                "default_public_message_en",
                "frontend_message_ko",
                "requires_bearer_auth",
                "field_supported",
                "category",
            ],
            f"http error {code}",
        )
        ordered_errors.append(entry)

    return ordered_errors


def cpp_string(value: str) -> str:
    return json.dumps(value, ensure_ascii=False)


def js_bool(value: bool) -> str:
    return "true" if value else "false"


def cpp_bool(value: bool) -> str:
    return "true" if value else "false"


def generate_backend_submission_status_cpp(statuses: list[dict]) -> str:
    rows = []
    for entry in statuses:
        rows.append(
            "\n".join(
                [
                    "        submission_status_spec{",
                    f"            submission_status::{entry['code']},",
                    f"            {cpp_string(entry['code'])},",
                    f"            {cpp_string(entry['db_enum_value'])},",
                    f"            {cpp_string(entry['public_label_ko'])},",
                    f"            {cpp_string(entry['badge_tone'])},",
                    f"            {cpp_string(entry['statistics_bucket'])},",
                    f"            {cpp_bool(entry['is_terminal'])},",
                    f"            {cpp_bool(entry['is_failure'])},",
                    f"            {cpp_bool(entry['filter_visible'])},",
                    f"            {cpp_bool(entry['statistics_visible'])}",
                    "        }",
                ]
            )
        )

    return "\n".join(
        [
            '#include "common/submission_status.hpp"',
            "",
            "#include <array>",
            "#include <span>",
            "",
            "namespace{",
            f"    constexpr std::array<submission_status_spec, {len(statuses)}> submission_status_specs{{{{",
            ",\n".join(rows),
            "    }};",
            "}",
            "",
            "std::span<const submission_status_spec> all_submission_status_specs(){",
            "    return submission_status_specs;",
            "}",
            "",
            "const submission_status_spec* find_submission_status_spec(submission_status status){",
            "    for(const auto& spec : submission_status_specs){",
            "        if(spec.status == status){",
            "            return &spec;",
            "        }",
            "    }",
            "",
            "    return nullptr;",
            "}",
            "",
        ]
    )


def generate_backend_http_error_cpp(errors: list[dict]) -> str:
    rows = []
    for entry in errors:
        rows.append(
            "\n".join(
                [
                    "        http_error_spec{",
                    f"            http_error_code::{entry['code']},",
                    f"            {cpp_string(entry['code'])},",
                    f"            boost::beast::http::status::{http_status_enum(entry['http_status'])},",
                    f"            {cpp_string(entry['default_public_message_en'])},",
                    f"            {cpp_string(entry['frontend_message_ko'])},",
                    f"            {cpp_string(entry.get('frontend_field_message_ko', ''))},",
                    f"            {cpp_bool(entry['requires_bearer_auth'])},",
                    f"            {cpp_bool(entry['field_supported'])},",
                    f"            {cpp_string(entry['category'])}",
                    "        }",
                ]
            )
        )

    return "\n".join(
        [
            '#include "error/http_error.hpp"',
            "",
            "#include <array>",
            "#include <span>",
            "",
            "namespace{",
            f"    constexpr std::array<http_error_spec, {len(errors)}> http_error_specs{{{{",
            ",\n".join(rows),
            "    }};",
            "}",
            "",
            "std::span<const http_error_spec> all_http_error_specs(){",
            "    return http_error_specs;",
            "}",
            "",
            "const http_error_spec* find_http_error_spec(http_error_code code){",
            "    for(const auto& spec : http_error_specs){",
            "        if(spec.code == code){",
            "            return &spec;",
            "        }",
            "    }",
            "",
            "    return nullptr;",
            "}",
            "",
        ]
    )


def http_status_enum(status_code: int) -> str:
    mapping = {
        400: "bad_request",
        401: "unauthorized",
        403: "forbidden",
        404: "not_found",
        405: "method_not_allowed",
        409: "conflict",
        413: "payload_too_large",
        500: "internal_server_error",
        503: "service_unavailable",
    }
    try:
        return mapping[status_code]
    except KeyError as exc:
        raise SystemExit(f"unsupported http status in manifest: {status_code}") from exc


def generate_frontend_submission_status_js(statuses: list[dict]) -> str:
    catalog_json = json.dumps(statuses, ensure_ascii=False, indent=2)
    return "\n".join(
        [
            "export const submissionStatusCatalog = " + catalog_json,
            "",
            "const submissionStatusMetaByCode = new Map(",
            "  submissionStatusCatalog.map((meta) => [meta.code, meta])",
            ")",
            "",
            "function normalizeStatusCode(status){",
            "  return typeof status === 'string' ? status.trim() : ''",
            "}",
            "",
            "export function getSubmissionStatusMeta(status){",
            "  const normalizedStatus = normalizeStatusCode(status)",
            "  return submissionStatusMetaByCode.get(normalizedStatus) || null",
            "}",
            "",
            "export function getSubmissionStatusLabel(status){",
            "  const meta = getSubmissionStatusMeta(status)",
            "  if (meta) {",
            "    return meta.public_label_ko",
            "  }",
            "",
            "  return normalizeStatusCode(status)",
            "}",
            "",
            "export function getSubmissionStatusTone(status){",
            "  return getSubmissionStatusMeta(status)?.badge_tone || 'neutral'",
            "}",
            "",
            "export function getSubmissionStatisticsFieldName(status){",
            "  const meta = getSubmissionStatusMeta(status)",
            "  if (!meta) {",
            "    return ''",
            "  }",
            "",
            "  return `${meta.statistics_bucket}_submission_count`",
            "}",
            "",
            "export const submissionStatusOptions = [",
            "  { value: '', label: '전체' },",
            "  ...submissionStatusCatalog",
            "    .filter((meta) => meta.filter_visible)",
            "    .map((meta) => ({ value: meta.code, label: meta.public_label_ko }))",
            "]",
            "",
            "export const submissionPollingStatuses = new Set(",
            "  submissionStatusCatalog",
            "    .filter((meta) => !meta.is_terminal)",
            "    .map((meta) => meta.code)",
            ")",
            "",
            "export const finishedSubmissionStatuses = new Set(",
            "  submissionStatusCatalog",
            "    .filter((meta) => meta.is_terminal)",
            "    .map((meta) => meta.code)",
            ")",
            "",
            "export const submissionStatisticsVisibleCatalog = submissionStatusCatalog",
            "  .filter((meta) => meta.statistics_visible)",
            "",
            "export const submissionStatisticsVisibleStatuses = submissionStatisticsVisibleCatalog",
            "  .map((meta) => meta.code)",
            "",
        ]
    )


def generate_frontend_http_error_js(errors: list[dict]) -> str:
    catalog_json = json.dumps(errors, ensure_ascii=False, indent=2)
    return "\n".join(
        [
            "export const httpErrorCatalog = " + catalog_json,
            "",
            "const httpErrorMetaByCode = new Map(",
            "  httpErrorCatalog.map((meta) => [meta.code, meta])",
            ")",
            "",
            "function normalizeCode(code){",
            "  return typeof code === 'string' ? code.trim() : ''",
            "}",
            "",
            "export function getHttpErrorMeta(code){",
            "  const normalizedCode = normalizeCode(code)",
            "  return httpErrorMetaByCode.get(normalizedCode) || null",
            "}",
            "",
            "export function getFrontendHttpErrorMessage(code, options = {}){",
            "  const meta = getHttpErrorMeta(code)",
            "  if (!meta) {",
            "    return ''",
            "  }",
            "",
            "  const fieldLabel = typeof options.fieldLabel === 'string'",
            "    ? options.fieldLabel.trim()",
            "    : ''",
            "",
            "  if (meta.field_supported && fieldLabel && typeof meta.frontend_field_message_ko === 'string') {",
            "    return meta.frontend_field_message_ko.replace('{field}', fieldLabel)",
            "  }",
            "",
            "  return typeof meta.frontend_message_ko === 'string'",
            "    ? meta.frontend_message_ko",
            "    : ''",
            "}",
            "",
        ]
    )


def write_generated_file(path: Path, content: str, check_only: bool) -> None:
    existing = path.read_text(encoding="utf-8") if path.exists() else None
    if check_only:
        if existing != content:
            raise SystemExit(f"generated file is stale: {path.relative_to(ROOT)}")
        return

    path.parent.mkdir(parents=True, exist_ok=True)
    if existing == content:
        return
    path.write_text(content, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="fail if generated files are stale")
    args = parser.parse_args()

    submission_status_enum_members = extract_enum_members(
        SUBMISSION_STATUS_HEADER_PATH,
        "submission_status",
    )
    http_error_enum_members = extract_enum_members(
        HTTP_ERROR_HEADER_PATH,
        "http_error_code",
    )

    statuses = validate_status_manifest(
        load_json(STATUS_MANIFEST_PATH)["statuses"],
        submission_status_enum_members,
    )
    errors = validate_http_error_manifest(
        load_json(HTTP_ERROR_MANIFEST_PATH)["errors"],
        http_error_enum_members,
    )

    outputs = {
        ROOT / "backend" / "src" / "generated" / "submission_status_catalog.cpp":
            generate_backend_submission_status_cpp(statuses),
        ROOT / "backend" / "src" / "generated" / "http_error_catalog.cpp":
            generate_backend_http_error_cpp(errors),
        ROOT / "frontend" / "src" / "generated" / "submissionStatusCatalog.js":
            generate_frontend_submission_status_js(statuses),
        ROOT / "frontend" / "src" / "generated" / "httpErrorCatalog.js":
            generate_frontend_http_error_js(errors),
    }

    for path, content in outputs.items():
        write_generated_file(path, content + "\n", args.check)

    return 0


if __name__ == "__main__":
    sys.exit(main())
