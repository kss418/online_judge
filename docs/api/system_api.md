# system_api

HTTP API handled by `system_router`.

## endpoint

### `GET /api/system/health`

Health check endpoint.

#### request

- request body: none

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `message` | `string` | always `ok` |

Example:

```json
{
  "message": "ok"
}
```
