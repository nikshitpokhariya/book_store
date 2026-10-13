# BookBazzar Windows Installer (Inno Setup)

This directory contains the Inno Setup 6 packaging configuration for the **BookBazzar** Qt 6 desktop application.

## Specifications
- **Application Name**: `BookBazzar`
- **Application Version**: `0.1.0`
- **Architecture**: Windows x64 (`x64compatible`)
- **Default Installation Path**: `{autopf}\BookBazzar` (resolves to `C:\Program Files\BookBazzar` for all-users / admin installations)
- **Output Setup Executable**: `frontend/dist/BookBazzar-Setup-0.1.0.exe`
- **Compression**: Solid `lzma2/ultra64`

## Packaged Content & Dependencies
The installer packages the pre-deployed release distribution located in `frontend/dist/BookBazzar/`:
- **Main Executable**: `BookBazzar.exe`
- **Qt 6 Libraries**: `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, `Qt6Network.dll`, `Qt6Svg.dll`
- **Qt 6 Plugins**:
  - `platforms/qwindows.dll` (Core Windows platform integration)
  - `styles/qmodernwindowsstyle.dll` (Windows UI styling)
  - `tls/qschannelbackend.dll`, `tls/qcertonlybackend.dll` (Native Windows SSL/TLS network transport)
  - `imageformats/qjpeg.dll`, `qpng.dll`, `qgif.dll`, `qico.dll`, `qsvg.dll` (Image decoders for covers)
  - `iconengines/qsvgicon.dll` (SVG UI icons)
  - `networkinformation/qnetworklistmanager.dll`
  - `generic/qtuiotouchplugin.dll`
  - `translations/qt_*.qm` (Qt internationalization)
- **Graphics & DirectX**: `d3dcompiler_47.dll`, `dxcompiler.dll`, `dxil.dll`, `opengl32sw.dll`
- **Visual C++ Runtime**: `vcruntime140.dll`, `msvcp140.dll`, `concrt140.dll`, `vccorlib140.dll`
- **Prerequisite Bootstrapper**: `vc_redist.x64.exe` (run automatically only if runtime registry check indicates the MSVC 2015-2022 v14.x redistributable is missing on the target host)

## Excluded Items
To prevent leakage and keep distribution lightweight:
- Source code (`*.cpp`, `*.h`, `CMakeLists.txt`)
- Environment & secret files (`.env`, `.env.*`)
- Build and intermediate artifacts (`*.obj`, `*.pdb`, `*.ilk`, `*.exp`, `*.lib`)
- Version control metadata (`.git`, `.gitignore`)

## How to Compile the Installer

### Step 1: Install Inno Setup 6
If Inno Setup is not yet installed on your system:
- **Via winget**:
  ```cmd
  winget install --id JRSoftware.InnoSetup -e --source winget
  ```
- **Via Official Website**:
  Download and install from: [https://jrsoftware.org/isdl.php](https://jrsoftware.org/isdl.php)

### Step 2: Build the Installer
Run the automated build script from the project root:

- **PowerShell**:
  ```powershell
  & ".\frontend\installer\build_installer.ps1"
  ```
- **Command Prompt / Double Click**:
  ```cmd
  frontend\installer\build_installer.bat
  ```
- **Direct Inno Setup Compiler**:
  ```cmd
  "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" "D:\1Hello-World\1pbl-oops\frontend\installer\BookBazzar.iss"
  ```

Upon completion, `frontend/dist/BookBazzar-Setup-0.1.0.exe` will be generated alongside the existing `BookBazzar-v0.1-Windows-x64.zip` release archive.
