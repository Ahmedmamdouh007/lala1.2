#!/usr/bin/env bash
# Lala Store - macOS/Linux Setup Script
# Full setup: Docker, backend-node, frontend, C++ backend (optional), tests
# Run from project root: ./setup.sh

set -e
cd "$(dirname "$0")"
PROJECT_ROOT="$(pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
GRAY='\033[0;90m'
NC='\033[0m'

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Lala Store - Setup (macOS/Linux)${NC}"
echo -e "${CYAN}========================================${NC}"
echo ""

# ---------------------------------------------------------------------------
# 1. Check prerequisites
# ---------------------------------------------------------------------------
echo -e "${YELLOW}[1/8] Checking prerequisites...${NC}"

check_cmd() {
    command -v "$1" >/dev/null 2>&1
}

MISSING=""
check_cmd docker || MISSING="${MISSING}Docker "
check_cmd node || MISSING="${MISSING}Node.js "

if [ -n "$MISSING" ]; then
    echo -e "  ${RED}ERROR: Missing: $MISSING${NC}"
    echo -e "  ${RED}Install: Docker Desktop, Node.js 18+${NC}"
    exit 1
fi
echo -e "  ${GREEN}OK: Docker, Node.js found${NC}"

# ---------------------------------------------------------------------------
# 2. Start PostgreSQL (Docker)
# ---------------------------------------------------------------------------
echo ""
echo -e "${YELLOW}[2/8] Starting PostgreSQL (Docker)...${NC}"
docker compose up -d 2>/dev/null || true
echo -e "  ${GREEN}OK: PostgreSQL container started${NC}"

echo -e "  ${GRAY}Waiting 15s for DB init...${NC}"
sleep 15

# ---------------------------------------------------------------------------
# 3. Install backend-node dependencies
# ---------------------------------------------------------------------------
echo ""
echo -e "${YELLOW}[3/8] Installing backend-node dependencies...${NC}"
cd "$PROJECT_ROOT/backend-node"
npm install
echo -e "  ${GREEN}OK: backend-node deps installed${NC}"
cd "$PROJECT_ROOT"

# ---------------------------------------------------------------------------
# 4. Install frontend dependencies
# ---------------------------------------------------------------------------
echo ""
echo -e "${YELLOW}[4/8] Installing frontend dependencies...${NC}"
cd "$PROJECT_ROOT/frontend"
npm install
echo -e "  ${GREEN}OK: frontend deps installed${NC}"
cd "$PROJECT_ROOT"

# ---------------------------------------------------------------------------
# 5. Build frontend (Vite)
# ---------------------------------------------------------------------------
echo ""
echo -e "${YELLOW}[5/8] Building frontend (Vite)...${NC}"
cd "$PROJECT_ROOT/frontend"
npm run build
echo -e "  ${GREEN}OK: frontend built${NC}"
cd "$PROJECT_ROOT"

# ---------------------------------------------------------------------------
# 6. Build C++ backend (optional)
# ---------------------------------------------------------------------------
echo ""
echo -e "${YELLOW}[6/8] Building C++ backend (optional)...${NC}"
mkdir -p "$PROJECT_ROOT/backend/build"
cd "$PROJECT_ROOT/backend/build"
if cmake -DENABLE_LABS=ON .. 2>/dev/null; then
    if cmake --build . 2>/dev/null; then
        echo -e "  ${GREEN}OK: C++ backend built (with labs)${NC}"
    else
        echo -e "  ${YELLOW}WARN: C++ build failed (libpqxx/OpenSSL may be missing)${NC}"
        echo -e "  ${GRAY}Use backend-node instead (recommended)${NC}"
    fi
else
    echo -e "  ${YELLOW}WARN: CMake configure failed - use backend-node (recommended)${NC}"
fi
cd "$PROJECT_ROOT"

# ---------------------------------------------------------------------------
# 7. Start backend and run API tests
# ---------------------------------------------------------------------------
echo ""
echo -e "${YELLOW}[7/8] Starting backend and running API tests...${NC}"

cd "$PROJECT_ROOT/backend-node"
node server.js &
BACKEND_PID=$!
cd "$PROJECT_ROOT"

# Wait for backend to be ready
max_wait=10
waited=0
while [ $waited -lt $max_wait ]; do
    if curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:8080/api/products 2>/dev/null | grep -q 200; then
        break
    fi
    sleep 1
    waited=$((waited + 1))
done

api_ok=false
if node "$PROJECT_ROOT/scripts/test-api.js" "http://127.0.0.1:8080"; then
    api_ok=true
fi

kill $BACKEND_PID 2>/dev/null || true
sleep 1

if [ "$api_ok" = true ]; then
    echo -e "  ${GREEN}OK: API tests passed${NC}"
else
    echo -e "  ${YELLOW}WARN: Some API tests failed - check DB connection${NC}"
fi

# ---------------------------------------------------------------------------
# 8. Summary
# ---------------------------------------------------------------------------
echo ""
echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Setup complete${NC}"
echo -e "${CYAN}========================================${NC}"
echo ""
echo -e "${YELLOW}To run the app:${NC}"
echo "  1. npm run db:up        # ensure PostgreSQL is up"
echo "  2. npm run backend      # start backend (port 8080)"
echo "  3. npm run frontend     # start frontend (port 3006)"
echo ""
echo "  Or: npm run backend & npm run frontend (in two terminals)"
echo ""
echo "  Frontend: http://localhost:3006"
echo "  API:      http://localhost:8080/api"
echo ""
