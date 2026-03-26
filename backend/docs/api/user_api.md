# user_api

HTTP API handled by `user_router`.

## endpoint

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
| `users` | `array` | ordered by admin first, then `user_id` ascending |

Each item in `users` contains:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | user id |
| `user_name` | `string` | display name |
| `user_login_id` | `string \| null` | login id when available |
| `is_admin` | `boolean` | admin flag |
| `created_at` | `string` | sign-up timestamp |

Example:

```json
{
  "user_count": 2,
  "users": [
    {
      "user_id": 1,
      "user_name": "admin",
      "user_login_id": "admin",
      "is_admin": true,
      "created_at": "2026-03-26 13:10:11.000000+09"
    },
    {
      "user_id": 7,
      "user_name": "alice",
      "user_login_id": "alice",
      "is_admin": false,
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
| `user_name` | `string` | current display name |
| `is_admin` | `boolean` | current admin flag |

Example:

```json
{
  "id": 7,
  "user_name": "Alice",
  "is_admin": false
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

### `PUT /api/user/{user_id}/admin`

Promote an existing user to admin. This endpoint is admin-only.

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
| `user_id` | `int64` | promoted user id |
| `is_admin` | `boolean` | always `true` |

Example:

```json
{
  "user_id": 7,
  "is_admin": true
}
```

#### error response

- missing or malformed bearer token: `401 Unauthorized`
- invalid, expired, or revoked token: `401 Unauthorized`
- authenticated but not admin: `401 Unauthorized`
- unknown `user_id`: `404 Not Found`
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

```json
{
  "error": {
    "code": "user_not_found",
    "message": "user not found"
  }
}
```
