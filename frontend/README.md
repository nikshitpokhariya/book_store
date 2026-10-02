# BookBazzar — Qt6 Desktop Frontend

A feature-rich C++17 desktop application built with **Qt6 Widgets** and **SQLite**, providing a complete peer-to-peer book marketplace experience for Windows. Users can sign up, browse books, list their own books for sale, place orders, and manage their inventory — all through a polished, locally-running graphical interface.

> [!NOTE]
> This frontend uses a **local SQLite database** (`bookbazzar.db`) stored in the system's App Data directory. It is an **independent, self-contained desktop application** and does not connect to the Drogon REST backend by default. See [backend README](../backend/README.md) for the REST API server.

---

## 1. Project Overview

**BookBazzar** is a Qt6 desktop application designed as the frontend layer of a college PBL (Project-Based Learning) book exchange marketplace. It provides:

- **User Authentication**: Local signup and login with a persistent SQLite user table.
- **Book Browsing & Discovery**: A rich home screen with recently listed books, search, and category filtering.
- **Selling Books**: A form-driven listing interface allowing sellers to upload a book cover image, set price, condition, ISBN, and category.
- **Order Management**: Buyers can place orders on available books; sellers manage their active listings.
- **Full UI Flow**: A complete multi-window navigation flow from Login → Home → Browse → Book Details → Order Placed / Listings Management.

---

## 2. Tech Stack

| Component | Technology | Version | Purpose |
| :--- | :--- | :--- | :--- |
| **Language** | C++ | ISO C++17 | Application logic |
| **UI Framework** | Qt Widgets | Qt 5 / Qt 6 | Window management, layouts, signals & slots |
| **Database** | SQLite (via Qt SQL) | 3.x | Local persistent storage for users, books, and orders |
| **Build System** | CMake | ≥ 3.16 | Cross-platform build generation with AUTOUIC/AUTOMOC |
| **UI Designer** | Qt Designer | Qt 5/6 | `.ui` file for Login window layout (`loginwindow.ui`) |

---

## 3. Project Structure

```
frontend/
├── CMakeLists.txt              # Qt6 CMake build configuration
│
├── src/                        # All C++ implementation files
│   ├── main.cpp                # Application entry point — initializes DB, shows LoginWindow
│   ├── database.cpp            # SQLite singleton — schema creation, all DB operations
│   ├── loginwindow.cpp         # Login screen with email/password authentication
│   ├── signupwindow.cpp        # Account registration with email validation
│   ├── dashboard.cpp           # Post-login navigation hub (legacy entry point)
│   ├── homewindow.cpp          # Main home screen — hero, categories, recent books
│   ├── browsewindow.cpp        # Full book catalogue with search & category filters
│   ├── bookdetailswindow.cpp   # Individual book details view with Buy/Order button
│   ├── listingswindow.cpp      # Seller's active listing management with delete
│   ├── orderswindow.cpp        # Buyer's order history and spend summary
│   ├── sellwindow.cpp          # Quick sell form (title, author, price, image)
│   └── sellbookwindow.cpp      # Extended book listing form (ISBN, edition, category)
│
├── include/                    # All C++ header files
│   ├── database.h              # Database class declaration & Book struct
│   ├── loginwindow.h           # LoginWindow class
│   ├── signupwindow.h          # SignupWindow class
│   ├── dashboard.h             # Dashboard class
│   ├── homewindow.h            # HomeWindow class
│   ├── browsewindow.h          # BrowseWindow class
│   ├── bookdetailswindow.h     # BookDetailsWindow class
│   ├── listingswindow.h        # ListingsWindow class
│   ├── orderswindow.h          # OrdersWindow class
│   ├── sellwindow.h            # SellWindow class
│   └── sellbookwindow.h        # SellBookWindow class
│
└── ui/
    └── loginwindow.ui          # Qt Designer layout file for the login window
```

---

## 4. Application Windows & Navigation Flow

```
main.cpp
  └── LoginWindow
        ├── [Login Success] ──────────────────► HomeWindow
        │                                           ├── [Browse Books] ──► BrowseWindow
        │                                           │                          └── [Book Card] ──► BookDetailsWindow
        │                                           │                                                  └── [Order] ──► Order placed
        │                                           ├── [My Listings] ───► ListingsWindow
        │                                           │                          └── [Add New] ──► SellBookWindow
        │                                           ├── [Sell a Book] ───► SellWindow
        │                                           ├── [My Orders] ────► OrdersWindow
        │                                           └── [Logout] ────────► Back to LoginWindow
        │
        └── [Sign Up] ──────────────────────────► SignupWindow
                                                      └── [Success] ──► Back to LoginWindow
```

---

## 5. Windows Reference

### `LoginWindow` — Entry Point
- **File**: [`src/loginwindow.cpp`](src/loginwindow.cpp) / [`include/loginwindow.h`](include/loginwindow.h)
- Split-panel layout (branding left, form right).
- Authenticates users against the local `users` SQLite table.
- Launches `HomeWindow` on success; links to `SignupWindow` for new accounts.
- Window size: `950×600` (minimum `800×500`).

### `SignupWindow` — Account Registration
- **File**: [`src/signupwindow.cpp`](src/signupwindow.cpp) / [`include/signupwindow.h`](include/signupwindow.h)
- Collects full name, email, phone, password, and confirm password.
- Validates email format with `QRegularExpression`.
- Minimum password length: 6 characters.
- Checks for duplicate email before inserting.

### `HomeWindow` — Main Application Hub
- **File**: [`src/homewindow.cpp`](src/homewindow.cpp) / [`include/homewindow.h`](include/homewindow.h)
- Window size: `1400×850` (minimum `1000×650`).
- Composed of named section widgets:
  - **Navigation Bar**: App name, search bar, nav links (Browse, Sell, Listings, Orders), logout.
  - **Hero Section**: Branded hero banner with CTA buttons.
  - **Category Section**: Clickable category tiles (Fiction, Science, Technology, History…).
  - **Recently Listed Section**: Dynamic book cards loaded from SQLite, with generated cover art fallback.
  - **Sell Section**: Promotional panel linking to the sell form.
  - **Why Section**: Feature highlights panel.
  - **Footer**: App credits and links.
- Book covers: Loads real images from disk path; if missing, generates a professional-looking color-gradient placeholder cover using `QPainter`.
- Clicking a book card opens `BookDetailsWindow`.

### `BrowseWindow` — Book Catalogue
- **File**: [`src/browsewindow.cpp`](src/browsewindow.cpp) / [`include/browsewindow.h`](include/browsewindow.h)
- Displays all `Available` books in a responsive grid layout.
- **Search**: Text search across title, author, and ISBN.
- **Category Filter**: Dropdown (`QComboBox`) to filter by book category.
- Each card shows cover image, title, author, condition, price, and location.
- Emits `bookSelected(int bookId)` signal to launch `BookDetailsWindow`.

### `BookDetailsWindow` — Book Details & Purchase
- **File**: [`src/bookdetailswindow.cpp`](src/bookdetailswindow.cpp) / [`include/bookdetailswindow.h`](include/bookdetailswindow.h)
- Loads full book metadata (title, author, ISBN, category, condition, price, description, seller, location) from SQLite.
- Displays cover image or generated placeholder.
- **Buy/Order Button**: Creates an entry in the `orders` table; updates book `status` to `Sold`.
- Seller name is shown as attribution.

### `ListingsWindow` — Seller's Inventory
- **File**: [`src/listingswindow.cpp`](src/listingswindow.cpp) / [`include/listingswindow.h`](include/listingswindow.h)
- Displays all books listed by the current logged-in user.
- Stats bar shows: Total listings, Available count, Sold count.
- Each card has a **Delete** button; confirms before removing from DB.
- **Add New Listing** button launches `SellBookWindow`.
- `refreshListings()` can be called from parent to reload after changes.

### `OrdersWindow` — Buyer's Purchase History
- **File**: [`src/orderswindow.cpp`](src/orderswindow.cpp) / [`include/orderswindow.h`](include/orderswindow.h)
- Shows all orders placed by the current user with book title, author, price, and date.
- Stats bar: Total orders placed, total amount spent.
- **Browse More Books** button links back to `BrowseWindow`.

### `SellWindow` — Quick Sell Form
- **File**: [`src/sellwindow.cpp`](src/sellwindow.cpp) / [`include/sellwindow.h`](include/sellwindow.h)
- Collects: title, author, ISBN, category (combo), condition (combo), price, description, location.
- **Image Upload**: File dialog to choose a cover image; copies it to the app's local data folder for persistence.
- Inserts a new row into the `books` SQLite table on submit.
- Emits `bookAdded()` on success so the home screen can refresh its recent books list.

### `SellBookWindow` — Extended Listing Form
- **File**: [`src/sellbookwindow.cpp`](src/sellbookwindow.cpp) / [`include/sellbookwindow.h`](include/sellbookwindow.h)
- Richer listing form with: title, author, ISBN, edition, category, condition, price (`QDoubleSpinBox`), location, and description.
- Image selection with preview.
- Emits `bookPublished()` signal on success.

### `Dashboard` — Legacy Navigation Hub
- **File**: [`src/dashboard.cpp`](src/dashboard.cpp) / [`include/dashboard.h`](include/dashboard.h)
- Early-iteration navigation panel (Buy, Sell, Profile, Logout buttons).
- Maintained for reference; main flow now uses `HomeWindow`.

---

## 6. Database Schema (SQLite)

The database file is stored at:
```
Windows: %APPDATA%\BookBazzar\bookbazzar.db
```

Automatically created on first launch by `Database::initialize()`. Foreign keys are enforced (`PRAGMA foreign_keys = ON`).

### `users` table
| Column | Type | Constraint | Description |
| :--- | :--- | :--- | :--- |
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Unique user ID |
| `name` | TEXT | NOT NULL | Full display name |
| `email` | TEXT | NOT NULL UNIQUE | Login identifier |
| `phone` | TEXT | — | Optional phone number |
| `password` | TEXT | NOT NULL | Stored password (plain-text; see note below) |
| `created_at` | DATETIME | DEFAULT CURRENT_TIMESTAMP | Registration timestamp |

> [!WARNING]
> Passwords are currently stored as plain text in the local SQLite database. This is acceptable for a PBL offline demo but should use `QCryptographicHash` (SHA-256) or Qt-compatible Argon2 before any networked deployment.

### `books` table
| Column | Type | Constraint | Description |
| :--- | :--- | :--- | :--- |
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Unique book listing ID |
| `seller_name` | TEXT | NOT NULL | Seller's display name |
| `title` | TEXT | NOT NULL | Book title |
| `author` | TEXT | NOT NULL | Author name |
| `isbn` | TEXT | — | Optional ISBN |
| `category` | TEXT | NOT NULL | Book genre/category |
| `condition` | TEXT | NOT NULL | `New`, `Like New`, `Good`, `Acceptable` |
| `price` | REAL | NOT NULL | Listing price (₹) |
| `description` | TEXT | — | Seller's description |
| `location` | TEXT | — | Seller's city/area |
| `image_path` | TEXT | — | Absolute path to local cover image |
| `status` | TEXT | DEFAULT `'Available'` | `Available` or `Sold` |
| `created_at` | DATETIME | DEFAULT CURRENT_TIMESTAMP | Listing timestamp |

### `orders` table
| Column | Type | Constraint | Description |
| :--- | :--- | :--- | :--- |
| `id` | INTEGER | PRIMARY KEY AUTOINCREMENT | Unique order ID |
| `book_id` | INTEGER | FK → `books.id` | Reference to purchased book |
| `buyer_name` | TEXT | NOT NULL | Buyer's display name |
| `order_date` | DATETIME | DEFAULT CURRENT_TIMESTAMP | Order placement time |
| `status` | TEXT | DEFAULT `'Pending'` | Order status |

---

## 7. Build & Run Instructions

### Prerequisites
- Windows 10/11
- **Qt 5.15+ or Qt 6.x** with the **Widgets** and **Sql** modules installed
- **Qt Creator** (recommended IDE) or CMake ≥ 3.16 with a C++17 compiler (MSVC 2019+ or MinGW)
- Qt SQLite driver (`QSQLITE`) — bundled with standard Qt distributions

### Option A: Open in Qt Creator (Easiest)
1. Open Qt Creator.
2. File → Open Project → select `frontend/CMakeLists.txt`.
3. Configure the kit (Qt 6.x, Desktop).
4. Click the green **Run** button (▶).

### Option B: CMake Command Line
```powershell
cd frontend

# Configure
cmake -B build -S . -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64"

# Build
cmake --build build --config Release

# Run
.\build\Release\BookBazzar.exe
```

> Replace `C:/Qt/6.x.x/msvc2019_64` with your actual Qt installation path.

---

## 8. Key Design Decisions

| Decision | Rationale |
| :--- | :--- |
| **SQLite via Qt SQL** | Zero-configuration local persistence; no external DB server needed for a desktop PBL app |
| **Singleton `Database` class** | One shared connection throughout app lifetime, avoids multi-connection overhead |
| **`QStandardPaths::AppDataLocation`** | Cross-platform path for user data; avoids writing to the installation directory |
| **Dynamically generated book covers** | If no image is provided, `QPainter` renders a professional gradient placeholder, preventing broken image icons |
| **Signal/Slot window navigation** | Windows communicate via Qt signals (`backRequested`, `bookAdded`, etc.) rather than raw pointers, maintaining loose coupling |
| **`refreshListings()` / `refreshRecentBooks()`** | Parent windows call these after child operations to keep UI in sync without full restart |

---

## 9. Current Implementation Status

### Completed ✅
- [x] SQLite database auto-initialization with `users`, `books`, and `orders` tables.
- [x] User registration with duplicate email detection and regex validation.
- [x] User login with credential verification against local database.
- [x] Home screen with hero, categories, recently listed books, and footer sections.
- [x] Dynamic book cards with real cover images or auto-generated gradient placeholder covers.
- [x] Browse window with live search (title/author/ISBN) and category filtering.
- [x] Book details view with full metadata display and order placement.
- [x] Seller listings management with stats bar and per-listing delete.
- [x] Buyer order history with total spend summary.
- [x] Two sell forms (`SellWindow` and `SellBookWindow`) with image picker and file copy.
- [x] Multi-window Qt signal/slot navigation flow.

### Known Limitations & Future Improvements 🔧
1. **Plain-text passwords**: Should be hashed with SHA-256 or Argon2 before any non-local deployment.
2. **No real-time backend sync**: Currently standalone SQLite only. Could be extended to call the Drogon REST backend (`backend/`) via `QNetworkAccessManager`.
3. **No exchange or review UI**: The Qt frontend does not yet implement the P2P book exchange or review/rating features that exist in the REST backend.
4. **Single-table orders**: The `orders` table is simplified. Could be expanded with delivery status tracking matching the backend's order lifecycle.
5. **Image storage**: Book images are copied to `%APPDATA%\BookBazzar\images\`. A future version could upload to a cloud object store.
