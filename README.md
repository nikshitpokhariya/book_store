# BookBazzar — Peer-to-Peer Book Marketplace & Barter Exchange Platform

[![C++20 Backend](https://img.shields.io/badge/C%2B%2B-20%20Backend-blue.svg?style=flat&logo=c%2B%2B)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![C++17 Frontend](https://img.shields.io/badge/C%2B%2B-17%20Frontend-blue.svg?style=flat&logo=c%2B%2B)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Qt 6](https://img.shields.io/badge/Qt-6.12.0-green.svg?style=flat&logo=qt)](https://www.qt.io/)
[![Drogon](https://img.shields.io/badge/Drogon-1.9.x-orange.svg?style=flat)](https://github.com/drogonframework/drogon)
[![MongoDB Atlas](https://img.shields.io/badge/MongoDB-Atlas%208.0-brightgreen.svg?style=flat&logo=mongodb)](https://www.mongodb.com/atlas)
[![Cloudinary](https://img.shields.io/badge/Cloudinary-CDN-blueviolet.svg?style=flat)](https://cloudinary.com/)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-informational.svg?style=flat&logo=cmake)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/badge/Release-v0.1.0-brightgreen.svg?style=flat)](https://github.com/nikshitpokhariya/book_store/releases/tag/v0.1.0)

A full-stack C++ application enabling college students to buy, sell, and directly barter pre-owned physical books — with cloud media, atomic transactions, and a native desktop GUI.

> **Scope:** Payment and shipping are simulated for demonstration purposes. No real financial transactions or courier integrations are active.

---

## Download

**[BookBazzar-Setup-0.1.0.exe](https://github.com/nikshitpokhariya/book_store/releases/download/v0.1.0/BookBazzar-Setup-0.1.0.exe)** — Windows x64 installer (v0.1.0)

> **Windows Defender SmartScreen notice:** Because this installer is newly published and does not yet have an established reputation, Windows may display a SmartScreen warning when you run it. This is standard behavior for newly released software without an extended-validation code-signing certificate. To proceed, click **More info → Run anyway** if you trust the source. Do not disable Windows Defender or other security protections on your system.

---

## Overview

BookBazzar is a monorepo with two coupled components:

| Component | Description |
| :--- | :--- |
| [`backend/`](backend/README.md) | C++20 Drogon REST API — auth, listings, cart, checkout, orders, exchanges, reviews, media upload |
| [`frontend/`](frontend/README.md) | Qt 6 desktop client — dark-mode GUI, async networking, Cloudinary integration |

**Core features:**

- **Marketplace** — browse, search, filter, and purchase single-copy pre-owned books; atomic checkout prevents double-buys
- **Barter Exchange** — propose and atomically execute zero-money peer-to-peer book swaps
- **Order Lifecycle** — `PLACED → DELIVERED` tracking, pre-shipment cancellation, post-delivery returns
- **Reviews** — verified purchase reviews gated behind `DELIVERED` status
- **Security** — Argon2id hashing, dual-token JWT/refresh auth, IDOR-protected routes

---

## Architecture

```mermaid
flowchart TD
    subgraph Client["Qt 6 Desktop Client (C++17)"]
        UI["Windows: Home · Browse · Cart · Checkout · Exchange · Orders · Sell"]
        SM["SessionManager — JWT & user state"]
        AC["ApiClient — QNetworkAccessManager"]
        IL["ImageLoader — async pixmap cache"]
        UI --> SM & AC & IL
        SM --> AC
    end
    subgraph Backend["Drogon C++20 REST API — :8080"]
        ROUTER["HTTP Router"]
        AUTH["AuthFilter — JWT guard"]
        CTRL["Controllers: Auth · Books · Cart · Orders · Exchanges · Reviews · Upload"]
        SRV["Business Services & State Machines"]
        REPO["Repositories — mongocxx / bsoncxx"]
        ROUTER --> AUTH --> CTRL --> SRV --> REPO
    end
    subgraph Cloud["Cloud Infrastructure"]
        ATLAS[("MongoDB Atlas 8.0 — Replica Set")]
        CDN["Cloudinary CDN — Signed Image & Video"]
        REPO --> ATLAS
        CTRL --> CDN
        IL -. cached .-> CDN
    end
    AC -- "HTTP/REST · JSON · Bearer JWT · Multipart" --> ROUTER
```

---

## Repository Structure

```
.
├── backend/                    # C++20 Drogon REST API
│   ├── CMakeLists.txt          # vcpkg-integrated build
│   ├── vcpkg.json              # drogon, mongocxx, argon2, jwt-cpp, jsoncpp
│   ├── config.json             # Drogon listener & thread config
│   ├── .env.example            # Required environment variables
│   ├── include/
│   │   ├── config/             # AppConfig — env loader singleton
│   │   ├── db/                 # MongoDatabase — client & index manager
│   │   ├── filters/            # AuthFilter — JWT middleware
│   │   ├── security/           # Argon2id · JWT · PaymentService
│   │   ├── models/             # Book, User, Cart, Order, Exchange, Review
│   │   ├── repositories/       # mongocxx data access layer
│   │   ├── services/           # Business logic & orchestration
│   │   └── controllers/        # Drogon HTTP controllers
│   ├── src/                    # Implementations
│   └── tests/                  # PowerShell integration suites & Postman collection
│
├── frontend/                   # Qt 6 Desktop Client (C++17)
│   ├── CMakeLists.txt          # Qt6 + AUTOMOC/AUTOUIC build
│   ├── include/                # Headers: auth, common, models, network, windows
│   ├── src/                    # Implementations: auth, common, network, windows
│   ├── installer/              # Inno Setup 6 packaging scripts & assets
│   ├── tests/                  # Qt Test suites
│   └── ui/                     # Qt Designer .ui forms
│
├── future.md                   # Production scaling roadmap
├── start_backend.bat           # One-click backend launcher
└── README.md
```

---

## Tech Stack

| Layer | Technology | Version |
| :--- | :--- | :---: |
| Backend language | C++ | ISO C++20 |
| Frontend language | C++ | ISO C++17 |
| HTTP framework | [Drogon](https://github.com/drogonframework/drogon) | 1.9.x |
| Database | MongoDB Atlas — mongocxx | 8.0 |
| Media CDN | Cloudinary | REST v1.1 |
| Desktop client | Qt Widgets | 6.12.0 |
| Password hashing | Argon2id + OpenSSL | 3.x |
| Auth tokens | jwt-cpp — HS256 + SHA-256 refresh | 0.7.x |
| Serialization | JsonCpp / QJsonDocument | 1.9.x / Qt 6 |
| Package manager | vcpkg manifest mode | — |
| Build system | CMake + Ninja | ≥ 3.16 (frontend) / ≥ 3.20 (backend) |

---

## Quick Start

### 1. Start the Backend

```powershell
# Recommended — sets all required env vars automatically
.\start_backend.bat
```

Or manually from `backend/`:

```powershell
$env:JWT_SECRET            = "your-high-entropy-secret"
$env:MONGO_URI             = "mongodb+srv://..."      # or mongodb://127.0.0.1:27017
$env:MONGO_DATABASE        = "book_exchange"
$env:CLOUDINARY_CLOUD_NAME = "your-cloud-name"
$env:CLOUDINARY_API_KEY    = "your-api-key"
$env:CLOUDINARY_API_SECRET = "your-api-secret"
.\build\Debug\BookExchangeBackend.exe
```

### 2. Start the Frontend

**Qt Creator (recommended):** Open `frontend/CMakeLists.txt` → select Qt 6 kit → `Ctrl+R`

**Terminal:**
```cmd
set PATH=C:\Qt\6.12.0\msvc2022_64\bin;%PATH%
.\frontend\build\Desktop_Qt_6_12_0_MSVC2022_64bit_Debug\BookBazzar.exe
```

---

## Documentation

| File | Contents |
| :--- | :--- |
| [backend/README.md](backend/README.md) | API reference, database schema, env config, build guide |
| [frontend/README.md](frontend/README.md) | UI architecture, window catalog, build & run instructions |
| [frontend/installer/README.md](frontend/installer/README.md) | Inno Setup 6 installer packaging guide |
| [future.md](future.md) | Production scaling roadmap — payments, logistics, WebSockets, mobile |

---

## License

MIT — see [LICENSE](LICENSE).