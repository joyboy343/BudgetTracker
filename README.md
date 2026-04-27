# Ledger — Personal Finance Web Application

A full-stack budgeting application with a **C++17 REST API backend** and a **React + TypeScript frontend**. Track income and expenses, set monthly budgets, visualize spending trends, and export reports.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Technology Choices & Justification](#technology-choices--justification)
3. [Prerequisites](#prerequisites)
4. [Quick Start (Docker — Recommended)](#quick-start-docker--recommended)
5. [Local Development (Without Docker)](#local-development-without-docker)
   - [Building the C++ Backend](#building-the-c-backend)
   - [Running the Backend](#running-the-backend)
   - [Running the Frontend](#running-the-frontend)
6. [Configuration](#configuration)
7. [Database Migrations & Seed Data](#database-migrations--seed-data)
8. [Running Tests](#running-tests)
9. [API Reference](#api-reference)
10. [Project Structure](#project-structure)
11. [Security Model](#security-model)
12. [Troubleshooting](#troubleshooting)

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                    Browser                           │
│        React + TypeScript + Tailwind CSS             │
│        Port 80 (Docker) / 5173 (dev)                 │
└──────────────────────┬──────────────────────────────┘
                       │  HTTP (REST JSON)
                       ▼
┌─────────────────────────────────────────────────────┐
│              nginx reverse proxy                     │
│  /api/* → C++ server    /* → React SPA              │
└──────────────────────┬──────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────┐
│           C++17 REST API Server                      │
│           cpp-httplib · Port 8080                    │
│           JWT auth · Rate limiting                   │
└──────────────────────┬──────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────┐
│              SQLite Database                         │
│         /data/budget.db  (Docker volume)             │
│         WAL mode · Foreign keys ON                   │
└─────────────────────────────────────────────────────┘
```

---

## Technology Choices & Justification

### Backend

| Choice | Library | Reason |
|--------|---------|--------|
| HTTP framework | **cpp-httplib v0.15** | Header-only, compiles in seconds, synchronous thread-per-connection model perfect for this scale. Drogon was considered but its async ORM added complexity without benefit for a personal-finance app. |
| JSON | **nlohmann/json v3.11** | Header-only, intuitive API, zero-copy parsing. |
| Database | **SQLite 3 (raw C API)** | ACID, single-file, no server to manage. WAL mode gives concurrent reads. |
| Auth | **JWT HS256 (OpenSSL)** | Stateless, refresh-token pattern avoids frequent re-logins. PBKDF2-SHA256 for password hashing. |
| Build | **CMake 3.16+** | Industry standard; FetchContent handles all dependencies. |
| Tests | **Catch2 v3** | BDD-style assertions, in-memory SQLite for isolation. |

### Frontend

| Choice | Library | Reason |
|--------|---------|--------|
| Framework | **React 18 + TypeScript** | Typed props prevent entire classes of runtime errors; Hooks API keeps components simple. |
| Build | **Vite 5** | Sub-second HMR during development. |
| Styling | **Tailwind CSS 3** | Utility-first avoids naming overhead; dark theme trivial. |
| Charts | **Chart.js 4 + react-chartjs-2** | Lightweight, declarative wrapper, covers bar/line charts needed. |
| Routing | **React Router v6** | Industry standard SPA routing. |
| HTTP | **axios** | Interceptor API makes JWT refresh transparent. |

---

## Prerequisites

### For Docker (easiest)
- [Docker Desktop](https://docs.docker.com/get-docker/) ≥ 24.0
- [Docker Compose](https://docs.docker.com/compose/install/) v2 (included in Docker Desktop)

### For local development
- **C++**: GCC 12+ or Clang 15+, CMake 3.16+
- **System libs**: `libsqlite3-dev`, `libssl-dev`, `zlib1g-dev`
- **Node.js**: v20+, npm v10+
- **Git** (CMake FetchContent clones dependencies)

---

## Quick Start (Docker — Recommended)

This is the single-command path. Everything builds and runs inside containers.

```bash
# 1. Clone the repository
git clone https://github.com/your-org/budget-app.git
cd budget-app

# 2. Build and start all services
docker compose up --build

# The first build takes 5–15 minutes (compiling C++ deps from source).
# Subsequent builds use Docker layer cache and are much faster.

# 3. Open the app
open http://localhost
# or: http://localhost:80 on Linux
```

The app is now running. Register an account or use the demo credentials.

### Loading Demo Data (optional)

```bash
# In a separate terminal while the stack is running:
docker compose --profile seed up seed
```

This loads 3 months of realistic transactions, 8 categories, and budgets into the database. After seeding, log in with:

- **Email**: `demo@budget.app`
- **Password**: `demo1234`

### Stopping the Stack

```bash
docker compose down          # stop containers (data preserved in volume)
docker compose down -v       # stop AND delete all data
```

---

## Local Development (Without Docker)

### Building the C++ Backend

```bash
cd server

# Ubuntu / Debian
sudo apt-get update && sudo apt-get install -y \
  build-essential cmake git \
  libsqlite3-dev libssl-dev zlib1g-dev

# macOS (Homebrew)
# brew install cmake openssl sqlite

# Configure (Debug build includes sanitizers)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build (uses all CPU cores)
cmake --build build --parallel $(nproc)

# Binaries produced:
#   build/budget_server   — the HTTP server
#   build/budget_tests    — the test suite
```

The build automatically downloads these header-only libraries via CMake FetchContent (requires internet on first build):
- `cpp-httplib` v0.15.3
- `nlohmann/json` v3.11.3
- `Catch2` v3.5.2

### Running the Backend

```bash
cd server

# Copy and edit config (optional — defaults work for local dev)
cp config.json config.local.json
# Edit: set db_path to a writable path, change jwt_secret

# Run with default config.json
./build/budget_server

# Run with custom config
./build/budget_server config.local.json

# Expected output:
# [2026-03-07T10:00:00Z] [info] Config loaded from config.json
# [2026-03-07T10:00:00Z] [info] DB Applied migration v1
# [2026-03-07T10:00:00Z] [info] Database ready: budget.db
# [2026-03-07T10:00:00Z] [info] Budget Server starting on 0.0.0.0:8080
```

Test the server is up:
```bash
curl http://localhost:8080/health
# → {"status":"ok"}
```

### Running the Frontend

```bash
cd client

# Install dependencies
npm install

# Copy env file
cp .env.example .env.local

# Start dev server (with HMR and API proxy to localhost:8080)
npm run dev

# Open: http://localhost:5173
```

The Vite dev server proxies `/api/*` requests to `http://localhost:8080`, so no CORS issues during development.

---

## Configuration

The backend reads `config.json` (path given as first CLI argument):

```json
{
  "port": 8080,
  "db_path": "/data/budget.db",
  "jwt_secret": "CHANGE-ME-use-a-long-random-string-at-least-32-chars",
  "jwt_expiry_seconds": 900,
  "refresh_expiry_seconds": 604800,
  "cors_origin": "http://localhost:5173",
  "rate_limit_rps": 30,
  "demo_mode": false,
  "log_level": "info"
}
```

| Key | Default | Description |
|-----|---------|-------------|
| `port` | 8080 | TCP port to listen on |
| `db_path` | `budget.db` | SQLite file path (absolute path recommended) |
| `jwt_secret` | — | **Change this in production.** Min 32 random chars. |
| `jwt_expiry_seconds` | 900 | Access token TTL (15 min) |
| `refresh_expiry_seconds` | 604800 | Refresh token TTL (7 days) |
| `cors_origin` | `http://localhost:5173` | Allowed CORS origin. In Docker, set to your domain. |
| `rate_limit_rps` | 30 | General rate limit per IP (requests/sec) |
| `demo_mode` | false | Reserved for CSV fallback mode |

---

## Database Migrations & Seed Data

### Migrations

Migrations run **automatically at startup**. The server creates a `schema_migrations` table and applies any unapplied SQL migrations in sequence. You never need to run migrations manually.

To inspect your database:
```bash
sqlite3 budget.db
sqlite> .tables
sqlite> SELECT * FROM schema_migrations;
sqlite> .quit
```

### Seed Data (local dev)

```bash
cd server
sqlite3 budget.db < seed/seed_data.sql
```

The seed creates:
- 1 demo user (`demo@budget.app` / `demo1234`)
- 8 categories (Food, Rent, Transport, Entertainment, Healthcare, Shopping, Utilities, Salary)
- 3 months of budgets (Jan–Mar 2026)
- ~45 realistic transactions across 3 months
- Correct balance computed from all transactions

---

## Running Tests

### C++ Unit & Integration Tests

```bash
cd server

# Build tests (done automatically during the build step)
cmake --build build --parallel

# Run all tests with verbose output
cd build && ctest --output-on-failure -V

# Or run the test binary directly for more detail
./build/budget_tests --reporter console

# Run a specific test tag
./build/budget_tests "[auth]"
./build/budget_tests "[transactions]"
./build/budget_tests "[reports]"
```

Test coverage includes:
- Password hashing round-trips and timing-safe comparison
- JWT creation, verification, expiry, and tampering detection
- Email validation and password strength
- Balance updates are atomic with transaction inserts
- Overdraft protection (with and without `force=true`)
- Transaction CRUD with balance reconciliation
- Undo last N transactions
- Date range and type filtering with pagination
- Category CRUD and uniqueness constraints
- Budget upsert behavior
- Monthly report calculations (income totals, per-category, budget vs actual, top expenses)
- Trend data chronological ordering

### Frontend Type Check

```bash
cd client
npm run typecheck   # runs tsc --noEmit
npm run lint        # runs ESLint
npm run build       # full production build (will fail on type errors)
```

---

## API Reference

Full OpenAPI 3.0 spec: [`docs/openapi.yaml`](docs/openapi.yaml)

You can view it interactively at [editor.swagger.io](https://editor.swagger.io) — paste the YAML.

### Quick reference

```
Base URL: http://localhost:8080/api/v1

Auth (no token required):
  POST /auth/register    { email, password }
  POST /auth/login       { email, password }
  POST /auth/refresh     { refresh_token }
  GET  /auth/me

Account:
  GET  /account
  POST /account/fund     { amount, note }

Categories:
  GET    /categories
  POST   /categories     { name, color, icon }
  PUT    /categories/:id { name, color, icon }
  DELETE /categories/:id

Transactions:
  GET    /transactions?start=&end=&category=&type=&page=&size=
  GET    /transactions/:id
  POST   /transactions   { date, amount, type, category_id, note, recurring }
  PUT    /transactions/:id
  DELETE /transactions/:id
  POST   /transactions/undo  { last_n }
  POST   /transactions/import  (CSV body)

Budgets:
  GET    /budgets?year_month=YYYY-MM
  POST   /budgets        { year_month, category_id, amount }
  DELETE /budgets/:id

Reports:
  GET /reports/monthly?year_month=YYYY-MM
  GET /reports/trend?months=6
  GET /reports/export?year_month=YYYY-MM&format=csv

System:
  GET /health
```

### Example curl session

```bash
BASE=http://localhost:8080/api/v1

# Register
curl -s -X POST $BASE/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"alice@example.com","password":"secret123"}' | jq .

# Login and capture token
TOKEN=$(curl -s -X POST $BASE/auth/login \
  -H "Content-Type: application/json" \
  -d '{"email":"alice@example.com","password":"secret123"}' | jq -r .access_token)

# Add income
curl -s -X POST $BASE/transactions \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"date":"2026-03-01","amount":3000,"type":"income","note":"Salary"}' | jq .

# Check balance
curl -s -H "Authorization: Bearer $TOKEN" $BASE/account | jq .

# Monthly report
curl -s -H "Authorization: Bearer $TOKEN" \
  "$BASE/reports/monthly?year_month=2026-03" | jq .
```

---

## Project Structure

```
budget-app/
├── docker-compose.yml          ← Single-command startup
│
├── server/                     ← C++ Backend
│   ├── CMakeLists.txt          ← Build system (FetchContent for all deps)
│   ├── config.json             ← Default configuration
│   ├── src/
│   │   ├── main.cpp            ← HTTP server, route registration
│   │   ├── config.hpp          ← Config loader
│   │   ├── models.hpp          ← Shared data structs + JSON helpers
│   │   ├── auth.hpp / auth.cpp ← JWT (HS256) + PBKDF2 password hashing
│   │   ├── database.hpp / .cpp ← Full SQLite wrapper (all CRUD + reports)
│   │   ├── handlers/
│   │   │   ├── auth_handler    ← Register, login, refresh, me
│   │   │   ├── transaction_handler ← CRUD, undo, CSV import
│   │   │   └── other_handlers  ← Categories, budgets, account, reports
│   │   └── middleware/
│   │       ├── auth_middleware.hpp  ← require_auth() JWT guard
│   │       └── rate_limiter.hpp     ← Token bucket per IP
│   ├── migrations/             ← (embedded in database.cpp migrate())
│   ├── seed/
│   │   └── seed_data.sql       ← 3 months of demo data
│   ├── tests/
│   │   ├── test_auth.cpp       ← Auth unit tests
│   │   ├── test_transactions.cpp ← DB integration tests
│   │   └── test_reports.cpp    ← Report calculation tests
│   └── docker/
│       └── Dockerfile          ← Multi-stage C++ build
│
├── client/                     ← React Frontend
│   ├── package.json
│   ├── vite.config.ts
│   ├── tailwind.config.js
│   ├── src/
│   │   ├── main.tsx            ← Entry point
│   │   ├── App.tsx             ← Router, layout, shared state
│   │   ├── index.css           ← Tailwind + custom CSS
│   │   ├── types/index.ts      ← All TypeScript types
│   │   ├── api/
│   │   │   ├── client.ts       ← Axios + JWT refresh interceptor
│   │   │   ├── auth.ts
│   │   │   ├── transactions.ts
│   │   │   └── index.ts        ← Categories, budgets, reports, account
│   │   ├── context/
│   │   │   └── AuthContext.tsx ← React auth state
│   │   ├── hooks/
│   │   │   └── useToast.ts     ← Toast notification hook
│   │   ├── components/
│   │   │   ├── Sidebar.tsx
│   │   │   ├── TransactionModal.tsx
│   │   │   ├── Charts.tsx      ← Chart.js wrappers
│   │   │   └── Toast.tsx
│   │   └── pages/
│   │       ├── Login.tsx
│   │       ├── Register.tsx
│   │       ├── Dashboard.tsx
│   │       ├── Transactions.tsx
│   │       ├── Categories.tsx  ← Categories + Budgets
│   │       └── Reports.tsx
│   └── docker/
│       ├── Dockerfile          ← Build React, serve via nginx
│       └── nginx.conf          ← SPA fallback + API proxy
│
├── docs/
│   └── openapi.yaml            ← Full OpenAPI 3.0 specification
│
└── .github/
    └── workflows/
        └── ci.yml              ← GitHub Actions: build, test, docker smoke test
```

---

## Security Model

| Concern | Implementation |
|---------|---------------|
| Password storage | PBKDF2-HMAC-SHA256, 100,000 iterations, 16-byte random salt, stored as `salt_hex:key_hex` |
| Tokens | JWT HS256 access tokens (15 min TTL) + refresh tokens (7 day TTL). Stateless — no server-side session store. |
| SQL injection | 100% parameterized queries via `sqlite3_bind_*` — no string concatenation in SQL. |
| CORS | Strict `Access-Control-Allow-Origin` restricted to configured `cors_origin`. |
| Rate limiting | Token bucket: 30 req/s general, 5 req/min for auth endpoints per IP. |
| Data isolation | All queries filter by `user_id` from the verified JWT — users cannot access each other's data. |
| Input validation | Server validates: date format (YYYY-MM-DD regex), amount > 0, type enum, name length limits. Client validates before submit. |

---

## Troubleshooting

### Docker build fails with "cmake not found" or "g++ not found"

The Dockerfile installs these in the build stage. Make sure you're running `docker compose build` from the project root, not inside the `server/` directory.

### "Cannot open DB: unable to open database file"

The server tries to write to `db_path` in `config.json`. In Docker, this is `/data/budget.db` which is mounted as a volume. Locally, ensure the path is writable:
```bash
touch budget.db   # creates the file
```

### Frontend shows "Network Error" when calling the API

1. **Local dev**: confirm the backend is running on port 8080: `curl http://localhost:8080/health`
2. **Docker**: the nginx proxy routes `/api/*` to the `server` container. Check both containers are healthy: `docker compose ps`

### C++ build fails with "OpenSSL not found"

```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# macOS
brew install openssl
export OPENSSL_ROOT_DIR=$(brew --prefix openssl)
cmake -B build -DOPENSSL_ROOT_DIR=$OPENSSL_ROOT_DIR
```

### "Token is invalid or expired"

Access tokens expire after 15 minutes. The frontend automatically refreshes them using the refresh token. If you're testing with curl, re-run the login command to get a fresh token.

### Port 80 already in use

Edit `docker-compose.yml` and change the client port mapping:
```yaml
ports:
  - "8081:80"   # change 8081 to any free port
```

### CMake FetchContent fails (no internet / proxy)

Pre-download and vendor the dependencies:
```bash
cd server
cmake -B build -DFETCHCONTENT_FULLY_DISCONNECTED=ON \
      -Dcpphttplib_DIR=/path/to/vendored/httplib \
      # etc.
```
Or pre-populate `~/.cmake/packages/` with the libraries.
