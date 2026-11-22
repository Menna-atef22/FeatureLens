
cmake_minimum_required(VERSION 3.16)

project(FeatureLens LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Qt Installation Path
set(CMAKE_PREFIX_PATH
    "C:/Qt/6.12.0/msvc2022_64"
    ${CMAKE_PREFIX_PATH}
)

# Find Qt6 or Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Find windeployqt
if(WIN32)
    get_target_property(
        QT_QMAKE_EXECUTABLE
        Qt${QT_VERSION_MAJOR}::qmake
        IMPORTED_LOCATION
    )

    if(QT_QMAKE_EXECUTABLE)
        get_filename_component(
            QT_BIN_DIR
            "${QT_QMAKE_EXECUTABLE}"
            DIRECTORY
        )

        find_program(
            WINDEPLOYQT_EXECUTABLE
            windeployqt
            HINTS "${QT_BIN_DIR}"
        )
    endif()
endif()

# Source Files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.h
)

# Project Directories
set(FEATURELENS_RESULTS_DIR "${CMAKE_SOURCE_DIR}/results")
set(FEATURELENS_IMAGES_DIR "${CMAKE_SOURCE_DIR}/images")

configure_file(
    src/Paths.h.in
    "${CMAKE_CURRENT_BINARY_DIR}/Paths.h"
    @ONLY
)

# Main Application
add_executable(FeatureLens ${SOURCES})

target_include_directories(FeatureLens PRIVATE
    src
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(FeatureLens PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
)

# Compiler Options
if(MSVC)
    target_compile_options(FeatureLens PRIVATE /utf-8 /W3)
endif()

# Deploy Qt Libraries
if(WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET FeatureLens POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --no-translations
            "$<TARGET_FILE:FeatureLens>"
        VERBATIM
    )
endif()

# Headless Batch Tool
add_executable(FeatureLensBatch
    tools/batch.cpp
    src/common/Image.cpp
    src/common/Draw.cpp
    src/features/HarrisDetector.cpp
    src/features/MinEigenDetector.cpp
    src/features/SiftDetector.cpp
    src/features/Matcher.cpp
)

target_include_directories(FeatureLensBatch PRIVATE src)
