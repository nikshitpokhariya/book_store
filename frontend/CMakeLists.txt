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

set(APP_COMMON_SOURCES
    src/network/ApiClient.cpp
    src/auth/SessionManager.cpp
    src/common/AppStyle.cpp
    src/common/ImageLoader.cpp
    src/common/StyledMessageBox.cpp
    src/common/CancelOrderDialog.cpp
    src/common/ProposeExchangeDialog.cpp
    src/common/VideoPlayerWidget.cpp
    src/common/VideoThumbnailHelper.cpp

    src/windows/bookdetailswindow.cpp
    src/windows/browsewindow.cpp
    src/windows/CartWindow.cpp
    src/windows/CheckoutWindow.cpp
    src/windows/ExchangeWindow.cpp
    src/windows/homewindow.cpp
    src/windows/listingswindow.cpp
    src/windows/loginwindow.cpp
    src/windows/OrderDetailWindow.cpp
    src/windows/orderswindow.cpp
    src/windows/sellwindow.cpp
    src/windows/signupwindow.cpp
    src/windows/ImageInspectionDialog.cpp

    include/network/ApiClient.h
    include/auth/SessionManager.h
    include/models/DataModels.h
    include/common/AppStyle.h
    include/common/ImageLoader.h
    include/common/StyledMessageBox.h
    include/common/CancelOrderDialog.h
    include/common/ProposeExchangeDialog.h
    include/common/VideoPlayerWidget.h
    include/common/VideoThumbnailHelper.h

    include/windows/bookdetailswindow.h
    include/windows/browsewindow.h
    include/windows/CartWindow.h
    include/windows/CheckoutWindow.h
    include/windows/ExchangeWindow.h
    include/windows/homewindow.h
    include/windows/listingswindow.h
    include/windows/loginwindow.h
    include/windows/OrderDetailWindow.h
    include/windows/orderswindow.h
    include/windows/sellwindow.h
    include/windows/signupwindow.h
    include/windows/ImageInspectionDialog.h

    ui/loginwindow.ui
)

set(PROJECT_SOURCES
    src/main.cpp
    ${APP_COMMON_SOURCES}
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
        ${CMAKE_CURRENT_SOURCE_DIR}/include/common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/windows
        ${CMAKE_CURRENT_SOURCE_DIR}/include/network
        ${CMAKE_CURRENT_SOURCE_DIR}/include/auth
        ${CMAKE_CURRENT_SOURCE_DIR}/include/models
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
        ${CMAKE_CURRENT_SOURCE_DIR}/src/windows
        ${CMAKE_CURRENT_SOURCE_DIR}/src/network
        ${CMAKE_CURRENT_SOURCE_DIR}/src/auth
)

# ==============================
# Link libraries
# ==============================

target_link_libraries(BookBazzar PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
)

if(WIN32)
    target_link_libraries(BookBazzar PRIVATE
        strmiids
        mfplat
        mfreadwrite
        mfuuid
    )
endif()

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
