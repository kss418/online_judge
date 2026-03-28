# problem_api

HTTP API handled by `problem_router`.

## endpoint

### `GET /api/problem`

List problems using an optional title filter. This endpoint is public. When a valid bearer token is
provided, the response also includes the current user's problem state for each listed problem.

#### request

- required header: none
- optional header:

| header | required | note |
|---|---|---|
| `Authorization` | no | format: `Bearer <token>`; when present and valid, `user_problem_state` is populated |

- supported query parameters:

| field | type | required | note |
|---|---|---|---|
| `title` | `string` | no | case-insensitive substring match |

If `title` is omitted, the endpoint returns the full problem list ordered by `problem_id` descending.

Example:

```text
GET /api/problem?title=A%20%2B%20B
```

```text
GET /api/problem
```

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `problem_count` | `int64` | number of returned problems |
| `problems` | `array<object>` | problem summaries ordered by `problem_id` descending |

Each problem summary contains:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | problem id |
| `title` | `string` | problem title |
| `version` | `int32` | current problem version |
| `submission_count` | `int64` | aggregate submission count across all users |
| `accepted_count` | `int64` | aggregate accepted count across all users |
| `user_problem_state` | `string \| null` | `solved`, `wrong`, or `null`; `null` is returned for anonymous requests, unattempted problems, and pending-only submissions |

Example:

```json
{
  "problem_count": 2,
  "problems": [
    {
      "problem_id": 1001,
      "title": "Multiply",
      "version": 1,
      "submission_count": 12,
      "accepted_count": 7,
      "user_problem_state": null
    },
    {
      "problem_id": 1000,
      "title": "A + B",
      "version": 3,
      "submission_count": 21,
      "accepted_count": 11,
      "user_problem_state": "solved"
    }
  ]
}
```

#### error response

- invalid query string or duplicate query parameter: `400 Bad Request`
- unsupported query parameter: `400 Bad Request`
- invalid, expired, or malformed bearer token when `Authorization` is provided: `401 Unauthorized`
- unexpected internal failure: `500 Internal Server Error`

If no problems match the filter, the endpoint returns `200 OK` with an empty `problems` array.

### `GET /api/problem/{problem_id}`

Get a public problem detail view. This endpoint returns visible metadata, limits, statement content when present, public samples, and aggregate submission statistics. When a valid bearer token is provided, the response also includes the current user's state for the problem. Hidden testcases are not exposed.

#### request

- request body: none
- optional header:

| header | required | note |
|---|---|---|
| `Authorization` | no | format: `Bearer <token>`; when present and valid, `user_problem_state` is populated |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | requested problem id |
| `title` | `string` | problem title |
| `version` | `int32` | current problem version |
| `limits` | `object` | current execution limits |
| `limits.memory_limit_mb` | `int32` | memory limit in megabytes |
| `limits.time_limit_ms` | `int32` | time limit in milliseconds |
| `statement` | `object \| null` | `null` when no statement has been stored yet |
| `statement.description` | `string` | present when `statement` is an object |
| `statement.input_format` | `string` | present when `statement` is an object |
| `statement.output_format` | `string` | present when `statement` is an object |
| `statement.note` | `string` | optional note field |
| `sample_count` | `int64` | number of public samples returned |
| `samples` | `array<object>` | public sample list ordered by `sample_order` ascending |
| `samples[].sample_order` | `int32` | sample order starting from 1 |
| `samples[].sample_input` | `string` | sample input text |
| `samples[].sample_output` | `string` | sample output text |
| `statistics` | `object` | aggregate submission statistics |
| `statistics.submission_count` | `int64` | number of submissions |
| `statistics.accepted_count` | `int64` | number of accepted submissions |
| `user_problem_state` | `string \| null` | `solved`, `wrong`, or `null`; `null` is returned for anonymous requests, unattempted problems, and pending-only submissions |

Example:

```json
{
  "problem_id": 1000,
  "title": "A + B",
  "version": 3,
  "limits": {
    "memory_limit_mb": 256,
    "time_limit_ms": 1000
  },
  "statement": {
    "description": "Print A+B.",
    "input_format": "Two integers A and B are given.",
    "output_format": "Print A+B.",
    "note": "1 <= A, B <= 10"
  },
  "sample_count": 1,
  "samples": [
    {
      "sample_order": 1,
      "sample_input": "1 2",
      "sample_output": "3"
    }
  ],
  "statistics": {
    "submission_count": 12,
    "accepted_count": 7
  },
  "user_problem_state": "solved"
}
```

#### error response

- invalid, expired, or malformed bearer token when `Authorization` is provided: `401 Unauthorized`
- unknown `problem_id`: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

Examples:

```json
{
  "error": {
    "code": "problem_not_found",
    "message": "problem not found"
  }
}
```

### `POST /api/problem`

Create a new problem and return the generated problem number. The problem number matches the database `problem_id` generated by the `bigserial` sequence. New problems are initialized with default limits: `time_limit_ms = 1000` and `memory_limit_mb = 256`.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- body fields:

| field | type | required | note |
|---|---|---|---|
| `title` | `string` | yes | must be non-empty |

Example:

```json
{
  "title": "A + B"
}
```

```text
Authorization: Bearer ...
```

#### success response

- status: `201 Created`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | generated problem id from the database sequence |

Example:

```json
{
  "problem_id": 1000
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- invalid json: `400 Bad Request`
- missing required fields: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

Examples:

```json
{
  "error": {
    "code": "admin_bearer_token_required",
    "message": "admin bearer token required"
  }
}
```

### `PUT /api/problem/{problem_id}/title`

Update an existing problem title. This endpoint requires an admin bearer token. Updating the title increments the problem version by 1.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

- body fields:

| field | type | required | note |
|---|---|---|---|
| `title` | `string` | yes | must be non-empty |

Example:

```json
{
  "title": "A + B Revised"
}
```

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | always `problem updated` |

Example:

```json
{
  "message": "problem updated"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- invalid json: `400 Bad Request`
- missing required fields: `400 Bad Request`
- unknown `problem_id`: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

### `DELETE /api/problem/{problem_id}`

Delete an existing problem. This endpoint requires an admin bearer token.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | always `problem deleted` |

Example:

```json
{
  "message": "problem deleted"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- unknown `problem_id`: `404 Not Found`
- delete blocked by a database constraint such as existing submissions: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

### `POST /api/problem/{problem_id}/rejudge`

Requeue all `wrong_answer` and `accepted` submissions for a problem. This endpoint requires an admin bearer token.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | currently `"problem submissions requeued"` |

Example:

```json
{
  "message": "problem submissions requeued"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- unknown `problem_id`: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `PUT /api/problem/{problem_id}/limits`

Update the limits of an existing problem.

#### request

- content-type: `application/json`
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

- body fields:

| field | type | required | note |
|---|---|---|---|
| `memory_limit_mb` | `int32` | yes | must be positive |
| `time_limit_ms` | `int32` | yes | must be positive |

Example:

```json
{
  "memory_limit_mb": 512,
  "time_limit_ms": 2000
}
```

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | always `problem limits updated` |

Example:

```json
{
  "message": "problem limits updated"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- invalid json: `400 Bad Request`
- missing required fields: `400 Bad Request`
- unknown `problem_id`: `400 Bad Request`
- invalid limit values: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

Example:

```json
{
  "error": {
    "code": "invalid_field",
    "message": "memory_limit_mb must be a positive integer",
    "field": "memory_limit_mb"
  }
}
```

### `PUT /api/problem/{problem_id}/statement`

Replace the statement content of an existing problem. This updates only the statement fields and increments the problem version. Limits, samples, and aggregate statistics are not modified by this endpoint.

#### request

- content-type: `application/json`
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

- body fields:

| field | type | required | note |
|---|---|---|---|
| `description` | `string` | yes | must be non-empty |
| `input_format` | `string` | yes | must be non-empty |
| `output_format` | `string` | yes | must be non-empty |
| `note` | `string \| null` | no | omit, pass `null`, or pass `""` to clear |

Example:

```json
{
  "description": "Print A+B.",
  "input_format": "Two integers A and B are given.",
  "output_format": "Print A+B.",
  "note": "1 <= A, B <= 10"
}
```

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | always `problem statement updated` |

Example:

```json
{
  "message": "problem statement updated"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- invalid json: `400 Bad Request`
- missing required fields: `400 Bad Request`
- unknown `problem_id`: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `GET /api/problem/{problem_id}/sample`

List public samples for an existing problem. Samples are managed as statement content, and this endpoint exposes sample ids and full sample payloads.

#### request

- request body: none

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `sample_count` | `int64` | number of returned public samples |
| `samples` | `array<object>` | editable sample list ordered by `sample_order` ascending |

Each sample object contains:

| field | type | note |
|---|---|---|
| `sample_id` | `int64` | sample id |
| `sample_order` | `int32` | sample order starting from 1 |
| `sample_input` | `string` | public sample input |
| `sample_output` | `string` | public sample output |

#### error response

- unknown `problem_id`: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

### `POST /api/problem/{problem_id}/sample`

Append one public sample to an existing problem. Samples are part of the statement content and are also returned by the public problem detail endpoint.

#### request

- content-type: `application/json`
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

#### success response

- status: `201 Created`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `sample_id` | `int64` | created sample id |
| `sample_order` | `int32` | assigned sample order |

Example:

```json
{
  "sample_id": 1,
  "sample_order": 1
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- unknown `problem_id`: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `PUT /api/problem/{problem_id}/sample/{sample_order}`

Replace one public sample of an existing problem. This endpoint is admin-only and increments the problem version.

#### request

- content-type: `application/json`
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameters:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |
| `sample_order` | `int32` | must be positive |

- body fields:

| field | type | required | note |
|---|---|---|---|
| `sample_input` | `string` | yes | may be empty |
| `sample_output` | `string` | yes | may be empty |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `sample_id` | `int64` | updated sample id |
| `sample_order` | `int32` | updated sample order |
| `sample_input` | `string` | updated sample input |
| `sample_output` | `string` | updated sample output |

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- invalid json: `400 Bad Request`
- missing required fields: `400 Bad Request`
- unknown `problem_id` or `sample_order`: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

### `DELETE /api/problem/{problem_id}/sample`

Delete the last public sample of an existing problem. Samples are part of the statement content, so this also changes the public problem detail payload and increments the problem version.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | always `problem sample deleted` |

Example:

```json
{
  "message": "problem sample deleted"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- unknown `problem_id` or missing sample to delete: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

Example:

```json
{
  "error": {
    "code": "invalid_sample_delete_request",
    "message": "failed to delete sample: invalid argument"
  }
}
```

### `GET /api/problem/{problem_id}/testcase`

List hidden testcases for an existing problem. This endpoint is admin-only.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `testcase_count` | `int64` | number of returned hidden testcases |
| `testcases` | `array<object>` | hidden testcase list ordered by `testcase_order` ascending |

Each testcase object contains:

| field | type | note |
|---|---|---|
| `testcase_id` | `int64` | testcase id |
| `testcase_order` | `int32` | testcase order starting from 1 |
| `testcase_input` | `string` | hidden testcase input |
| `testcase_output` | `string` | hidden testcase output |

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- unknown `problem_id`: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

### `POST /api/problem/{problem_id}/testcase`

Append one hidden testcase to an existing problem. The created testcase is not returned by the public problem detail endpoint, but the problem version is incremented so judge-side testcase sync can detect the update.

#### request

- content-type: `application/json`
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

- body fields:

| field | type | required | note |
|---|---|---|---|
| `testcase_input` | `string` | yes | may be empty for no-input problems |
| `testcase_output` | `string` | yes | may be empty for problems with empty output |

Example:

```json
{
  "testcase_input": "1 2\n",
  "testcase_output": "3\n"
}
```

#### success response

- status: `201 Created`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `testcase_id` | `int64` | created hidden testcase id |
| `testcase_order` | `int32` | appended order starting from 1 |

Example:

```json
{
  "testcase_id": 1,
  "testcase_order": 1
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- invalid json: `400 Bad Request`
- missing required fields: `400 Bad Request`
- unknown `problem_id`: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `POST /api/problem/{problem_id}/testcase/zip`

Replace all hidden testcases of an existing problem from a ZIP archive. This endpoint is admin-only. The uploaded archive replaces the full hidden testcase set in one operation, and the problem version is incremented once when the testcase set changes.

#### request

- content-type: `application/zip`
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

- body:
  - raw ZIP binary payload
  - allowed testcase file names: `001.in`, `001.out` through `999.in`, `999.out`
  - testcase numbers must start at `001` and be continuous without gaps
  - each testcase number must contain both `.in` and `.out`
  - empty files are allowed
  - any extra files, duplicate testcase files, or unmatched pairs are rejected

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | always `problem testcases uploaded` |
| `testcase_count` | `int32` | number of uploaded hidden testcases |

Example:

```json
{
  "message": "problem testcases uploaded",
  "testcase_count": 3
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- unknown `problem_id`: `404 Not Found`
- invalid zip payload or invalid testcase file layout: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

### `PUT /api/problem/{problem_id}/testcase/{testcase_order}`

Replace one hidden testcase of an existing problem. This endpoint is admin-only.

#### request

- content-type: `application/json`
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |
| `testcase_order` | `int32` | must be positive |

- body fields:

| field | type | required | note |
|---|---|---|---|
| `testcase_input` | `string` | yes | may be empty |
| `testcase_output` | `string` | yes | may be empty |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `testcase_id` | `int64` | updated testcase id |
| `testcase_order` | `int32` | updated testcase order |
| `testcase_input` | `string` | updated testcase input |
| `testcase_output` | `string` | updated testcase output |

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- invalid json: `400 Bad Request`
- missing required fields: `400 Bad Request`
- unknown `problem_id` or `testcase_order`: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

### `POST /api/problem/{problem_id}/testcase/move`

Move one hidden testcase to another order and shift the affected range. This endpoint is admin-only.

#### request

- content-type: `application/json`
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

- body fields:

| field | type | required | note |
|---|---|---|---|
| `source_testcase_order` | `int32` | yes | must be positive |
| `target_testcase_order` | `int32` | yes | must be positive |

Example:

```json
{
  "source_testcase_order": 5,
  "target_testcase_order": 2
}
```

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | always `problem testcase moved` |

Example:

```json
{
  "message": "problem testcase moved"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- invalid json: `400 Bad Request`
- invalid body fields: `400 Bad Request`
- unknown `problem_id`: `404 Not Found`
- invalid testcase move request such as unknown testcase order: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

### `DELETE /api/problem/{problem_id}/testcase/{testcase_order}`

Delete one hidden testcase of an existing problem and shift all later testcase orders down by one. This endpoint is admin-only.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |
| `testcase_order` | `int32` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | always `problem testcase deleted` |

Example:

```json
{
  "message": "problem testcase deleted"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- invalid `problem_id` or `testcase_order`, or no testcase exists at the requested order: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

Examples:

```json
{
  "error": {
    "code": "invalid_testcase_delete_request",
    "message": "failed to delete testcase: invalid argument"
  }
}
```

### `DELETE /api/problem/{problem_id}/testcase/all`

Delete all hidden testcases of an existing problem. If no hidden testcases exist, the request still succeeds as a no-op. When one or more hidden testcases are removed, the problem version is incremented so judge-side testcase sync can detect the update.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | always `problem testcases deleted` |

Example:

```json
{
  "message": "problem testcases deleted"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- unknown `problem_id`: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`
