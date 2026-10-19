cmake_minimum_required(VERSION 3.16)

project(mcp_synology_spreadsheet VERSION 0.1.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Jenis build" FORCE)
endif()

option(PERINGATAN_SEBAGAI_GALAT "Anggap peringatan kompilasi sebagai galat (untuk CI)" OFF)
option(BANGUN_TES "Bangun tes di tes/ bila ada" ON)

# Pustaka inti: semua modul selain main, supaya tes bisa menautkannya.
add_library(pustaka_inti STATIC
    src/alat.c
    src/catat.c
    src/json.c
    src/klien_api.c
    src/konfigurasi.c
    src/pengodean.c
    src/penyangga.c)
target_include_directories(pustaka_inti PUBLIC src)

if(WIN32)
    target_sources(pustaka_inti PRIVATE src/http_windows.c)
    target_link_libraries(pustaka_inti PUBLIC winhttp)
else()
    target_sources(pustaka_inti PRIVATE src/http_tanpa_dukungan.c)
endif()

if(MSVC)
    target_compile_options(pustaka_inti PUBLIC /W4)
    target_compile_definitions(pustaka_inti PUBLIC _CRT_SECURE_NO_WARNINGS)
    if(PERINGATAN_SEBAGAI_GALAT)
        target_compile_options(pustaka_inti PUBLIC /WX)
    endif()
else()
    target_compile_options(pustaka_inti PUBLIC -Wall -Wextra -Wpedantic -Wshadow)
    if(PERINGATAN_SEBAGAI_GALAT)
        target_compile_options(pustaka_inti PUBLIC -Werror)
    endif()
endif()

add_executable(mcp-synology-spreadsheet src/main.c)
target_compile_definitions(mcp-synology-spreadsheet PRIVATE VERSI_SERVER="${PROJECT_VERSION}")
target_link_libraries(mcp-synology-spreadsheet PRIVATE pustaka_inti)

if(BANGUN_TES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tes/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tes)
endif()
