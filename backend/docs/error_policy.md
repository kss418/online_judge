# error_policy

This document defines the public HTTP error contract used by `http_server`.

The goal is to keep `status`, `error.code`, and `error.message` stable even if
service, repository, or infrastructure internals change.

## rules

### request parsing and request validation

- HTTP request parsing and request-shape validation should use
  [`request_error`](../include/error/request_error.hpp).
- This includes malformed JSON, invalid query strings, duplicate or unsupported
  query parameters, missing required fields, invalid field values, and invalid
  field lengths.
- Preferred rule:
  - protocol or request-shape failure -> `request_error`
  - business or domain validation failure after parsing -> `validation_error`

Examples:

- malformed JSON body -> `400 invalid_json`
- missing `user_login_id` -> `400 missing_field`
- unsupported `language` value -> `400 invalid_field`
- invalid anonymous `state` query -> `400 invalid_query_parameter`

### missing resources

- Required missing resources should be represented in the service layer as
  `service_error::not_found`.
- HTTP handlers and guards should prefer the common
  `service_error::not_found -> http_error_code::not_found -> 404`
  mapping.
- Do not introduce handler-local or guard-local `404` branches when the common
  mapping already expresses the contract.
- Prefer explicit service results over `optional` or `bool` when the missing
  state should become a public `404`.

Examples:

- unknown `problem_id` -> `404 not_found`
- unknown `submission_id` -> `404 not_found`
- unknown `user_id` -> `404 not_found`

### special public error codes

Most endpoints should use generic public codes such as `validation_error`,
`unauthorized`, `forbidden`, `not_found`, and `conflict`.

Special public codes should be added only when they satisfy at least one of the
following:

- clients need to react differently than they would to the generic code
- the code exposes a stable product or auth concept that matters to users
- several internal failures are intentionally collapsed into one public concept

Current special public codes intentionally kept in the contract:

- `invalid_credentials`
- `invalid_or_expired_token`
- `missing_or_invalid_bearer_token`
- `admin_bearer_token_required`
- `superadmin_bearer_token_required`
- `submission_banned`
- `invalid_testcase_zip`

Avoid one-off operation-specific codes when a generic code plus message is
enough.

### internal server errors

- Public `500` responses must stay generic.
- The response body should use `internal_server_error` with the default public
  message.
- Internal details must be written to server logs, not returned to clients.
- Use `http_response_util::create_internal_server_error(...)` when an endpoint
  needs to log detail and return a public `500`.

## testing

Public error contracts should be covered by flow tests.

- Main snapshot test:
  [`test/http_error_contract_flow.sh`](../test/http_error_contract_flow.sh)
- This contract flow should be updated whenever a stable public error contract
  changes.
- Endpoint-specific flow tests may still check behavior, but the shared
  contract test should remain the primary place for common `400`, `401`, `404`,
  and `409` rules.

