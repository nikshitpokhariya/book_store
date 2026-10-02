$env:JWT_SECRET = "WhWRp7LUnP+BUdQaOxcjpf67hKIBcnk3WAqTrTbzfjOy+CWOdXlgIqsqGTCTHYu7"
$env:MONGO_URI = "mongodb://127.0.0.1:27017"
$env:MONGO_DATABASE = "book_exchange_qa_test"

Write-Host "Starting BookExchangeBackend on port 8080..."
& ".\build\Debug\BookExchangeBackend.exe"
