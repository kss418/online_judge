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
cmake -S backend -B backend/build -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=backend/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_INSTALLED_DIR=backend/vcpkg_installed

cmake --build backend/build --parallel 2 --target http_server judge_server
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

If your default `node` is too old, you can point the script to another install:

```bash
FRONTEND_BIN_DIR=/path/to/node/bin ./scripts/dev_tunnel.sh
```
