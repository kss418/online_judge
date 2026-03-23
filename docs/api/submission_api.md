# submission_api

HTTP API handled by `submission_router`.

## endpoint

### `POST /api/submission/{problem_id}`

Create a new submission for a problem. The user-visible problem number is the same as the database `problem_id`.

#### request

- content-type: `application/json`
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | must be positive |

- body fields:

| field | type | required | note |
|---|---|---|---|
| `language` | `string` | yes | must be non-empty |
| `source_code` | `string` | yes | must be non-empty |

Example:

```json
{
  "language": "cpp",
  "source_code": "#include <iostream>\nint main(){ return 0; }\n"
}
```

#### success response

- status: `201 Created`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `submission_id` | `int64` | created submission id |
| `status` | `string` | initial submission status, currently `queued` |

Example:

```json
{
  "submission_id": 1,
  "status": "queued"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- invalid or empty request body: `400 Bad Request`
- missing required fields: `400 Bad Request`
- invalid `problem_id` or unknown problem: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

Examples:

```json
{
  "error": {
    "code": "missing_field",
    "message": "required field: language",
    "field": "language"
  }
}
```

```json
{
  "error": {
    "code": "bad_request",
    "message": "failed to create submission: invalid argument"
  }
}
```

### `GET /api/submission/{submission_id}`

Get a single submission detail view. This endpoint is public and does not require authentication.

#### request

- request body: none
- required header: none
- path parameter:

| field | type | note |
|---|---|---|
| `submission_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `submission_id` | `int64` | submission id |
| `user_id` | `int64` | submitter user id |
| `problem_id` | `int64` | related problem id |
| `language` | `string` | submitted language |
| `status` | `string` | current submission status |
| `score` | `int16 \| null` | score, null before judging finishes |
| `compile_output` | `string \| null` | compiler stderr when compilation fails |
| `judge_output` | `string \| null` | runtime or judge stderr when available |
| `elapsed_ms` | `int64 \| null` | max elapsed wall-clock time among executed testcases; `null` before execution starts or when compilation fails |
| `max_rss_kb` | `int64 \| null` | max resident set size among executed testcases in kilobytes; `null` before execution starts or when compilation fails |
| `created_at` | `string` | creation timestamp |
| `updated_at` | `string` | last update timestamp |

The detail response does not expose `source_code`. Use `GET /api/submission/{submission_id}/source` for owner/admin-only source access.

Example:

```json
{
  "submission_id": 12,
  "user_id": 7,
  "problem_id": 3,
  "language": "cpp",
  "status": "compile_error",
  "score": 0,
  "compile_output": "main.cpp: In function ...",
  "judge_output": null,
  "elapsed_ms": null,
  "max_rss_kb": null,
  "created_at": "2026-03-19 14:02:11.123456+09",
  "updated_at": "2026-03-19 14:02:12.123456+09"
}
```

#### error response

- unknown `submission_id`: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

Example:

```json
{
  "error": {
    "code": "not_found",
    "message": "failed to get submission detail: invalid argument"
  }
}
```

### `GET /api/submission/{submission_id}/source`

Get the submitted source code for a single submission. This endpoint requires a bearer token, and only the submission owner or an admin can access it.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `submission_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `submission_id` | `int64` | submission id |
| `user_id` | `int64` | submitter user id |
| `problem_id` | `int64` | related problem id |
| `language` | `string` | submitted language |
| `source_code` | `string` | exact submitted source code |
| `compile_output` | `string \| null` | compiler stderr when compilation fails |
| `judge_output` | `string \| null` | runtime or judge stderr when available |

Example:

```json
{
  "submission_id": 12,
  "user_id": 7,
  "problem_id": 3,
  "language": "cpp",
  "source_code": "#include <iostream>\nint main(){ return 0; }\n",
  "compile_output": null,
  "judge_output": null
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not owner/admin: `403 Forbidden`
- unknown `submission_id`: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

Examples:

```json
{
  "error": {
    "code": "missing_or_invalid_bearer_token",
    "message": "missing or invalid bearer token"
  }
}
```

```json
{
  "error": {
    "code": "forbidden",
    "message": "submission source access denied"
  }
}
```

### `GET /api/submission?user_id=...&problem_id=...&status=...`

List submissions using optional filters. This endpoint is public and does not require authentication.

#### request

- required header: none
- supported query parameters:

| field | type | required | note |
|---|---|---|---|
| `top` | `int64` | no | return submissions with `submission_id <= top`; must be positive |
| `user_id` | `int64` | no | must be positive |
| `problem_id` | `int64` | no | must be positive |
| `status` | `string` | no | one of `queued`, `judging`, `accepted`, `wrong_answer`, `time_limit_exceeded`, `memory_limit_exceeded`, `runtime_error`, `compile_error`, `output_exceeded` |

All query parameters are optional. If omitted, the endpoint returns the full submission list. Parameters can be combined.
The response always returns at most 20 submissions, ordered by `submission_id` descending.
If `top` is omitted, the latest submission id is used as the starting point.

Example:

```text
GET /api/submission?top=120&user_id=7&problem_id=3&status=accepted
```

```text
GET /api/submission
```

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `submission_count` | `int64` | number of returned submissions, maximum 20 |
| `submissions` | `array` | newest-first submission summaries, maximum 20 items |

Each submission summary contains:

| field | type | note |
|---|---|---|
| `submission_id` | `int64` | submission id |
| `user_id` | `int64` | submitter user id |
| `problem_id` | `int64` | related problem id |
| `language` | `string` | submitted language |
| `status` | `string` | current submission status |
| `score` | `int16 \| null` | score, null before judging finishes |
| `elapsed_ms` | `int64 \| null` | max elapsed wall-clock time among executed testcases; `null` before execution starts or when compilation fails |
| `max_rss_kb` | `int64 \| null` | max resident set size among executed testcases in kilobytes; `null` before execution starts or when compilation fails |
| `created_at` | `string` | creation timestamp |
| `updated_at` | `string` | last update timestamp |

Example:

```json
{
  "submission_count": 2,
  "submissions": [
    {
      "submission_id": 12,
      "user_id": 7,
      "problem_id": 3,
      "language": "cpp",
      "status": "accepted",
      "score": 100,
      "elapsed_ms": 612,
      "max_rss_kb": 18432,
      "created_at": "2026-03-19 14:02:11.123456+09",
      "updated_at": "2026-03-19 14:02:12.123456+09"
    },
    {
      "submission_id": 9,
      "user_id": 7,
      "problem_id": 3,
      "language": "python",
      "status": "accepted",
      "score": 100,
      "elapsed_ms": 31,
      "max_rss_kb": 12288,
      "created_at": "2026-03-19 13:58:44.000000+09",
      "updated_at": "2026-03-19 13:58:45.000000+09"
    }
  ]
}
```

#### error response

- invalid query string or duplicate query parameter: `400 Bad Request`
- invalid query parameter value: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

If no submissions match the filters, the endpoint returns `200 OK` with an empty `submissions` array.
