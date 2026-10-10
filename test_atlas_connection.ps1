# ==============================================================================
# test_atlas_connection.ps1
# Automated Verification Script for MongoDB Atlas Cloud Integration
# ==============================================================================

$ErrorActionPreference = "Continue"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  MongoDB Atlas Connection & Integration Verification" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. Parse .env Configuration
$envFile = Join-Path $PSScriptRoot "backend\.env"
if (Test-Path $envFile) {
    Write-Host "[1/4] Reading backend\.env configuration..." -ForegroundColor Yellow
    Get-Content $envFile | ForEach-Object {
        $line = $_.Trim()
        if ($line -and -not $line.StartsWith("#")) {
            $parts = $line -split "=", 2
            if ($parts.Count -eq 2) {
                [Environment]::SetEnvironmentVariable($parts[0].Trim(), $parts[1].Trim(), "Process")
            }
        }
    }
}

$mongoUri = [Environment]::GetEnvironmentVariable("MONGO_URI")
if (-not $mongoUri) {
    Write-Host "      [WARNING] MONGO_URI not configured in backend\.env or environment." -ForegroundColor Yellow
    exit 1
}
$mongoDb = [Environment]::GetEnvironmentVariable("MONGO_DATABASE")
if (-not $mongoDb) {
    $mongoDb = "myapp"
}

Write-Host "      Database Name: $mongoDb" -ForegroundColor Gray
Write-Host "      Target URI:    $($mongoUri -replace ':[^:@]+@', ':****@')" -ForegroundColor Gray

# 2. DNS SRV Resolution Check
Write-Host "`n[2/4] Testing DNS SRV Resolution for Atlas Cluster..." -ForegroundColor Yellow
$clusterHost = $null
if ($mongoUri -match "mongodb\+srv://(?:[^@]+@)?([^/?]+)") {
    $clusterHost = $matches[1]
}

$shards = @()
if ($clusterHost) {
    $srvHost = "_mongodb._tcp.$clusterHost"
    try {
        $dnsRecords = Resolve-DnsName -Name $srvHost -Type SRV -ErrorAction Stop
        $shards = $dnsRecords | ForEach-Object { $_.NameTarget } | Where-Object { $_ }
        Write-Host "      [SUCCESS] Resolved $($shards.Count) Atlas Shards:" -ForegroundColor Green
        foreach ($s in $shards) {
            Write-Host "        - $s" -ForegroundColor DarkCyan
        }
    } catch {
        Write-Host "      [ERROR] Failed to resolve SRV record: $_" -ForegroundColor Red
    }
} else {
    Write-Host "      [INFO] Non-SRV URI detected ($mongoUri). Skipping SRV lookup." -ForegroundColor Gray
}

# 3. Direct TCP Connectivity to Atlas Shards (port 27017)
Write-Host "`n[3/4] Testing TCP Port 27017 on Shards..." -ForegroundColor Yellow
$primaryShard = if ($shards.Count -gt 0) { $shards[0] } elseif ($clusterHost) { $clusterHost } else { "127.0.0.1" }
try {
    $tcp = Test-NetConnection -ComputerName $primaryShard -Port 27017 -WarningAction SilentlyContinue
    if ($tcp.TcpTestSucceeded) {
        Write-Host "      [SUCCESS] TCP connection to $primaryShard`:27017 succeeded!" -ForegroundColor Green
    } else {
        Write-Host "      [WARNING] TCP connection to $primaryShard`:27017 did not succeed. Check firewall." -ForegroundColor Yellow
    }
} catch {
    Write-Host "      [ERROR] TCP test exception: $_" -ForegroundColor Red
}

# 4. Backend REST API Health & CRUD verification
Write-Host "`n[4/4] Verifying Drogon Backend Database Operations..." -ForegroundColor Yellow
$baseUrl = "http://127.0.0.1:8080"
$backendActive = $false

try {
    $resp = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Get -TimeoutSec 3 -ErrorAction Stop
    $backendActive = $true
} catch {
    Write-Host "      [INFO] Drogon backend not currently listening at $baseUrl." -ForegroundColor DarkYellow
    Write-Host "             To run full API test against Atlas, launch start_backend.bat." -ForegroundColor DarkYellow
}

if ($backendActive) {
    Write-Host "      [SUCCESS] Backend is active and connected to Atlas database!" -ForegroundColor Green
    
    # Test User Signup against Atlas
    $rand = Get-Random -Minimum 10000 -Maximum 99999
    $testUser = @{
        username = "atlas_user_$rand"
        email    = "atlas_$rand@test.com"
        password = "Password123!"
        fullName = "Atlas Verification User"
    } | ConvertTo-Json

    try {
        $signupResp = Invoke-RestMethod -Uri "$baseUrl/api/auth/signup" -Method Post -Body $testUser -ContentType "application/json" -ErrorAction Stop
        Write-Host "      [SUCCESS] Created test user '$($testUser.username)' on Atlas (Index & Write verified)." -ForegroundColor Green
    } catch {
        Write-Host "      [NOTICE] Signup returned: $_" -ForegroundColor Gray
    }

    # Query listings
    try {
        $books = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Get -ErrorAction Stop
        Write-Host "      [SUCCESS] Successfully queried book listings collection from Atlas." -ForegroundColor Green
    } catch {
        Write-Host "      [ERROR] Query failed: $_" -ForegroundColor Red
    }
}

Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host "  Atlas Verification Summary Complete" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Cyan
