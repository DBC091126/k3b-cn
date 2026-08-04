cmake_minimum_required(VERSION 3.16)
project(k3b_cn VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(APP_SOURCES
    src/main.cpp
    src/Settings.cpp
    src/JobRunner.cpp
    src/Backend.cpp
    src/Projects.cpp
    src/MainWindow.cpp
    src/pages/WelcomePage.cpp
    src/pages/DataProjectPage.cpp
    src/pages/AudioProjectPage.cpp
    src/pages/CopyDiscPage.cpp
    src/pages/IsoToolsPage.cpp
    src/pages/MediaInfoPage.cpp
    src/pages/ErasePage.cpp
    src/pages/BurnJobPage.cpp
    src/pages/BurnOptionsDialog.cpp
)

set(APP_HEADERS
    src/Settings.h
    src/Types.h
    src/Icons.h
    src/JobRunner.h
    src/Backend.h
    src/Projects.h
    src/MainWindow.h
    src/pages/WelcomePage.h
    src/pages/DataProjectPage.h
    src/pages/AudioProjectPage.h
    src/pages/CopyDiscPage.h
    src/pages/IsoToolsPage.h
    src/pages/MediaInfoPage.h
    src/pages/ErasePage.h
    src/pages/BurnJobPage.h
    src/pages/BurnOptionsDialog.h
)

qt_add_executable(k3b-cn ${APP_SOURCES} ${APP_HEADERS})

target_compile_definitions(k3b-cn PRIVATE
    QT_NO_CAST_FROM_ASCII
)

set_target_properties(k3b-cn PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(k3b-cn PRIVATE Qt6::Widgets)

install(TARGETS k3b-cn RUNTIME DESTINATION bin)
