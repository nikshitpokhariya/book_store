cmake_minimum_required(VERSION 3.20)

project(BookExchangeBackend)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================
# Dependencies
# ==============================

find_package(Drogon CONFIG REQUIRED)

find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

find_package(OpenSSL REQUIRED)

find_package(jwt-cpp CONFIG REQUIRED)

find_package(JsonCpp CONFIG REQUIRED)

find_library(ARGON2_LIBRARY argon2 REQUIRED)

# ==============================
# Source files
# ==============================

file(GLOB_RECURSE SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

# ==============================
# Executable
# ==============================

add_executable(${PROJECT_NAME}
    ${SOURCES}
)

# ==============================
# Project headers
# ==============================

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

# ==============================
# Compile definitions
# ==============================

# On Windows, prevent <windows.h> from defining min/max macros
# which corrupt bsoncxx, mongocxx, and bcrypt.h headers.
if(WIN32)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
    )
endif()

# ==============================
# Libraries
# ==============================

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Drogon::Drogon

        mongo::mongocxx_shared
        mongo::bsoncxx_shared

        OpenSSL::Crypto

        jwt-cpp::jwt-cpp

        JsonCpp::JsonCpp

        ${ARGON2_LIBRARY}
)