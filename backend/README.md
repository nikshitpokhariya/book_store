# Book Exchange Backend

A high-performance C++20 REST API built with the [Drogon](https://github.com/drogonframework/drogon) web framework and MongoDB. The backend powers a peer-to-peer book marketplace and book exchange platform (`BookExchangeBackend`), featuring user authentication, listing management, persistent carts, atomic checkout with demo payment simulation, demo delivery fulfillment, two-sided peer-to-peer book exchanges, verified-purchase reviews, order cancellation with inventory restoration, and return workflows.

> [!IMPORTANT]
> **Simulated Subsystems Disclaimer**
> - **Payment is strictly simulated** for demonstration and educational purposes (`UPI`, `CARD`, `COD`). No real financial transactions, payment gateways, or banking networks are involved.
> - **Shipping and delivery are simulated** via in-memory tracking IDs and status progressions. No real third-party courier APIs (Delhivery, Shiprocket, BlueDart) are integrated.
> - **One listing represents exactly one physical book copy.** Inventory quantities cannot exceed 1, and physical listings are atomically transferred or delisted upon transaction completion.

---

## 1. Project Overview

`BookExchangeBackend` is designed as a secure, modular, and performant backend for college PBL (Project-Based Learning) and production-grade demonstration. It addresses three core marketplace interactions:
1. **Cash-based Peer-to-Peer Commerce**: Users buy and sell pre-owned physical books with single-copy inventory guarantees.
2. **Direct Peer-to-Peer Book Exchange**: Users swap physical books atomically without money changing hands.
3. **Post-Purchase Assurance & Reputation**: Buyers can track delivery, cancel orders prior to dispatch, request returns after delivery, and leave verified ratings and reviews.

---

## 2. System Architecture

The project adheres to a clean layered architecture with clear separation of concerns:

> **Repository Structure**: This backend lives at `backend/` inside the monorepo. The Qt desktop frontend is at `frontend/`. See the root `README.md` or the [frontend README](../frontend/README.md) for the frontend overview.

```
backend/
├── CMakeLists.txt              # Build system & vcpkg dependency management
├── vcpkg.json                  # C++ library manifest (Drogon, mongocxx, Argon2, jwt-cpp)
├── config.json                 # Drogon listener, threading & upload configuration
├── start_server.ps1            # PowerShell helper script to set env vars and launch server
├── include/
│   ├── config/AppConfig.h      # Environment variable configuration loader (Singleton)
│   ├── db/MongoDatabase.h      # Thread-safe MongoDB client & automated index manager
│   ├── filters/AuthFilter.h    # Drogon Bearer JWT authentication filter
│   ├── security/
│   │   ├── PasswordHasher.hpp  # Argon2id password hasher with OpenSSL CSPRNG salt
│   │   ├── TokenService.hpp    # HS256 JWT & SHA-256 refresh token manager
│   │   └── PaymentService.hpp  # Pluggable demo payment simulation provider
│   ├── models/                 # Plain domain structs (User, Session, Book, Cart, Order, Exchange, Review)
│   ├── repositories/           # MongoDB data access layer using mongocxx/bsoncxx
│   ├── services/               # Business logic & transaction orchestration
│   └── controllers/            # Drogon HTTP REST controllers
├── src/                        # Implementations matching include/
│   ├── controllers/UploadController.cpp # Cloudinary multipart media upload implementation
│   └── main.cpp                # Application entrypoint & graceful signal handler
└── tests/                      # Automated PowerShell integration test suites & Postman collection
    ├── test_qa_suite.ps1        # Master QA suite (94 tests, 100% pass rate)
    ├── test_atlas_connection.ps1# MongoDB Atlas DNS SRV & TCP connectivity verification
    ├── test_cloudinary_upload.ps1 # Cloudinary media upload & CDN delivery verification
    ├── test_marketplace.ps1     # Phase 1 — Book marketplace (16 tests)
    ├── test_buy_sell.ps1        # Phase 2 — Cart, checkout & payment (23 tests)
    ├── test_exchange.ps1        # Phase 3 — Peer-to-peer exchange (24 tests)
    ├── test_phase4.ps1          # Phase 4 — Reviews, cancel & returns (24 tests)
    └── BookExchangeBackend.postman_collection.json
```

---

## 3. Tech Stack

| Component | Technology | Version / Standard | Purpose |
| :--- | :--- | :--- | :--- |
| **Language** | C++ | ISO C++20 (`/std:c++20`) | Core backend language |
| **Web Framework** | Drogon | 1.9.x | High-throughput asynchronous non-blocking HTTP server |
| **Database** | MongoDB | 7.x / mongocxx 3.10+ | Document database for persistent storage |
| **Password Hashing** | Argon2 | Argon2id | Memory-hard password hashing |
| **Cryptography** | OpenSSL | 3.x | CSPRNG salt generation, SHA-256 token hashing |
| **Authentication** | jwt-cpp | 0.7.x | HS256 JSON Web Token encode/verify |
| **JSON Serialization** | JsonCpp | 1.9.x | High-speed JSON serialization and parsing |
| **Package Management** | vcpkg | Manifest mode | Dependency acquisition and toolchain integration |
| **Build System** | CMake | >= 3.20 | Multi-platform build generation |

---

## 4. Authentication & Security

- **Argon2id Password Protection**: Passwords are never stored in plaintext. They are salted with 16 bytes of OpenSSL CSPRNG randomness and hashed using memory-hard Argon2id parameters.
- **Dual-Token Architecture**:
  - **Access Token**: Short-lived (15 minutes) HS256-signed JWT carrying `userId` and `username`.
  - **Refresh Token**: Long-lived (7 days) cryptographically random hex token stored as a SHA-256 hash in MongoDB and transmitted via `HttpOnly`, `SameSite=Lax` cookies.
- **Middleware Authorization (`AuthFilter`)**: Drogon filter intercepts protected routes, validates the JWT signature, and attaches `userId` to the request context.
- **No Sensitive Field Leakage**: User password hashes, refresh token secrets, and internal private identifiers are scrubbed from all public API responses.

---

## 5. Core Subsystems

### 5.1 Book Marketplace (Browse & Search)
- **Listing Creation**: Authenticated sellers list books with ISBN, title, author, price, condition (`New`, `Like New`, `Very Good`, `Good`, `Acceptable`), category, and description.
- **Full Search & Filter Engine**: Case-insensitive substring search matching title, author, and ISBN; filters by category, condition, and price ranges (`minPrice`, `maxPrice`).
- **Rating Cache**: Every book maintains aggregated `averageRating` (rounded to 1 decimal place) and `reviewCount`, updated in real time as reviews are created, edited, or deleted.
- **Listing Privacy**: Seller email and password hashes are never exposed publicly. Only public seller metadata (`id`, `username`) is shared.

### 5.2 Shopping Cart (`/api/cart`)
- Persistent authenticated shopping cart stored in MongoDB.
- **Single-Copy Enforced**: Cart quantities are clamped to 1. Duplicate additions return `400 Bad Request`.
- **Self-Purchase Prevention**: Sellers cannot add their own books to their cart.
- **Stale Cart Detection**: When viewing a cart, books sold, exchanged, or removed by others are flagged with `isAvailable: false`.

### 5.3 Buy/Sell, Checkout & Demo Payment (`/api/checkout`)
- **Server-Side Price Calculation**: Totals are calculated dynamically on the server; client-provided prices are ignored.
- **₹0 Shipping**: Shipping cost is fixed at ₹0 because delivery is included in product listing prices.
- **Double-Purchase Prevention**: Atomic conditional update (`acquireForPurchase`) changes status from `available` to `sold`. If two buyers simultaneously checkout the same physical book, only one succeeds; the second buyer receives `409 Conflict` and all other books in their cart are rolled back.
- **Demo Payment Simulation**:
  - `UPI` / `CARD`: Automatically transitions payment status to `paid` with a unique transaction ID (`DEMO_TXN_<hex>`).
  - `COD`: Payment status set to `pending`.
  - Simulated failures (`simulatePaymentFailure: true`) immediately release all acquired inventory.

### 5.4 Order & Delivery Simulation (`/api/orders`)
- Server-generated tracking ID (`DEMO_TRACK_<hex>`).
- Strict delivery lifecycle: `PLACED` → `CONFIRMED` → `PACKED` → `SHIPPED` → `OUT_FOR_DELIVERY` → `DELIVERED`.
- **Seller-Only Delivery Progression**: Only sellers of items in the order can advance shipping status; buyers receive `403 Forbidden` if they attempt modification.
- Complete audit trail preserved in `history` array with timestamps and notes.

### 5.5 Order Cancellation (`/api/orders/:id/cancel`)
- Allowed **only before fulfillment reaches `SHIPPED`** (i.e. while in `PLACED`, `CONFIRMED`, or `PACKED`).
- Once `SHIPPED` or `DELIVERED`, cancellation is rejected with `400 Bad Request`.
- On cancellation:
  - Order status transitions to `CANCELLED`.
  - Payment status transitions to `refunded` (or `cancelled` for COD).
  - **Physical book inventory is restored**: status transitions back to `available` and the listing immediately reappears in public Browse & Search.

### 5.6 Return Workflow (`/api/orders/:id/return`)
- Suggested PBL-level lifecycle: `DELIVERED` → `RETURN_REQUESTED` → `RETURN_APPROVED` / `RETURN_REJECTED` → `RETURNED`.
- **Buyer Request**: Buyer initiates return on a `DELIVERED` order with a reason (`POST /api/orders/:id/return`).
- **Seller Review**: Seller approves or rejects the return (`POST /api/orders/:id/return/process`).
  - If approved: Order status transitions to `RETURNED`, payment status to `refunded`, and book listing status transitions to conservative state `returned` (delisted from public view).
  - If rejected: Order status transitions to `RETURN_REJECTED`.

### 5.7 Peer-to-Peer Book Exchange (`/api/exchanges`)
- User B proposes swapping an owned book for User A's book.
- **Zero Inventory Locking while Pending**: Books are not locked while an exchange request is pending; both users may continue to sell or exchange them elsewhere.
- **Atomic Two-Sided Ownership Transfer**:
  - Receiver accepts the exchange.
  - Two-phase conditional updates swap ownership and set status to `exchanged`.
  - If either listing was sold or modified concurrently, Phase 1 is automatically rolled back and `409 Conflict` is returned.
- Exchanged books immediately disappear from public Browse & Search.

### 5.8 Reviews & Ratings (`/api/reviews`)
- **Verified Purchase Requirement**: A user can only review a book from an order that has reached `DELIVERED` status.
- **No Self-Reviews**: Sellers are prevented from reviewing their own listings.
- **Duplicate Prevention**: Compound unique index `{ orderId: 1, bookId: 1 }` guarantees only one review per qualifying purchase.
- **Ratings & Aggregates**: Ratings validated from 1 to 5; book average rating and review counts are recalculated automatically on create, edit, or delete.
- **Public Privacy**: Public review listings display username, rating, comment, and timestamp without exposing order IDs or payment details.

### 5.9 Media Upload & Cloudinary CDN (`/api/upload`)
- **Multipart Form Upload**: Accepts multipart/form-data payloads with raw file bytes (images: JPEG, PNG, WEBP, GIF; condition videos: MP4, MOV, WEBM).
- **Signed Cloudinary Upload**: Computes SHA-1 cryptographic signatures (`timestamp=... + apiSecret`) ensuring secure server-to-server asset creation.
- **Dual Engine Pipeline**: Utilizes Drogon's asynchronous `HttpClient` with automatic fallback to native curl subprocess for high network resilience across Windows TLS configurations.
- **Immediate CDN URLs**: Returns `{ "success": true, "url": "https://res.cloudinary.com/.../image/upload/...", "public_id": "..." }` for direct persistence in book listings.

---

## 6. Database Schema & Indexes

Indexes are verified and automatically created at server startup in `MongoDatabase::ensureIndexes()`:

1. **`users`**:
   - `{ username: 1 }` (unique)
   - `{ email: 1 }` (unique)
2. **`sessions`**:
   - `{ refreshTokenHash: 1 }` (unique)
   - `{ expiresAt: 1 }` (TTL index)
3. **`books`**:
   - `{ status: 1, createdAt: -1 }`
   - `{ ownerId: 1, createdAt: -1 }`
   - `{ status: 1, category: 1, price: 1 }`
   - `{ status: 1, price: 1 }`
   - `{ isbn: 1 }`
4. **`carts`**:
   - `{ userId: 1 }` (unique)
5. **`orders`**:
   - `{ orderNumber: 1 }` (unique)
   - `{ buyerId: 1, createdAt: -1 }`
   - `{ "items.sellerId": 1, createdAt: -1 }`
   - `{ "items.bookId": 1 }`
   - `{ status: 1, createdAt: -1 }`
6. **`exchanges`**:
   - `{ exchangeNumber: 1 }` (unique)
   - `{ requesterId: 1, createdAt: -1 }`
   - `{ receiverId: 1, createdAt: -1 }`
   - `{ "requestedBook.bookId": 1 }`
   - `{ "offeredBook.bookId": 1 }`
   - `{ status: 1, createdAt: -1 }`
7. **`reviews`**:
   - `{ bookId: 1, createdAt: -1 }`
   - `{ reviewerId: 1, createdAt: -1 }`
   - `{ orderId: 1, bookId: 1 }` (unique)

---

## 7. Complete API Catalog

All responses return standard JSON envelopes:
- **Success**: `{ "success": true, ... }`
- **Error**: `{ "success": false, "message": "..." }`

### Authentication APIs (`/api/auth`)
| Method | Endpoint | Auth | Description |
| :--- | :--- | :---: | :--- |
| `POST` | `/api/auth/signup` | Public | Register new user account |
| `POST` | `/api/auth/login` | Public | Authenticate user, return JWT and refresh cookie |
| `POST` | `/api/auth/logout` | Bearer | Invalidate refresh token and clear cookie |
| `POST` | `/api/auth/refresh` | Cookie | Rotate refresh token and issue new JWT |
| `GET` | `/api/auth/me` | Bearer | Fetch profile of authenticated user |

### Book Marketplace APIs (`/api/books`)
| Method | Endpoint | Auth | Description |
| :--- | :--- | :---: | :--- |
| `POST` | `/api/books` | Bearer | Create a new book listing |
| `GET` | `/api/books` | Public | Browse/Search available books (query: `search`, `category`, `condition`, `minPrice`, `maxPrice`, `sort`, `page`, `limit`) |
| `GET` | `/api/books/my` | Bearer | Retrieve seller's own listings (including historical/sold) |
| `GET` | `/api/books/:id` | Public/Bearer | Get book details (includes `averageRating` and `reviewCount`) |
| `PUT` | `/api/books/:id` | Bearer | Update owned book listing |
| `DELETE` | `/api/books/:id` | Bearer | Remove owned listing (soft-delete to `removed`) |

### Shopping Cart APIs (`/api/cart`)
| Method | Endpoint | Auth | Description |
| :--- | :--- | :---: | :--- |
| `GET` | `/api/cart` | Bearer | View cart items, subtotal, and stale flags |
| `POST` | `/api/cart/items` | Bearer | Add book to cart (`{ "bookId": "..." }`) |
| `DELETE` | `/api/cart/items/:bookId` | Bearer | Remove single book from cart |
| `DELETE` | `/api/cart` | Bearer | Clear entire cart |

### Checkout & Orders APIs (`/api/checkout`, `/api/orders`)
| Method | Endpoint | Auth | Description |
| :--- | :--- | :---: | :--- |
| `POST` | `/api/checkout` | Bearer | Place order with demo payment, ₹0 shipping, and atomic lock |
| `GET` | `/api/orders` | Bearer | Retrieve buyer's order history |
| `GET` | `/api/orders/:id` | Bearer | Retrieve order details (buyer or seller only) |
| `GET` | `/api/orders/seller/history` | Bearer | Retrieve seller's sales history |
| `PATCH` | `/api/orders/:id/status` | Bearer | Advance delivery status (seller only) |
| `POST` | `/api/orders/:id/cancel` | Bearer | Cancel order before shipping & restore book availability |
| `POST` | `/api/orders/:id/return` | Bearer | Request return on DELIVERED order (buyer only) |
| `POST` | `/api/orders/:id/return/process`| Bearer | Approve or reject return (seller only) |

### Book Exchange APIs (`/api/exchanges`)
| Method | Endpoint | Auth | Description |
| :--- | :--- | :---: | :--- |
| `POST` | `/api/exchanges` | Bearer | Propose exchange offering an owned book for a requested book |
| `GET` | `/api/exchanges/sent` | Bearer | View sent exchange requests |
| `GET` | `/api/exchanges/received` | Bearer | View received exchange requests |
| `GET` | `/api/exchanges/history` | Bearer | View private exchange history |
| `GET` | `/api/exchanges/:id` | Bearer | View exchange details (participants only) |
| `POST` | `/api/exchanges/:id/accept` | Bearer | Accept exchange & execute atomic ownership swap (receiver only) |
| `POST` | `/api/exchanges/:id/reject` | Bearer | Reject exchange request (receiver only) |
| `POST` | `/api/exchanges/:id/cancel` | Bearer | Cancel pending exchange request (requester only) |

### Reviews APIs (`/api/reviews`)
| Method | Endpoint | Auth | Description |
| :--- | :--- | :---: | :--- |
| `POST` | `/api/reviews` | Bearer | Submit review for verified DELIVERED purchase (`{ orderId, bookId, rating, comment }`) |
| `GET` | `/api/books/:id/reviews` | Public | View public reviews for a book (paginated) |
| `GET` | `/api/reviews/my` | Bearer | View reviews written by authenticated user |
| `PUT` | `/api/reviews/:id` | Bearer | Edit rating or comment of own review |
| `DELETE` | `/api/reviews/:id` | Bearer | Delete own review and update book aggregate rating |

### Media Upload APIs (`/api/upload`)
| Method | Endpoint | Auth | Description |
| :--- | :--- | :---: | :--- |
| `POST` | `/api/upload` | Public/Bearer | Multipart file upload to Cloudinary CDN (`file` field). Returns `{ success: true, url, public_id }` |

---

## 8. Environment Variables

Configuration is loaded from `backend/.env` or system environment variables:

| Variable | Default Value | Description |
| :--- | :--- | :--- |
| `JWT_SECRET` | *(Required)* | High-entropy secret key (≥32 chars) for HS256 JWT signature verification |
| `JWT_ISSUER` | `myapp-api` | Token issuer claim identifier |
| `MONGO_URI` | `mongodb://127.0.0.1:27017` | MongoDB connection URI (supports `mongodb+srv://` Atlas clusters) |
| `MONGO_DATABASE` | `myapp` | Target MongoDB database name |
| `ACCESS_TOKEN_LIFETIME` | `900` | Access token lifespan in seconds (15 minutes) |
| `REFRESH_TOKEN_LIFETIME` | `2592000` | Refresh token lifespan in seconds (30 days) |
| `SECURE_COOKIES` | `false` | Sets `Secure` flag on refresh token HTTP-only cookies (enable for HTTPS) |
| `CLOUDINARY_CLOUD_NAME` | *(Optional)* | Cloudinary cloud identifier for media uploads |
| `CLOUDINARY_API_KEY` | *(Optional)* | Cloudinary API Key for signed REST uploads |
| `CLOUDINARY_API_SECRET` | *(Optional)* | Cloudinary API Secret for SHA-1 upload signatures |
| `PORT` | `8080` | HTTP listener port (configured in `config.json`) |

---

## 9. Build & Run Instructions

### Prerequisites
- Windows 10/11 with Visual Studio 2022 (C++20 Desktop workload)
- [vcpkg](https://github.com/microsoft/vcpkg) installed at `C:/dev/vcpkg`
- MongoDB server running locally or accessible remotely

### 1. Configure Build
```powershell
cd backend
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### 2. Compile Executable
```powershell
cmake --build build --config Debug
```

### 3. Launch Backend Server
```powershell
$env:JWT_SECRET = "WhWRp7LUnP+BUdQaOxcjpf67hKIBcnk3WAqTrTbzfjOy+CWOdXlgIqsqGTCTHYu7"
$env:MONGO_URI = "mongodb://127.0.0.1:27017"
$env:MONGO_DATABASE = "book_exchange"
.\build\Debug\BookExchangeBackend.exe
```

---

## 10. Testing & QA

A dedicated, comprehensive QA & Integration test suite was developed to audit the complete API surface, business workflows, concurrency controls, and security posture across Phases 1 through 4.

### Test Architecture & Test Suites
The automated test suites are located in `backend/tests/`:

| Suite | Script | Tests | Description |
| :--- | :--- | :---: | :--- |
| **Master QA Suite** | `tests/test_qa_suite.ps1` | **94** | Complete end-to-end integration, security, fuzzing, upload, and boundary test suite |
| **Marketplace Slice** | `tests/test_marketplace.ps1` | **16** | Phase 1 listing CRUD, browse, multi-criteria search, pagination, and ownership |
| **Buy/Sell Slice** | `tests/test_buy_sell.ps1` | **23** | Phase 2 cart, stale cart detection, checkout, demo payment, and concurrency |
| **Exchange Slice** | `tests/test_exchange.ps1` | **24** | Phase 3 peer-to-peer book exchange, atomic swap, and cross-slice conflict |
| **Product Hardening**| `tests/test_phase4.ps1` | **24** | Phase 4 reviews, rating calculation, order cancellation, and return workflow |
| **Atlas Cloud Check**| `tests/test_atlas_connection.ps1` | Verification | Automated DNS SRV resolution and TCP 27017 Atlas cluster verification |
| **Cloudinary Check** | `tests/test_cloudinary_upload.ps1` | Verification | Signed multipart upload and HTTP 200 CDN image delivery verification |

```powershell
# Run the Master QA Suite against a running server (All 94 tests):
powershell -ExecutionPolicy Bypass -File tests/test_qa_suite.ps1

# Run individual vertical slice regressions:
powershell -ExecutionPolicy Bypass -File tests/test_marketplace.ps1
powershell -ExecutionPolicy Bypass -File tests/test_buy_sell.ps1
powershell -ExecutionPolicy Bypass -File tests/test_exchange.ps1
powershell -ExecutionPolicy Bypass -File tests/test_phase4.ps1

# Verify cloud integrations:
powershell -ExecutionPolicy Bypass -File tests/test_atlas_connection.ps1
powershell -ExecutionPolicy Bypass -File tests/test_cloudinary_upload.ps1
```

### Test Categories & Coverage (94 Tests Total)
1. **Authentication & Session Management (16 tests)**:
   - Signup, duplicate username/email rejection, password boundary checks (8-char minimum), Argon2id verification.
   - Login, invalid password, non-existent user handling.
   - Access token issuance and verification, HttpOnly refresh cookie rotation, session revocation upon logout.
   - Tampered JWT signature and malformed Bearer token rejection.
2. **Book Marketplace (25 tests)**:
   - Authenticated listing creation with schema validation (title, author, ISBN, condition, price bounds).
   - Unauthenticated write rejection; ownership derivation from authenticated JWT (cannot forge `ownerId`).
   - Public browse and case-insensitive multi-field search (`title`, `author`, `isbn`).
   - Filtering by condition, category, and price range; ascending and descending price sorting.
   - Offset pagination with metadata (`page`, `limit`, `total`, `totalPages`, `hasNextPage`, `hasPrevPage`).
   - IDOR protection: Non-owners cannot update or soft-delete other users' listings.
3. **Cart & Stale Cart Management (11 tests)**:
   - Add/remove items, clear cart, and subtotal calculation.
   - Single-copy physical inventory enforcement: Quantity > 1 and duplicate additions return `400 Bad Request`.
   - Self-purchase prevention: Sellers cannot add their own listings to cart.
   - **Mandatory Stale Cart Detection**: When another user purchases a book, any cart containing that book flags it as `isAvailable: false`. Attempting checkout immediately rejects with `409 Conflict`.
4. **Buy/Sell, Demo Payment & Concurrency (6 tests)**:
   - Server-enforced financial calculations: Client-submitted totals or prices are ignored; ₹0 shipping enforced.
   - Demo payment methods: `CARD` and `UPI` yield `paymentStatus: paid`; `COD` yields `paymentStatus: pending`.
   - Payment failure simulation rolls back acquired inventory to `available`.
   - **Double-Purchase Concurrency Test**: Two simultaneous checkouts targeting the same physical book result in exactly one successful order and one clean `409 Conflict` failure.
5. **Order Lifecycle & Delivery Simulation (6 tests)**:
   - Order creation with snapshots of book metadata, buyer ID, and seller ID.
   - Delivery progression: `PLACED` -> `CONFIRMED` -> `PACKED` -> `SHIPPED` -> `OUT_FOR_DELIVERY` -> `DELIVERED`.
   - Strict transition validation: Disallows illegal backwards jumps (e.g., `DELIVERED` -> `PLACED`).
   - Seller-only fulfillment authorization: Buyers receive `403 Forbidden` if attempting to change delivery status.
6. **Peer-to-Peer Book Exchange (9 tests)**:
   - Valid proposals (`PENDING`), self-request rejection, offering unowned books rejection.
   - Private visibility: Requesters view in `/sent`, owners view in `/received`; unrelated third parties receive `403 Forbidden`.
   - Rejection and cancellation workflows preserve book availability.
   - Atomic two-sided ownership swap: Status switches to `COMPLETED`, book owners swap, and both books are removed from public browse and search.
   - Concurrency conflict: Attempting to exchange a book that was purchased simultaneously fails with `409 Conflict`.
7. **Reviews & Ratings (7 tests)**:
   - Purchase verification: Reviews are restricted to buyers who received the book (`DELIVERED` status).
   - Non-purchaser, seller self-review, and duplicate review attempts are rejected with `400 Bad Request`.
   - Rating boundary enforcement (1 to 5).
   - Real-time rating aggregation: Book `averageRating` and `reviewCount` dynamically recalculate on review creation, edit, or deletion.
   - IDOR protection: Users cannot edit or delete reviews written by other users.
8. **Order Cancellation & Return Workflow (5 tests)**:
   - Cancellation permitted before shipping (`PLACED`, `CONFIRMED`), immediately refunding payment and restoring book status to `available`.
   - Cancellation rejected after order has entered `SHIPPED` or `DELIVERED`.
   - Post-delivery return workflow: Buyer requests return (`RETURN_REQUESTED`); seller approves (`RETURNED`) or rejects (`RETURN_REJECTED`).
   - Returned books are conservatively marked `returned` and excluded from public browse to prevent relisting uninspected stock.
9. **Security Audit & Input Fuzzing (6 tests)**:
   - IDOR verification across all private resources (listings, carts, orders, exchanges, reviews).
   - Fuzzing with malformed JSON, empty bodies, type confusion, oversized strings (10,000+ chars), and SQL/Mongo injection patterns.
   - Zero sensitive information leakage: Error responses never expose C++ stack traces, internal MongoDB drivers, password hashes, or JWT secrets.
10. **Media Upload & CDN Verification (3 tests)**:
    - Empty request body to `/api/upload` returns `400 Bad Request`.
    - Non-multipart payload returns `400 Bad Request`.
    - Multipart image upload handled gracefully with signed Cloudinary integration or clean error response.

---

## 11. Current Implementation Status

### Completed (Working)
- [x] Full Argon2id password hashing and rotating JWT/refresh token authentication.
- [x] Complete book marketplace with Browse, Search, Filtering, Sorting, and Pagination.
- [x] Authenticated single-copy persistent cart with stale unavailable item detection.
- [x] Server-side price calculation with ₹0 shipping.
- [x] Race-condition-free atomic checkout preventing double purchases.
- [x] Pluggable demo payment provider (`UPI`, `CARD`, `COD`) with simulated failure rollback.
- [x] Delivery lifecycle simulation (`PLACED` through `DELIVERED`) with seller fulfillment authorization.
- [x] Peer-to-peer book exchange with atomic two-sided ownership transfer and rollback guarantees.
- [x] Verified-purchase review system with duplicate prevention, author edit/delete, and real-time rating aggregates.
- [x] Pre-shipping order cancellation with automatic inventory availability restoration.
- [x] PBL-level return request and approval/rejection workflow with conservative `returned` delisting.
- [x] Private buyer and seller transaction histories.
- [x] **Cloudinary CDN Media Uploads**: Multipart form handling via `/api/upload` with signed REST upload and native curl fallback.
- [x] **MongoDB Atlas Cloud Integration**: Seamless connectivity to cloud-hosted clusters with DNS SRV resolution.

---

## 12. Future Implementations & Roadmap

For an in-depth architectural analysis and implementation plan for commercial production scaling (Real Payment Gateways via Razorpay/Stripe, 3PL Courier Logistics via Shiprocket/Delhivery, WebSockets, End-to-End Chat, AI ISBN barcode scanning, and Docker/K8s deployment), refer to the root project roadmap:
👉 **[`future.md`](../future.md)**
