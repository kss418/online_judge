# submission_api

HTTP API handled by `submission_router`.

## endpoint

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
