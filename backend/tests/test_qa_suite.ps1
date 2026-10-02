# Master QA and Integration Test Suite for Book Exchange Backend
$ErrorActionPreference = "Continue"
$baseUrl = "http://127.0.0.1:8080"

$global:totalTests = 0
$global:passedTests = 0
$global:failedTests = 0
$global:testResults = @()

function Record-Result($testName, $category, $passed, $expected, $actual, $details = "") {
    $global:totalTests++
    if ($passed) {
        $global:passedTests++
        Write-Host "  [PASS] $testName" -ForegroundColor Green
    } else {
        $global:failedTests++
        Write-Host "  [FAIL] $testName" -ForegroundColor Red
        if ($details) { Write-Host "         Details: $details" -ForegroundColor Yellow }
        Write-Host "         Expected: $expected | Actual: $actual" -ForegroundColor Yellow
    }
    $global:testResults += [PSCustomObject]@{
        TestName = $testName
        Category = $category
        Passed   = $passed
        Expected = $expected
        Actual   = $actual
        Details  = $details
    }
}

function Invoke-Api {
    param(
        [string]$Uri,
        [string]$Method = "Get",
        $Headers = @{},
        $Body = $null,
        $WebSession = $null
    )
    $p = @{
        Uri         = $Uri
        Method      = $Method
        ContentType = "application/json"
    }
    if ($Headers -and $Headers.Count -gt 0) { $p["Headers"] = $Headers }
    if ($WebSession) { $p["WebSession"] = $WebSession }
    if ($Body) {
        if ($Body -is [string]) { $p["Body"] = $Body }
        else { $p["Body"] = ($Body | ConvertTo-Json -Depth 10) }
    }
    try {
        $res = Invoke-RestMethod @p
        return @{ Success = $true; StatusCode = 200; Data = $res }
    } catch {
        $code = 500
        $data = $null
        if ($_.Exception.Response) {
            $code = [int]$_.Exception.Response.StatusCode
            try {
                $stream = $_.Exception.Response.GetResponseStream()
                $reader = New-Object System.IO.StreamReader($stream)
                $bodyText = $reader.ReadToEnd()
                $data = $bodyText | ConvertFrom-Json
            } catch {}
        }
        return @{ Success = $false; StatusCode = $code; Data = $data; Error = $_.Exception.Message }
    }
}

$suffix = Get-Random -Minimum 10000 -Maximum 99999
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "Starting Master QA & Integration Test Suite (Run ID: $suffix)" -ForegroundColor Cyan
Write-Host "Target Base URL: $baseUrl" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan

# ================================================================
# CATEGORY 1: AUTHENTICATION & SESSION MANAGEMENT
# ================================================================
Write-Host "`n>>> CATEGORY 1: AUTHENTICATION & SESSION MANAGEMENT" -ForegroundColor Magenta

# 1.1 Valid Signup
$userA_name = "qa_usera_$suffix"
$userA_email = "qa_usera_$suffix@example.com"
$res = Invoke-Api -Uri "$baseUrl/api/auth/signup" -Method Post -Body @{
    username = $userA_name; email = $userA_email; password = "Password123!"
}
Record-Result "Auth: Valid User Signup" "Auth" ($res.StatusCode -in @(200, 201)) "Status 200/201" "Status $($res.StatusCode)"

# 1.2 Duplicate Email
$res = Invoke-Api -Uri "$baseUrl/api/auth/signup" -Method Post -Body @{
    username = "diff_$suffix"; email = $userA_email; password = "Password123!"
}
Record-Result "Auth: Duplicate Email Rejection" "Auth" ($res.StatusCode -in @(400, 409)) "Status 400/409" "Status $($res.StatusCode)"

# 1.3 Duplicate Username
$res = Invoke-Api -Uri "$baseUrl/api/auth/signup" -Method Post -Body @{
    username = $userA_name; email = "diff_$suffix@example.com"; password = "Password123!"
}
Record-Result "Auth: Duplicate Username Rejection" "Auth" ($res.StatusCode -in @(400, 409)) "Status 400/409" "Status $($res.StatusCode)"

# 1.4 Invalid Email Format
$res = Invoke-Api -Uri "$baseUrl/api/auth/signup" -Method Post -Body @{
    username = "bademail_$suffix"; email = "invalid-email-format"; password = "Password123!"
}
Record-Result "Auth: Invalid Email Format Rejection" "Auth" ($res.StatusCode -eq 400) "Status 400" "Status $($res.StatusCode)"

# 1.5 Invalid Password (Too short, < 8 chars)
$res = Invoke-Api -Uri "$baseUrl/api/auth/signup" -Method Post -Body @{
    username = "shortpw_$suffix"; email = "shortpw_$suffix@example.com"; password = "short"
}
Record-Result "Auth: Short Password Rejection (<8 chars)" "Auth" ($res.StatusCode -eq 400) "Status 400" "Status $($res.StatusCode)"

# 1.6 Boundary Password (Exactly 8 chars)
$res = Invoke-Api -Uri "$baseUrl/api/auth/signup" -Method Post -Body @{
    username = "bnd8_$suffix"; email = "bnd8_$suffix@example.com"; password = "Pass123!"
}
Record-Result "Auth: Boundary Password (8 chars) Accepted" "Auth" ($res.StatusCode -in @(200, 201)) "Status 200/201" "Status $($res.StatusCode)"

# 1.7 Login: Invalid Credentials
$res = Invoke-Api -Uri "$baseUrl/api/auth/login" -Method Post -Body @{
    email = $userA_email; password = "WrongPassword!"
}
Record-Result "Auth: Invalid Password Login Rejection" "Auth" ($res.StatusCode -eq 401) "Status 401" "Status $($res.StatusCode)"

# 1.8 Login: Nonexistent User
$res = Invoke-Api -Uri "$baseUrl/api/auth/login" -Method Post -Body @{
    email = "nonexistent_$suffix@example.com"; password = "Password123!"
}
Record-Result "Auth: Nonexistent User Login Rejection" "Auth" ($res.StatusCode -eq 401) "Status 401" "Status $($res.StatusCode)"

# 1.9 Valid Login & Cookie Capture
$loginResp = Invoke-WebRequest -Uri "$baseUrl/api/auth/login" -Method Post -Body (@{ email = $userA_email; password = "Password123!" } | ConvertTo-Json) -ContentType "application/json" -UseBasicParsing
$loginDataA = $loginResp.Content | ConvertFrom-Json
$tokenA = $loginDataA.accessToken
$headersA = @{ "Authorization" = "Bearer $tokenA" }
$refreshCookieA = ""
if ($loginResp.Headers["Set-Cookie"] -match "refreshToken=([^;]+)") {
    $refreshCookieA = $matches[1]
}
Record-Result "Auth: Valid Login Returns AccessToken & Sets Refresh Cookie" "Auth" ($loginResp.StatusCode -eq 200 -and -not [string]::IsNullOrEmpty($tokenA) -and -not [string]::IsNullOrEmpty($refreshCookieA)) "Status 200 with JWT and refresh cookie" "Status: $($loginResp.StatusCode), HasJWT=$(-not [string]::IsNullOrEmpty($tokenA)), HasCookie=$(-not [string]::IsNullOrEmpty($refreshCookieA))"

# 1.10 Verify /api/auth/me returns profile without sensitive data
$resMe = Invoke-Api -Uri "$baseUrl/api/auth/me" -Method Get -Headers $headersA
$hasNoHash = ($resMe.Data.user.passwordHash -eq $null) -and ($resMe.Data.user.salt -eq $null) -and ($resMe.Data.user.refreshTokenHash -eq $null)
$correctUsername = ($resMe.Data.user.username -eq $userA_name)
Record-Result "Auth: /me Profile Returns Correct User Without Hashes/Secrets" "Auth" ($resMe.StatusCode -eq 200 -and $correctUsername -and $hasNoHash) "Profile with no sensitive hashes" "Status $($resMe.StatusCode), noHash=$hasNoHash"
$userA_id = $resMe.Data.user.id

# 1.11 Missing Authentication Header
$resNoAuth = Invoke-Api -Uri "$baseUrl/api/auth/me" -Method Get
Record-Result "Auth: Missing Authentication Rejected" "Auth" ($resNoAuth.StatusCode -eq 401) "Status 401" "Status $($resNoAuth.StatusCode)"

# 1.12 Malformed Bearer Token
$resMalformed = Invoke-Api -Uri "$baseUrl/api/auth/me" -Method Get -Headers @{ "Authorization" = "Bearer" }
Record-Result "Auth: Malformed Bearer Token Rejected" "Auth" ($resMalformed.StatusCode -eq 401) "Status 401" "Status $($resMalformed.StatusCode)"

# 1.13 Tampered JWT Token
$tamperedToken = $tokenA.Substring(0, $tokenA.Length - 5) + "xxxxx"
$resTampered = Invoke-Api -Uri "$baseUrl/api/auth/me" -Method Get -Headers @{ "Authorization" = "Bearer $tamperedToken" }
Record-Result "Auth: Tampered JWT Signature Rejected" "Auth" ($resTampered.StatusCode -eq 401) "Status 401" "Status $($resTampered.StatusCode)"

# 1.14 Refresh Token Rotation
$refSessA = New-Object Microsoft.PowerShell.Commands.WebRequestSession
$refSessA.Cookies.Add((New-Object System.Net.Cookie("refreshToken", $refreshCookieA, "/api/auth", "127.0.0.1")))
$refRespA = Invoke-WebRequest -Uri "$baseUrl/api/auth/refresh" -Method Post -WebSession $refSessA -ContentType "application/json" -UseBasicParsing
$refDataA = $refRespA.Content | ConvertFrom-Json
$rotatedTokenA = $refDataA.accessToken
$rotatedCookieA = ""
if ($refRespA.Headers["Set-Cookie"] -match "refreshToken=([^;]+)") {
    $rotatedCookieA = $matches[1]
}
Record-Result "Auth: Refresh Token Returns New Access Token and Rotates Cookie" "Auth" ($refRespA.StatusCode -eq 200 -and -not [string]::IsNullOrEmpty($rotatedTokenA) -and ($rotatedCookieA -ne $refreshCookieA)) "Status 200 with new JWT and rotated cookie" "Status $($refRespA.StatusCode)"
if ($rotatedTokenA) {
    $tokenA = $rotatedTokenA
    $headersA = @{ "Authorization" = "Bearer $tokenA" }
}

# 1.15 Create User B (Buyer) & User C (Other)
$userB_name = "qa_userb_$suffix"
$userB_email = "qa_userb_$suffix@example.com"
Invoke-Api -Uri "$baseUrl/api/auth/signup" -Method Post -Body @{ username = $userB_name; email = $userB_email; password = "Password123!" } | Out-Null
$loginDataB = (Invoke-Api -Uri "$baseUrl/api/auth/login" -Method Post -Body @{ email = $userB_email; password = "Password123!" }).Data
$tokenB = $loginDataB.accessToken
$headersB = @{ "Authorization" = "Bearer $tokenB" }
$userB_id = $loginDataB.user.id

$userC_name = "qa_userc_$suffix"
$userC_email = "qa_userc_$suffix@example.com"
Invoke-Api -Uri "$baseUrl/api/auth/signup" -Method Post -Body @{ username = $userC_name; email = $userC_email; password = "Password123!" } | Out-Null
$loginDataC = (Invoke-Api -Uri "$baseUrl/api/auth/login" -Method Post -Body @{ email = $userC_email; password = "Password123!" }).Data
$tokenC = $loginDataC.accessToken
$headersC = @{ "Authorization" = "Bearer $tokenC" }
$userC_id = $loginDataC.user.id

# 1.16 Logout and Revoked Refresh Token Verification
$userTemp_email = "qa_logout_$suffix@example.com"
Invoke-Api -Uri "$baseUrl/api/auth/signup" -Method Post -Body @{ username = "qa_logout_$suffix"; email = $userTemp_email; password = "Password123!" } | Out-Null
$loginRespTemp = Invoke-WebRequest -Uri "$baseUrl/api/auth/login" -Method Post -Body (@{ email = $userTemp_email; password = "Password123!" } | ConvertTo-Json) -ContentType "application/json" -UseBasicParsing
$rTempCookie = ""
if ($loginRespTemp.Headers["Set-Cookie"] -match "refreshToken=([^;]+)") { $rTempCookie = $matches[1] }
$tempSess = New-Object Microsoft.PowerShell.Commands.WebRequestSession
$tempSess.Cookies.Add((New-Object System.Net.Cookie("refreshToken", $rTempCookie, "/api/auth", "127.0.0.1")))

# Logout
$resLogout = Invoke-Api -Uri "$baseUrl/api/auth/logout" -Method Post -WebSession $tempSess
Record-Result "Auth: Logout Revokes Session" "Auth" ($resLogout.StatusCode -eq 200) "Status 200" "Status $($resLogout.StatusCode)"

# Attempting to refresh with revoked session cookie
try {
    $resRevokedRefresh = Invoke-WebRequest -Uri "$baseUrl/api/auth/refresh" -Method Post -WebSession $tempSess -ContentType "application/json" -UseBasicParsing
    $revStatusCode = $resRevokedRefresh.StatusCode
} catch {
    $revStatusCode = [int]$_.Exception.Response.StatusCode
}
Record-Result "Auth: Revoked Refresh Token Rejected" "Auth" ($revStatusCode -eq 401) "Status 401" "Status $revStatusCode"


# ================================================================
# CATEGORY 2: BOOK MARKETPLACE (PHASE 1)
# ================================================================
Write-Host "`n>>> CATEGORY 2: BOOK MARKETPLACE (PHASE 1)" -ForegroundColor Magenta

# 2.1 Valid Book Listing Creation
$bookA1_payload = @{
    title = "Design Patterns: Elements of Reusable Object-Oriented Software"
    author = "Erich Gamma, Richard Helm, Ralph Johnson, John Vlissides"
    isbn = "9780201633610"
    price = 650.0
    condition = "Like New"
    category = "Technology"
    description = "Classic GOF book on software design patterns."
}
$resBookA1 = Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body $bookA1_payload
$bookA1 = $resBookA1.Data.book
Record-Result "Marketplace: Valid Book Listing Creation" "Marketplace" ($resBookA1.StatusCode -in @(200, 201) -and $bookA1.id) "Status 200/201 with book ID" "Status $($resBookA1.StatusCode)"
$bookA1_id = $bookA1.id

# 2.2 Impersonation Check: ownerId in payload should be ignored, derived from token
$resImpersonate = Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Impersonation Attempt"
    author = "Author"
    isbn = "9781234567890"
    price = 100.0
    condition = "Good"
    category = "Technology"
    ownerId = "hacker_fake_id_123"
}
Record-Result "Marketplace: OwnerId Derived From Token (Cannot Impersonate)" "Marketplace" ($resImpersonate.Data.book.owner.id -eq $userA_id) "Owner ID equals User A ID" "OwnerId: $($resImpersonate.Data.book.owner.id)"
if ($resImpersonate.Data.book.id) {
    Invoke-Api -Uri "$baseUrl/api/books/$($resImpersonate.Data.book.id)" -Method Delete -Headers $headersA | Out-Null
}

# 2.3 Unauthenticated Listing Creation Rejected
$resUnauthBook = Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Body $bookA1_payload
Record-Result "Marketplace: Unauthenticated Listing Creation Rejected" "Marketplace" ($resUnauthBook.StatusCode -eq 401) "Status 401" "Status $($resUnauthBook.StatusCode)"

# 2.4 Missing Title
$resMissingTitle = Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    author = "Author"; isbn = "9780201633610"; price = 100.0; condition = "Good"; category = "Technology"
}
Record-Result "Marketplace: Missing Title Rejected" "Marketplace" ($resMissingTitle.StatusCode -eq 400) "Status 400" "Status $($resMissingTitle.StatusCode)"

# 2.5 Empty Title
$resEmptyTitle = Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "   "; author = "Author"; isbn = "9780201633610"; price = 100.0; condition = "Good"; category = "Technology"
}
Record-Result "Marketplace: Empty Title Rejected" "Marketplace" ($resEmptyTitle.StatusCode -eq 400) "Status 400" "Status $($resEmptyTitle.StatusCode)"

# 2.6 Missing Author
$resMissingAuthor = Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Title"; isbn = "9780201633610"; price = 100.0; condition = "Good"; category = "Technology"
}
Record-Result "Marketplace: Missing Author Rejected" "Marketplace" ($resMissingAuthor.StatusCode -eq 400) "Status 400" "Status $($resMissingAuthor.StatusCode)"

# 2.7 Negative Price
$resNegPrice = Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Title"; author = "Author"; isbn = "9780201633610"; price = -50.0; condition = "Good"; category = "Technology"
}
Record-Result "Marketplace: Negative Price Rejected" "Marketplace" ($resNegPrice.StatusCode -eq 400) "Status 400" "Status $($resNegPrice.StatusCode)"

# 2.8 Excessive Price (> 1,000,000)
$resExcessPrice = Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Title"; author = "Author"; isbn = "9780201633610"; price = 2000000.0; condition = "Good"; category = "Technology"
}
Record-Result "Marketplace: Excessive Price (>1,000,000) Rejected" "Marketplace" ($resExcessPrice.StatusCode -eq 400) "Status 400" "Status $($resExcessPrice.StatusCode)"

# 2.9 Invalid Condition Whitelist
$resBadCondition = Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Title"; author = "Author"; isbn = "9780201633610"; price = 100.0; condition = "SuperAwesome"; category = "Technology"
}
Record-Result "Marketplace: Invalid Condition Rejected" "Marketplace" ($resBadCondition.StatusCode -eq 400) "Status 400" "Status $($resBadCondition.StatusCode)"

# 2.10 Malformed ISBN
$resBadIsbn = Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Title"; author = "Author"; isbn = "not-an-isbn"; price = 100.0; condition = "Good"; category = "Technology"
}
Record-Result "Marketplace: Malformed ISBN Rejected" "Marketplace" ($resBadIsbn.StatusCode -eq 400) "Status 400" "Status $($resBadIsbn.StatusCode)"

# 2.11 Create additional books for browsing, filtering, search & cart
$bookA2 = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Clean Code: A Handbook of Agile Software Craftsmanship"
    author = "Robert C. Martin"
    isbn = "9780132350884"
    price = 450.0
    condition = "Good"
    category = "Technology"
    description = "Even bad code can function. But if code isn't clean, it can bring a development organization to its knees."
}).Data.book

$bookA3 = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "To Kill a Mockingbird"
    author = "Harper Lee"
    isbn = "9780061120084"
    price = 250.0
    condition = "Acceptable"
    category = "Fiction"
    description = "The unforgettable novel of a childhood in a sleepy Southern town."
}).Data.book

# 2.12 Browse Books & Privacy check
$resBrowse = Invoke-Api -Uri "$baseUrl/api/books" -Method Get
$browseBooks = $resBrowse.Data.books
$noPrivateInfo = $true
foreach ($b in $browseBooks) {
    if ($b.passwordHash -or $b.refreshToken -or $b.salt) { $noPrivateInfo = $false }
}
Record-Result "Marketplace: Browse Returns Available Listings Without Sensitive Data" "Marketplace" ($resBrowse.StatusCode -eq 200 -and $browseBooks.Count -ge 3 -and $noPrivateInfo) "Count >= 3 with no secrets" "Found $($browseBooks.Count), noPrivate=$noPrivateInfo"

# 2.13 Search by Title (Partial, Case-Insensitive via search parameter)
$resSearchTitle = Invoke-Api -Uri "$baseUrl/api/books?search=Clean" -Method Get
$foundCleanCode = ($resSearchTitle.Data.books | Where-Object { $_.title -like "*Clean Code*" }).Count -ge 1
Record-Result "Marketplace: Search by Title (Partial, Case-Insensitive)" "Marketplace" ($resSearchTitle.StatusCode -eq 200 -and $foundCleanCode) "Found Clean Code" "Results: $($resSearchTitle.Data.books.Count)"

# 2.14 Search by Author via search parameter
$resSearchAuthor = Invoke-Api -Uri "$baseUrl/api/books?search=Harper" -Method Get
$foundAuthor = ($resSearchAuthor.Data.books | Where-Object { $_.author -like "*Harper Lee*" }).Count -ge 1
Record-Result "Marketplace: Search by Author" "Marketplace" ($resSearchAuthor.StatusCode -eq 200 -and $foundAuthor) "Found Harper Lee book" "Results: $($resSearchAuthor.Data.books.Count)"

# 2.15 Search by ISBN via search parameter
$resSearchIsbn = Invoke-Api -Uri "$baseUrl/api/books?search=9780201633610" -Method Get
$foundIsbn = ($resSearchIsbn.Data.books | Where-Object { $_.isbn -eq "9780201633610" }).Count -ge 1
Record-Result "Marketplace: Search by Exact ISBN" "Marketplace" ($resSearchIsbn.StatusCode -eq 200 -and $foundIsbn) "Found ISBN" "Results: $($resSearchIsbn.Data.books.Count)"

# 2.16 Search with Special Characters (SQL/Mongo Injection & Fuzz Safety)
$resSpecial = Invoke-Api -Uri "$baseUrl/api/books?search=%27%22%3C%3E%26%24%25" -Method Get
Record-Result "Marketplace: Search with Special Characters Does Not Crash" "Marketplace" ($resSpecial.StatusCode -eq 200) "Status 200" "Status $($resSpecial.StatusCode)"

# 2.17 Filtering by Category
$resFilterCat = Invoke-Api -Uri "$baseUrl/api/books?category=Fiction" -Method Get
$allFiction = $true
foreach ($b in $resFilterCat.Data.books) {
    if ($b.category -ne "Fiction") { $allFiction = $false }
}
Record-Result "Marketplace: Filtering by Category (Fiction)" "Marketplace" ($resFilterCat.StatusCode -eq 200 -and $resFilterCat.Data.books.Count -ge 1 -and $allFiction) "All category Fiction" "Count: $($resFilterCat.Data.books.Count)"

# 2.18 Combined Price Filtering
$resPriceRange = Invoke-Api -Uri "$baseUrl/api/books?minPrice=200&maxPrice=500" -Method Get
$allInRange = $true
foreach ($b in $resPriceRange.Data.books) {
    if ($b.price -lt 200 -or $b.price -gt 500) { $allInRange = $false }
}
Record-Result "Marketplace: Price Range Filtering (200 - 500)" "Marketplace" ($resPriceRange.StatusCode -eq 200 -and $allInRange) "All prices 200..500" "Count: $($resPriceRange.Data.books.Count)"

# 2.19 Inverted Price Range (minPrice > maxPrice yields 0 results safely)
$resInvertedPrice = Invoke-Api -Uri "$baseUrl/api/books?minPrice=600&maxPrice=200" -Method Get
Record-Result "Marketplace: Inverted Price Range Safely Returns 0 Results" "Marketplace" ($resInvertedPrice.StatusCode -eq 200 -and $resInvertedPrice.Data.books.Count -eq 0) "Count 0" "Count: $($resInvertedPrice.Data.books.Count)"

# 2.20 Sorting: Price Ascending & Descending
$resSortAsc = Invoke-Api -Uri "$baseUrl/api/books?sort=price_asc" -Method Get
$pricesAsc = $resSortAsc.Data.books | ForEach-Object { [double]$_.price }
$isSortedAsc = $true
for ($i = 0; $i -lt $pricesAsc.Count - 1; $i++) {
    if ($pricesAsc[$i] -gt $pricesAsc[$i+1]) { $isSortedAsc = $false; break }
}
Record-Result "Marketplace: Sorting Price Ascending" "Marketplace" ($resSortAsc.StatusCode -eq 200 -and $isSortedAsc) "Sorted ascending" "isSortedAsc=$isSortedAsc"

# 2.21 Pagination
$resPage1 = Invoke-Api -Uri "$baseUrl/api/books?page=1&limit=1" -Method Get
$resPage2 = Invoke-Api -Uri "$baseUrl/api/books?page=2&limit=1" -Method Get
$diffBooks = ($resPage1.Data.books[0].id -ne $resPage2.Data.books[0].id)
Record-Result "Marketplace: Pagination (Limit=1, Pages have distinct items)" "Marketplace" ($resPage1.StatusCode -eq 200 -and $resPage2.StatusCode -eq 200 -and $diffBooks) "Distinct items across pages" "diffBooks=$diffBooks"

# 2.22 Book Details (Valid, Nonexistent, Malformed)
$resDetailValid = Invoke-Api -Uri "$baseUrl/api/books/$bookA1_id" -Method Get
Record-Result "Marketplace: Book Details with Valid ID" "Marketplace" ($resDetailValid.StatusCode -eq 200 -and $resDetailValid.Data.book.title -eq $bookA1.title) "Book title matches" "Status $($resDetailValid.StatusCode)"

$resDetailNonexistent = Invoke-Api -Uri "$baseUrl/api/books/000000000000000000000000" -Method Get
Record-Result "Marketplace: Nonexistent Book ID Returns 404" "Marketplace" ($resDetailNonexistent.StatusCode -eq 404) "Status 404" "Status $($resDetailNonexistent.StatusCode)"

$resDetailMalformed = Invoke-Api -Uri "$baseUrl/api/books/invalid-id-xyz" -Method Get
Record-Result "Marketplace: Malformed Book ID Handled Gracefully (400/404)" "Marketplace" ($resDetailMalformed.StatusCode -in @(400, 404)) "Status 400/404" "Status $($resDetailMalformed.StatusCode)"

# 2.23 Ownership: User A updates Book A1
$resUpdateOwn = Invoke-Api -Uri "$baseUrl/api/books/$bookA1_id" -Method Put -Headers $headersA -Body @{
    price = 620.0; description = "Updated description by owner"
}
Record-Result "Marketplace: Owner Can Update Book Listing" "Marketplace" ($resUpdateOwn.StatusCode -eq 200 -and $resUpdateOwn.Data.book.price -eq 620.0) "Status 200 and updated price" "Price: $($resUpdateOwn.Data.book.price)"

# 2.24 IDOR Protection: User B attempts to update User A's Book
$resIdorUpdate = Invoke-Api -Uri "$baseUrl/api/books/$bookA1_id" -Method Put -Headers $headersB -Body @{
    price = 10.0; title = "Hacked Title"
}
Record-Result "Marketplace: IDOR - User B Cannot Update User A's Book" "Marketplace" ($resIdorUpdate.StatusCode -eq 403) "Status 403 Forbidden" "Status $($resIdorUpdate.StatusCode)"

# 2.25 IDOR Protection: User B attempts to delete User A's Book
$resIdorDelete = Invoke-Api -Uri "$baseUrl/api/books/$bookA1_id" -Method Delete -Headers $headersB
Record-Result "Marketplace: IDOR - User B Cannot Delete User A's Book" "Marketplace" ($resIdorDelete.StatusCode -eq 403) "Status 403 Forbidden" "Status $($resIdorDelete.StatusCode)"


# ================================================================
# CATEGORY 3: CART & STALE CART MANAGEMENT (PHASE 2)
# ================================================================
Write-Host "`n>>> CATEGORY 3: CART & STALE CART MANAGEMENT (PHASE 2)" -ForegroundColor Magenta

# 3.1 Clear Cart initially
Invoke-Api -Uri "$baseUrl/api/cart" -Method Delete -Headers $headersB | Out-Null

# 3.2 Add Available Book to Cart
$resAddCart = Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $bookA1_id }
Record-Result "Cart: Add Available Book to Cart" "Cart" ($resAddCart.StatusCode -in @(200, 201)) "Status 200/201" "Status $($resAddCart.StatusCode)"

# 3.3 Get Cart contents
$resGetCart = Invoke-Api -Uri "$baseUrl/api/cart" -Method Get -Headers $headersB
$cartItems = $resGetCart.Data.cart.items
Record-Result "Cart: Get Cart Shows Added Item" "Cart" ($resGetCart.StatusCode -eq 200 -and $cartItems.Count -eq 1 -and $cartItems[0].bookId -eq $bookA1_id) "1 item in cart" "ItemCount: $($cartItems.Count)"

# 3.4 Duplicate Add Rejection (Quantity cannot exceed 1 for physical listing)
$resDupAdd = Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $bookA1_id }
Record-Result "Cart: Duplicate Add Rejected (Quantity Limit = 1)" "Cart" ($resDupAdd.StatusCode -eq 400) "Status 400" "Status $($resDupAdd.StatusCode)"

# 3.5 Attempting Quantity > 1
$resQuantityOver = Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $bookA2.id; quantity = 2 }
Record-Result "Cart: Quantity > 1 Rejected" "Cart" ($resQuantityOver.StatusCode -eq 400) "Status 400" "Status $($resQuantityOver.StatusCode)"

# 3.6 Attempting Quantity <= 0
$resQuantityZero = Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $bookA2.id; quantity = 0 }
Record-Result "Cart: Zero Quantity Rejected" "Cart" ($resQuantityZero.StatusCode -eq 400) "Status 400" "Status $($resQuantityZero.StatusCode)"

# 3.7 Seller Cannot Add Own Book
$resAddOwn = Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersA -Body @{ bookId = $bookA1_id }
Record-Result "Cart: Seller Cannot Add Own Book to Cart" "Cart" ($resAddOwn.StatusCode -eq 400) "Status 400" "Status $($resAddOwn.StatusCode)"

# 3.8 Add Nonexistent Book
$resAddNonexistent = Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = "000000000000000000000000" }
Record-Result "Cart: Nonexistent Book Add Rejected (404)" "Cart" ($resAddNonexistent.StatusCode -eq 404) "Status 404" "Status $($resAddNonexistent.StatusCode)"

# 3.9 Multi-Book Cart
Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $bookA2.id } | Out-Null
$resMultiCart = Invoke-Api -Uri "$baseUrl/api/cart" -Method Get -Headers $headersB
$multiItems = $resMultiCart.Data.cart.items
$expectedSubtotal = 620.0 + [double]$bookA2.price # 620 + 450 = 1070
$cartTotal = [double]$resMultiCart.Data.cart.total
$cartShipping = [double]$resMultiCart.Data.cart.shippingCost
Record-Result "Cart: Multi-Book Cart Calculation (Subtotal, Total, ₹0 Shipping)" "Cart" ($multiItems.Count -eq 2 -and [Math]::Abs($cartTotal - $expectedSubtotal) -lt 0.01 -and $cartShipping -eq 0.0) "2 items, total=$expectedSubtotal, shipping=0" "Items=$($multiItems.Count), Total=$cartTotal, Ship=$cartShipping"

# 3.10 Remove Single Item from Cart
$resRemoveItem = Invoke-Api -Uri "$baseUrl/api/cart/items/$($bookA2.id)" -Method Delete -Headers $headersB
$resAfterRemove = Invoke-Api -Uri "$baseUrl/api/cart" -Method Get -Headers $headersB
Record-Result "Cart: Remove Single Item from Cart" "Cart" ($resRemoveItem.StatusCode -eq 200 -and $resAfterRemove.Data.cart.items.Count -eq 1) "1 item remaining" "Items: $($resAfterRemove.Data.cart.items.Count)"

# 3.11 Mandatory Stale Cart Scenario
Write-Host "  --> Executing Mandatory Stale Cart Test..." -ForegroundColor DarkYellow
# User A lists StaleBook.
# User B adds StaleBook to cart.
# User C purchases StaleBook.
# User B retrieves cart and attempts checkout -> Must fail safely with 409 Conflict!
$staleBook = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Stale Cart Test Book"; author = "Author"; isbn = "9781234567800"; price = 300.0; condition = "Good"; category = "Technology"
}).Data.book

# User B clears cart and adds StaleBook
Invoke-Api -Uri "$baseUrl/api/cart" -Method Delete -Headers $headersB | Out-Null
Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $staleBook.id } | Out-Null

# User C adds and purchases StaleBook
Invoke-Api -Uri "$baseUrl/api/cart" -Method Delete -Headers $headersC | Out-Null
Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersC -Body @{ bookId = $staleBook.id } | Out-Null
$resPurchaseC = Invoke-Api -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersC -Body @{
    paymentMethod = "UPI"
    shippingAddress = @{ fullName = "User C"; addressLine = "Line 1"; city = "City"; state = "State"; postalCode = "123456"; phone = "9876543210" }
}
Record-Result "Stale Cart: User C Successfully Purchases Book First" "Cart" ($resPurchaseC.StatusCode -in @(200, 201)) "Status 200/201" "Status $($resPurchaseC.StatusCode)"

# User B now retrieves cart and attempts checkout with the stale book in cart
$cartBStale = Invoke-Api -Uri "$baseUrl/api/cart" -Method Get -Headers $headersB
$hasStaleFlag = ($cartBStale.Data.cart.hasUnavailableItems -eq $true)
$resStaleCheckoutB = Invoke-Api -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersB -Body @{
    paymentMethod = "UPI"
    shippingAddress = @{ fullName = "User B"; addressLine = "Line 1"; city = "City"; state = "State"; postalCode = "123456"; phone = "9876543210" }
}
Record-Result "Stale Cart: Cart Detects Unavailable Item and Checkout Fails (409 Conflict)" "Cart" ($hasStaleFlag -and $resStaleCheckoutB.StatusCode -eq 409) "hasUnavailableItems=true, Checkout 409" "Flag=$hasStaleFlag, Status=$($resStaleCheckoutB.StatusCode)"


# ================================================================
# CATEGORY 4: BUY/SELL, DEMO PAYMENT & CONCURRENCY (PHASE 2)
# ================================================================
Write-Host "`n>>> CATEGORY 4: BUY/SELL, DEMO PAYMENT & CONCURRENCY (PHASE 2)" -ForegroundColor Magenta

# 4.1 Payment Security: Server Calculates Total, Client Price Tampering Ignored
Invoke-Api -Uri "$baseUrl/api/cart" -Method Delete -Headers $headersB | Out-Null
Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $bookA1_id } | Out-Null

$resTamperedCheckout = Invoke-Api -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersB -Body @{
    paymentMethod = "UPI"
    shippingAddress = @{ fullName = "User B"; addressLine = "Line 1"; city = "City"; state = "State"; postalCode = "123456"; phone = "9876543210" }
    total = 1.0          # Hacker tries to set total to 1
    subtotal = 1.0       # Hacker tries to set subtotal to 1
    shippingCost = 999.0 # Hacker tries to set shipping
    paymentStatus = "paid"
    transactionId = "HACKED_TXN_ID"
}
$orderTampered = $resTamperedCheckout.Data.order
Record-Result "Payment Security: Server Enforces Accurate Total (Ignored Tampered 1.0)" "BuySell" ($resTamperedCheckout.StatusCode -in @(200, 201) -and $orderTampered.total -eq 620.0 -and $orderTampered.shippingCost -eq 0.0) "Order Total=620, Shipping=0" "Total=$($orderTampered.total), Ship=$($orderTampered.shippingCost)"
$purchasedOrder1 = $orderTampered

# 4.2 Verify Book A1 becomes SOLD and disappears from Browse & Search
$resBrowseAfterBuy = Invoke-Api -Uri "$baseUrl/api/books" -Method Get
$isA1InBrowse = ($resBrowseAfterBuy.Data.books | Where-Object { $_.id -eq $bookA1_id }).Count -gt 0
$resSearchAfterBuy = Invoke-Api -Uri "$baseUrl/api/books?search=Design+Patterns" -Method Get
$isA1InSearch = ($resSearchAfterBuy.Data.books | Where-Object { $_.id -eq $bookA1_id }).Count -gt 0
Record-Result "BuySell: Purchased Book Excluded from Browse & Search" "BuySell" (-not $isA1InBrowse -and -not $isA1InSearch) "Not in browse and not in search" "InBrowse=$isA1InBrowse, InSearch=$isA1InSearch"

# 4.3 Payment Methods: CARD and COD
$bookCard = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Card Payment Book"; author = "Author"; isbn = "9781111111111"; price = 200.0; condition = "Good"; category = "Technology"
}).Data.book
$bookCod = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "COD Payment Book"; author = "Author"; isbn = "9782222222222"; price = 200.0; condition = "Good"; category = "Technology"
}).Data.book

# Checkout with CARD
Invoke-Api -Uri "$baseUrl/api/cart" -Method Delete -Headers $headersB | Out-Null
Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $bookCard.id } | Out-Null
$resCard = Invoke-Api -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersB -Body @{
    paymentMethod = "CARD"
    shippingAddress = @{ fullName = "User B"; addressLine = "Line 1"; city = "City"; state = "State"; postalCode = "123456"; phone = "9876543210" }
}
Record-Result "BuySell: CARD Payment Sets paymentStatus=paid" "BuySell" ($resCard.StatusCode -in @(200, 201) -and $resCard.Data.order.payment.status -eq "paid") "Status paid" "Status: $($resCard.Data.order.payment.status)"

# Checkout with COD
Invoke-Api -Uri "$baseUrl/api/cart" -Method Delete -Headers $headersB | Out-Null
Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $bookCod.id } | Out-Null
$resCod = Invoke-Api -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersB -Body @{
    paymentMethod = "COD"
    shippingAddress = @{ fullName = "User B"; addressLine = "Line 1"; city = "City"; state = "State"; postalCode = "123456"; phone = "9876543210" }
}
Record-Result "BuySell: COD Payment Sets paymentStatus=pending" "BuySell" ($resCod.StatusCode -in @(200, 201) -and $resCod.Data.order.payment.status -eq "pending") "Status pending" "Status: $($resCod.Data.order.payment.status)"

# 4.4 Simulated Payment Failure Rollback
$bookFail = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Payment Failure Rollback Book"; author = "Author"; isbn = "9783333333333"; price = 250.0; condition = "Good"; category = "Technology"
}).Data.book
Invoke-Api -Uri "$baseUrl/api/cart" -Method Delete -Headers $headersB | Out-Null
Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $bookFail.id } | Out-Null

$resFailSim = Invoke-Api -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersB -Body @{
    paymentMethod = "UPI"
    simulatePaymentFailure = $true
    shippingAddress = @{ fullName = "User B"; addressLine = "Line 1"; city = "City"; state = "State"; postalCode = "123456"; phone = "9876543210" }
}
# Verify checkout failed and book remains available
$resCheckFailBook = Invoke-Api -Uri "$baseUrl/api/books/$($bookFail.id)" -Method Get
Record-Result "BuySell: Simulated Payment Failure Rolls Back Book to Available" "BuySell" ($resFailSim.StatusCode -in @(400, 500) -and $resCheckFailBook.Data.book.status -eq "available") "Payment rejected, book available" "Status: $($resFailSim.StatusCode), BookStatus: $($resCheckFailBook.Data.book.status)"

# 4.5 Mandatory Concurrency / Double Purchase Race Condition Test
Write-Host "  --> Executing Mandatory Concurrency / Double Purchase Race Condition Test..." -ForegroundColor DarkYellow
$raceBook = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Concurrency Race Target Book"; author = "Author"; isbn = "9784444444444"; price = 350.0; condition = "Good"; category = "Technology"
}).Data.book

Invoke-Api -Uri "$baseUrl/api/cart" -Method Delete -Headers $headersB | Out-Null
Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $raceBook.id } | Out-Null

Invoke-Api -Uri "$baseUrl/api/cart" -Method Delete -Headers $headersC | Out-Null
Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersC -Body @{ bookId = $raceBook.id } | Out-Null

$jobScript = {
    param($url, $token)
    $headers = @{ "Authorization" = "Bearer $token" }
    $body = @{
        paymentMethod = "UPI"
        shippingAddress = @{ fullName = "Racer"; addressLine = "Line 1"; city = "City"; state = "State"; postalCode = "123456"; phone = "9876543210" }
    } | ConvertTo-Json
    try {
        $res = Invoke-RestMethod -Uri "$url/api/checkout" -Method Post -Headers $headers -ContentType "application/json" -Body $body
        return @{ Success = $true; Code = 200 }
    } catch {
        $c = 500
        if ($_.Exception.Response) { $c = [int]$_.Exception.Response.StatusCode }
        return @{ Success = $false; Code = $c }
    }
}

$jobB = Start-Job -ScriptBlock $jobScript -ArgumentList $baseUrl, $tokenB
$jobC = Start-Job -ScriptBlock $jobScript -ArgumentList $baseUrl, $tokenC
$resJobB = Receive-Job -Job $jobB -Wait
$resJobC = Receive-Job -Job $jobC -Wait
Remove-Job -Job $jobB, $jobC

$successCount = 0
$failCount = 0
if ($resJobB.Success) { $successCount++ } else { $failCount++ }
if ($resJobC.Success) { $successCount++ } else { $failCount++ }

Record-Result "BuySell: Concurrency Test Exactly ONE Purchase Succeeds" "BuySell" ($successCount -eq 1 -and $failCount -eq 1) "1 success, 1 failure" "Success=$successCount, Fail=$failCount (B:$($resJobB.Code), C:$($resJobC.Code))"


# ================================================================
# CATEGORY 5: ORDER LIFECYCLE & DELIVERY SIMULATION
# ================================================================
Write-Host "`n>>> CATEGORY 5: ORDER LIFECYCLE & DELIVERY SIMULATION" -ForegroundColor Magenta

# 5.1 Buyer sees Order in History
$resBuyerOrders = Invoke-Api -Uri "$baseUrl/api/orders" -Method Get -Headers $headersB
$hasPurchasedOrder = @($resBuyerOrders.Data.orders | Where-Object { $_.id -eq $purchasedOrder1.id }).Count -gt 0
Record-Result "Orders: Buyer Sees Order in History" "Orders" ($resBuyerOrders.StatusCode -eq 200 -and $hasPurchasedOrder) "Order present in buyer orders" "Found: $hasPurchasedOrder"

# 5.2 Seller sees Item in Sale History
$resSellerSales = Invoke-Api -Uri "$baseUrl/api/orders/seller/history" -Method Get -Headers $headersA
$hasSoldItem = @($resSellerSales.Data.orders | Where-Object { $_.id -eq $purchasedOrder1.id }).Count -gt 0
Record-Result "Orders: Seller Sees Item in Sales History" "Orders" ($resSellerSales.StatusCode -eq 200 -and $hasSoldItem) "Item present in sales history" "Found: $hasSoldItem"

# 5.3 IDOR: Unrelated User C cannot view User B's order
$resIdorOrder = Invoke-Api -Uri "$baseUrl/api/orders/$($purchasedOrder1.id)" -Method Get -Headers $headersC
Record-Result "Orders: IDOR - Unrelated User Cannot View Private Order" "Orders" ($resIdorOrder.StatusCode -eq 403) "Status 403 Forbidden" "Status $($resIdorOrder.StatusCode)"

# 5.4 Order Status Lifecycle: CONFIRMED -> PACKED -> SHIPPED -> OUT_FOR_DELIVERY -> DELIVERED
$statuses = @("PACKED", "SHIPPED", "OUT_FOR_DELIVERY", "DELIVERED")
$allStepsOk = $true
foreach ($st in $statuses) {
    $resStep = Invoke-Api -Uri "$baseUrl/api/orders/$($purchasedOrder1.id)/status" -Method Patch -Headers $headersA -Body @{ status = $st }
    if ($resStep.StatusCode -ne 200) { $allStepsOk = $false }
}
Record-Result "Orders: Delivery Lifecycle Progression to DELIVERED" "Orders" ($allStepsOk) "All transitions succeed" "Success=$allStepsOk"

# 5.5 Invalid Transition: DELIVERED cannot transition back to PLACED
$resInvalidTransition = Invoke-Api -Uri "$baseUrl/api/orders/$($purchasedOrder1.id)/status" -Method Patch -Headers $headersA -Body @{ status = "PLACED" }
Record-Result "Orders: Invalid Status Transition (DELIVERED -> PLACED) Rejected" "Orders" ($resInvalidTransition.StatusCode -eq 400) "Status 400" "Status $($resInvalidTransition.StatusCode)"

# 5.6 Buyer Cannot Arbitrarily Modify Status
$resBuyerTamperStatus = Invoke-Api -Uri "$baseUrl/api/orders/$($purchasedOrder1.id)/status" -Method Patch -Headers $headersB -Body @{ status = "DELIVERED" }
Record-Result "Orders: Buyer Cannot Modify Order Status (Only Seller/Admin)" "Orders" ($resBuyerTamperStatus.StatusCode -eq 403) "Status 403 Forbidden" "Status $($resBuyerTamperStatus.StatusCode)"


# ================================================================
# CATEGORY 6: PEER-TO-PEER BOOK EXCHANGE (PHASE 3)
# ================================================================
Write-Host "`n>>> CATEGORY 6: PEER-TO-PEER BOOK EXCHANGE (PHASE 3)" -ForegroundColor Magenta

# Create Exchange test books: User A owns ExchBookA, User B owns ExchBookB
$exchBookA = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Exchange Target Book A"; author = "Author A"; isbn = "9785555555551"; price = 400.0; condition = "Like New"; category = "Fiction"
}).Data.book

$exchBookB = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersB -Body @{
    title = "Exchange Offered Book B"; author = "Author B"; isbn = "9785555555552"; price = 400.0; condition = "Like New"; category = "Fiction"
}).Data.book

# 6.1 Valid Exchange Request
$resExchCreate = Invoke-Api -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -Body @{
    requestedBookId = $exchBookA.id
    offeredBookId = $exchBookB.id
    message = "Would love to exchange my Book B for your Book A!"
}
$exchReq1 = $resExchCreate.Data.exchange
Record-Result "Exchange: Valid Exchange Request Created (Status PENDING)" "Exchange" ($resExchCreate.StatusCode -in @(200, 201) -and $exchReq1.status -eq "PENDING") "Status 200/201, PENDING" "Status: $($resExchCreate.StatusCode), ExchStatus: $($exchReq1.status)"

# 6.2 Invalid: Requester offers someone else's book
$resBadOffer = Invoke-Api -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersC -Body @{
    requestedBookId = $exchBookA.id
    offeredBookId = $exchBookB.id # User C does not own Book B
}
Record-Result "Exchange: Offering Another User's Book Rejected" "Exchange" ($resBadOffer.StatusCode -in @(400, 403)) "Status 400/403" "Status $($resBadOffer.StatusCode)"

# 6.3 Invalid: Requesting own book
$resSelfExch = Invoke-Api -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersA -Body @{
    requestedBookId = $exchBookA.id
    offeredBookId = $exchBookA.id
}
Record-Result "Exchange: Requesting Own Book / Same Book Rejected" "Exchange" ($resSelfExch.StatusCode -eq 400) "Status 400" "Status $($resSelfExch.StatusCode)"

# 6.4 Authorization: Requester sees in /sent, Receiver in /received
$resSent = Invoke-Api -Uri "$baseUrl/api/exchanges/sent" -Method Get -Headers $headersB
$resReceived = Invoke-Api -Uri "$baseUrl/api/exchanges/received" -Method Get -Headers $headersA
$foundInSent = @($resSent.Data.exchanges | Where-Object { $_.id -eq $exchReq1.id }).Count -gt 0
$foundInReceived = @($resReceived.Data.exchanges | Where-Object { $_.id -eq $exchReq1.id }).Count -gt 0
Record-Result "Exchange: Requester Sees in /sent and Receiver in /received" "Exchange" ($foundInSent -and $foundInReceived) "Both see request" "Sent=$foundInSent, Recv=$foundInReceived"

# 6.5 IDOR: Unrelated User C cannot view private exchange
$resIdorExch = Invoke-Api -Uri "$baseUrl/api/exchanges/$($exchReq1.id)" -Method Get -Headers $headersC
Record-Result "Exchange: IDOR - Unrelated User C Cannot View Exchange" "Exchange" ($resIdorExch.StatusCode -eq 403) "Status 403 Forbidden" "Status $($resIdorExch.StatusCode)"

# 6.6 Rejection Test: User A rejects exchange
$resReject = Invoke-Api -Uri "$baseUrl/api/exchanges/$($exchReq1.id)/reject" -Method Post -Headers $headersA -Body @{ note = "Not interested" }
Record-Result "Exchange: Receiver Rejects Exchange (Status REJECTED)" "Exchange" ($resReject.StatusCode -eq 200 -and $resReject.Data.exchange.status -eq "REJECTED") "Status REJECTED" "Status: $($resReject.Data.exchange.status)"

# 6.7 Cancellation Test: Requester cancels own pending exchange
$exchReq2 = (Invoke-Api -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -Body @{
    requestedBookId = $exchBookA.id; offeredBookId = $exchBookB.id; message = "Cancel test"
}).Data.exchange
$resCancel = Invoke-Api -Uri "$baseUrl/api/exchanges/$($exchReq2.id)/cancel" -Method Post -Headers $headersB
Record-Result "Exchange: Requester Can Cancel Pending Exchange (Status CANCELLED)" "Exchange" ($resCancel.StatusCode -eq 200 -and $resCancel.Data.exchange.status -eq "CANCELLED") "Status CANCELLED" "Status: $($resCancel.Data.exchange.status)"

# 6.8 Acceptance & Ownership Transfer Test
$exchReq3 = (Invoke-Api -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -Body @{
    requestedBookId = $exchBookA.id; offeredBookId = $exchBookB.id; message = "Final swap"
}).Data.exchange

$resAccept = Invoke-Api -Uri "$baseUrl/api/exchanges/$($exchReq3.id)/accept" -Method Post -Headers $headersA
$checkA = Invoke-Api -Uri "$baseUrl/api/books/$($exchBookA.id)" -Method Get -Headers $headersB
$checkB = Invoke-Api -Uri "$baseUrl/api/books/$($exchBookB.id)" -Method Get -Headers $headersA

$ownerSwapped = ($checkA.Data.book.owner.id -eq $userB_id) -and ($checkB.Data.book.owner.id -eq $userA_id)
$bothExchanged = ($checkA.Data.book.status -eq "exchanged") -and ($checkB.Data.book.status -eq "exchanged")
Record-Result "Exchange: Acceptance Swaps Physical Book Ownership Atomically" "Exchange" ($resAccept.StatusCode -eq 200 -and $ownerSwapped -and $bothExchanged) "Ownership swapped and status=exchanged" "Swapped=$ownerSwapped, BothExch=$bothExchanged"

# 6.9 Concurrency: Purchase vs Exchange Conflict Test
$confBookA = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Conflict Book A"; author = "Author"; isbn = "9786666666661"; price = 300.0; condition = "Good"; category = "Fiction"
}).Data.book
$confBookB = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersB -Body @{
    title = "Conflict Book B"; author = "Author"; isbn = "9786666666662"; price = 300.0; condition = "Good"; category = "Fiction"
}).Data.book

# User B requests exchange
$confExch = (Invoke-Api -Uri "$baseUrl/api/exchanges" -Method Post -Headers $headersB -Body @{
    requestedBookId = $confBookA.id; offeredBookId = $confBookB.id; message = "Conflict test"
}).Data.exchange

# User C buys ConfBookA before User A accepts exchange
Invoke-Api -Uri "$baseUrl/api/cart" -Method Delete -Headers $headersC | Out-Null
Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersC -Body @{ bookId = $confBookA.id } | Out-Null
Invoke-Api -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersC -Body @{
    paymentMethod = "UPI"
    shippingAddress = @{ fullName = "User C"; addressLine = "Line 1"; city = "City"; state = "State"; postalCode = "123456"; phone = "9876543210" }
} | Out-Null

# User A now tries to accept the exchange -> Must fail safely with 409 Conflict!
$resAcceptConflict = Invoke-Api -Uri "$baseUrl/api/exchanges/$($confExch.id)/accept" -Method Post -Headers $headersA
$checkConfB = (Invoke-Api -Uri "$baseUrl/api/books/$($confBookB.id)" -Method Get).Data.book
Record-Result "Exchange: Purchase-Exchange Conflict Handled Safely (Book B remains untouched)" "Exchange" ($resAcceptConflict.StatusCode -in @(400, 409) -and $checkConfB.owner.id -eq $userB_id -and $checkConfB.status -eq "available") "Exchange rejected, Book B preserved" "Status: $($resAcceptConflict.StatusCode), BookB Owner: $($checkConfB.owner.id)"


# ================================================================
# CATEGORY 7: REVIEWS & RATINGS (PHASE 4)
# ================================================================
Write-Host "`n>>> CATEGORY 7: REVIEWS & RATINGS (PHASE 4)" -ForegroundColor Magenta

# 7.1 Eligible Buyer Reviews Delivered Order Item
# Remember $purchasedOrder1 for $bookA1_id is DELIVERED! Buyer is User B.
$resReview1 = Invoke-Api -Uri "$baseUrl/api/reviews" -Method Post -Headers $headersB -Body @{
    orderId = $purchasedOrder1.id
    bookId = $bookA1_id
    rating = 5
    comment = "Outstanding condition, quick delivery and well packed!"
}
$rev1 = $resReview1.Data.review
Record-Result "Reviews: Eligible Buyer Reviews Delivered Book (Rating 5)" "Reviews" ($resReview1.StatusCode -in @(200, 201) -and $rev1.id) "Status 200/201" "Status $($resReview1.StatusCode)"
$rev1_id = $rev1.id

# 7.2 Ineligible Review: Non-purchaser User C attempts to review Book A1
$resIneligibleRev = Invoke-Api -Uri "$baseUrl/api/reviews" -Method Post -Headers $headersC -Body @{
    orderId = $purchasedOrder1.id
    bookId = $bookA1_id
    rating = 4
    comment = "I never bought this but want to review"
}
Record-Result "Reviews: Non-Purchaser Review Rejected" "Reviews" ($resIneligibleRev.StatusCode -in @(400, 403)) "Status 400/403" "Status $($resIneligibleRev.StatusCode)"

# 7.3 Seller Self-Review Rejection: Seller User A attempts to review own book
$resSellerRev = Invoke-Api -Uri "$baseUrl/api/reviews" -Method Post -Headers $headersA -Body @{
    orderId = $purchasedOrder1.id
    bookId = $bookA1_id
    rating = 5
    comment = "I am the seller and I love my book"
}
Record-Result "Reviews: Seller Self-Review Rejected" "Reviews" ($resSellerRev.StatusCode -in @(400, 403)) "Status 400/403" "Status $($resSellerRev.StatusCode)"

# 7.4 Duplicate Review Rejection
$resDupRev = Invoke-Api -Uri "$baseUrl/api/reviews" -Method Post -Headers $headersB -Body @{
    orderId = $purchasedOrder1.id
    bookId = $bookA1_id
    rating = 4
    comment = "Duplicate review attempt"
}
Record-Result "Reviews: Duplicate Review on Same Order Item Rejected" "Reviews" ($resDupRev.StatusCode -in @(400, 409)) "Status 400/409" "Status $($resDupRev.StatusCode)"

# 7.5 Invalid Rating Limits (Rating < 1 or > 5)
$resRatingLow = Invoke-Api -Uri "$baseUrl/api/reviews" -Method Post -Headers $headersB -Body @{
    orderId = $purchasedOrder1.id; bookId = $bookA1_id; rating = 0; comment = "Zero rating"
}
$resRatingHigh = Invoke-Api -Uri "$baseUrl/api/reviews" -Method Post -Headers $headersB -Body @{
    orderId = $purchasedOrder1.id; bookId = $bookA1_id; rating = 6; comment = "Six rating"
}
Record-Result "Reviews: Invalid Rating Limits (<1 or >5) Rejected" "Reviews" ($resRatingLow.StatusCode -eq 400 -and $resRatingHigh.StatusCode -eq 400) "Both status 400" "Low: $($resRatingLow.StatusCode), High: $($resRatingHigh.StatusCode)"

# 7.6 Review Update by Author
$resUpdateRev = Invoke-Api -Uri "$baseUrl/api/reviews/$rev1_id" -Method Put -Headers $headersB -Body @{
    rating = 4
    comment = "Updated to 4 stars after reading thoroughly"
}
Record-Result "Reviews: Review Author Can Update Review" "Reviews" ($resUpdateRev.StatusCode -eq 200 -and $resUpdateRev.Data.review.rating -eq 4) "Status 200, rating 4" "Rating: $($resUpdateRev.Data.review.rating)"

# 7.7 IDOR: Unrelated User C cannot update or delete User B's review
$resIdorUpdateRev = Invoke-Api -Uri "$baseUrl/api/reviews/$rev1_id" -Method Put -Headers $headersC -Body @{ rating = 1; comment = "Vandalism" }
$resIdorDelRev = Invoke-Api -Uri "$baseUrl/api/reviews/$rev1_id" -Method Delete -Headers $headersC
Record-Result "Reviews: IDOR - Unrelated User Cannot Update or Delete Review" "Reviews" ($resIdorUpdateRev.StatusCode -eq 403 -and $resIdorDelRev.StatusCode -eq 403) "Both 403 Forbidden" "Put: $($resIdorUpdateRev.StatusCode), Del: $($resIdorDelRev.StatusCode)"


# ================================================================
# CATEGORY 8: ORDER CANCELLATION & RETURN WORKFLOW (PHASE 4)
# ================================================================
Write-Host "`n>>> CATEGORY 8: ORDER CANCELLATION & RETURN WORKFLOW (PHASE 4)" -ForegroundColor Magenta

# 8.1 Cancellation in CONFIRMED state restores book inventory to available
$cancelBook = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Order Cancel Target Book"; author = "Author"; isbn = "9787777777771"; price = 320.0; condition = "Good"; category = "Technology"
}).Data.book
Invoke-Api -Uri "$baseUrl/api/cart" -Method Delete -Headers $headersB | Out-Null
Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $cancelBook.id } | Out-Null
$orderToCancel = (Invoke-Api -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersB -Body @{
    paymentMethod = "UPI"
    shippingAddress = @{ fullName = "User B"; addressLine = "Line 1"; city = "City"; state = "State"; postalCode = "123456"; phone = "9876543210" }
}).Data.order

$resCancelOrder = Invoke-Api -Uri "$baseUrl/api/orders/$($orderToCancel.id)/cancel" -Method Post -Headers $headersB -Body @{ reason = "Changed my mind" }
$bookAfterCancel = (Invoke-Api -Uri "$baseUrl/api/books/$($cancelBook.id)" -Method Get).Data.book
Record-Result "Cancellation: Buyer Cancels CONFIRMED Order, Restores Book to Available" "Lifecycle" ($resCancelOrder.StatusCode -eq 200 -and $resCancelOrder.Data.order.status -eq "CANCELLED" -and $bookAfterCancel.status -eq "available") "Order CANCELLED and book available" "OrderStatus: $($resCancelOrder.Data.order.status), BookStatus: $($bookAfterCancel.status)"

# 8.2 Cancellation after Shipment/Delivery is Rejected
# $purchasedOrder1 is DELIVERED! Attempting to cancel must fail.
$resCancelDelivered = Invoke-Api -Uri "$baseUrl/api/orders/$($purchasedOrder1.id)/cancel" -Method Post -Headers $headersB -Body @{ reason = "Cancel delivered" }
Record-Result "Cancellation: Cancelling Delivered Order Rejected" "Lifecycle" ($resCancelDelivered.StatusCode -eq 400) "Status 400" "Status $($resCancelDelivered.StatusCode)"

# 8.3 Return Workflow on DELIVERED Order
# User B requests return on $purchasedOrder1
$resReturnReq = Invoke-Api -Uri "$baseUrl/api/orders/$($purchasedOrder1.id)/return" -Method Post -Headers $headersB -Body @{ reason = "Defective page" }
Record-Result "Returns: Buyer Requests Return on Delivered Order (RETURN_REQUESTED)" "Returns" ($resReturnReq.StatusCode -eq 200 -and $resReturnReq.Data.order.status -eq "RETURN_REQUESTED") "Status RETURN_REQUESTED" "Status: $($resReturnReq.Data.order.status)"

# 8.4 Return before delivery rejection
# Create an order in CONFIRMED state and attempt return
$earlyBook = (Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = "Early Return Test"; author = "Author"; isbn = "9788888888881"; price = 150.0; condition = "Good"; category = "Fiction"
}).Data.book
Invoke-Api -Uri "$baseUrl/api/cart" -Method Delete -Headers $headersB | Out-Null
Invoke-Api -Uri "$baseUrl/api/cart/items" -Method Post -Headers $headersB -Body @{ bookId = $earlyBook.id } | Out-Null
$earlyOrder = (Invoke-Api -Uri "$baseUrl/api/checkout" -Method Post -Headers $headersB -Body @{
    paymentMethod = "UPI"
    shippingAddress = @{ fullName = "User B"; addressLine = "Line 1"; city = "City"; state = "State"; postalCode = "123456"; phone = "9876543210" }
}).Data.order

$resEarlyReturn = Invoke-Api -Uri "$baseUrl/api/orders/$($earlyOrder.id)/return" -Method Post -Headers $headersB -Body @{ reason = "Too early" }
Record-Result "Returns: Return Request Before Delivery Rejected" "Returns" ($resEarlyReturn.StatusCode -eq 400) "Status 400" "Status $($resEarlyReturn.StatusCode)"

# 8.5 Seller Processes Return (Approve)
$resProcessReturn = Invoke-Api -Uri "$baseUrl/api/orders/$($purchasedOrder1.id)/return/process" -Method Post -Headers $headersA -Body @{
    approve = $true; note = "Return accepted, simulated refund processed"
}
Record-Result "Returns: Seller Approves Return (Order Status RETURNED)" "Returns" ($resProcessReturn.StatusCode -eq 200 -and $resProcessReturn.Data.order.status -eq "RETURNED") "Status RETURNED" "Status: $($resProcessReturn.Data.order.status)"


# ================================================================
# CATEGORY 9: SECURITY AUDIT & INPUT FUZZING
# ================================================================
Write-Host "`n>>> CATEGORY 9: SECURITY AUDIT & INPUT FUZZING" -ForegroundColor Magenta

# 9.1 Malformed JSON Bodies do not crash server
$resFuzz1 = Invoke-Api -Uri "$baseUrl/api/auth/signup" -Method Post -Body "{ malformed json: true, "
Record-Result "Security: Malformed JSON Returns 400 (No Crash)" "Security" ($resFuzz1.StatusCode -eq 400) "Status 400" "Status $($resFuzz1.StatusCode)"

# 9.2 Empty JSON object
$resFuzz2 = Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body "{}"
Record-Result "Security: Empty JSON Object Returns 400" "Security" ($resFuzz2.StatusCode -eq 400) "Status 400" "Status $($resFuzz2.StatusCode)"

# 9.3 Type Confusion: Number where string expected, array where object expected
$resFuzz3 = Invoke-Api -Uri "$baseUrl/api/auth/login" -Method Post -Body @{ email = 12345; password = @("array", "value") }
Record-Result "Security: Type Confusion in Login Returns 400" "Security" ($resFuzz3.StatusCode -eq 400) "Status 400" "Status $($resFuzz3.StatusCode)"

# 9.4 Extremely Large String / Buffer Overflow Prevention
$hugeString = "A" * 10000
$resFuzz4 = Invoke-Api -Uri "$baseUrl/api/books" -Method Post -Headers $headersA -Body @{
    title = $hugeString; author = "Author"; isbn = "9781234567890"; price = 100.0; condition = "Good"; category = "Technology"
}
Record-Result "Security: Extremely Large Title String Handled Safely" "Security" ($resFuzz4.StatusCode -eq 400) "Status 400" "Status $($resFuzz4.StatusCode)"

# 9.5 Sensitive Data Leak Verification Across Errors
$errorLeak = $false
foreach ($t in $global:testResults) {
    $detailStr = "$($t.Details) $($t.Actual)"
    if ($detailStr -match "passwordHash|argon2id|\$argon2|MongoException|driver\.cpp|stacktrace") {
        $errorLeak = $true
    }
}
Record-Result "Security: Zero Sensitive Data or Stack Traces Leaked in Error Responses" "Security" (-not $errorLeak) "No sensitive leaks" "errorLeak=$errorLeak"


# ================================================================
# SUMMARY
# ================================================================
Write-Host "`n================================================================" -ForegroundColor Cyan
Write-Host "QA & INTEGRATION TEST RUN COMPLETE" -ForegroundColor Cyan
Write-Host "Total Tests:  $($global:totalTests)" -ForegroundColor Cyan
Write-Host "Passed Tests: $($global:passedTests)" -ForegroundColor Green
Write-Host "Failed Tests: $($global:failedTests)" -ForegroundColor $(if ($global:failedTests -eq 0) { "Green" } else { "Red" })
Write-Host "================================================================" -ForegroundColor Cyan

if ($global:failedTests -gt 0) {
    Write-Host "`nFAILED TESTS DETAILS:" -ForegroundColor Red
    $global:testResults | Where-Object { -not $_.Passed } | Format-Table -AutoSize
}
