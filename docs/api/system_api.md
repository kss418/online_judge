# system_api

HTTP API handled by `system_router`.

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
