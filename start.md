# 🚀 BookBazzar Startup & Execution Guide

This guide walks you through manually starting all services to run the **BookBazzar** project on your Windows machine with **Qt Creator**.

---

## 📋 System Architecture & Startup Order

To run the application properly, the components must be started in the following order:

```text
1. MongoDB Database (:27017)
          ⬇
2. C++ Drogon REST Backend (:8080)
          ⬇
3. Qt Creator Desktop Frontend (BookBazzar GUI)
```

---

## ⚡ Option A: Quick Start (Using 1-Click Batch Files)

We have created two batch scripts in the project root for fast startup:

1. **Start MongoDB**:
   - Double-click [`start_mongo.bat`](file:///d:/1Hello-World/1pbl-oops/start_mongo.bat)  
   - *Keep this terminal window open.*

2. **Start Backend Server**:
   - Double-click [`start_backend.bat`](file:///d:/1Hello-World/1pbl-oops/start_backend.bat)  
   - *Keep this terminal window open.*

3. **Start Qt Creator & Run Frontend**:
   - Open Qt Creator ➡️ Open `frontend/CMakeLists.txt` ➡️ Press `Ctrl + R` (or the green **Run** button).

---

## 🛠️ Option B: Manual Terminal Startup (Step-by-Step)

If you prefer to start each component manually via PowerShell or Command Prompt, follow these exact steps:

### Step 1: Start MongoDB

MongoDB must be running on `127.0.0.1:27017` using the project's [`mongodb_data`](file:///d:/1Hello-World/1pbl-oops/mongodb_data) folder:

Open a new **PowerShell** window in the project root (`d:\1Hello-World\1pbl-oops`) and run:

```powershell
& "C:\Program Files\MongoDB\Server\8.3\bin\mongod.exe" --dbpath "mongodb_data" --bind_ip 127.0.0.1 --port 27017
```

> 💡 **Note**: Leave this terminal running in the background. It will log incoming database connections.

---

### Step 2: Start the Drogon Backend Server

The backend REST API listens on `http://127.0.0.1:8080`.

Open a **second PowerShell** window and run:

```powershell
# 1. Change to the backend directory (important: config.json is loaded from here)
cd d:\1Hello-World\1pbl-oops\backend

# 2. Run the start script
.\start_server.ps1
```

*(Alternatively, run the compiled binary directly from the backend folder):*

```powershell
cd d:\1Hello-World\1pbl-oops\backend
$env:JWT_SECRET = "WhWRp7LUnP+BUdQaOxcjpf67hKIBcnk3WAqTrTbzfjOy+CWOdXlgIqsqGTCTHYu7"
$env:MONGO_URI = "mongodb://127.0.0.1:27017"
$env:MONGO_DATABASE = "myapp"
.\build\Debug\BookExchangeBackend.exe
```

When started successfully, you will see:
```text
========================================
 BookExchangeBackend Starting
 Port: 8080
 Mongo URI: mongodb://127.0.0.1:27017
 Database: myapp
========================================
```

#### ✅ Verify Backend Is Responding
You can test that the backend is alive by opening a third terminal or your browser:
- **Browser**: Visit [http://127.0.0.1:8080/api/books](http://127.0.0.1:8080/api/books)
- **PowerShell**:
  ```powershell
  Invoke-RestMethod http://127.0.0.1:8080/api/books
  ```
  *(Should return `{ "success": true, "data": [...] }`)*

---

### Step 3: Run the Desktop GUI in Qt Creator

Now that the backend is active, launch the Qt frontend interface:

1. Launch **Qt Creator**.
2. Click **File** ➡️ **Open File or Project...** (or press `Ctrl + O`).
3. Select [`frontend/CMakeLists.txt`](file:///d:/1Hello-World/1pbl-oops/frontend/CMakeLists.txt).
4. When prompted to configure the project, choose your kit (e.g., **Desktop Qt 6.x MSVC 2022 64bit**) and click **Configure Project**.
5. Once loaded, click the green **Run** arrow button (or press `Ctrl + R`).
6. The **BookBazzar** client window will appear and connect to `http://127.0.0.1:8080`.

#### Or Run the Already Compiled Executable Directly:
If you already built the frontend in Qt Creator earlier, you can launch it immediately without opening the editor:
```powershell
& "d:\1Hello-World\1pbl-oops\frontend\build\Desktop_Qt_6_12_0_MSVC2022_64bit_Debug\BookBazzar.exe"
```

---

## 🔧 Rebuilding the Backend (If you modify C++ backend code)

If you modify any files in `backend/`:

```powershell
cd d:\1Hello-World\1pbl-oops\backend
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug
```

---

## ⚠️ Common Troubleshooting

| Issue | Cause | Solution |
| :--- | :--- | :--- |
| **"Failed to connect to MongoDB"** | `mongod.exe` is not running. | Run [`start_mongo.bat`](file:///d:/1Hello-World/1pbl-oops/start_mongo.bat) or execute Step 1 first. |
| **"Address already in use (8080)"** | A previous instance of the backend is still running. | Run `Stop-Process -Name BookExchangeBackend -Force` in PowerShell, then re-launch. |
| **"Cannot open config.json"** | Working directory is not `backend/`. | Ensure you `cd backend` before launching `BookExchangeBackend.exe`. |
| **"Network Error / Connection Refused in Qt GUI"** | Backend server is not running on port 8080. | Launch `start_backend.bat` before performing actions in the GUI. |

---

## 📁 Key File Locations Reference

- Project Root: [`d:\1Hello-World\1pbl-oops\`](file:///d:/1Hello-World/1pbl-oops/)
- MongoDB Starter: [`start_mongo.bat`](file:///d:/1Hello-World/1pbl-oops/start_mongo.bat)
- Backend Starter: [`start_backend.bat`](file:///d:/1Hello-World/1pbl-oops/start_backend.bat)
- Backend Source & Config: [`backend/`](file:///d:/1Hello-World/1pbl-oops/backend/)
- Frontend Qt Project: [`frontend/CMakeLists.txt`](file:///d:/1Hello-World/1pbl-oops/frontend/CMakeLists.txt)
- Main Documentation: [`README.md`](file:///d:/1Hello-World/1pbl-oops/README.md)
