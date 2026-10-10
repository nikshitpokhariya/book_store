$baseUrl = "http://localhost:8080"

function Assert-Condition($condition, $message) {
    if ($condition) {
        Write-Host "[PASS] $message" -ForegroundColor Green
    } else {
        Write-Host "[FAIL] $message" -ForegroundColor Red
        throw "Assertion failed: $message"
    }
}

Write-Host "=== 1. TEST AUTHENTICATION ==="
$rand = Get-Random -Minimum 1000 -Maximum 9999
$userA = @{
    username = "seller_$rand"
    email = "seller_$rand@example.com"
    password = "Password123!"
} | ConvertTo-Json

$userB = @{
    username = "buyer_$rand"
    email = "buyer_$rand@example.com"
    password = "Password123!"
} | ConvertTo-Json

$signupA = Invoke-RestMethod -Uri "$baseUrl/api/auth/signup" -Method Post -ContentType "application/json" -Body $userA
Assert-Condition ($signupA.success -eq $true) "User A signup succeeded"

$signupB = Invoke-RestMethod -Uri "$baseUrl/api/auth/signup" -Method Post -ContentType "application/json" -Body $userB
Assert-Condition ($signupB.success -eq $true) "User B signup succeeded"

$loginA = Invoke-RestMethod -Uri "$baseUrl/api/auth/login" -Method Post -ContentType "application/json" -Body $userA
$tokenA = $loginA.accessToken
Assert-Condition (-not [string]::IsNullOrEmpty($tokenA)) "User A logged in with token"

$loginB = Invoke-RestMethod -Uri "$baseUrl/api/auth/login" -Method Post -ContentType "application/json" -Body $userB
$tokenB = $loginB.accessToken
Assert-Condition (-not [string]::IsNullOrEmpty($tokenB)) "User B logged in with token"

# Test /api/auth/me
$headersA = @{ Authorization = "Bearer $tokenA" }
$headersB = @{ Authorization = "Bearer $tokenB" }

$meA = Invoke-RestMethod -Uri "$baseUrl/api/auth/me" -Method Get -Headers $headersA
Assert-Condition ($meA.user.username -eq "seller_$rand") "Auth /api/auth/me returned correct user profile"

Write-Host "`n=== 2. TEST UNAUTHENTICATED ACCESS TO PROTECTED OPERATIONS ==="
try {
    Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Body "{}"
    Assert-Condition $false "POST /api/books should fail without auth"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 401) "POST /api/books returns 401 Unauthorized without token"
}

try {
    Invoke-RestMethod -Uri "$baseUrl/api/books/my" -Method Get
    Assert-Condition $false "GET /api/books/my should fail without auth"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 401) "GET /api/books/my returns 401 Unauthorized without token"
}

Write-Host "`n=== 3. TEST INVALID INPUT VALIDATION ==="
# Missing fields
try {
    Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Headers $headersA -Body (@{
        title = "Incomplete Book"
    } | ConvertTo-Json)
    Assert-Condition $false "Should fail with missing fields"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 400) "POST /api/books returns 400 when required fields are missing"
}

# Negative price
try {
    Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Headers $headersA -Body (@{
        title = "Negative Price Book"
        author = "Author Name"
        category = "Fiction"
        price = -50
        condition = "Good"
    } | ConvertTo-Json)
    Assert-Condition $false "Should fail with negative price"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 400) "POST /api/books returns 400 when price is negative"
}

# Invalid condition
try {
    Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Headers $headersA -Body (@{
        title = "Invalid Condition Book"
        author = "Author Name"
        category = "Fiction"
        price = 100
        condition = "Destroyed"
    } | ConvertTo-Json)
    Assert-Condition $false "Should fail with invalid condition"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 400) "POST /api/books returns 400 when condition is invalid"
}

# Invalid ISBN
try {
    Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Headers $headersA -Body (@{
        title = "Bad ISBN Book"
        author = "Author Name"
        category = "Fiction"
        price = 100
        condition = "Good"
        isbn = "not-an-isbn!@#"
    } | ConvertTo-Json)
    Assert-Condition $false "Should fail with bad ISBN"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 400) "POST /api/books returns 400 when ISBN has invalid chars"
}

Write-Host "`n=== 4. TEST AUTHENTICATED LISTING CREATION ==="
$book1Data = @{
    title = "Clean Code"
    author = "Robert C. Martin"
    isbn = "978-0132350884"
    description = "A handbook of agile software craftsmanship"
    category = "Technology"
    price = 400.0
    condition = "Very Good"
    coverImage = "https://example.com/clean_code.jpg"
} | ConvertTo-Json

$create1 = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Headers $headersA -Body $book1Data
Assert-Condition ($create1.success -eq $true) "Book 1 created successfully"
$book1Id = $create1.book.id
Assert-Condition (-not [string]::IsNullOrEmpty($book1Id)) "Created book has valid ID: $book1Id"
Assert-Condition ($create1.book.owner.username -eq "seller_$rand") "Created book has correct seller username"
Assert-Condition ($create1.book.status -eq "available") "Book defaults to available status"

$book2Data = @{
    title = "The Pragmatic Programmer"
    author = "Andrew Hunt and David Thomas"
    isbn = "978-0201616224"
    description = "From journeyman to master"
    category = "Technology"
    price = 600.0
    condition = "Like New"
} | ConvertTo-Json

$create2 = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Headers $headersA -Body $book2Data
$book2Id = $create2.book.id

$book3Data = @{
    title = "Dune"
    author = "Frank Herbert"
    isbn = "978-0441172719"
    description = "Epic science fiction masterpiece"
    category = "Sci-Fi"
    price = 250.0
    condition = "Good"
} | ConvertTo-Json

$create3 = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Post -ContentType "application/json" -Headers $headersB -Body $book3Data
$book3Id = $create3.book.id
Assert-Condition ($create3.book.owner.username -eq "buyer_$rand") "Book 3 owned by User B"

Write-Host "`n=== 5. TEST PUBLIC BROWSE ==="
$browse = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Get
Assert-Condition ($browse.success -eq $true) "Public Browse returned success"
Assert-Condition ($browse.books.Count -ge 3) "Browse returned all available books ($($browse.books.Count))"
$firstBook = $browse.books[0]
Assert-Condition ($null -eq $firstBook.passwordHash) "Seller password hash is not exposed"
Assert-Condition ($null -eq $firstBook.email) "Seller email is not exposed"
Assert-Condition (-not [string]::IsNullOrEmpty($firstBook.owner.username)) "Seller username is provided publicly"

Write-Host "`n=== 6. TEST SEARCH ==="
$searchTitle = Invoke-RestMethod -Uri "$baseUrl/api/books?search=clean" -Method Get
Assert-Condition ($searchTitle.books.Count -ge 1) "Search by title found results"
Assert-Condition ($searchTitle.books[0].title -eq "Clean Code") "Search found 'Clean Code'"

$searchAuthor = Invoke-RestMethod -Uri "$baseUrl/api/books?search=Herbert" -Method Get
Assert-Condition ($searchAuthor.books.Count -ge 1) "Search by author found Dune"
Assert-Condition ($searchAuthor.books[0].title -eq "Dune") "Search matched author Frank Herbert"

$searchIsbn = Invoke-RestMethod -Uri "$baseUrl/api/books?search=0201616224" -Method Get
Assert-Condition ($searchIsbn.books.Count -ge 1) "Search by ISBN found The Pragmatic Programmer"

Write-Host "`n=== 7. TEST FILTERING ==="
$filterCategory = Invoke-RestMethod -Uri "$baseUrl/api/books?category=Sci-Fi" -Method Get
Assert-Condition ($filterCategory.books.Count -ge 1 -and ($filterCategory.books | Where-Object { $_.category -ne "Sci-Fi" }).Count -eq 0) "Filter by category 'Sci-Fi' returned only Sci-Fi books"

$filterCondition = Invoke-RestMethod -Uri "$baseUrl/api/books?condition=Like New" -Method Get
Assert-Condition ($filterCondition.books.Count -ge 1 -and $filterCondition.books[0].condition -eq "Like New") "Filter by condition 'Like New' succeeded"

$filterPrice = Invoke-RestMethod -Uri "$baseUrl/api/books?minPrice=300&maxPrice=500" -Method Get
Assert-Condition ($filterPrice.books.Count -ge 1 -and ($filterPrice.books | Where-Object { $_.price -lt 300 -or $_.price -gt 500 }).Count -eq 0) "Filter by price range [300, 500] returned books in range"

Write-Host "`n=== 8. TEST SORTING ==="
$sortPriceAsc = Invoke-RestMethod -Uri "$baseUrl/api/books?sort=price_asc" -Method Get
$prices = $sortPriceAsc.books | ForEach-Object { [double]$_.price }
$isSortedAsc = $true
for ($i = 0; $i -lt $prices.Count - 1; $i++) {
    if ($prices[$i] -gt $prices[$i+1]) { $isSortedAsc = $false }
}
Assert-Condition $isSortedAsc "sort=price_asc correctly sorted books by price ascending"

$sortPriceDesc = Invoke-RestMethod -Uri "$baseUrl/api/books?sort=price_desc" -Method Get
$pricesDesc = $sortPriceDesc.books | ForEach-Object { [double]$_.price }
$isSortedDesc = $true
for ($i = 0; $i -lt $pricesDesc.Count - 1; $i++) {
    if ($pricesDesc[$i] -lt $pricesDesc[$i+1]) { $isSortedDesc = $false }
}
Assert-Condition $isSortedDesc "sort=price_desc correctly sorted books by price descending"

Write-Host "`n=== 9. TEST PAGINATION ==="
$page1 = Invoke-RestMethod -Uri "$baseUrl/api/books?page=1&limit=2" -Method Get
Assert-Condition ($page1.books.Count -eq 2) "Page 1 returned limit of 2 items"
Assert-Condition ($page1.pagination.page -eq 1) "Pagination metadata page is 1"
Assert-Condition ($page1.pagination.limit -eq 2) "Pagination metadata limit is 2"
Assert-Condition ($page1.pagination.hasNextPage -eq $true) "Pagination hasNextPage is true"

$page2 = Invoke-RestMethod -Uri "$baseUrl/api/books?page=2&limit=2" -Method Get
Assert-Condition ($page2.books.Count -ge 1) "Page 2 returned remaining items"
Assert-Condition ($page2.pagination.page -eq 2) "Pagination metadata page is 2"
Assert-Condition ($page2.pagination.hasPrevPage -eq $true) "Pagination hasPrevPage is true"

Write-Host "`n=== 10. TEST BOOK DETAILS ==="
$details = Invoke-RestMethod -Uri "$baseUrl/api/books/$book1Id" -Method Get
Assert-Condition ($details.book.id -eq $book1Id) "Book details returned for valid ID"
Assert-Condition ($details.book.title -eq "Clean Code") "Book details title is accurate"
Assert-Condition ($details.book.owner.username -eq "seller_$rand") "Book details include seller public info"

# Invalid ID format
try {
    Invoke-RestMethod -Uri "$baseUrl/api/books/invalid-hex-id" -Method Get
    Assert-Condition $false "Should fail with invalid ID"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 400) "GET with invalid hex ID returns 400 Bad Request"
}

# Non-existent ID
try {
    Invoke-RestMethod -Uri "$baseUrl/api/books/000000000000000000000000" -Method Get
    Assert-Condition $false "Should fail with non-existent ID"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 404) "GET with non-existent ObjectId returns 404 Not Found"
}

Write-Host "`n=== 11. TEST OWNER OPERATIONS & AUTHORIZATION ==="
# User B attempts to edit User A's book
try {
    Invoke-RestMethod -Uri "$baseUrl/api/books/$book1Id" -Method Put -ContentType "application/json" -Headers $headersB -Body (@{
        price = 100.0
    } | ConvertTo-Json)
    Assert-Condition $false "Non-owner should not be able to update listing"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 403) "PUT by non-owner returns 403 Forbidden"
}

# User A updates their own book
$updatePayload = @{
    price = 450.0
    description = "Updated description with new notes"
} | ConvertTo-Json
$updateRes = Invoke-RestMethod -Uri "$baseUrl/api/books/$book1Id" -Method Put -ContentType "application/json" -Headers $headersA -Body $updatePayload
Assert-Condition ($updateRes.success -eq $true) "Owner update succeeded"
Assert-Condition ($updateRes.book.price -eq 450.0) "Updated price verified in response"

$verifyDetails = Invoke-RestMethod -Uri "$baseUrl/api/books/$book1Id" -Method Get
Assert-Condition ($verifyDetails.book.price -eq 450.0) "Updated price persisted in database"

# User's own listings endpoint
$myListings = Invoke-RestMethod -Uri "$baseUrl/api/books/my" -Method Get -Headers $headersA
Assert-Condition ($myListings.books.Count -ge 2) "Owner listings returned User A's books"

Write-Host "`n=== 12. TEST AVAILABILITY MANAGEMENT & EXCLUSION OF REMOVED LISTINGS ==="
# User B attempts to delete User A's book
try {
    Invoke-RestMethod -Uri "$baseUrl/api/books/$book1Id" -Method Delete -Headers $headersB
    Assert-Condition $false "Non-owner should not be able to delete listing"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 403) "DELETE by non-owner returns 403 Forbidden"
}

# User A removes their listing
$deleteRes = Invoke-RestMethod -Uri "$baseUrl/api/books/$book1Id" -Method Delete -Headers $headersA
Assert-Condition ($deleteRes.success -eq $true) "Owner removed book listing"

# Verify removed book is EXCLUDED from public Browse
$browseAfterDelete = Invoke-RestMethod -Uri "$baseUrl/api/books" -Method Get
$foundInBrowse = $browseAfterDelete.books | Where-Object { $_.id -eq $book1Id }
Assert-Condition ($null -eq $foundInBrowse) "Removed book is EXCLUDED from public Browse"

# Verify removed book is EXCLUDED from Search
$searchAfterDelete = Invoke-RestMethod -Uri "$baseUrl/api/books?search=Clean" -Method Get
$foundInSearch = $searchAfterDelete.books | Where-Object { $_.id -eq $book1Id }
Assert-Condition ($null -eq $foundInSearch) "Removed book is EXCLUDED from Search"

# Verify public GET /api/books/:id returns 404 for removed book
try {
    Invoke-RestMethod -Uri "$baseUrl/api/books/$book1Id" -Method Get
    Assert-Condition $false "Public GET for removed book should return 404"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 404) "Public GET for removed book returns 404 Not Found"
}

# Verify owner CAN still see their own removed book in /api/books/my
$myListingsAfter = Invoke-RestMethod -Uri "$baseUrl/api/books/my" -Method Get -Headers $headersA
$foundInMy = $myListingsAfter.books | Where-Object { $_.id -eq $book1Id }
Assert-Condition ($null -ne $foundInMy) "Owner can still see historical removed listing in /api/books/my"
Assert-Condition ($foundInMy.status -eq "removed") "Historical listing status is 'removed'"

# Verify owner cannot modify a removed listing
try {
    Invoke-RestMethod -Uri "$baseUrl/api/books/$book1Id" -Method Put -ContentType "application/json" -Headers $headersA -Body (@{ price = 999 } | ConvertTo-Json)
    Assert-Condition $false "Cannot modify removed listing"
} catch {
    Assert-Condition ($_.Exception.Response.StatusCode -eq 400) "PUT on removed listing returns 400 Bad Request"
}

Write-Host "`n========================================="
Write-Host "ALL 16 MARKETPLACE VERTICAL SLICE TESTS PASSED!" -ForegroundColor Green
Write-Host "========================================="
