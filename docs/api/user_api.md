# user_api

HTTP API handled by `user_router`.

## endpoint

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
