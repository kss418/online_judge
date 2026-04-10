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

### `GET /api/system/status`

Admin-only operational status endpoint.

#### request

- request body: none
- authentication: admin bearer token required

#### success response

- status: `200 OK`
- content-type: `application/json; charset=utf-8`
- body fields:

| field | type | note |
|---|---|---|
| `http` | `object` | current HTTP server runtime snapshot |
| `http.db_pool` | `object` | DB pool capacity snapshot |
| `http.db_pool.size` | `int64` | configured HTTP DB pool size |
| `http.db_pool.available_count` | `int64` | immediately available connection leases |
| `http.handler_queue` | `object` | handler worker backlog snapshot |
| `http.handler_queue.backlog_count` | `int64` | pending handler task count |
| `http.handler_queue.limit` | `int64 \| null` | configured queue limit, or `null` when unbounded |
| `judge` | `object` | aggregate judge runtime snapshot |
| `judge.queue_depth` | `int64` | ready `submission_queue` backlog count |
| `judge.live_instance_count` | `int64` | live judge process count |
| `judge.configured_worker_count` | `int64` | sum of configured workers across live judge processes |
| `judge.active_worker_count` | `int64` | sum of currently busy workers across live judge processes |
| `judge.snapshot_cache` | `object` | aggregate testcase snapshot cache counters across live judge processes |
| `judge.snapshot_cache.hit_count` | `int64` | cumulative cache hit count |
| `judge.snapshot_cache.miss_count` | `int64` | cumulative cache miss count |
| `judge.last_sandbox_self_check` | `object` | latest reported sandbox startup self-check result |
| `judge.last_sandbox_self_check.status` | `string` | `passed`, `failed`, or `unknown` |
| `judge.last_sandbox_self_check.checked_at` | `string \| null` | latest self-check timestamp |
| `judge.last_sandbox_self_check.message` | `string \| null` | failure detail or informational message |

Example:

```json
{
  "http": {
    "db_pool": {
      "size": 16,
      "available_count": 11
    },
    "handler_queue": {
      "backlog_count": 3,
      "limit": 32
    }
  },
  "judge": {
    "queue_depth": 14,
    "live_instance_count": 1,
    "configured_worker_count": 8,
    "active_worker_count": 2,
    "snapshot_cache": {
      "hit_count": 1024,
      "miss_count": 18
    },
    "last_sandbox_self_check": {
      "status": "passed",
      "checked_at": "2026-04-10 03:12:45+00",
      "message": null
    }
  }
}
```
