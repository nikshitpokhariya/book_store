if (-not $env:JWT_SECRET) {
    $env:JWT_SECRET = "WhWRp7LUnP+BUdQaOxcjpf67hKIBcnk3WAqTrTbzfjOy+CWOdXlgIqsqGTCTHYu7"
}
if (-not $env:MONGO_URI) {
    $env:MONGO_URI = "mongodb://127.0.0.1:27017"
}
if (-not $env:MONGO_DATABASE) {
    $env:MONGO_DATABASE = "myapp"
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " BookExchangeBackend Starting" -ForegroundColor Green
Write-Host " Port: 8080" -ForegroundColor Yellow
Write-Host " Mongo URI: $env:MONGO_URI" -ForegroundColor Yellow
Write-Host " Database: $env:MONGO_DATABASE" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan

& ".\build\Debug\BookExchangeBackend.exe"
