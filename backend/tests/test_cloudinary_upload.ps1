# ==============================================================================
# test_cloudinary_upload.ps1
# Automated Verification Script for Cloudinary Media Upload & CDN Serving
# ==============================================================================

$ErrorActionPreference = "Continue"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  Cloudinary Upload & Delivery Verification" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. Parse .env Configuration
$possibleEnvPaths = @(
    (Join-Path $PSScriptRoot "..\.env"),
    (Join-Path $PSScriptRoot ".env"),
    (Join-Path $PSScriptRoot "backend\.env")
)

$envFile = $null
foreach ($p in $possibleEnvPaths) {
    if (Test-Path $p) {
        $envFile = (Resolve-Path $p).Path
        break
    }
}

if ($envFile) {
    Write-Host "[1/4] Reading $envFile configuration..." -ForegroundColor Yellow
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

$cloudName = [Environment]::GetEnvironmentVariable("CLOUDINARY_CLOUD_NAME")
$apiKey = [Environment]::GetEnvironmentVariable("CLOUDINARY_API_KEY")
$apiSecret = [Environment]::GetEnvironmentVariable("CLOUDINARY_API_SECRET")

if (-not $cloudName -or -not $apiKey -or -not $apiSecret) {
    Write-Host "      [WARNING] Cloudinary credentials not configured in .env or environment." -ForegroundColor Yellow
    exit 1
}

Write-Host "      Cloud Name: $cloudName" -ForegroundColor Gray
Write-Host "      API Key:    $apiKey" -ForegroundColor Gray

# 2. Create Sample Test Image
Write-Host "`n[2/4] Generating sample book cover test image..." -ForegroundColor Yellow
$sampleImgPath = Join-Path $PSScriptRoot "sample_test_cover.gif"
$sampleBytes = [Convert]::FromBase64String("R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7")
[IO.File]::WriteAllBytes($sampleImgPath, $sampleBytes)
Write-Host "      [SUCCESS] Generated sample test image at: $sampleImgPath" -ForegroundColor Green

# 3. Upload to Cloudinary (via Drogon backend /api/upload or Direct Signed API)
Write-Host "`n[3/4] Performing Upload to Cloudinary..." -ForegroundColor Yellow
$backendUrl = "http://127.0.0.1:8080/api/upload"
$uploadedUrl = $null

try {
    # Test Drogon backend endpoint if listening
    Write-Host "      Attempting upload via Drogon backend ($backendUrl)..." -ForegroundColor Gray
    $curlOut = & curl.exe -s -X POST $backendUrl -F "file=@$sampleImgPath"
    if ($curlOut) {
        $json = $curlOut | ConvertFrom-Json -ErrorAction SilentlyContinue
        if ($json -and $json.url) {
            $uploadedUrl = $json.url
            Write-Host "      [SUCCESS] Uploaded via Drogon backend /api/upload!" -ForegroundColor Green
        }
    }
} catch {
    # Backend may not be running yet
}

if (-not $uploadedUrl) {
    Write-Host "      [INFO] Drogon backend not currently listening or did not return URL." -ForegroundColor DarkYellow
    Write-Host "             Testing direct Cloudinary signed REST endpoint with configured credentials..." -ForegroundColor DarkYellow

    $timestamp = [int64](([DateTimeOffset]::UtcNow).ToUnixTimeSeconds())
    $toSign = "timestamp=${timestamp}${apiSecret}"

    $sha1 = [System.Security.Cryptography.SHA1]::Create()
    $hashBytes = $sha1.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($toSign))
    $signature = [BitConverter]::ToString($hashBytes).Replace("-", "").ToLower()

    $directUrl = "https://api.cloudinary.com/v1_1/$cloudName/image/upload"
    $cldOut = & curl.exe -s -X POST $directUrl `
      -F "file=@$sampleImgPath" `
      -F "api_key=$apiKey" `
      -F "timestamp=$timestamp" `
      -F "signature=$signature"

    if ($cldOut) {
        $json = $cldOut | ConvertFrom-Json -ErrorAction SilentlyContinue
        if ($json -and $json.secure_url) {
            $uploadedUrl = $json.secure_url
            Write-Host "      [SUCCESS] Direct signed upload to Cloudinary succeeded!" -ForegroundColor Green
        } else {
            Write-Host "      [ERROR] Upload response: $cldOut" -ForegroundColor Red
        }
    }
}

# 4. Verify Returned Cloudinary URL Serves HTTP 200
Write-Host "`n[4/4] Verifying Cloudinary CDN Image Delivery..." -ForegroundColor Yellow
if ($uploadedUrl) {
    Write-Host "      Cloudinary Asset URL: $uploadedUrl" -ForegroundColor Cyan

    $httpCode = & curl.exe -s -o nul -w "%{http_code}" "$uploadedUrl"
    if ($httpCode -eq "200") {
        Write-Host "      [SUCCESS] CDN Delivery Verified: HTTP 200 OK via Cloudinary CDN!" -ForegroundColor Green
    } else {
        Write-Host "      [WARNING] Cloudinary CDN returned HTTP status: $httpCode" -ForegroundColor Yellow
    }
} else {
    Write-Host "      [ERROR] Upload failed; unable to test CDN retrieval." -ForegroundColor Red
}

# Cleanup
if (Test-Path $sampleImgPath) {
    Remove-Item $sampleImgPath -Force -ErrorAction SilentlyContinue
}

Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host "  Cloudinary Verification Summary Complete" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Cyan
