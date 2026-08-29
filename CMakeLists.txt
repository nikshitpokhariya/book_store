cmake_minimum_required(VERSION 3.16)

project(BookBazzar VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

set(PROJECT_SOURCES
    main.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(BookBazzar
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}

        database.h
        database.cpp

        loginwindow.h
        loginwindow.cpp

        signupwindow.h
        signupwindow.cpp

        dashboard.h
        dashboard.cpp
        loginwindow.ui
    )

else()
    if(ANDROID)
        add_library(BookBazzar SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(BookBazzar
            ${PROJECT_SOURCES}

            database.h
            database.cpp

            loginwindow.h
            loginwindow.cpp

            signupwindow.h
            signupwindow.cpp

            dashboard.h
            dashboard.cpp
        )
    endif()
endif()

target_link_libraries(BookBazzar PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

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