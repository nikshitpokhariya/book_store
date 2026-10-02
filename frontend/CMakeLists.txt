cmake_minimum_required(VERSION 3.16)

project(BookBazzar VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================
# Qt Components
# ==============================

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network)

# ==============================
# Source files
# ==============================

set(PROJECT_SOURCES
    src/main.cpp
    src/database.cpp
    src/loginwindow.cpp
    src/signupwindow.cpp
    src/dashboard.cpp
    src/homewindow.cpp
    src/browsewindow.cpp
    src/bookdetailswindow.cpp
    src/listingswindow.cpp
    src/orderswindow.cpp
    src/sellwindow.cpp
    src/sellbookwindow.cpp
    src/network/ApiClient.cpp
    src/auth/SessionManager.cpp
    src/windows/CartWindow.cpp
    src/windows/CheckoutWindow.cpp
    src/windows/OrderDetailWindow.cpp
    src/windows/ExchangeWindow.cpp

    include/database.h
    include/loginwindow.h
    include/signupwindow.h
    include/dashboard.h
    include/homewindow.h
    include/browsewindow.h
    include/bookdetailswindow.h
    include/listingswindow.h
    include/orderswindow.h
    include/sellwindow.h
    include/sellbookwindow.h
    include/network/ApiClient.h
    include/auth/SessionManager.h
    include/models/DataModels.h
    include/windows/CartWindow.h
    include/windows/CheckoutWindow.h
    include/windows/OrderDetailWindow.h
    include/windows/ExchangeWindow.h

    ui/loginwindow.ui
)

# ==============================
# Executable
# ==============================

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(BookBazzar
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(BookBazzar SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(BookBazzar
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# ==============================
# Include directories
# ==============================

target_include_directories(BookBazzar
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/network
        ${CMAKE_CURRENT_SOURCE_DIR}/include/auth
        ${CMAKE_CURRENT_SOURCE_DIR}/include/models
        ${CMAKE_CURRENT_SOURCE_DIR}/include/windows
        ${CMAKE_CURRENT_SOURCE_DIR}/src/network
        ${CMAKE_CURRENT_SOURCE_DIR}/src/auth
        ${CMAKE_CURRENT_SOURCE_DIR}/src/windows
)

# ==============================
# Link libraries
# ==============================

target_link_libraries(BookBazzar PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
)

# ==============================
# Bundle / install settings
# ==============================

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.BookBazzar)
endif()

set_target_properties(BookBazzar PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)

install(TARGETS BookBazzar
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(BookBazzar)
endif()

# ==============================
# Testing
# ==============================
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test QUIET)
if(Qt${QT_VERSION_MAJOR}Test_FOUND)
    enable_testing()
    add_executable(BookBazzarTests
        tests/main_test.cpp
        tests/test_datamodels.cpp
        tests/test_sessionmanager.cpp
        tests/test_validation.cpp
        tests/test_apiclient.cpp
        src/network/ApiClient.cpp
        src/auth/SessionManager.cpp
        include/network/ApiClient.h
        include/auth/SessionManager.h
    )
    target_include_directories(BookBazzarTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/network
        ${CMAKE_CURRENT_SOURCE_DIR}/include/auth
        ${CMAKE_CURRENT_SOURCE_DIR}/include/models
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    target_link_libraries(BookBazzarTests PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME BookBazzarTests COMMAND BookBazzarTests)

    add_executable(BookBazzarGuiSmokeTest
        tests/gui_smoke_test.cpp
        src/database.cpp
        src/loginwindow.cpp
        src/signupwindow.cpp
        src/dashboard.cpp
        src/homewindow.cpp
        src/browsewindow.cpp
        src/bookdetailswindow.cpp
        src/listingswindow.cpp
        src/orderswindow.cpp
        src/sellwindow.cpp
        src/sellbookwindow.cpp
        src/network/ApiClient.cpp
        src/auth/SessionManager.cpp
        src/windows/CartWindow.cpp
        src/windows/CheckoutWindow.cpp
        src/windows/OrderDetailWindow.cpp
        src/windows/ExchangeWindow.cpp
        include/database.h
        include/loginwindow.h
        include/signupwindow.h
        include/dashboard.h
        include/homewindow.h
        include/browsewindow.h
        include/bookdetailswindow.h
        include/listingswindow.h
        include/orderswindow.h
        include/sellwindow.h
        include/sellbookwindow.h
        include/network/ApiClient.h
        include/auth/SessionManager.h
        include/models/DataModels.h
        include/windows/CartWindow.h
        include/windows/CheckoutWindow.h
        include/windows/OrderDetailWindow.h
        include/windows/ExchangeWindow.h
        ui/loginwindow.ui
    )
    target_include_directories(BookBazzarGuiSmokeTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/network
        ${CMAKE_CURRENT_SOURCE_DIR}/include/auth
        ${CMAKE_CURRENT_SOURCE_DIR}/include/models
        ${CMAKE_CURRENT_SOURCE_DIR}/include/windows
        ${CMAKE_CURRENT_SOURCE_DIR}/src/network
        ${CMAKE_CURRENT_SOURCE_DIR}/src/auth
        ${CMAKE_CURRENT_SOURCE_DIR}/src/windows
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    target_link_libraries(BookBazzarGuiSmokeTest PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Sql
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME BookBazzarGuiSmokeTest COMMAND BookBazzarGuiSmokeTest)
endif()