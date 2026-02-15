# Lala 1.2 - Windows Setup Script
# Full setup: Docker, backend-node, frontend, C++ backend (optional), tests
# Run from project root: .\setup.ps1

$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot
Set-Location $ProjectRoot

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Lala 1.2 - Setup (Windows)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# ---------------------------------------------------------------------------
# 1. Check prerequisites
# ---------------------------------------------------------------------------
Write-Host "[1/8] Checking prerequisites..." -ForegroundColor Yellow

$prereqs = @{}
try {
    $null = docker --version 2>$null
    $prereqs['Docker'] = $true
} catch { $prereqs['Docker'] = $false }

try {
    $v = node --version 2>$null
    $prereqs['Node.js'] = ($null -ne $v)
} catch { $prereqs['Node.js'] = $false }

$missing = $prereqs.GetEnumerator() | Where-Object { -not $_.Value } | ForEach-Object { $_.Key }
if ($missing.Count -gt 0) {
    Write-Host "  ERROR: Missing: $($missing -join ', ')" -ForegroundColor Red
    Write-Host "  Install: Docker Desktop, Node.js 18+" -ForegroundColor Red
    exit 1
}
Write-Host "  OK: Docker, Node.js found" -ForegroundColor Green

# ---------------------------------------------------------------------------
# 2. Start PostgreSQL (Docker)
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "[2/8] Starting PostgreSQL (Docker)..." -ForegroundColor Yellow
docker compose up -d 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  WARN: docker compose failed - may already be running" -ForegroundColor DarkYellow
} else {
    Write-Host "  OK: PostgreSQL container started" -ForegroundColor Green
}

Write-Host "  Waiting 15s for DB init..." -ForegroundColor Gray
Start-Sleep -Seconds 15

# ---------------------------------------------------------------------------
# 3. Install backend-node dependencies
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "[3/8] Installing backend-node dependencies..." -ForegroundColor Yellow
Set-Location "$ProjectRoot\backend-node"
npm install 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: npm install failed" -ForegroundColor Red
    Set-Location $ProjectRoot
    exit 1
}
Write-Host "  OK: backend-node deps installed" -ForegroundColor Green
Set-Location $ProjectRoot

# ---------------------------------------------------------------------------
# 4. Install frontend dependencies
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "[4/8] Installing frontend dependencies..." -ForegroundColor Yellow
Set-Location "$ProjectRoot\frontend"
npm install 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: npm install failed" -ForegroundColor Red
    Set-Location $ProjectRoot
    exit 1
}
Write-Host "  OK: frontend deps installed" -ForegroundColor Green
Set-Location $ProjectRoot

# ---------------------------------------------------------------------------
# 5. Build frontend (Vite)
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "[5/8] Building frontend (Vite)..." -ForegroundColor Yellow
Set-Location "$ProjectRoot\frontend"
npm run build 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: frontend build failed" -ForegroundColor Red
    Set-Location $ProjectRoot
    exit 1
}
Write-Host "  OK: frontend built" -ForegroundColor Green
Set-Location $ProjectRoot

# ---------------------------------------------------------------------------
# 6. Build C++ backend (optional)
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "[6/8] Building C++ backend (optional)..." -ForegroundColor Yellow
$backendBuildDir = "$ProjectRoot\backend\build"
if (-not (Test-Path $backendBuildDir)) {
    New-Item -ItemType Directory -Path $backendBuildDir -Force | Out-Null
}
Set-Location $backendBuildDir

$cmakeArgs = @("-DENABLE_LABS=ON", "-DBUILD_MEMORY_LABS=ON", "..")
& cmake @cmakeArgs 2>&1 | Out-Null
if ($LASTEXITCODE -eq 0) {
    cmake --build . --config Release 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  OK: C++ backend built (with labs)" -ForegroundColor Green
    } else {
        Write-Host "  WARN: C++ build failed (libpqxx/OpenSSL may be missing)" -ForegroundColor DarkYellow
        Write-Host "  Use backend-node instead (recommended)" -ForegroundColor Gray
    }
} else {
    Write-Host "  WARN: CMake configure failed - use backend-node (recommended)" -ForegroundColor DarkYellow
}
Set-Location $ProjectRoot

# ---------------------------------------------------------------------------
# 7. Start backend and run API tests
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "[7/8] Starting backend and running API tests..." -ForegroundColor Yellow

$backendProc = Start-Process -FilePath "node" -ArgumentList "server.js" -WorkingDirectory "$ProjectRoot\backend-node" -PassThru -WindowStyle Hidden
$maxWait = 10
$waited = 0
while ($waited -lt $maxWait) {
    Start-Sleep -Seconds 1
    $waited++
    try {
        $r = Invoke-WebRequest -Uri "http://127.0.0.1:8080/api/products" -UseBasicParsing -TimeoutSec 2 -ErrorAction Stop
        break
    } catch { }
}
if ($waited -ge $maxWait) {
    Write-Host "  WARN: Backend did not respond in time" -ForegroundColor DarkYellow
}

$apiOk = $false
try {
    $testOutput = node "$ProjectRoot\scripts\test-api.js" "http://127.0.0.1:8080" 2>&1
    $testOutput | ForEach-Object { Write-Host "  $_" }
    $apiOk = ($LASTEXITCODE -eq 0)
} catch {
    Write-Host "  ERROR: API test failed" -ForegroundColor Red
}

Stop-Process -Id $backendProc.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

if (-not $apiOk) {
    Write-Host "  WARN: Some API tests failed - check DB connection" -ForegroundColor DarkYellow
} else {
    Write-Host "  OK: API tests passed" -ForegroundColor Green
}

# ---------------------------------------------------------------------------
# 8. Summary
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Setup complete" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "To run the app:" -ForegroundColor White
Write-Host "  1. npm run db:up        # ensure PostgreSQL is up" -ForegroundColor Gray
Write-Host "  2. npm run backend      # start backend (port 8080)" -ForegroundColor Gray
Write-Host "  3. npm run frontend     # start frontend (port 3006)" -ForegroundColor Gray
Write-Host ""
Write-Host "  Or: npm run backend & npm run frontend (in two terminals)" -ForegroundColor Gray
Write-Host ""
Write-Host "  Frontend: http://localhost:3006" -ForegroundColor Gray
Write-Host "  API:      http://localhost:8080/api" -ForegroundColor Gray
Write-Host "  Test:     npm run test:api (with backend running)" -ForegroundColor Gray
Write-Host ""
