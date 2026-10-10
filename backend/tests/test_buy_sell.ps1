$baseUrl = "http://localhost:8080"

function Assert-Condition($condition, $message) {
    if ($condition) {
        Write-Host "[PASS] $message" -ForegroundColor Green
    } else {
        Write-Host "[FAIL] $message" -ForegroundColor Red
        throw "Assertion failed: $message"
    }
}

Write-Host "=== 1. SETUP USERS & AUTHENTICATION ==="
$rand = Get-Random -Minimum 1000 -Maximum 9999
$buyerData = @{
    username = "buyer_$rand"
    email = "buyer_$rand@example.com"
    password = "Password123!"
} | ConvertTo-Json

$sellerData = @{
    username = "seller_$rand"
    email = "seller_$rand@example.com"
    password = "Password123!"
} | ConvertTo-Json

$otherData = @{
    username = "other_$rand"
    email = "other_$rand@example.com"
    password = "Password123!"
} | ConvertTo-Json

Invoke-RestMethod -Uri "$baseUrl/api/auth/signup" -Method Post -ContentType "application/json" -Body $buyerData | Out-Null
Invoke-RestMethod -Uri "$baseUrl/api/auth/signup" -Method Post -ContentType "application/json" -Body $sellerData | Out-Null
Invoke-RestMethod -Uri "$baseUrl/api/auth/signup" -Method Post -ContentType "application/json" -Body $otherData | Out-Null

$buyerLogin = Invoke-RestMethod -Uri "$baseUrl/api/auth/login" -Method Post -ContentType "application/json" -Body $buyerData
$buyerToken = $buyerLogin.accessToken
$buyerHeaders = @{ Authorization = "Bearer $buyerToken" }

$sellerLogin = Invoke-RestMethod -Uri "$baseUrl/api/auth/login" -Method Post -ContentType "application/json" -Body $sellerData
$sellerToken = $sellerLogin.accessToken
$sellerHeaders = @{ Authorization = "Bearer $sellerToken" }

$otherLogin = Invoke-RestMethod -Uri "$baseUrl/api/auth/login" -Method Post -ContentType "application/json" -Body $otherData
$otherToken = $otherLogin.accessToken
$otherHeaders = @{ Authorization = "Bearer $otherToken" }

Assert-Condition (-not [string]::IsNullOrEmpty($buyerToken)) "Buyer authenticated"
Assert-Condition (-not [string]::IsNullOrEmpty($sellerToken)) "Seller authenticated"

Write-Host "`n=== 2. CREATE SELLER LISTINGS ==="
$book1 = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Headers $sellerHeaders -Body (@{
    title = "Design Patterns"
    author = "Erich Gamma et al."
    category = "Technology"
    price = 500.0
    condition = "Like New"
} | ConvertTo-Json)
$book1Id = $book1.book.id

$book2 = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Headers $sellerHeaders -Body (@{
    title = "Refactoring"
    author = "Martin Fowler"
    category = "Technology"
    price = 350.0
    condition = "Very Good"
} | ConvertTo-Json)
$book2Id = $book2.book.id

$book3 = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Headers $sellerHeaders -Body (@{
    title = "Domain-Driven Design"
    author = "Eric Evans"
    category = "Technology"
    price = 450.0
    condition = "Good"
} | ConvertTo-Json)
$book3Id = $book3.book.id

Assert-Condition (-not [string]::IsNullOrEmpty($book1Id)) "Listing 1 created ($book1Id)"
Assert-Condition (-not [string]::IsNullOrEmpty($book2Id)) "Listing 2 created ($book2Id)"
Assert-Condition (-not [string]::IsNullOrEmpty($book3Id)) "Listing 3 created ($book3Id)"

Write-Host "`n=== 3. TEST CART OPERATIONS & RESTRICTIONS ==="
# Unauthenticated cart access
try {
    Invoke-RestMethod -Uri "$baseUrl/api/cart" -Method Get
    Assert-Condition $false "Unauthenticated cart access should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 401) "GET /api/cart returns 401 Unauthorized without token"
}

# Seller tries to add own book to cart
try {
    Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -ContentType "application/json" -Headers $sellerHeaders -Body (@{ bookId = $book1Id } | ConvertTo-Json)
    Assert-Condition $false "Seller should not be able to add own book to cart"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 400) "Adding own listing to cart returns 400 Bad Request"
}

# Buyer adds Book 1 to cart
$addRes1 = Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -ContentType "application/json" -Headers $buyerHeaders -Body (@{ bookId = $book1Id } | ConvertTo-Json)
Assert-Condition ($addRes1.success -eq $true) "Book 1 added to buyer's cart"

# Buyer tries to add Book 1 AGAIN (quantity > 1 violation)
try {
    Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -ContentType "application/json" -Headers $buyerHeaders -Body (@{ bookId = $book1Id } | ConvertTo-Json)
    Assert-Condition $false "Duplicate add should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 400) "Adding duplicate book rejected (quantity limit = 1)"
}

# Buyer adds Book 2 to cart
$addRes2 = Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -ContentType "application/json" -Headers $buyerHeaders -Body (@{ bookId = $book2Id } | ConvertTo-Json)
Assert-Condition ($addRes2.success -eq $true) "Book 2 added to buyer's cart"

# Verify Cart Contents & Pricing
$cart = Invoke-RestMethod -Uri "$baseUrl/api/cart" -Method Get -Headers $buyerHeaders
Assert-Condition ($cart.cart.items.Count -eq 2) "Cart contains 2 items"
Assert-Condition ($cart.cart.subtotal -eq 850.0) "Cart subtotal is 850.0 (500 + 350)"
Assert-Condition ($cart.cart.shippingCost -eq 0.0) "Shipping cost is 0.0"
Assert-Condition ($cart.cart.total -eq 850.0) "Cart total is 850.0"
Assert-Condition ($cart.cart.hasUnavailableItems -eq $false) "hasUnavailableItems is false"

# Test removing an item from cart
$remRes = Invoke-RestMethod -Uri "$baseUrl/api/cart/items/$book2Id" -Method Delete -Headers $buyerHeaders
Assert-Condition ($remRes.success -eq $true) "Item removed from cart"
$cartAfterRem = Invoke-RestMethod -Uri "$baseUrl/api/cart" -Method Get -Headers $buyerHeaders
Assert-Condition ($cartAfterRem.cart.items.Count -eq 1) "Cart has 1 item after removal"
Assert-Condition ($cartAfterRem.cart.subtotal -eq 500.0) "Cart subtotal updated to 500.0"

# Re-add Book 2
Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -ContentType "application/json" -Headers $buyerHeaders -Body (@{ bookId = $book2Id } | ConvertTo-Json) | Out-Null

Write-Host "`n=== 4. TEST STALE CART HANDLING ==="
# Add Book 3 to cart
Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -ContentType "application/json" -Headers $buyerHeaders -Body (@{ bookId = $book3Id } | ConvertTo-Json) | Out-Null

# Seller deletes/removes Book 3 externally
Invoke-RestMethod -Uri "$baseUrl/api/books/$book3Id" -Method Delete -Headers $sellerHeaders | Out-Null

# Buyer checks cart: Book 3 should now be marked isAvailable = false
$staleCart = Invoke-RestMethod -Uri "$baseUrl/api/cart" -Method Get -Headers $buyerHeaders
Assert-Condition ($staleCart.cart.hasUnavailableItems -eq $true) "Cart detects stale unavailable items"
$item3 = $staleCart.cart.items | Where-Object { $_.bookId -eq $book3Id }
Assert-Condition ($item3.isAvailable -eq $false) "Stale book item has isAvailable: false"

# Attempting checkout with stale item MUST fail
$shippingAddr = @{
    fullName = "Alice Buyer"
    addressLine = "456 Market St"
    city = "Mumbai"
    state = "Maharashtra"
    postalCode = "400001"
    phone = "9876543210"
}
try {
    Invoke-RestMethod -Uri "$baseUrl/api/checkout" -Method Post -ContentType "application/json" -Headers $buyerHeaders -Body (@{
        paymentMethod = "UPI"
        shippingAddress = $shippingAddr
    } | ConvertTo-Json)
    Assert-Condition $false "Checkout with stale item should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 409) "Checkout with stale item returns 409 Conflict"
}

# Buyer removes the unavailable book from cart
Invoke-RestMethod -Uri "$baseUrl/api/cart/items/$book3Id" -Method Delete -Headers $buyerHeaders | Out-Null
$cleanCart = Invoke-RestMethod -Uri "$baseUrl/api/cart" -Method Get -Headers $buyerHeaders
Assert-Condition ($cleanCart.cart.hasUnavailableItems -eq $false) "Cart is clean after removing stale item"

Write-Host "`n=== 5. TEST DEMO PAYMENT FAILURE & INVENTORY ROLLBACK ==="
try {
    Invoke-RestMethod -Uri "$baseUrl/api/checkout" -Method Post -ContentType "application/json" -Headers $buyerHeaders -Body (@{
        paymentMethod = "UPI"
        shippingAddress = $shippingAddr
        simulatePaymentFailure = $true
    } | ConvertTo-Json)
    Assert-Condition $false "Checkout with simulated payment failure should fail"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 400) "Simulated payment failure returns 400 Bad Request"
}

# Verify books are STILL AVAILABLE in the marketplace
$book1Check = Invoke-RestMethod -Uri "$baseUrl/api/books/$book1Id" -Method Get
Assert-Condition ($book1Check.book.status -eq "available") "Book 1 remained available after payment failure"
$book2Check = Invoke-RestMethod -Uri "$baseUrl/api/books/$book2Id" -Method Get
Assert-Condition ($book2Check.book.status -eq "available") "Book 2 remained available after payment failure"

Write-Host "`n=== 6. TEST SUCCESSFUL CHECKOUT (MULTI-BOOK UPI PURCHASE) ==="
$orderRes = Invoke-RestMethod -Uri "$baseUrl/api/checkout" -Method Post -ContentType "application/json" -Headers $buyerHeaders -Body (@{
    paymentMethod = "UPI"
    shippingAddress = $shippingAddr
} | ConvertTo-Json)

Assert-Condition ($orderRes.success -eq $true) "Checkout succeeded"
$order = $orderRes.order
Assert-Condition (-not [string]::IsNullOrEmpty($order.id)) "Order created with ID: $($order.id)"
Assert-Condition ($order.orderNumber.StartsWith("ORD-")) "Order number formatted: $($order.orderNumber)"
Assert-Condition ($order.subtotal -eq 850.0) "Server-side subtotal is 850.0"
Assert-Condition ($order.shippingCost -eq 0.0) "Shipping cost is 0.0"
Assert-Condition ($order.total -eq 850.0) "Server-side total is 850.0"
Assert-Condition ($order.payment.status -eq "paid") "Payment status is 'paid'"
Assert-Condition ($order.payment.transactionId.StartsWith("DEMO_TXN_")) "Transaction ID generated: $($order.payment.transactionId)"
Assert-Condition ($order.shipping.trackingId.StartsWith("DEMO_TRACK_")) "Tracking ID generated: $($order.shipping.trackingId)"
Assert-Condition ($order.items.Count -eq 2) "Order contains 2 item snapshots"

# Verify Cart was cleared of purchased books
$postCart = Invoke-RestMethod -Uri "$baseUrl/api/cart" -Method Get -Headers $buyerHeaders
Assert-Condition ($postCart.cart.items.Count -eq 0) "Purchased books cleared from buyer's cart"

# Verify Books now disappear from public Browse
$browse = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Get
$foundB1 = $browse.books | Where-Object { $_.id -eq $book1Id }
$foundB2 = $browse.books | Where-Object { $_.id -eq $book2Id }
Assert-Condition ($null -eq $foundB1 -and $null -eq $foundB2) "Purchased books no longer appear in public Browse"

Write-Host "`n=== 7. TEST COD CHECKOUT ==="
# Seller creates Book 4
$book4 = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Headers $sellerHeaders -Body (@{
    title = "Structure and Interpretation of Computer Programs"
    author = "Harold Abelson"
    category = "Computer Science"
    price = 600.0
    condition = "Like New"
} | ConvertTo-Json)
$book4Id = $book4.book.id

# Buyer adds Book 4 to cart and checks out via COD
Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -ContentType "application/json" -Headers $buyerHeaders -Body (@{ bookId = $book4Id } | ConvertTo-Json) | Out-Null
$codOrderRes = Invoke-RestMethod -Uri "$baseUrl/api/checkout" -Method Post -ContentType "application/json" -Headers $buyerHeaders -Body (@{
    paymentMethod = "COD"
    shippingAddress = $shippingAddr
} | ConvertTo-Json)

Assert-Condition ($codOrderRes.order.payment.method -eq "COD") "Payment method is COD"
Assert-Condition ($codOrderRes.order.payment.status -eq "pending") "COD payment status is pending"
Assert-Condition ($codOrderRes.order.status -eq "CONFIRMED") "COD order status is CONFIRMED"

Write-Host "`n=== 8. TEST CONCURRENCY / DOUBLE-PURCHASE REJECTION ==="
# Seller creates Book 5
$book5 = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Headers $sellerHeaders -Body (@{
    title = "Operating System Concepts"
    author = "Silberschatz"
    category = "Computer Science"
    price = 700.0
    condition = "Very Good"
} | ConvertTo-Json)
$book5Id = $book5.book.id

# Buyer adds Book 5 to cart
Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -ContentType "application/json" -Headers $buyerHeaders -Body (@{ bookId = $book5Id } | ConvertTo-Json) | Out-Null

# Other user also adds Book 5 to cart
Invoke-RestMethod -Uri "$baseUrl/api/cart/items" -Method Post -ContentType "application/json" -Headers $otherHeaders -Body (@{ bookId = $book5Id } | ConvertTo-Json) | Out-Null

# Buyer successfully checks out Book 5
$firstCheckout = Invoke-RestMethod -Uri "$baseUrl/api/checkout" -Method Post -ContentType "application/json" -Headers $buyerHeaders -Body (@{
    paymentMethod = "UPI"
    shippingAddress = $shippingAddr
} | ConvertTo-Json)
Assert-Condition ($firstCheckout.success -eq $true) "Buyer 1 acquired Book 5"

# Other user now tries to check out Book 5 -> MUST BE REJECTED
try {
    Invoke-RestMethod -Uri "$baseUrl/api/checkout" -Method Post -ContentType "application/json" -Headers $otherHeaders -Body (@{
        paymentMethod = "UPI"
        shippingAddress = $shippingAddr
    } | ConvertTo-Json)
    Assert-Condition $false "Concurrent buyer 2 should not be able to purchase already sold book"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 409) "Concurrent checkout rejected with 409 Conflict"
}

Write-Host "`n=== 9. TEST ORDER HISTORY & PRIVACY ==="
# Buyer views their order history
$buyerOrders = Invoke-RestMethod -Uri "$baseUrl/api/orders" -Method Get -Headers $buyerHeaders
Assert-Condition ($buyerOrders.orders.Count -ge 3) "Buyer sees all placed orders ($($buyerOrders.orders.Count))"

# Buyer inspects single order details
$singleOrder = Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order.id)" -Method Get -Headers $buyerHeaders
Assert-Condition ($singleOrder.order.id -eq $order.id) "Buyer can fetch own order details"

# Other user attempts to view buyer's private order -> 403 Forbidden
try {
    Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order.id)" -Method Get -Headers $otherHeaders
    Assert-Condition $false "Unauthorized user should not view another user's order"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 403) "Unauthorized order access rejected with 403 Forbidden"
}

# Seller views sales history
$sellerSales = Invoke-RestMethod -Uri "$baseUrl/api/orders/seller/history" -Method Get -Headers $sellerHeaders
Assert-Condition ($sellerSales.orders.Count -ge 1) "Seller sees their sales history"
$sellerItem = $sellerSales.orders[0].items | Where-Object { $_.sellerId -eq $sellerLogin.user.id }
Assert-Condition ($null -ne $sellerItem) "Seller sales record contains sold items"

Write-Host "`n=== 10. TEST DELIVERY LIFECYCLE PROGRESSION ==="
# Buyer attempts to advance delivery status -> Forbidden
try {
    Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order.id)/status" -Method Patch -ContentType "application/json" -Headers $buyerHeaders -Body (@{
        status = "DELIVERED"
    } | ConvertTo-Json)
    Assert-Condition $false "Buyer cannot mark order delivered"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 403) "Buyer forbidden from changing delivery status (403)"
}

# Seller advances status: CONFIRMED -> PACKED -> SHIPPED -> OUT_FOR_DELIVERY -> DELIVERED
$statusSteps = @("PACKED", "SHIPPED", "OUT_FOR_DELIVERY", "DELIVERED")
foreach ($step in $statusSteps) {
    $patchRes = Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order.id)/status" -Method Patch -ContentType "application/json" -Headers $sellerHeaders -Body (@{
        status = $step
        note = "Package transitioned to $step"
    } | ConvertTo-Json)
    Assert-Condition ($patchRes.order.status -eq $step) "Order status transitioned to $step"
    Assert-Condition ($patchRes.order.shipping.status -eq $step) "Shipping status updated to $step"
}

# Verify final history count
$finalOrder = Invoke-RestMethod -Uri "$baseUrl/api/orders/$($order.id)" -Method Get -Headers $buyerHeaders
Assert-Condition ($finalOrder.order.status -eq "DELIVERED") "Final order state is DELIVERED"
Assert-Condition ($finalOrder.order.history.Count -ge 5) "Full audit trail preserved in history ($($finalOrder.order.history.Count) entries)"

Write-Host "`n========================================="
Write-Host "ALL BUY/SELL VERTICAL SLICE TESTS PASSED!" -ForegroundColor Green
Write-Host "========================================="
