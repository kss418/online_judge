# auth_api

HTTP API handled by `auth_router`.

## endpoint

### `POST /api/auth/sign-up`

Create a local user and issue an auth token in the same request.

#### request

- content-type: `application/json`
- body fields:

| field | type | required | note |
|---|---|---|---|
| `user_login_id` | `string` | yes | must be 4 to 15 characters |
| `raw_password` | `string` | yes | must be 4 to 15 characters |

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
| `permission_level` | `int32` | currently `0` on sign-up |
| `role_name` | `string` | currently `user` on sign-up |
| `user_login_id` | `string` | login identifier |
| `token` | `string` | raw bearer token |

Example:

```json
{
  "user_id": 1,
  "permission_level": 0,
  "role_name": "user",
  "user_login_id": "alice",
  "token": "..."
}
```

#### error response

- invalid or empty request body: `400 Bad Request` with `invalid json`
- missing required fields: `400 Bad Request`
- invalid `user_login_id` or `raw_password` length: `400 Bad Request`
- duplicate `user_login_id`: `409 Conflict`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

Examples:

```json
{
  "error": {
    "code": "invalid_json",
    "message": "invalid json"
  }
}
```

```json
{
  "error": {
    "code": "missing_field",
    "message": "required field: user_login_id",
    "field": "user_login_id"
  }
}
```

```json
{
  "error": {
    "code": "invalid_length",
    "message": "user_login_id must be between 4 and 15 characters",
    "field": "user_login_id"
  }
}
```

```json
{
  "error": {
    "code": "conflict",
    "message": "conflict"
  }
}
```

### `POST /api/auth/login`

Authenticate an existing local user and issue a new auth token.

#### request

- content-type: `application/json`
- body fields:

| field | type | required | note |
|---|---|---|---|
| `user_login_id` | `string` | yes | must be 4 to 15 characters |
| `raw_password` | `string` | yes | must be 4 to 15 characters |

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
| `permission_level` | `int32` | current permission level |
| `role_name` | `string` | derived role label |
| `user_login_id` | `string` | login identifier |
| `token` | `string` | newly issued raw bearer token |

Example:

```json
{
  "user_id": 1,
  "permission_level": 1,
  "role_name": "admin",
  "user_login_id": "alice",
  "token": "..."
}
```

#### error response

- invalid or empty request body: `400 Bad Request` with `invalid json`
- missing required fields: `400 Bad Request`
- invalid `user_login_id` or `raw_password` length: `400 Bad Request`
- wrong `user_login_id` or `raw_password`: `401 Unauthorized`
- unexpected internal failure: `500 Internal Server Error`

Error bodies are returned as JSON with an `error` object containing `code`, `message`, and an optional `field`.

Examples:

```json
{
  "error": {
    "code": "invalid_credentials",
    "message": "invalid credentials"
  }
}
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
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | always `logged out` |

Example:

```json
{
  "message": "logged out"
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
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | always `token renewed` |

Example:

```json
{
  "message": "token renewed"
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
