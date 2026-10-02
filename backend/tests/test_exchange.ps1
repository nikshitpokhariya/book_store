# Comprehensive test script for Phase 3: Book Exchange Vertical Slice

$baseUrl = "http://127.0.0.1:8080"

function Assert-Condition($condition, $message) {
    if (-not $condition) {
        Write-Host "[FAIL] $message" -ForegroundColor Red
        throw "Assertion failed: $message"
    } else {
        Write-Host "[PASS] $message" -ForegroundColor Green
    }
}

function Register-And-Login($username, $email, $password) {
    $regBody = @{ username = $username; email = $email; password = $password } | ConvertTo-Json
    try {
        Invoke-RestMethod -Uri "$baseUrl/api/auth/signup" -Method Post -Body $regBody -ContentType "application/json" | Out-Null
    } catch {
        # Already exists is fine
    }

    $loginBody = @{ email = $email; password = $password } | ConvertTo-Json
    $res = Invoke-RestMethod -Uri "$baseUrl/api/auth/login" -Method Post -Body $loginBody -ContentType "application/json"
    return @{ "Authorization" = "Bearer $($res.accessToken)" }
}

$suffix = Get-Random -Minimum 1000 -Maximum 9999
$headersA = Register-And-Login "alice_$suffix" "alice_$suffix@example.com" "Password123!"
$headersB = Register-And-Login "bob_$suffix" "bob_$suffix@example.com" "Password123!"
$headersC = Register-And-Login "charlie_$suffix" "charlie_$suffix@example.com" "Password123!"

Write-Host "=== Setting up Users ===" -ForegroundColor Cyan
Assert-Condition ($headersA.Authorization -ne $null) "Alice logged in successfully"
Assert-Condition ($headersB.Authorization -ne $null) "Bob logged in successfully"
Assert-Condition ($headersC.Authorization -ne $null) "Charlie logged in successfully"

Write-Host "`n=== Creating Books ===" -ForegroundColor Cyan
# User A creates Book A1 and Book A2
$bookA1 = (Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -ContentType "application/json" -Body (@{
    title = "Clean Architecture by Alice"
    author = "Robert C. Martin"
    isbn = "9780134494166"
    price = 650
    condition = "Like New"
    category = "Technology"
    description = "Clean Architecture first edition"
} | ConvertTo-Json)).book

$bookA2 = (Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -ContentType "application/json" -Body (@{
    title = "Design Patterns by Alice"
    author = "GoF"
    isbn = "9780201633610"
    price = 800
    condition = "Good"
    category = "Technology"
    description = "Classic design patterns"
} | ConvertTo-Json)).book

# User B creates Book B1 and Book B2
$bookB1 = (Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -Headers $headersB -ContentType "application/json" -Body (@{
    title = "Refactoring by Bob"
    author = "Martin Fowler"
    isbn = "9780134757599"
    price = 700
    condition = "Like New"
    category = "Technology"
    description = "Refactoring 2nd edition"
} | ConvertTo-Json)).book

$bookB2 = (Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -Headers $headersB -ContentType "application/json" -Body (@{
    title = "The Pragmatic Programmer by Bob"
    author = "Andy Hunt"
    isbn = "9780201616224"
    price = 750
    condition = "Good"
    category = "Technology"
    description = "Pragmatic Programmer 20th anniversary"
} | ConvertTo-Json)).book

Assert-Condition ($bookA1.id -ne $null) "Alice created Book A1 ($($bookA1.id))"
Assert-Condition ($bookB1.id -ne $null) "Bob created Book B1 ($($bookB1.id))"

Write-Host "`n=== Test 1: Validation Failures on Exchange Creation ===" -ForegroundColor Cyan

# 1.1 Bob tries to request his own book
try {
    Invoke-RestMethod -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -ContentType "application/json" -Body (@{
        requestedBookId = $bookB1.id
        offeredBookId = $bookB2.id
    } | ConvertTo-Json) | Out-Null
    Assert-Condition $false "Requesting own book should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 400) "Requesting own book fails with 400 Bad Request"
}

# 1.2 Bob tries to offer Alice's book
try {
    Invoke-RestMethod -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -ContentType "application/json" -Body (@{
        requestedBookId = $bookA1.id
        offeredBookId = $bookA2.id
    } | ConvertTo-Json) | Out-Null
    Assert-Condition $false "Offering someone else's book should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 400) "Offering someone else's book fails with 400 Bad Request"
}

# 1.3 Bob tries to offer and request the same book
try {
    Invoke-RestMethod -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -ContentType "application/json" -Body (@{
        requestedBookId = $bookA1.id
        offeredBookId = $bookA1.id
    } | ConvertTo-Json) | Out-Null
    Assert-Condition $false "Offering and requesting same book should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 400) "Same requested and offered book fails with 400 Bad Request"
}

Write-Host "`n=== Test 2: Valid Exchange Creation & Pending State ===" -ForegroundColor Cyan
# Bob requests Alice's Book A1, offering Bob's Book B1
$exch1Res = Invoke-RestMethod -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -ContentType "application/json" -Body (@{
    requestedBookId = $bookA1.id
    offeredBookId = $bookB1.id
    message = "Hey Alice, let's swap Clean Architecture for Refactoring!"
} | ConvertTo-Json)

$exchange1 = $exch1Res.exchange
Assert-Condition ($exchange1.id -ne $null) "Exchange request created ($($exchange1.exchangeNumber))"
Assert-Condition ($exchange1.status -eq "PENDING") "Initial status is PENDING"
Assert-Condition ($exchange1.requesterUsername -ne $null) "Requester username recorded"
Assert-Condition ($exchange1.requestedBook.title -eq "Clean Architecture by Alice") "Requested book snapshot recorded"
Assert-Condition ($exchange1.offeredBook.title -eq "Refactoring by Bob") "Offered book snapshot recorded"

# Verify books are still public and available while exchange is PENDING
$browseRes = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Get
$cleanArch = $browseRes.books | Where-Object { $_.id -eq $bookA1.id }
$refact = $browseRes.books | Where-Object { $_.id -eq $bookB1.id }
Assert-Condition ($cleanArch -ne $null -and $cleanArch.status -eq "available") "Book A1 remains available in browse while pending"
Assert-Condition ($refact -ne $null -and $refact.status -eq "available") "Book B1 remains available in browse while pending"

Write-Host "`n=== Test 3: Sent & Received Requests & Authorization ===" -ForegroundColor Cyan
# Bob checks sent requests
$sentByBob = Invoke-RestMethod -Uri "$baseUrl/api/exchanges/sent" -Method Get -Headers $headersB
Assert-Condition ($sentByBob.exchanges.Count -ge 1) "Bob sees at least 1 sent request"
$foundSent = $sentByBob.exchanges | Where-Object { $_.id -eq $exchange1.id }
Assert-Condition ($foundSent -ne $null) "Bob's sent requests contains exchange1"

# Alice checks received requests
$receivedByAlice = Invoke-RestMethod -Uri "$baseUrl/api/exchanges/received" -Method Get -Headers $headersA
Assert-Condition ($receivedByAlice.exchanges.Count -ge 1) "Alice sees at least 1 received request"
$foundReceived = $receivedByAlice.exchanges | Where-Object { $_.id -eq $exchange1.id }
Assert-Condition ($foundReceived -ne $null) "Alice's received requests contains exchange1"

# Charlie checks received (should NOT see exchange1)
$receivedByCharlie = Invoke-RestMethod -Uri "$baseUrl/api/exchanges/received" -Method Get -Headers $headersC
$charlieFound = $receivedByCharlie.exchanges | Where-Object { $_.id -eq $exchange1.id }
Assert-Condition ($charlieFound -eq $null) "Charlie does not see Alice's received request"

# Charlie tries to view details of exchange1
try {
    Invoke-RestMethod -Uri "$baseUrl/api/exchanges/$($exchange1.id)" -Method Get -Headers $headersC | Out-Null
    Assert-Condition $false "Charlie should not be able to view exchange1"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 403) "Charlie viewing exchange1 receives 403 Forbidden"
}

# Charlie tries to accept exchange1
try {
    Invoke-RestMethod -Uri "$baseUrl/api/exchanges/$($exchange1.id)/accept" -Method Post -Headers $headersC | Out-Null
    Assert-Condition $false "Charlie accepting exchange1 should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 403) "Charlie accepting exchange1 receives 403 Forbidden"
}

# Bob tries to accept his own sent request (only receiver Alice can accept)
try {
    Invoke-RestMethod -Uri "$baseUrl/api/exchanges/$($exchange1.id)/accept" -Method Post -Headers $headersB | Out-Null
    Assert-Condition $false "Bob accepting his own sent exchange should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 403) "Requester cannot accept own exchange (403 Forbidden)"
}

Write-Host "`n=== Test 4: Rejection Workflow ===" -ForegroundColor Cyan
# Create exchange 2: Bob offers Book B2 for Alice's Book A2
$exch2 = (Invoke-RestMethod -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -ContentType "application/json" -Body (@{
    requestedBookId = $bookA2.id
    offeredBookId = $bookB2.id
} | ConvertTo-Json)).exchange

# Alice rejects exchange 2
$rejectRes = Invoke-RestMethod -Uri "$baseUrl/api/exchanges/$($exch2.id)/reject" -Method Post -Headers $headersA
Assert-Condition ($rejectRes.exchange.status -eq "REJECTED") "Alice successfully rejected exchange2"

# Books remain available and unchanged
$bookA2AfterReject = (Invoke-RestMethod -Uri "$baseUrl/api/books/$($bookA2.id)" -Method Get).book
Assert-Condition ($bookA2AfterReject.status -eq "available") "Book A2 still available after rejection"

Write-Host "`n=== Test 5: Cancellation Workflow ===" -ForegroundColor Cyan
# Create exchange 3: Bob offers Book B2 for Alice's Book A2
$exch3 = (Invoke-RestMethod -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -ContentType "application/json" -Body (@{
    requestedBookId = $bookA2.id
    offeredBookId = $bookB2.id
} | ConvertTo-Json)).exchange

# Alice tries to cancel (only Bob the requester can cancel)
try {
    Invoke-RestMethod -Uri "$baseUrl/api/exchanges/$($exch3.id)/cancel" -Method Post -Headers $headersA | Out-Null
    Assert-Condition $false "Receiver Alice cannot cancel request"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 403) "Receiver cancelling returns 403 Forbidden"
}

# Bob cancels exchange 3
$cancelRes = Invoke-RestMethod -Uri "$baseUrl/api/exchanges/$($exch3.id)/cancel" -Method Post -Headers $headersB
Assert-Condition ($cancelRes.exchange.status -eq "CANCELLED") "Bob successfully cancelled exchange3"

Write-Host "`n=== Test 6: Acceptance & Atomic Two-Sided Ownership Swap ===" -ForegroundColor Cyan
# Alice accepts exchange 1 (Alice owns Book A1, Bob owns Book B1)
$acceptRes = Invoke-RestMethod -Uri "$baseUrl/api/exchanges/$($exchange1.id)/accept" -Method Post -Headers $headersA
Assert-Condition ($acceptRes.exchange.status -eq "COMPLETED") "Exchange status is COMPLETED"
Assert-Condition ($acceptRes.exchange.completedAt -ne $null) "completedAt timestamp is set"

# Check Book A1: Public unauthenticated view returns 404 (hidden from public)
try {
    Invoke-RestMethod -Uri "$baseUrl/api/books/$($bookA1.id)" -Method Get | Out-Null
    Assert-Condition $false "Exchanged book should not be publicly accessible via getById"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 404) "Exchanged book is hidden from public (404 Not Found)"
}

# New owner Bob can view Book A1 via authenticated getById
$bookA1After = (Invoke-RestMethod -Uri "$baseUrl/api/books/$($bookA1.id)" -Method Get -Headers $headersB).book
Assert-Condition ($bookA1After.status -eq "exchanged") "Book A1 status is exchanged"
Assert-Condition ($bookA1After.owner.id -eq $exchange1.requesterId) "Book A1 owner is now Bob!"

# New owner Alice can view Book B1 via authenticated getById
$bookB1After = (Invoke-RestMethod -Uri "$baseUrl/api/books/$($bookB1.id)" -Method Get -Headers $headersA).book
Assert-Condition ($bookB1After.status -eq "exchanged") "Book B1 status is exchanged"
Assert-Condition ($bookB1After.owner.id -eq $exchange1.receiverId) "Book B1 owner is now Alice!"

# Verify public browse & search NO LONGER lists Book A1 or Book B1
$browseAfter = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Get
$browseBooks = $browseAfter.books
$foundA1 = $browseBooks | Where-Object { $_.id -eq $bookA1.id }
$foundB1 = $browseBooks | Where-Object { $_.id -eq $bookB1.id }
Assert-Condition ($foundA1 -eq $null) "Book A1 no longer appears in public Browse"
Assert-Condition ($foundB1 -eq $null) "Book B1 no longer appears in public Browse"

$searchAfter = Invoke-RestMethod -Uri "$baseUrl/api/books?search=Refactoring" -Method Get
$foundRefact = $searchAfter.books | Where-Object { $_.id -eq $bookB1.id }
Assert-Condition ($foundRefact -eq $null) "Exchanged book no longer appears in search"

Write-Host "`n=== Test 7: Concurrency & Cross-Slice Interactions ===" -ForegroundColor Cyan

# 7.1 Exchanged book CANNOT be added to cart
try {
    Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersC -ContentType "application/json" -Body (@{
        bookId = $bookA1.id
    } | ConvertTo-Json) | Out-Null
    Assert-Condition $false "Adding exchanged book to cart should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 400) "Adding exchanged book to cart fails with 400 Bad Request"
}

# 7.1b Book added to cart when available, then exchanged before checkout
$bookA_race = (Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -ContentType "application/json" -Body (@{
    title = "Race Condition Book by Alice"
    author = "Concurrency Master"
    isbn = "9780134494100"
    price = 500
    condition = "Good"
    category = "Technology"
    description = "Testing race conditions"
} | ConvertTo-Json)).book

# Charlie adds Book A_race to cart while it is available
Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersC -ContentType "application/json" -Body (@{
    bookId = $bookA_race.id
} | ConvertTo-Json) | Out-Null

# Bob requests exchange for Book A_race offering Book B2
$exch_race = (Invoke-RestMethod -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -ContentType "application/json" -Body (@{
    requestedBookId = $bookA_race.id
    offeredBookId = $bookB2.id
} | ConvertTo-Json)).exchange

# Alice accepts the exchange!
Invoke-RestMethod -Uri "$baseUrl/api/exchanges/$($exch_race.id)/accept" -Method Post -Headers $headersA | Out-Null

# Now Charlie tries to checkout the book that was exchanged!
try {
    Invoke-RestMethod -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersC -ContentType "application/json" -Body (@{
        paymentMethod = "CARD"
        shippingAddress = @{ fullName = "Charlie Brown"; addressLine = "456 Oak Avenue"; city = "Metropolis"; state = "State"; postalCode = "123456"; phone = "9876543210" }
    } | ConvertTo-Json) | Out-Null
    Assert-Condition $false "Exchanged book should not be purchasable"
} catch {
    $status = $_.Exception.Response.StatusCode.value__
    $respBody = $_.ErrorDetails.Message
    Assert-Condition ($status -eq 409) "Checkout of exchanged book fails with 409 Conflict (actual was: $status)"
}

# 7.2 Purchased book CANNOT be exchanged
# Charlie creates Book C1
$bookC1 = (Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -Headers $headersC -ContentType "application/json" -Body (@{
    title = "SICP by Charlie"
    author = "Abelson & Sussman"
    isbn = "9780262510874"
    price = 900
    condition = "New"
    category = "Technology"
    description = "Structure and Interpretation of Computer Programs"
} | ConvertTo-Json)).book

# Bob creates a fresh Book B3
$bookB3 = (Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -Headers $headersB -ContentType "application/json" -Body (@{
    title = "Domain-Driven Design by Bob"
    author = "Eric Evans"
    isbn = "9780321125217"
    price = 850
    condition = "Like New"
    category = "Technology"
    description = "Tackling complexity in software"
} | ConvertTo-Json)).book

# Bob creates an exchange request for C1 offering B3 before C1 is purchased
$exch4 = (Invoke-RestMethod -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -ContentType "application/json" -Body (@{
    requestedBookId = $bookC1.id
    offeredBookId = $bookB3.id
} | ConvertTo-Json)).exchange

# Alice purchases C1 right now!
Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersA -ContentType "application/json" -Body (@{
    bookId = $bookC1.id
} | ConvertTo-Json)
$orderRes = Invoke-RestMethod -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersA -ContentType "application/json" -Body (@{
    paymentMethod = "UPI"
    shippingAddress = @{ fullName = "Alice Smith"; addressLine = "123 Main Street"; city = "Metropolis"; state = "State"; postalCode = "123456"; phone = "9876543210" }
} | ConvertTo-Json)
Assert-Condition ($orderRes.order.orderNumber -ne $null) "Alice successfully purchased Book C1"

# Charlie tries to accept Bob's exchange for C1 (which was just sold!)
try {
    Invoke-RestMethod -Uri "$baseUrl/api/exchanges/$($exch4.id)/accept" -Method Post -Headers $headersC | Out-Null
    Assert-Condition $false "Accepting exchange for sold book must fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 409) "Accepting exchange for sold book fails with 409 Conflict"
}

# 7.3 Cannot create exchange request for already sold or exchanged book
try {
    Invoke-RestMethod -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -ContentType "application/json" -Body (@{
        requestedBookId = $bookC1.id
        offeredBookId = $bookB3.id
    } | ConvertTo-Json) | Out-Null
    Assert-Condition $false "Creating exchange for sold book must fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 400) "Creating exchange for sold book fails with 400 Bad Request"
}

Write-Host "`n=== Test 8: Private Exchange History ===" -ForegroundColor Cyan
$aliceHistory = Invoke-RestMethod -Uri "$baseUrl/api/exchanges/history" -Method Get -Headers $headersA
Assert-Condition ($aliceHistory.exchanges.Count -ge 1) "Alice sees her exchange history"
$aliceExch1 = $aliceHistory.exchanges | Where-Object { $_.id -eq $exchange1.id }
Assert-Condition ($aliceExch1 -ne $null) "Alice history contains exchange 1"

$bobHistory = Invoke-RestMethod -Uri "$baseUrl/api/exchanges/history" -Method Get -Headers $headersB
Assert-Condition ($bobHistory.exchanges.Count -ge 1) "Bob sees his exchange history"
$bobExch1 = $bobHistory.exchanges | Where-Object { $_.id -eq $exchange1.id }
Assert-Condition ($bobExch1 -ne $null) "Bob history contains exchange 1"

$charlieHistory = Invoke-RestMethod -Uri "$baseUrl/api/exchanges/history" -Method Get -Headers $headersC
$charlieExch1 = $charlieHistory.exchanges | Where-Object { $_.id -eq $exchange1.id }
Assert-Condition ($charlieExch1 -eq $null) "Charlie does NOT see Alice and Bob's exchange in his history"

Write-Host "`nALL PHASE 3 EXCHANGE TESTS PASSED SUCCESSFULLY!" -ForegroundColor Green
