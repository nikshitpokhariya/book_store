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

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

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
)

# ==============================
# Link libraries
# ==============================

target_link_libraries(BookBazzar PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
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