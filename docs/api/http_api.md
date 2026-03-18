# http_api

Current HTTP API exposed by `http_server`.

## endpoint

### `GET /api/system/health`

Health check endpoint.

#### request

- request body: none

#### success response

- status: `200 OK`
- content-type: `text/plain; charset=utf-8`
- body:

```text
ok
```

### `POST /api/auth/sign-up`

Create a local user and issue an auth token in the same request.

#### request

- content-type: `application/json`
- body fields:

| field | type | required | note |
|---|---|---|---|
| `user_login_id` | `string` | yes | must be non-empty |
| `raw_password` | `string` | yes | must be non-empty |

Example:

```json
{
  "user_login_id": "alice",
  "raw_password": "password123"
}
```

#### success response

- status: `201 Created`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | created user id |
| `is_admin` | `boolean` | currently `false` on sign-up |
| `token` | `string` | raw bearer token |

Example:

```json
{
  "user_id": 1,
  "is_admin": false,
  "token": "..."
}
```

#### error response

- invalid or empty request body: `400 Bad Request` with `invalid json`
- missing required fields: `400 Bad Request`
- duplicate `user_login_id`: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are currently returned as plain text.

Examples:

invalid json
```

```text
required fields: user_login_id, raw_password
```

```text
failed to sign up: invalid argument
```

### `POST /api/auth/login`

Authenticate an existing local user and issue a new auth token.

#### request

- content-type: `application/json`
- body fields:

| field | type | required | note |
|---|---|---|---|
| `user_login_id` | `string` | yes | must be non-empty |
| `raw_password` | `string` | yes | must be non-empty |

Example:

```json
{
  "user_login_id": "alice",
  "raw_password": "password123"
}
```

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | authenticated user id |
| `is_admin` | `boolean` | current admin flag |
| `token` | `string` | newly issued raw bearer token |

Example:

```json
{
  "user_id": 1,
  "is_admin": false,
  "token": "..."
}
```

#### error response

- invalid or empty request body: `400 Bad Request` with `invalid json`
- missing required fields: `400 Bad Request`
- wrong `user_login_id` or `raw_password`: `401 Unauthorized`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are currently returned as plain text.

Examples:

```text
invalid credentials
```

### `POST /api/auth/logout`

Revoke the current bearer token.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <token>` |

Example:

```text
Authorization: Bearer ...
```

#### success response

- status: `200 OK`
- content-type: `text/plain; charset=utf-8`
- body:

```text
logged out
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are currently returned as plain text.

Examples:

```text
missing or invalid bearer token
```

```text
invalid, expired, or revoked token
```

### `POST /api/auth/token/renew`

Extend the expiration time of the current bearer token.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <token>` |

Example:

```text
Authorization: Bearer ...
```

#### success response

- status: `200 OK`
- content-type: `text/plain; charset=utf-8`
- body:

```text
token renewed
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are currently returned as plain text.

Examples:

```text
missing or invalid bearer token
```

```text
invalid, expired, or revoked token
```

### `POST /api/submission`

Create a new submission for a problem. The user-visible problem number is the same as the database `problem_id`.

#### request

- content-type: `application/json`
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <token>` |

- body fields:

| field | type | required | note |
|---|---|---|---|
| `problem_id` | `int64` | yes | must be positive |
| `language` | `string` | yes | must be non-empty |
| `source_code` | `string` | yes | must be non-empty |

Example:

```json
{
  "problem_id": 1000,
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

Error bodies are currently returned as plain text.

Examples:

```text
required fields: problem_id, language, source_code
```

```text
failed to create submission: foreign key violation
```

### `GET /api/problem/{problem_id}`

Get a public problem detail view. This endpoint returns visible metadata, limits, statement content when present, public samples, and aggregate submission statistics. Hidden testcases are not exposed.

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
| `problem_id` | `int64` | requested problem id |
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

Example:

```json
{
  "problem_id": 1000,
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
  }
}
```

#### error response

- unknown `problem_id`: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are currently returned as plain text.

Examples:

```text
problem not found
```

### `POST /api/problem`

Create a new problem and return the generated problem number. The problem number matches the database `problem_id` generated by the `bigserial` sequence. New problems are initialized with default limits: `time_limit_ms = 1000` and `memory_limit_mb = 256`.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

Example:

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
- unexpected internal failure: `500 Internal Server Error`

Error bodies are currently returned as plain text.

Examples:

```text
admin bearer token required
```

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
- content-type: `text/plain; charset=utf-8`
- body:

```text
problem limits updated
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

Error bodies are currently returned as plain text.

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
- content-type: `text/plain; charset=utf-8`
- body:

```text
problem statement updated
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- invalid json: `400 Bad Request`
- missing required fields: `400 Bad Request`
- invalid `note` type: `400 Bad Request`
- unknown `problem_id`: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are currently returned as plain text.

## note

- Current HTTP routes are defined in [`http_dispatcher.hpp`](/home/kss418/online_judge/include/http_server/http_dispatcher.hpp).
- Auth request handling logic lives in [`auth_handler.cpp`](/home/kss418/online_judge/src/http_handler/auth_handler.cpp).
- Problem request handling logic lives in [`problem_handler.cpp`](/home/kss418/online_judge/src/http_handler/problem_handler.cpp).
- Submission request handling logic lives in [`submission_handler.cpp`](/home/kss418/online_judge/src/http_handler/submission_handler.cpp).
