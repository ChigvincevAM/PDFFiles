cmake_minimum_required(VERSION 3.10)
project(PDFFiles)
set(CMAKE_CXX_STANDARD 17)
set(TARGET PDFFiles)

option(CASE_INSENSITIVE "Case insensitive method names" OFF)
option(STATIC_CRT "Static CRT linkage" ON)
option(OUT_PARAMS "Support output parameters" OFF)

set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")

list(APPEND SOURCES
        src/addin.def
        src/stdafx.h
        src/dllmain.cpp
        src/exports.cpp
        src/Component.cpp
        src/Component.h
        src/PDFFiles.cpp
        src/PDFFiles.h
        src/StringConverter.cpp
        src/StringConverter.h)

if (ANDROID)
    list(APPEND SOURCES
            src/jnienv.cpp
            src/jnienv.h)
endif ()

# ---- Target ----
add_library(${TARGET} SHARED ${SOURCES} "src/GdiplusManager.h" "src/GdiplusManager.cpp" "src/Logger.h" "src/Logger.cpp" "src/VariantUtils.h" "src/VariantUtils.cpp" "src/FileSystemUtils.h" "src/FileSystemUtils.cpp" "src/PdfSplitManager.h" "src/PdfSplitManager.cpp" "src/PdfProcessor.h" "src/PdfProcessor.cpp" "src/ImageProcessor.h" "src/ImageProcessor.cpp")

# ---- ����� PoDoFo ----
find_package(podofo CONFIG REQUIRED)
target_link_libraries(${TARGET} PRIVATE podofo::podofo)

target_compile_definitions(${TARGET} PRIVATE
        UNICODE
        _UNICODE)

if (CASE_INSENSITIVE)
    target_compile_definitions(${TARGET} PRIVATE CASE_INSENSITIVE)
endif ()

if (OUT_PARAMS)
    target_compile_definitions(${TARGET} PRIVATE OUT_PARAMS)
endif ()

target_include_directories(${TARGET} PRIVATE include)

# ---- Windows ----
if (WIN32 AND NOT MSVC)
    message(FATAL_ERROR "Must be compiled with MSVC on Windows")
endif ()

if (WIN32)
    if (STATIC_CRT)
        set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        
        foreach(CompilerFlag ${CompilerFlags})
            string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
            set(${CompilerFlag} "${${CompilerFlag}}" CACHE STRING "" FORCE)
        endforeach()
    endif()
    
    target_compile_definitions(${TARGET} PRIVATE
            _WINDOWS
            _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
    target_compile_options(${TARGET} PRIVATE /utf-8)
endif ()

# ---- Linux ----
if (UNIX)
    if (TARGET_ARCH STREQUAL "x86")
        set(CMAKE_C_FLAGS "-m32 ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-m32 ${CMAKE_CXX_FLAGS}")
    endif ()
endif ()

# ---- Android ----
if (ANDROID)
    if (CMAKE_BUILD_TYPE STREQUAL Release)
        add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND ${CMAKE_STRIP} ${CMAKE_SHARED_LIBRARY_PREFIX}${TARGET}${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif ()
endif ()