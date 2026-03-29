# user_schema

Base migration: `scripts/migrate_user_schema.sh`

## table

### `user_info`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `user_id` | `bigserial` | no | | pk; source of truth for user ids |
| `created_at` | `timestamptz` | no | `now()` | sign-up timestamp |
| `updated_at` | `timestamptz` | no | `now()` | user row update timestamp |

Constraints:

- `user_info_pkey`

## cross-schema relation

- `users.user_id -> user_info.user_id` is enforced when `auth_schema` is applied.

## shared table

### `schema_migrations`

Shared table used together with `auth_schema`, `submission_schema`, and `problem_schema`.

| column | type | nullable | default |
|---|---|---|---|
| `version` | `text` | no |  |
| `applied_at` | `timestamptz` | no | `now()` |
