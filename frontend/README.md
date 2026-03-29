# Frontend

`frontend/` is a standalone Vue 3 + Vite SPA workspace for the online judge UI.

## Requirements

- Node.js `20.19+` or `22.12+`
- npm `10+`

If you use `nvm`, this workspace includes `frontend/.nvmrc` with `22.12.0`.

## First Run

```bash
cd frontend
cp .env.example .env.local
npm install
npm run dev
```

The dev server runs on `http://127.0.0.1:5173`.

## Backend Connection

By default the frontend uses:

- `VITE_API_BASE_URL=/api`
- `VITE_API_PROXY_TARGET=http://127.0.0.1:8080`

That means Vite proxies browser requests like `/api/system/health` to the backend server at `127.0.0.1:8080`, so you do not need separate CORS setup during development.

## Backend Commands

Build the backend binaries:

```bash
./backend/scripts/build_backend.sh
```

The script configures `backend/build` and builds with `BACKEND_BUILD_PARALLEL=2` by default.
If you need a different limit for your machine, override it explicitly:

```bash
BACKEND_BUILD_PARALLEL=1 ./backend/scripts/build_backend.sh
```

Run the HTTP API server:

```bash
cd backend
HTTP_PORT=8080 ./http_server
```

Then start the frontend:

```bash
cd frontend
npm run dev
```

## Cloudflare Tunnel Dev Script

If you want to open the backend, judge server, frontend dev server, and a Cloudflare quick tunnel in one shot, run:

```bash
./scripts/dev_tunnel.sh
```

The script:

- loads `backend/.env`
- checks the frontend Node.js version before starting any long-running process
- starts `backend/http_server`
- starts `backend/judge_server`
- creates a Cloudflare quick tunnel for `http://127.0.0.1:5173`
- passes the tunnel host into Vite `allowedHosts` automatically at runtime

You can stop everything with `Ctrl+C`.

If you want `dev_tunnel.sh` to rebuild the backend before starting, run:

```bash
DEV_TUNNEL_BUILD_BACKEND=1 ./scripts/dev_tunnel.sh
```

If your default `node` is too old, you can point the script to another install:

```bash
FRONTEND_BIN_DIR=/path/to/node/bin ./scripts/dev_tunnel.sh
```
