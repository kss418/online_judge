# user_api

HTTP API handled by `user_router`.

## endpoint

### `GET /api/user/list`

Get the public user list. This endpoint supports search by login id substring and excludes superadmin accounts.

#### request

- request body: none
- required header: none
- optional query:

| key | type | note |
|---|---|---|
| `q` | `string` | filters by `user_login_id` substring |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `user_count` | `int64` | total users in the response |
| `users` | `array` | ordered by `user_id` ascending |

Each item in `users` contains:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | account number |
| `user_login_id` | `string` | login identifier |
| `solved_problem_count` | `int64` | number of solved problems |
| `accepted_submission_count` | `int64` | total accepted submissions |
| `submission_count` | `int64` | total submissions |
| `created_at` | `string` | sign-up timestamp |

Example:

```json
{
  "user_count": 2,
  "users": [
    {
      "user_id": 1,
      "user_login_id": "alice",
      "solved_problem_count": 12,
      "accepted_submission_count": 24,
      "submission_count": 48,
      "created_at": "2026-03-26 13:14:02.000000+09"
    },
    {
      "user_id": 2,
      "user_login_id": "bob",
      "solved_problem_count": 0,
      "accepted_submission_count": 0,
      "submission_count": 0,
      "created_at": "2026-03-26 13:15:17.000000+09"
    }
  ]
}
```

#### error response

- invalid query string or unsupported query parameter: `400 Bad Request`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `GET /api/user/id/{user_login_id}`

Get the public summary for a single user by login id.

#### request

- request body: none
- required header: none

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | account number |
| `user_login_id` | `string` | login identifier |
| `created_at` | `string` | sign-up timestamp |

Example:

```json
{
  "user_id": 7,
  "user_login_id": "alice",
  "created_at": "2026-03-26 13:14:02.000000+09"
}
```

#### error response

- unknown user id: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `GET /api/user/{user_id}/statistics`

Get submission statistics for a single user by id.

#### request

- request body: none
- required header: none

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | requested user id |
| `submission_count` | `int64` | total submissions |
| `queued_submission_count` | `int64` | queued submissions |
| `judging_submission_count` | `int64` | judging submissions |
| `accepted_submission_count` | `int64` | accepted submissions |
| `wrong_answer_submission_count` | `int64` | wrong answer submissions |
| `time_limit_exceeded_submission_count` | `int64` | time limit exceeded submissions |
| `memory_limit_exceeded_submission_count` | `int64` | memory limit exceeded submissions |
| `runtime_error_submission_count` | `int64` | runtime error submissions |
| `compile_error_submission_count` | `int64` | compile error submissions |
| `output_exceeded_submission_count` | `int64` | output exceeded submissions |
| `infra_failure_submission_count` | `int64` | infrastructure failure submissions |
| `last_submission_at` | `string \| null` | timestamp of the most recent submission |
| `last_accepted_at` | `string \| null` | timestamp of the most recent accepted submission |
| `updated_at` | `string` | statistics row updated timestamp |

#### error response

- unknown user id: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `GET /api/user/{user_id}/solved-problems`

Get the solved problem list for a single user by id.

#### request

- request body: none
- required header: none

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `solved_problem_count` | `int64` | total solved problems in the response |
| `solved_problems` | `array` | solved problem summaries ordered by `problem_id` descending |

Each item in `solved_problems` contains:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | solved problem id |
| `title` | `string` | problem title |
| `version` | `int32` | current problem version |
| `submission_count` | `int64` | total submissions on the problem |
| `accepted_count` | `int64` | total accepted submissions on the problem |
| `user_problem_state` | `string` | always `solved` |

#### error response

- unknown user id: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `GET /api/user/{user_id}/wrong-problems`

Get the unresolved wrong-problem list for a single user by id.

#### request

- request body: none
- required header: none

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `wrong_problem_count` | `int64` | total wrong problems in the response |
| `wrong_problems` | `array` | wrong problem summaries ordered by `problem_id` descending |

Each item in `wrong_problems` contains:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | wrong problem id |
| `title` | `string` | problem title |
| `version` | `int32` | current problem version |
| `submission_count` | `int64` | total submissions on the problem |
| `accepted_count` | `int64` | total accepted submissions on the problem |
| `user_problem_state` | `string` | always `wrong` |

#### error response

- unknown user id: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `GET /api/user`

Get the full user list for management. This endpoint is admin-only.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `user_count` | `int64` | total user count in the response |
| `users` | `array` | ordered by `permission_level` descending, then `user_id` ascending |

Each item in `users` contains:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | user id |
| `user_login_id` | `string` | login identifier |
| `permission_level` | `int32` | numeric permission level |
| `role_name` | `string` | derived role label |
| `created_at` | `string` | sign-up timestamp |

Example:

```json
{
  "user_count": 2,
  "users": [
    {
      "user_id": 1,
      "user_login_id": "admin",
      "permission_level": 2,
      "role_name": "superadmin",
      "created_at": "2026-03-26 13:10:11.000000+09"
    },
    {
      "user_id": 7,
      "user_login_id": "alice",
      "permission_level": 0,
      "role_name": "user",
      "created_at": "2026-03-26 13:14:02.000000+09"
    }
  ]
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `GET /api/user/{user_id}/submission-ban`

Get the current submission-ban timestamp for a single user. This endpoint is admin-only.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | requested user id |
| `submission_banned_until` | `string \| null` | current submission-ban expiry timestamp, or `null` when not set |

Example:

```json
{
  "user_id": 7,
  "submission_banned_until": "2026-03-29 16:42:31.123456+09"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- unknown `user_id`: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `GET /api/user/me`

Get the currently authenticated user profile. This endpoint requires a bearer token.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <token>` |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `id` | `int64` | authenticated user id |
| `user_login_id` | `string` | current login identifier |
| `permission_level` | `int32` | current permission level |
| `role_name` | `string` | derived role label |

Example:

```json
{
  "id": 7,
  "user_login_id": "alice",
  "permission_level": 1,
  "role_name": "admin"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
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
    "code": "invalid_or_expired_token",
    "message": "invalid, expired, or revoked token"
  }
}
```

### `GET /api/user/me/submission-ban`

Get the current submission-ban timestamp for the authenticated user.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <token>` |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | authenticated user id |
| `submission_banned_until` | `string \| null` | current submission-ban expiry timestamp, or `null` when not set |

Example:

```json
{
  "user_id": 7,
  "submission_banned_until": "2026-03-29 16:42:31.123456+09"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `GET /api/user/me/statistics`

Get submission statistics for the currently authenticated user. This endpoint requires a bearer token.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <token>` |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | authenticated user id |
| `submission_count` | `int64` | total submissions |
| `queued_submission_count` | `int64` | queued submissions |
| `judging_submission_count` | `int64` | judging submissions |
| `accepted_submission_count` | `int64` | accepted submissions |
| `wrong_answer_submission_count` | `int64` | wrong answer submissions |
| `time_limit_exceeded_submission_count` | `int64` | time limit exceeded submissions |
| `memory_limit_exceeded_submission_count` | `int64` | memory limit exceeded submissions |
| `runtime_error_submission_count` | `int64` | runtime error submissions |
| `compile_error_submission_count` | `int64` | compile error submissions |
| `output_exceeded_submission_count` | `int64` | output exceeded submissions |
| `infra_failure_submission_count` | `int64` | infrastructure failure submissions |
| `last_submission_at` | `string \| null` | timestamp of the most recent submission |
| `last_accepted_at` | `string \| null` | timestamp of the most recent accepted submission |
| `updated_at` | `string` | statistics row updated timestamp |

Example:

```json
{
  "user_id": 7,
  "submission_count": 12,
  "queued_submission_count": 1,
  "judging_submission_count": 0,
  "accepted_submission_count": 3,
  "wrong_answer_submission_count": 4,
  "time_limit_exceeded_submission_count": 1,
  "memory_limit_exceeded_submission_count": 1,
  "runtime_error_submission_count": 2,
  "compile_error_submission_count": 1,
  "output_exceeded_submission_count": 0,
  "infra_failure_submission_count": 0,
  "last_submission_at": "2026-03-28 17:10:11.000000+09",
  "last_accepted_at": "2026-03-28 16:42:08.000000+09",
  "updated_at": "2026-03-28 17:10:11.000000+09"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `GET /api/user/me/solved-problems`

Get the solved problem list for the currently authenticated user. This endpoint requires a bearer token.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <token>` |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `solved_problem_count` | `int64` | total solved problems in the response |
| `solved_problems` | `array` | solved problem summaries ordered by `problem_id` descending |

Each item in `solved_problems` contains:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | solved problem id |
| `title` | `string` | problem title |
| `version` | `int32` | current problem version |
| `submission_count` | `int64` | total submissions on the problem |
| `accepted_count` | `int64` | total accepted submissions on the problem |
| `user_problem_state` | `string` | always `solved` |

Example:

```json
{
  "solved_problem_count": 2,
  "solved_problems": [
    {
      "problem_id": 15,
      "title": "A + B",
      "version": 3,
      "submission_count": 124,
      "accepted_count": 61,
      "user_problem_state": "solved"
    },
    {
      "problem_id": 3,
      "title": "Hello Output",
      "version": 1,
      "submission_count": 44,
      "accepted_count": 38,
      "user_problem_state": "solved"
    }
  ]
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `GET /api/user/me/wrong-problems`

Get the wrong problem list for the currently authenticated user. This endpoint requires a bearer token.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <token>` |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `wrong_problem_count` | `int64` | total wrong problems in the response |
| `wrong_problems` | `array` | wrong problem summaries ordered by `problem_id` descending |

Each item in `wrong_problems` contains:

| field | type | note |
|---|---|---|
| `problem_id` | `int64` | wrong problem id |
| `title` | `string` | problem title |
| `version` | `int32` | current problem version |
| `submission_count` | `int64` | total submissions on the problem |
| `accepted_count` | `int64` | total accepted submissions on the problem |
| `user_problem_state` | `string` | always `wrong` |

Example:

```json
{
  "wrong_problem_count": 2,
  "wrong_problems": [
    {
      "problem_id": 11,
      "title": "Stack Machine",
      "version": 2,
      "submission_count": 98,
      "accepted_count": 21,
      "user_problem_state": "wrong"
    },
    {
      "problem_id": 2,
      "title": "Echo Until EOF",
      "version": 1,
      "submission_count": 40,
      "accepted_count": 19,
      "user_problem_state": "wrong"
    }
  ]
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

### `PUT /api/user/{user_id}/admin`

Promote an existing user to the `admin` role. This endpoint is superadmin-only.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <superadmin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | updated user id |
| `permission_level` | `int32` | always `1` |
| `role_name` | `string` | always `admin` |

Example:

```json
{
  "user_id": 7,
  "permission_level": 1,
  "role_name": "admin"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not superadmin: `401 Unauthorized`
- unknown `user_id`: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

### `PUT /api/user/{user_id}/user`

Demote an existing user to the regular `user` role. This endpoint is superadmin-only.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <superadmin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | updated user id |
| `permission_level` | `int32` | always `0` |
| `role_name` | `string` | always `user` |

Example:

```json
{
  "user_id": 7,
  "permission_level": 0,
  "role_name": "user"
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not superadmin: `401 Unauthorized`
- unknown `user_id`: `404 Not Found`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

Examples:

```json
{
  "error": {
    "code": "superadmin_bearer_token_required",
    "message": "superadmin bearer token required"
  }
}
```

```json
{
  "error": {
    "code": "not_found",
    "message": "not found"
  }
}
```

### `POST /api/user/{user_id}/submission-ban`

Set or refresh a user's submission ban for a duration in minutes. This endpoint is admin-only.

#### request

- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | must be positive |

- request body:

| field | type | note |
|---|---|---|
| `duration_minutes` | `int32` | must be greater than `0` |

#### success response

- status: `201 Created`
- content-type: `application/json; charset=utf-8`

| field | type | note |
|---|---|---|
| `user_id` | `int64` | updated user id |
| `duration_minutes` | `int32` | applied duration |
| `submission_banned_until` | `string` | computed ban expiry timestamp |

### `DELETE /api/user/{user_id}/submission-ban`

Clear a user's submission ban. This endpoint is admin-only.

#### request

- request body: none
- required header:

| header | required | note |
|---|---|---|
| `Authorization` | yes | format: `Bearer <admin-token>` |

- path parameter:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | must be positive |

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`

| field | type | note |
|---|---|---|
| `message` | `string` | always `user submission ban cleared` |
