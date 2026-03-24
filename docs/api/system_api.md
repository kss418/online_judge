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

### `GET /api/system/supported-languages`

List supported submission languages. This endpoint is public and does not require authentication.

#### request

- request body: none

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `language_count` | `int64` | number of supported languages |
| `languages` | `array<object>` | supported language metadata |
| `languages[].language` | `string` | submission language code |
| `languages[].source_extension` | `string` | source file extension used by the judge |

Example:

```json
{
  "language_count": 3,
  "languages": [
    {
      "language": "cpp",
      "source_extension": ".cpp"
    },
    {
      "language": "python",
      "source_extension": ".py"
    },
    {
      "language": "java",
      "source_extension": ".java"
    }
  ]
}
```
