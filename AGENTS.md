# AGENTS.md

## Project Goal
- Build an online judge using C++ with Boost as a core dependency.
- Prioritize maintainable server architecture, clear module boundaries, and testability.

## Naming Convention
- Primary convention: `lower_snake_case`.
- Apply `lower_snake_case` to:
  - namespaces
  - classes/structs/types
  - functions/methods
  - variables/parameters
  - filenames/directories
- Macros/constants use `UPPER_SNAKE_CASE`.

## Collaboration Rule For Codex
- For every code change, verify naming matches the convention above.
- If a naming mismatch is found, fix it in the same change when possible.
- If immediate fix is risky (API break, large refactor), report it explicitly with a safe migration suggestion.

## Command Permission Policy
- Database and network-related commands must be executed with elevated permission requests.
- All `psql` commands must be executed with `sandbox_permissions: "require_escalated"`.
- Use `sandbox_permissions: "require_escalated"` for those commands.
- Always submit the command for user approval first (with a short justification question), then run only after approval.

## C++ Brace Style
- Use `){` with no space between `)` and `{` (for function definitions, conditionals, loops, and lambdas).
- Use `const{` with no space between `const` and `{` in function definitions.

## C++ Namespace Implementation Style
- In `.cpp` files, do not implement functions inside a `namespace ... { ... }` block.
- Define namespace functions with the `namespace_name::function_name(...)` form instead.

## Expected Variable Naming
- When receiving `std::expected`, name variables with the `_exp` suffix (for example, `run_exp`).
- When receiving `std::optional`, name variables with the `_opt` suffix (for example, `route_handler_opt`).
- When using iterators, name variables with the `_it` suffix (for example, `authorization_field_it`).
- Do not use the `_result` suffix for `std::expected` variables.

## HTTP API Path Convention
- Use `/api/...` for HTTP API endpoints.
- Do not use versioned paths like `/api/v1/...` unless explicitly requested.
