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

```text
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
