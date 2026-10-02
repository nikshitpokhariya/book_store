# Comprehensive Phase 4 test script: Reviews, Cancellation, Returns, and Hardening

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
    } catch {}

    $loginBody = @{ email = $email; password = $password } | ConvertTo-Json
    $res = Invoke-RestMethod -Uri "$baseUrl/api/auth/login" -Method Post -Body $loginBody -ContentType "application/json"
    return @{ "Authorization" = "Bearer $($res.accessToken)" }
}

$suffix = Get-Random -Minimum 1000 -Maximum 9999
Write-Host "=== Setting up Users ===" -ForegroundColor Cyan
$headersSeller = Register-And-Login "p4_seller_$suffix" "p4_seller_$suffix@example.com" "Password123!"
$headersBuyer = Register-And-Login "p4_buyer_$suffix" "p4_buyer_$suffix@example.com" "Password123!"
$headersOther = Register-And-Login "p4_other_$suffix" "p4_other_$suffix@example.com" "Password123!"

Assert-Condition ($headersSeller.Authorization -ne $null) "Seller authenticated"
Assert-Condition ($headersBuyer.Authorization -ne $null) "Buyer authenticated"
Assert-Condition ($headersOther.Authorization -ne $null) "Third-party user authenticated"

Write-Host "`n=== SECTION 1: BUY FLOW, DELIVERY & REVIEWS ===" -ForegroundColor Cyan

# 1.1 Seller lists Book A1
$book1 = (Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -Headers $headersSeller -ContentType "application/json" -Body (@{
    title = "Design Patterns (Gang of Four)"
    author = "Erich Gamma et al."
    isbn = "9780201633610"
    price = 600
    condition = "Like New"
    category = "Technology"
    description = "Classic GOF design patterns"
} | ConvertTo-Json)).book
Assert-Condition ($book1.id -ne $null) "Seller created Book A1 ($($book1.id))"

# 1.2 Buyer buys Book A1
Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{
    bookId = $book1.id
} | ConvertTo-Json) | Out-Null

$orderRes1 = Invoke-RestMethod -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{
    paymentMethod = "UPI"
    shippingAddress = @{ fullName = "Buyer Bob"; addressLine = "123 Tech Blvd"; city = "Bengaluru"; state = "Karnataka"; postalCode = "560001"; phone = "9876543210" }
} | ConvertTo-Json)
$order1 = $orderRes1.order
Assert-Condition ($order1.id -ne $null) "Buyer checked out order 1 ($($order1.orderNumber))"
Assert-Condition ($order1.status -eq "CONFIRMED") "Order status is initially CONFIRMED"

# 1.3 Buyer attempts review before delivery (Should fail)
try {
    Invoke-RestMethod -Uri "$baseUrl/api/reviews" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{
        orderId = $order1.id
        bookId = $book1.id
        rating = 5
        comment = "Tried to review too early!"
    } | ConvertTo-Json) | Out-Null
    Assert-Condition $false "Review before delivery should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 400) "Review before delivery rejected with 400 Bad Request"
}

# 1.4 Seller attempts to review own book (Should fail)
try {
    Invoke-RestMethod -Uri "$baseUrl/api/reviews" -Method Post -Headers $headersSeller -ContentType "application/json" -Body (@{
        orderId = $order1.id
        bookId = $book1.id
        rating = 5
        comment = "Self review attempt!"
    } | ConvertTo-Json) | Out-Null
    Assert-Condition $false "Seller review of own book should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 403 -or $_.Exception.Response.StatusCode.value__ -eq 400) "Seller review of own book rejected"
}

# 1.5 Seller progresses delivery through to DELIVERED
Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order1.id)/status" -Method Patch -Headers $headersSeller -ContentType "application/json" -Body (@{ status = "PACKED"; note = "Book packed in bubble wrap" } | ConvertTo-Json) | Out-Null
Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order1.id)/status" -Method Patch -Headers $headersSeller -ContentType "application/json" -Body (@{ status = "SHIPPED"; note = "Handed to courier" } | ConvertTo-Json) | Out-Null
Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order1.id)/status" -Method Patch -Headers $headersSeller -ContentType "application/json" -Body (@{ status = "OUT_FOR_DELIVERY"; note = "Out for delivery" } | ConvertTo-Json) | Out-Null
$deliveredOrder = (Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order1.id)/status" -Method Patch -Headers $headersSeller -ContentType "application/json" -Body (@{ status = "DELIVERED"; note = "Delivered to buyer" } | ConvertTo-Json)).order
Assert-Condition ($deliveredOrder.status -eq "DELIVERED") "Order progressed to DELIVERED"

# 1.6 Buyer submits valid review after delivery
$reviewRes1 = Invoke-RestMethod -Uri "$baseUrl/api/reviews" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{
    orderId = $order1.id
    bookId = $book1.id
    rating = 5
    comment = "Outstanding book, mint condition, arrived fast!"
} | ConvertTo-Json)
$review1 = $reviewRes1.review
Assert-Condition ($review1.id -ne $null) "Review created successfully"
Assert-Condition ($review1.rating -eq 5) "Review rating is 5"

# 1.7 Buyer attempts duplicate review for same order & book (Should fail)
try {
    Invoke-RestMethod -Uri "$baseUrl/api/reviews" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{
        orderId = $order1.id
        bookId = $book1.id
        rating = 4
        comment = "Duplicate review attempt"
    } | ConvertTo-Json) | Out-Null
    Assert-Condition $false "Duplicate review should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 400) "Duplicate review rejected with 400 Bad Request"
}

# 1.8 Public review query (No private order/payment info leaked)
$publicReviews = Invoke-RestMethod -Uri "$baseUrl/api/books/$($book1.id)/reviews" -Method Get
Assert-Condition ($publicReviews.reviews.Count -ge 1) "Public reviews found for Book A1"
$pubRev = $publicReviews.reviews[0]
Assert-Condition ($pubRev.reviewerUsername -ne $null) "Public review contains reviewer username"
Assert-Condition ($pubRev.rating -eq 5) "Public review contains rating"
Assert-Condition ($pubRev.comment -ne $null) "Public review contains comment text"
Assert-Condition ($pubRev.payment -eq $null) "Public review does NOT contain payment info"
Assert-Condition ($pubRev.orderId -eq $null) "Public review does NOT leak order ID"

# 1.9 Verify Book rating aggregate
$book1Details = (Invoke-RestMethod -Uri "$baseUrl/api/books/$($book1.id)" -Method Get -Headers $headersSeller).book
Assert-Condition ($book1Details.averageRating -eq 5.0) "Book averageRating is 5.0"
Assert-Condition ($book1Details.reviewCount -eq 1) "Book reviewCount is 1"

# 1.10 Buyer edits own review (Updates rating to 4)
$editRes = Invoke-RestMethod -Uri "$baseUrl/api/reviews/$($review1.id)" -Method Put -Headers $headersBuyer -ContentType "application/json" -Body (@{
    rating = 4
    comment = "Still very good, but slight spine crease."
} | ConvertTo-Json)
Assert-Condition ($editRes.review.rating -eq 4) "Review rating updated to 4"

# 1.11 Verify updated Book rating aggregate
$book1DetailsAfterEdit = (Invoke-RestMethod -Uri "$baseUrl/api/books/$($book1.id)" -Method Get -Headers $headersSeller).book
Assert-Condition ($book1DetailsAfterEdit.averageRating -eq 4.0) "Book averageRating updated to 4.0"

# 1.12 Unauthorized user cannot edit or delete review
try {
    Invoke-RestMethod -Uri "$baseUrl/api/reviews/$($review1.id)" -Method Delete -Headers $headersOther | Out-Null
    Assert-Condition $false "Unauthorized review deletion should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 403) "Unauthorized review deletion rejected with 403 Forbidden"
}

# 1.13 Buyer views own reviews
$myReviews = Invoke-RestMethod -Uri "$baseUrl/api/reviews/my" -Method Get -Headers $headersBuyer
Assert-Condition ($myReviews.reviews.Count -ge 1) "Buyer can retrieve their own reviews"

Write-Host "`n=== SECTION 2: ORDER CANCELLATION & INVENTORY RESTORATION ===" -ForegroundColor Cyan

# 2.1 Seller lists Book A2
$book2 = (Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -Headers $headersSeller -ContentType "application/json" -Body (@{
    title = "C++ Concurrency in Action"
    author = "Anthony Williams"
    isbn = "9781617294693"
    price = 750
    condition = "New"
    category = "Technology"
    description = "Multithreading and concurrency in C++"
} | ConvertTo-Json)).book

# 2.2 Buyer buys Book A2
Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{ bookId = $book2.id } | ConvertTo-Json) | Out-Null
$orderRes2 = Invoke-RestMethod -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{
    paymentMethod = "UPI"
    shippingAddress = @{ fullName = "Buyer Bob"; addressLine = "123 Tech Blvd"; city = "Bengaluru"; state = "Karnataka"; postalCode = "560001"; phone = "9876543210" }
} | ConvertTo-Json)
$order2 = $orderRes2.order

# Verify Book A2 is sold and not in Browse
$browseBeforeCancel = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Get
$foundB2 = $browseBeforeCancel.books | Where-Object { $_.id -eq $book2.id }
Assert-Condition ($foundB2 -eq $null) "Book A2 is not in public browse while sold"

# 2.3 Buyer cancels order 2 while in PLACED state
$cancelRes = Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order2.id)/cancel" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{
    reason = "Changed mind before shipping"
} | ConvertTo-Json)
Assert-Condition ($cancelRes.order.status -eq "CANCELLED") "Order status transitioned to CANCELLED"
Assert-Condition ($cancelRes.order.payment.status -eq "refunded") "Payment status transitioned to refunded"

# 2.4 Verify Book A2 inventory availability is restored!
$browseAfterCancel = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Get
$restoredB2 = $browseAfterCancel.books | Where-Object { $_.id -eq $book2.id }
Assert-Condition ($restoredB2 -ne $null -and $restoredB2.status -eq "available") "Book A2 successfully restored to available in Browse!"

# 2.5 Cancellation not allowed after order has reached SHIPPED
# Seller lists Book A3
$book3 = (Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -Headers $headersSeller -ContentType "application/json" -Body (@{
    title = "Effective Modern C++"
    author = "Scott Meyers"
    isbn = "9781491903995"
    price = 650
    condition = "Good"
    category = "Technology"
    description = "42 specific ways to improve C++11 and C++14"
} | ConvertTo-Json)).book

Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{ bookId = $book3.id } | ConvertTo-Json) | Out-Null
$order3 = (Invoke-RestMethod -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{
    paymentMethod = "CARD"
    shippingAddress = @{ fullName = "Buyer Bob"; addressLine = "123 Tech Blvd"; city = "Bengaluru"; state = "Karnataka"; postalCode = "560001"; phone = "9876543210" }
} | ConvertTo-Json)).order

# Advance to SHIPPED
Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order3.id)/status" -Method Patch -Headers $headersSeller -ContentType "application/json" -Body (@{ status = "SHIPPED"; note = "In transit" } | ConvertTo-Json) | Out-Null

# Buyer attempts cancellation on SHIPPED order (Should fail)
try {
    Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order3.id)/cancel" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{ reason = "Too late" } | ConvertTo-Json) | Out-Null
    Assert-Condition $false "Cancelling a shipped order should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 400) "Cancellation of SHIPPED order rejected with 400 Bad Request"
}

Write-Host "`n=== SECTION 3: RETURN WORKFLOW ===" -ForegroundColor Cyan

# 3.1 Advance order 3 to DELIVERED
Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order3.id)/status" -Method Patch -Headers $headersSeller -ContentType "application/json" -Body (@{ status = "DELIVERED"; note = "Delivered package" } | ConvertTo-Json) | Out-Null

# 3.2 Buyer requests return on order 3
$returnReqRes = Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order3.id)/return" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{
    reason = "Wrong edition received"
} | ConvertTo-Json)
Assert-Condition ($returnReqRes.order.status -eq "RETURN_REQUESTED") "Order status is now RETURN_REQUESTED"

# 3.3 Seller rejects return
$rejectReturnRes = Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order3.id)/return/process" -Method Post -Headers $headersSeller -ContentType "application/json" -Body (@{
    approve = $false
    note = "Item matches listing description exactly"
} | ConvertTo-Json)
Assert-Condition ($rejectReturnRes.order.status -eq "RETURN_REJECTED") "Order status transitioned to RETURN_REJECTED"

# 3.4 Successful return flow: Book A4
$book4 = (Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -Headers $headersSeller -ContentType "application/json" -Body (@{
    title = "Operating System Concepts"
    author = "Silberschatz"
    isbn = "9781118063330"
    price = 800
    condition = "Acceptable"
    category = "Technology"
    description = "Classic dinosaur book"
} | ConvertTo-Json)).book

Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{ bookId = $book4.id } | ConvertTo-Json) | Out-Null
$order4 = (Invoke-RestMethod -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{
    paymentMethod = "UPI"
    shippingAddress = @{ fullName = "Buyer Bob"; addressLine = "123 Tech Blvd"; city = "Bengaluru"; state = "Karnataka"; postalCode = "560001"; phone = "9876543210" }
} | ConvertTo-Json)).order

# Seller delivers order 4
Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order4.id)/status" -Method Patch -Headers $headersSeller -ContentType "application/json" -Body (@{ status = "DELIVERED"; note = "Delivered" } | ConvertTo-Json) | Out-Null

# Buyer requests return
Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order4.id)/return" -Method Post -Headers $headersBuyer -ContentType "application/json" -Body (@{
    reason = "Damaged binding"
} | ConvertTo-Json) | Out-Null

# Seller approves return
$approveReturnRes = Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order4.id)/return/process" -Method Post -Headers $headersSeller -ContentType "application/json" -Body (@{
    approve = $true
    note = "Return received and accepted"
} | ConvertTo-Json)
Assert-Condition ($approveReturnRes.order.status -eq "RETURNED") "Order status is now RETURNED"
Assert-Condition ($approveReturnRes.order.payment.status -eq "refunded") "Payment status is refunded"

# 3.5 Conservative Listing State: Book A4 status is 'returned' and NOT available in Browse
$book4Details = (Invoke-RestMethod -Uri "$baseUrl/api/books/$($book4.id)" -Method Get -Headers $headersSeller).book
Assert-Condition ($book4Details.status -eq "returned") "Book A4 listing status is 'returned'"

$browseAfterReturn = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Get
$foundB4 = $browseAfterReturn.books | Where-Object { $_.id -eq $book4.id }
Assert-Condition ($foundB4 -eq $null) "Returned book does NOT automatically reappear in public browse"

# 3.6 Attempting to add 'returned' book to cart fails
try {
    Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersOther -ContentType "application/json" -Body (@{ bookId = $book4.id } | ConvertTo-Json) | Out-Null
    Assert-Condition $false "Adding returned book to cart should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode.value__ -eq 400) "Adding returned book to cart rejected with 400 Bad Request"
}

Write-Host "`nALL PHASE 4 INTEGRATION TESTS PASSED SUCCESSFULLY!" -ForegroundColor Green
