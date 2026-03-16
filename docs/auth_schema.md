# auth_schema

Base migration: `scripts/migrate_auth_schema.sh`

## table

### `users`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `user_id` | `bigint` | no | | pk |
| `is_admin` | `boolean` | no | `false` | source of truth for admin privilege |
| `created_at` | `timestamptz` | no | `now()` |  |
| `updated_at` | `timestamptz` | no | `now()` |  |

Constraints:

- `users_pkey`

### `auth_tokens`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `token_id` | `bigserial` | no | | pk |
| `user_id` | `bigint` | no | | fk -> `users(user_id)` on delete cascade |
| `token_hash` | `text` | no | | output of `token_util::hash_token()`; application currently writes lowercase SHA-512 hex |
| `issued_at` | `timestamptz` | no | `now()` |  |
| `expires_at` | `timestamptz` | no | | token expiration time |
| `revoked_at` | `timestamptz` | yes | | null means not explicitly revoked |
| `last_used_at` | `timestamptz` | yes | | updated when token is successfully used |

Constraints:

- `auth_tokens_pkey`
- `auth_tokens_user_id_fkey`
- `auth_tokens_token_hash_unique`
- `auth_tokens_token_hash_not_blank`
- `auth_tokens_expires_at_order_check`
- `auth_tokens_last_used_at_order_check`
- `auth_tokens_revoked_at_order_check`

Indexes:

- `auth_tokens_user_issued_idx (user_id, issued_at desc)`
- `auth_tokens_expires_at_idx (expires_at)`
- `auth_tokens_active_user_expires_idx (user_id, expires_at desc) where revoked_at is null`

## cross-schema relation

- `auth_tokens.user_id -> users.user_id`
- Existing `submissions.user_id` values are backfilled into `users.user_id` when the auth migration runs and `submissions` already exists.
- `submissions.user_id -> users.user_id` is enforced when both schemas are applied.

## shared table

### `schema_migrations`

Shared table used together with `submission_schema` and `problem_schema`.

| column | type | nullable | default |
|---|---|---|---|
| `version` | `text` | no |  |
| `applied_at` | `timestamptz` | no | `now()` |
