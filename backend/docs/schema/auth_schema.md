# auth_schema

Base migration: `scripts/migrate_auth_schema.sh`

## table

### `users`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `user_id` | `bigserial` | no | | pk |
| `user_name` | `text` | no | | display name for the user |
| `user_login_id` | `text` | yes | | login identifier for local auth |
| `user_password_hash` | `text` | yes | | password hash for local auth |
| `is_admin` | `boolean` | no | `false` | source of truth for admin privilege |
| `created_at` | `timestamptz` | no | `now()` |  |
| `updated_at` | `timestamptz` | no | `now()` |  |

Constraints:

- `users_pkey`
- `users_user_name_not_blank`
- `users_user_login_id_not_blank`
- `users_user_password_hash_not_blank`

Indexes:

- `users_user_name_unique_idx (user_name)`
- `users_user_login_id_unique_idx (user_login_id) where user_login_id is not null`

### `auth_tokens`

| column | type | nullable | default | note |
|---|---|---|---|---|
| `token_id` | `bigserial` | no | | pk |
| `user_id` | `bigint` | no | | fk -> `users(user_id)` on delete cascade |
| `token_hash` | `text` | no | | output of `crypto_util::sha512_hex()`; application currently writes lowercase SHA-512 hex |
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
- `submissions.user_id -> users.user_id` is enforced when both schemas are applied.

## shared table

### `schema_migrations`

Shared table used together with `submission_schema` and `problem_schema`.

| column | type | nullable | default |
|---|---|---|---|
| `version` | `text` | no |  |
| `applied_at` | `timestamptz` | no | `now()` |
