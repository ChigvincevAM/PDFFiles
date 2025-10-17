cmake_minimum_required(VERSION 3.10)
project(PDFFiles)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(TARGET PDFFiles)

# ---- Опции ----
option(CASE_INSENSITIVE "Case insensitive method names" OFF)
option(STATIC_CRT "Static CRT linkage" ON)
option(OUT_PARAMS "Support output parameters" OFF)
option(BUILD_STATIC_LIB "Build as static library" OFF)

# ---- Определение архитектуры ----
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH "x64")
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCH "x86")
    set(VCPKG_TARGET_TRIPLET "x86-windows-static" CACHE STRING "")
else()
    message(FATAL_ERROR "Unsupported architecture")
endif()

message(STATUS "Architecture: ${ARCH}")
message(STATUS "VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")

# ---- Исходные файлы ----
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
    src/StringConverter.h
    src/GdiplusManager.h
    src/GdiplusManager.cpp
    src/Logger.h
    src/Logger.cpp
    src/VariantUtils.h
    src/VariantUtils.cpp
    src/FileSystemUtils.h
    src/FileSystemUtils.cpp
    src/PdfSplitManager.h
    src/PdfSplitManager.cpp
    src/PdfProcessor.h
    src/PdfProcessor.cpp
    src/ImageProcessor.h
    src/ImageProcessor.cpp)

if(ANDROID)
    list(APPEND SOURCES
        src/jnienv.cpp
        src/jnienv.h)
endif()

# ---- Создание библиотеки ----
if(BUILD_STATIC_LIB)
    message(STATUS "Building as STATIC library")
    add_library(${TARGET} STATIC ${SOURCES})
else()
    message(STATUS "Building as SHARED library (DLL)")
    add_library(${TARGET} SHARED ${SOURCES})
endif()

# ---- Поиск PoDoFo ----
find_package(podofo CONFIG REQUIRED)
target_link_libraries(${TARGET} PRIVATE podofo::podofo)

# ---- Определения компилятора ----
target_compile_definitions(${TARGET} PRIVATE
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS)

if(CASE_INSENSITIVE)
    target_compile_definitions(${TARGET} PRIVATE CASE_INSENSITIVE)
endif()

if(OUT_PARAMS)
    target_compile_definitions(${TARGET} PRIVATE OUT_PARAMS)
endif()

target_include_directories(${TARGET} PRIVATE include)

# ---- Windows специфика ----
if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR "Must be compiled with MSVC on Windows")
endif()

if(WIN32)
    # ---- Флаги компилятора для MSVC ----
    target_compile_options(${TARGET} PRIVATE
        /W3
        /WX-
        /utf-8
        /std:c++17
        /Zc:inline)
    
    # ---- Static CRT linkage ----
    if(STATIC_CRT)
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
        message(STATUS "Static CRT linkage enabled")
    endif()
    
    # ---- Windows определения ----
    target_compile_definitions(${TARGET} PRIVATE
        _WINDOWS
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    
    # ---- Архитектура для Windows ----
    if(ARCH STREQUAL "x64")
        message(STATUS "Building for x64 Windows")
        target_compile_definitions(${TARGET} PRIVATE _WIN64)
        set(OUTPUT_SUBDIR "x64")
    elseif(ARCH STREQUAL "x86")
        message(STATUS "Building for x86 Windows")
        target_compile_options(${TARGET} PRIVATE /arch:IA32)
        set(OUTPUT_SUBDIR "x86")
    endif()
    
    # ====================================================================
    # ДОБАВИТЬ ИМЯ АРХИТЕКТУРЫ К DLL/LIB
    # ====================================================================
    if(NOT BUILD_STATIC_LIB)
        set_target_properties(${TARGET} PROPERTIES
            OUTPUT_NAME "${TARGET}_${ARCH}"
        )
        message(STATUS "DLL output name: ${TARGET}_${ARCH}.dll")
    else()
        set_target_properties(${TARGET} PROPERTIES
            OUTPUT_NAME "${TARGET}_${ARCH}"
        )
        message(STATUS "LIB output name: ${TARGET}_${ARCH}.lib")
    endif()
    
    # ---- Установка выходных директорий ----
    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${OUTPUT_SUBDIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${OUTPUT_SUBDIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${OUTPUT_SUBDIR}"
    )
    
    # ---- Для Debug конфигурации ----
    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/${OUTPUT_SUBDIR}/Debug"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib/${OUTPUT_SUBDIR}/Debug"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib/${OUTPUT_SUBDIR}/Debug"
    )
    
    # ---- Для Release конфигурации ----
    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/${OUTPUT_SUBDIR}/Release"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib/${OUTPUT_SUBDIR}/Release"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib/${OUTPUT_SUBDIR}/Release"
    )
endif()

# ---- Linux специфика ----
if(UNIX AND NOT APPLE AND NOT ANDROID)
    message(STATUS "Building for Linux")
    
    target_compile_options(${TARGET} PRIVATE
        -std=c++17
        -Wall
        -Wextra)
    
    if(ARCH STREQUAL "x86")
        message(STATUS "Building 32-bit on Linux")
        target_compile_options(${TARGET} PRIVATE -m32)
        target_link_options(${TARGET} PRIVATE -m32)
    elseif(ARCH STREQUAL "x64")
        message(STATUS "Building 64-bit on Linux")
        target_compile_options(${TARGET} PRIVATE -m64)
        target_link_options(${TARGET} PRIVATE -m64)
    endif()
    
    set_target_properties(${TARGET} PROPERTIES
        OUTPUT_NAME "${TARGET}_${ARCH}"
    )
endif()

# ---- Android специфика ----
if(ANDROID)
    message(STATUS "Building for Android")
    if(CMAKE_BUILD_TYPE STREQUAL Release)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_STRIP} 
            ${CMAKE_SHARED_LIBRARY_PREFIX}${TARGET}${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif()
endif()

# ---- Вывод информации о сборке ----
message(STATUS "=======================================")
message(STATUS "Target: ${TARGET}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Architecture: ${ARCH}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Library type: $(if BUILD_STATIC_LIB,STATIC,SHARED)")
message(STATUS "=======================================")