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
| `users` | `array` | ordered by `permission_level` descending, then `user_id` ascending |

Each item in `users` contains:

| field | type | note |
|---|---|---|
| `user_id` | `int64` | user id |
| `user_name` | `string` | display name |
| `user_login_id` | `string \| null` | login id when available |
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
      "user_name": "admin",
      "user_login_id": "admin",
      "permission_level": 2,
      "role_name": "superadmin",
      "created_at": "2026-03-26 13:10:11.000000+09"
    },
    {
      "user_id": 7,
      "user_name": "alice",
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
| `permission_level` | `int32` | current permission level |
| `role_name` | `string` | derived role label |

Example:

```json
{
  "id": 7,
  "user_name": "Alice",
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
    "code": "user_not_found",
    "message": "user not found"
  }
}
```
