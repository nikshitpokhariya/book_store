# BookExchangeBackend

C++20 REST API backend for BookBazzar, built with [Drogon](https://github.com/drogonframework/drogon) and MongoDB Atlas. Provides authentication, book listings, cart, atomic checkout, order lifecycle, peer-to-peer barter exchanges, verified reviews, and Cloudinary media upload.

> **Simulated subsystems:** Payment (`UPI`, `CARD`, `COD`) and shipping/delivery are simulated for demonstration. No real payment gateways or courier APIs are integrated. Each book listing represents exactly one physical copy.

---

## Architecture

Clean layered architecture with strict separation of concerns:

```
backend/
├── CMakeLists.txt              # Build system & vcpkg dependency management
├── vcpkg.json                  # C++ library manifest
├── config.json                 # Drogon listener, threading & upload config
├── .env.example                # Environment variable template
├── include/
│   ├── config/AppConfig.h      # Env var loader — Singleton
│   ├── db/MongoDatabase.h      # Thread-safe MongoDB client & index manager
│   ├── filters/AuthFilter.h    # Drogon Bearer JWT authentication filter
│   ├── security/
│   │   ├── PasswordHasher.hpp  # Argon2id hasher — OpenSSL CSPRNG salt
│   │   ├── TokenService.hpp    # HS256 JWT & SHA-256 refresh token manager
│   │   └── PaymentService.hpp  # Demo payment simulation provider
│   ├── models/                 # Domain structs: User, Session, Book, Cart, Order, Exchange, Review
│   ├── repositories/           # MongoDB data access — mongocxx/bsoncxx
│   ├── services/               # Business logic & transaction orchestration
│   └── controllers/            # Drogon HTTP REST controllers
├── src/                        # Implementations matching include/
└── tests/                      # PowerShell integration suites & Postman collection
```

---

## Tech Stack

| Component | Technology | Version |
| :--- | :--- | :--- |
| Language | C++ | ISO C++20 |
| HTTP framework | Drogon | 1.9.x |
| Database | MongoDB Atlas — mongocxx | 7.x / 8.0 |
| Password hashing | Argon2id | — |
| Cryptography | OpenSSL | 3.x |
| Auth tokens | jwt-cpp — HS256 | 0.7.x |
| JSON | JsonCpp | 1.9.x |
| Package manager | vcpkg manifest mode | — |
| Build system | CMake | ≥ 3.20 |

---

## Authentication & Security

- **Argon2id Password Hashing** — 16-byte OpenSSL CSPRNG salt, memory-hard parameters, never stored in plaintext.
- **Dual-Token Auth**
  - *Access token*: 15-minute HS256-signed JWT carrying `userId` and `username`.
  - *Refresh token*: 30-day cryptographically random hex token stored as SHA-256 hash in MongoDB, delivered via `HttpOnly SameSite=Lax` cookie.
- **AuthFilter** — Drogon middleware validates JWT signature and injects `userId` into request context on all protected routes.
- **No Sensitive Leakage** — Password hashes, refresh token hashes, and internal IDs are scrubbed from all API responses.

---

## Core Subsystems

### Marketplace — `/api/books`
- Authenticated sellers list books with ISBN, title, author, price, condition, and category.
- Case-insensitive full-text search on title, author, and ISBN; filter by condition, category, and price range; ascending/descending price sort; offset pagination with `page`, `limit`, `total`, `totalPages`.
- Book aggregate `averageRating` and `reviewCount` maintained in real-time.
- IDOR protection — non-owners cannot update or soft-delete others' listings.

### Shopping Cart — `/api/cart`
- Persistent per-user cart stored in MongoDB.
- Single-copy enforced — quantity > 1 and duplicate additions return `400`.
- Self-purchase prevention — sellers cannot add their own books.
- Stale detection — GET cart flags sold/exchanged/removed books as `isAvailable: false`.

### Checkout & Payment — `/api/checkout`
- Server-side price calculation — client prices ignored; ₹0 shipping.
- Atomic `acquireForPurchase` — conditional update from `available` → `sold`; concurrent second checkout receives `409 Conflict` with full cart rollback.
- Demo payment: `UPI`/`CARD` → `paid` with `DEMO_TXN_<hex>`; `COD` → `pending`; `simulatePaymentFailure` releases inventory.

### Order Lifecycle — `/api/orders`
- Strict progression: `PLACED → CONFIRMED → PACKED → SHIPPED → OUT_FOR_DELIVERY → DELIVERED`.
- Seller-only fulfillment advancement; buyer receives `403 Forbidden` on status mutation.
- Full audit trail in `history[]` with timestamps and notes.
- Logistics tracking ID: `TRK-IN-<date>-<hex>` generated automatically by backend.

### Cancellation & Returns — `/api/orders/:id/cancel`, `/api/orders/:id/return`
- Cancellation allowed before `SHIPPED`; rejected after. On cancel: status → `CANCELLED`, payment → `refunded`/`cancelled`, book → `available`.
- Return flow: buyer requests (`RETURN_REQUESTED`) → seller approves (`RETURNED`) or rejects (`RETURN_REJECTED`). Approved: book → `returned` (delisted).

### Barter Exchange — `/api/exchanges`
- Peer proposes swap of an owned book for another's listing.
- No inventory lock while pending — books remain listable elsewhere.
- Atomic two-sided ownership transfer on accept; concurrent marketplace purchase causes `409 Conflict` and automatic rollback.
- Exchanged books immediately removed from public search.

### Reviews & Ratings — `/api/reviews`
- Verified purchase gate — reviewer must have a `DELIVERED` order for the book.
- Self-review prevention; compound unique index `{ orderId, bookId }` prevents duplicates.
- Rating range 1–5; `averageRating` and `reviewCount` recalculated on create/edit/delete.
- Public listings scrub order IDs and payment metadata.

### Media Upload — `/api/upload`
- Multipart form upload (JPEG, PNG, WEBP, GIF, MP4, MOV, WEBM).
- SHA-1 signed Cloudinary upload (`timestamp + apiSecret`); dual engine — Drogon `HttpClient` with native curl fallback.
- Returns `{ "success": true, "url": "https://res.cloudinary.com/...", "public_id": "..." }`.

---

## Database Schema & Indexes

Indexes auto-created at startup in `MongoDatabase::ensureIndexes()`:

| Collection | Index |
| :--- | :--- |
| `users` | `{ username: 1 }` unique, `{ email: 1 }` unique |
| `sessions` | `{ refreshTokenHash: 1 }` unique, `{ expiresAt: 1 }` TTL |
| `books` | `{ status, createdAt }`, `{ ownerId, createdAt }`, `{ status, category, price }`, `{ isbn }` |
| `carts` | `{ userId: 1 }` unique |
| `orders` | `{ orderNumber }` unique, `{ buyerId, createdAt }`, `{ items.sellerId, createdAt }`, `{ items.bookId }` |
| `exchanges` | `{ exchangeNumber }` unique, `{ requesterId }`, `{ receiverId }`, `{ requestedBook.bookId }`, `{ offeredBook.bookId }` |
| `reviews` | `{ bookId, createdAt }`, `{ reviewerId }`, `{ orderId, bookId }` unique |

---

## API Reference

All responses: `{ "success": true, ... }` or `{ "success": false, "message": "..." }`.  
Protected routes require `Authorization: Bearer <jwt>`.

### Auth — `/api/auth`
| Method | Route | Auth | Description |
| :--- | :--- | :---: | :--- |
| `POST` | `/api/auth/signup` | Public | Register with Argon2id-hashed password |
| `POST` | `/api/auth/login` | Public | Authenticate; return JWT + refresh cookie |
| `POST` | `/api/auth/logout` | Bearer | Revoke refresh token |
| `POST` | `/api/auth/refresh` | Cookie | Rotate refresh token; issue new JWT |
| `GET` | `/api/auth/me` | Bearer | Fetch authenticated user profile |

### Books — `/api/books`
| Method | Route | Auth | Description |
| :--- | :--- | :---: | :--- |
| `GET` | `/api/books` | Public | Browse/search (`search`, `category`, `condition`, `minPrice`, `maxPrice`, `sort`, `page`, `limit`) |
| `GET` | `/api/books/:id` | Public | Book detail with `averageRating` and `reviewCount` |
| `GET` | `/api/books/my` | Bearer | Seller's own listings (including sold/historical) |
| `POST` | `/api/books` | Bearer | Create listing |
| `PUT` | `/api/books/:id` | Bearer | Update owned listing |
| `DELETE` | `/api/books/:id` | Bearer | Soft-delete owned listing |

### Cart — `/api/cart`
| Method | Route | Auth | Description |
| :--- | :--- | :---: | :--- |
| `GET` | `/api/cart` | Bearer | View cart with stale flags |
| `POST` | `/api/cart/items` | Bearer | Add book `{ "bookId": "..." }` |
| `DELETE` | `/api/cart/items/:bookId` | Bearer | Remove single item |
| `DELETE` | `/api/cart` | Bearer | Clear entire cart |

### Checkout & Orders — `/api/checkout`, `/api/orders`
| Method | Route | Auth | Description |
| :--- | :--- | :---: | :--- |
| `POST` | `/api/checkout` | Bearer | Atomic checkout with demo payment |
| `GET` | `/api/orders` | Bearer | Buyer order history |
| `GET` | `/api/orders/:id` | Bearer | Order detail (buyer or seller) |
| `GET` | `/api/orders/seller/history` | Bearer | Seller sales history |
| `PATCH` | `/api/orders/:id/status` | Bearer | Advance delivery status (seller only) |
| `POST` | `/api/orders/:id/cancel` | Bearer | Cancel before shipping |
| `POST` | `/api/orders/:id/return` | Bearer | Request return (buyer, post-delivery) |
| `POST` | `/api/orders/:id/return/process` | Bearer | Approve/reject return (seller) |

### Exchanges — `/api/exchanges`
| Method | Route | Auth | Description |
| :--- | :--- | :---: | :--- |
| `POST` | `/api/exchanges` | Bearer | Propose barter swap |
| `GET` | `/api/exchanges/sent` | Bearer | Sent proposals |
| `GET` | `/api/exchanges/received` | Bearer | Received proposals |
| `GET` | `/api/exchanges/history` | Bearer | Completed exchange history |
| `GET` | `/api/exchanges/:id` | Bearer | Exchange detail (participants only) |
| `POST` | `/api/exchanges/:id/accept` | Bearer | Accept — atomic ownership swap |
| `POST` | `/api/exchanges/:id/reject` | Bearer | Reject (receiver) |
| `POST` | `/api/exchanges/:id/cancel` | Bearer | Cancel (requester) |

### Reviews — `/api/reviews`
| Method | Route | Auth | Description |
| :--- | :--- | :---: | :--- |
| `POST` | `/api/reviews` | Bearer | Submit review for DELIVERED purchase |
| `GET` | `/api/books/:id/reviews` | Public | Public reviews (paginated, scrubbed) |
| `GET` | `/api/reviews/my` | Bearer | Own reviews |
| `PUT` | `/api/reviews/:id` | Bearer | Edit own review |
| `DELETE` | `/api/reviews/:id` | Bearer | Delete own review |

### Upload — `/api/upload`
| Method | Route | Auth | Description |
| :--- | :--- | :---: | :--- |
| `POST` | `/api/upload` | Public | Multipart file → Cloudinary CDN |

---

## Environment Variables

Loaded from `backend/.env` or system environment:

| Variable | Default | Description |
| :--- | :--- | :--- |
| `JWT_SECRET` | *(required)* | ≥ 32-char high-entropy secret for HS256 signing |
| `JWT_ISSUER` | `myapp-api` | Token issuer claim |
| `MONGO_URI` | `mongodb://127.0.0.1:27017` | MongoDB connection string (supports `mongodb+srv://`) |
| `MONGO_DATABASE` | `myapp` | Target database name |
| `ACCESS_TOKEN_LIFETIME` | `900` | Access token TTL in seconds (15 min) |
| `REFRESH_TOKEN_LIFETIME` | `2592000` | Refresh token TTL in seconds (30 days) |
| `SECURE_COOKIES` | `false` | Set `Secure` flag on refresh cookie (enable for HTTPS) |
| `CLOUDINARY_CLOUD_NAME` | *(optional)* | Cloudinary cloud identifier |
| `CLOUDINARY_API_KEY` | *(optional)* | Cloudinary API key |
| `CLOUDINARY_API_SECRET` | *(optional)* | Cloudinary API secret for SHA-1 signatures |

---

## Build & Run

### Prerequisites
- Windows 10/11, Visual Studio 2022 (C++20 Desktop workload)
- [vcpkg](https://github.com/microsoft/vcpkg) at `C:/dev/vcpkg`
- MongoDB running locally or accessible via Atlas URI

### Build

```powershell
cd backend
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug
```

### Run

```powershell
$env:JWT_SECRET    = "your-secret"
$env:MONGO_URI     = "mongodb://127.0.0.1:27017"
$env:MONGO_DATABASE = "book_exchange"
.\build\Debug\BookExchangeBackend.exe
```

Server starts on `http://127.0.0.1:8080`.

---

## Related

- [Frontend README](../frontend/README.md) — Qt 6 desktop client
- [future.md](../future.md) — Production scaling roadmap