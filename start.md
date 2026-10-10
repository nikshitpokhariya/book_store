# 🚀 BookBazzar Startup & Execution Guide

This guide walks you through starting all services to run the **BookBazzar** project with **MongoDB Atlas Cloud Database** and **Cloudinary CDN Media Storage**.

---

## 📋 System Architecture & Cloud Flow

```text
1. MongoDB Atlas Cloud Database (mongodb+srv://) & Cloudinary CDN
                     ⬇
2. C++ Drogon REST Backend (:8080)
                     ⬇
3. Qt Desktop Frontend (BookBazzar GUI)
```

---

## ⚡ Option A: Quick Start (1-Click Startup)

1. **Start Backend Server**:
   - Double-click [`start_backend.bat`](file:///start_backend.bat)  
   - Connects directly to **MongoDB Atlas** (`myapp` cluster) and enables Cloudinary uploads.
   - *Keep this terminal window open.*

2. **Start Qt Creator & Run Frontend**:
   - Open Qt Creator ➡️ Open `frontend/CMakeLists.txt` ➡️ Press `Ctrl + R` (or the green **Run** button).
   - Alternatively, build using Ninja and Qt 6 MinGW:
     ```powershell
     cmake -S frontend -B frontend/build -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.11.2/mingw_64"
     cmake --build frontend/build --target BookBazzar
     .\frontend\build\BookBazzar.exe
     ```

---

## 🛠️ Verification Scripts

Two automated verification scripts are included in the repository root:

1. **Verify MongoDB Atlas Connectivity**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\test_atlas_connection.ps1
   ```
   Checks DNS SRV shard resolution, TCP connectivity on port 27017, and backend database CRUD operations.

2. **Verify Cloudinary Media Upload & CDN Delivery**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\test_cloudinary_upload.ps1
   ```
   Tests signed multipart uploads to Cloudinary and verifies HTTP 200 delivery from the Cloudinary CDN.

---

## ⚠️ Common Troubleshooting

| Issue | Cause | Solution |
| :--- | :--- | :--- |
| **"Failed to connect to MongoDB Atlas"** | DNS SRV resolution or outbound port 27017 blocked. | Run `.\test_atlas_connection.ps1` to diagnose network connection. |
| **"Address already in use (8080)"** | A previous instance of the backend is still running. | Run `Stop-Process -Name BookExchangeBackend -Force` in PowerShell, then re-launch. |
| **"Cannot open config.json"** | Working directory is not `backend/`. | Launch via `start_backend.bat` or `cd backend` first. |
| **"Network Error in Qt GUI"** | Backend server is not running on port 8080. | Launch `start_backend.bat` before performing actions in the GUI. |
