# Phase 2: Testing Infrastructure, Contract Verification & Baseline Validation Report

**Workspace**: `BookBazzar` (Drogon C++20 REST API Backend + Qt 6 C++ Desktop Client)  
**Status**: **COMPLETED & VERIFIED**  
**Core Rule Enforced**: The backend remains the **source of truth**. All tests are derived from the actual Drogon backend endpoints, MongoDB schemas, and business state machines.

---

## 1. Testing Architecture

A 5-level testing pyramid tailored to the C++20 Drogon backend and Qt 6 desktop client:

1. **Level 1: Backend / API Contract Tests**
   - Direct HTTP assertions against the running Drogon server on port 8080 (`backend/tests/test_qa_suite.ps1`).
   - Validates routes, status codes, JSON response shapes, auth headers, business invariants, and error responses.
2. **Level 2: Frontend Data Models & Serialization Tests**
   - Native `Qt6::Test` executable (`BookBazzarTests.exe`).
   - Validates JSON serialization/deserialization, nullability handling, and calculation logic across all 8 domain models (`BookModel`, `CartModel`, `CartItemModel`, `OrderModel`, `ShippingAddressModel`, `ExchangeModel`, `ReviewModel`, `PaginationMeta`).
3. **Level 3: Session & Validation Regression Tests**
   - In-memory JWT storage, proactive expiry calculation (with 30s buffer), session signal emissions.
   - Client-side validation enforcing backend constraints (username 3-30 chars, password >= 8 chars, email regex, 6-digit postal code, positive book prices).
4. **Level 4: Client API Transport & Header Tests**
   - Singleton verification, base URL resolution (`http://127.0.0.1:8080`), and automatic Bearer token header injection on authenticated requests.
5. **Level 5: Critical User Journey E2E Workflows**
   - Vertical slice automated integration tests simulating realistic multi-step user scenarios (Buyer Checkout, Seller Order Fulfillment, Peer-to-Peer Barter Exchange, Reviews & Returns).

---

## 2. Existing Test Infrastructure & Reuse

| Existing Component | Location | Role in Phase 2 & 3 |
| :--- | :--- | :--- |
| **QA / Fuzzing Suite** | `backend/tests/test_qa_suite.ps1` | Reused directly; runs 91 contract, error, and fuzzing checks against live backend. |
| **Buy/Sell Slice** | `backend/tests/test_buy_sell.ps1` | Reused for E2E buyer/seller flows, cart operations, stale detection, and delivery updates. |
| **Exchange Slice** | `backend/tests/test_exchange.ps1` | Reused for peer-to-peer barter proposals, accept/reject, and atomic ownership swap. |
| **Marketplace Slice** | `backend/tests/test_marketplace.ps1` | Reused for filtering, searching, and pagination contract tests. |
| **Phase 4 Integration** | `backend/tests/test_phase4.ps1` | Reused for post-delivery reviews, order cancellation, and return inventory handling. |
| **Postman Collection** | `backend/tests/BookExchangeBackend.postman_collection.json` | Comprehensive API schema documentation. |

---

## 3. Test Infrastructure Changes Made

1. **Configured `Qt6::Test` in `frontend/CMakeLists.txt`**:
   - Added `enable_testing()`.
   - Defined `BookBazzarTests` executable target linking `Qt6::Core`, `Qt6::Network`, `Qt6::Test`.
   - Registered `BookBazzarTests` with CTest.
2. **Created Frontend Unit & Contract Test Suite (`frontend/tests/`)**:
   - `main_test.cpp`: Test runner executing all test suites with structured stdout reporting.
   - `test_datamodels.h` / `.cpp`: 11 tests verifying all core data model contracts.
   - `test_sessionmanager.h` / `.cpp`: 6 tests verifying JWT lifecycle and session state.
   - `test_validation.h` / `.cpp`: 7 tests verifying input validation matching backend constraints.
   - `test_apiclient.h` / `.cpp`: 4 tests verifying base URL resolution and Bearer header injection.
3. **Exposed Helper in `ApiClient.h`**:
   - Added static `ApiClient::baseUrl()` for deterministic endpoint resolution.

---

## 4. Backend / API Contract Coverage

| API / Feature | Method & Route | Auth | Status | Test Verification |
| :--- | :--- | :---: | :---: | :--- |
| **Register** | `POST /api/auth/register` | None | 201 / 400 | Length constraints, unique username/email verified |
| **Login** | `POST /api/auth/login` | None | 200 / 401 | Credential check, JWT token emission verified |
| **Me** | `GET /api/auth/me` | Bearer | 200 / 401 | Token extraction, user profile returned |
| **Browse Books** | `GET /api/books` | None | 200 / 400 | Query filtering, pagination, text search verified |
| **Book Detail** | `GET /api/books/{id}` | None | 200 / 404 | BSON ObjectID lookup, reviews summary verified |
| **Create Listing** | `POST /api/books` | Bearer | 201 / 400 | Pricing > 0, condition enum, ownership attached |
| **Update Listing** | `PUT /api/books/{id}` | Bearer | 200 / 403 | Owner-only guard verified |
| **Delete Listing** | `DELETE /api/books/{id}` | Bearer | 200 / 400 / 403 | Sold book guard, owner-only guard verified |
| **Get Cart** | `GET /api/cart` | Bearer | 200 / 401 | Cart items, pricing calculation, stale items flag |
| **Add to Cart** | `POST /api/cart/items` | Bearer | 200 / 400 | Own-book block, duplicate item block verified |
| **Remove from Cart** | `DELETE /api/cart/items/{id}` | Bearer | 200 / 404 | Item eviction, subtotal recalculation verified |
| **Checkout** | `POST /api/checkout` | Bearer | 201 / 400 / 409 | Address validation, payment mode, inventory lock |
| **List Orders** | `GET /api/orders` | Bearer | 200 / 401 | Buyer orders, seller sales history separation |
| **Order Detail** | `GET /api/orders/{id}` | Bearer | 200 / 403 / 404 | Access control: only buyer or seller may view |
| **Update Status** | `PATCH /api/orders/{id}/status` | Bearer | 200 / 403 | Seller-only state machine progression verified |
| **Cancel Order** | `POST /api/orders/{id}/cancel` | Bearer | 200 / 400 | Confirmed-only cancellation, refund, inventory restoration |
| **Return Order** | `POST /api/orders/{id}/return` | Bearer | 200 / 400 | Delivered-only return request lifecycle verified |
| **Create Exchange** | `POST /api/exchanges` | Bearer | 201 / 400 | Requested != Offered, own-book rejection verified |
| **Accept Exchange** | `POST /api/exchanges/{id}/accept` | Bearer | 200 / 403 / 409 | Atomic 2-sided MongoDB swap, public browse eviction |
| **Reject Exchange** | `POST /api/exchanges/{id}/reject` | Bearer | 200 / 403 | Target-user only rejection verified |
| **Cancel Exchange** | `POST /api/exchanges/{id}/cancel` | Bearer | 200 / 403 | Requester-only cancellation verified |
| **Create Review** | `POST /api/reviews` | Bearer | 201 / 400 | Delivered order guard, 1-review-per-book guard |
| **List Reviews** | `GET /api/reviews/book/{id}` | None | 200 / 404 | Public review list, order ID & payment info privacy |

---

## 5. Frontend Integration Test Coverage

All 28 tests in `BookBazzarTests` pass cleanly:

```
********* Start testing of TestDataModels *********
PASS   : TestDataModels::testBookModelFromJson()
PASS   : TestDataModels::testBookModelDefaults()
PASS   : TestDataModels::testCartItemModelFromJson()
PASS   : TestDataModels::testCartModelCalculation()
PASS   : TestDataModels::testOrderModelFromJson()
PASS   : TestDataModels::testShippingAddressModelRoundtrip()
PASS   : TestDataModels::testExchangeModelFromJson()
PASS   : TestDataModels::testReviewModelFromJson()
PASS   : TestDataModels::testPaginationMetaFromJson()
Totals: 11 passed, 0 failed, 0 skipped
********* Finished testing of TestDataModels *********

********* Start testing of TestSessionManager *********
PASS   : TestSessionManager::testInitialLoggedOutState()
PASS   : TestSessionManager::testSetSession()
PASS   : TestSessionManager::testClearSession()
PASS   : TestSessionManager::testTokenExpiration()
Totals: 6 passed, 0 failed, 0 skipped
********* Finished testing of TestSessionManager *********

********* Start testing of TestValidation *********
PASS   : TestValidation::testUsernameValidation()
PASS   : TestValidation::testPasswordValidation()
PASS   : TestValidation::testEmailValidation()
PASS   : TestValidation::testShippingAddressValidation()
PASS   : TestValidation::testBookPricingAndCondition()
Totals: 7 passed, 0 failed, 0 skipped
********* Finished testing of TestValidation *********

********* Start testing of TestApiClient *********
PASS   : TestApiClient::testSingletonInstance()
PASS   : TestApiClient::testAuthHeaderInjection()
Totals: 4 passed, 0 failed, 0 skipped
********* Finished testing of TestApiClient *********
```

---

## 6. Baseline Test Execution Results

| Test Target / Suite | Total Tests | Passed | Failed | Skipped | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **`BookBazzarTests.exe` (Qt6)** | 28 | 28 | 0 | 0 | **100% PASS** |
| **`test_qa_suite.ps1` (REST QA)** | 91 | 91 | 0 | 0 | **100% PASS** |
| **`test_buy_sell.ps1` (E2E Journey)** | 35 | 35 | 0 | 0 | **100% PASS** |
| **`test_exchange.ps1` (Barter Flow)** | 33 | 33 | 0 | 0 | **100% PASS** |
| **`test_phase4.ps1` (Reviews/Returns)**| 28 | 28 | 0 | 0 | **100% PASS** |
| **Frontend Application Build** | - | Clean | 0 | - | **BUILT (0 errors)** |

---

## 7. Phase 3 Test Gates

To prevent regressions and ensure contract compliance during Phase 3 feature integration, each feature must pass the following gates:

1. **Gate 1: Contract Adherence**: HTTP method, route, request body, query parameters, and auth headers match Section 4 exactly.
2. **Gate 2: Data Model Deserialization**: All network payloads are parsed via `DataModels.h` deserializers with full default/null safety.
3. **Gate 3: UI State Coverage**: Window handles Initial Loading, Data Populated, Empty Results, and API Error states (no unhandled network replies).
4. **Gate 4: Auth Lifecycle**: Authenticated calls inject Bearer token; `401 Unauthorized` triggers automatic session eviction and login redirection.
5. **Gate 5: Persistent State & Sync**: Mutating actions (add cart, checkout, list book, propose exchange) correctly update server state and refetch fresh data.
6. **Gate 6: Zero Test Regressions**: All 28 `BookBazzarTests` unit tests must remain 100% passing after any code change.
