# ========================================================
# Release-Only Triplet for Linux x64
# Avoids building Debug variants of dependencies to cut
# peak RAM usage and build time by ~50% on Render.
# ========================================================
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_BUILD_TYPE release)
