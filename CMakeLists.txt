cmake_minimum_required(VERSION 3.20)

project(psql_bm25s LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(ICU QUIET COMPONENTS i18n uc data)
if(ICU_FOUND)
    set(PSQL_BM25S_ICU_TARGETS ICU::i18n ICU::uc)
    if(TARGET ICU::data)
        list(APPEND PSQL_BM25S_ICU_TARGETS ICU::data)
    endif()
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(
            ICU_PKG
            QUIET
            IMPORTED_TARGET
            icu-i18n
            icu-uc
        )
    endif()

    if(TARGET PkgConfig::ICU_PKG)
        set(PSQL_BM25S_ICU_TARGETS PkgConfig::ICU_PKG)
    else()
        set(
            PSQL_BM25S_ICU_PREFIX_HINTS
            /opt/homebrew/opt/icu4c@78
            /opt/homebrew/opt/icu4c@77
            /opt/homebrew/opt/icu4c
            /usr/local/opt/icu4c
        )
        find_path(
            PSQL_BM25S_ICU_INCLUDE_DIR
            unicode/ubrk.h
            PATH_SUFFIXES include
            PATHS ${PSQL_BM25S_ICU_PREFIX_HINTS}
        )
        find_library(
            PSQL_BM25S_ICU_I18N_LIB
            NAMES icui18n
            PATH_SUFFIXES lib
            PATHS ${PSQL_BM25S_ICU_PREFIX_HINTS}
        )
        find_library(
            PSQL_BM25S_ICU_UC_LIB
            NAMES icuuc
            PATH_SUFFIXES lib
            PATHS ${PSQL_BM25S_ICU_PREFIX_HINTS}
        )
        find_library(
            PSQL_BM25S_ICU_DATA_LIB
            NAMES icudata
            PATH_SUFFIXES lib
            PATHS ${PSQL_BM25S_ICU_PREFIX_HINTS}
        )
        if(
            NOT PSQL_BM25S_ICU_INCLUDE_DIR
            OR NOT PSQL_BM25S_ICU_I18N_LIB
            OR NOT PSQL_BM25S_ICU_UC_LIB
            OR NOT PSQL_BM25S_ICU_DATA_LIB
        )
            message(FATAL_ERROR 'Unable to locate ICU headers and libraries')
        endif()

        add_library(psql_bm25s_icu INTERFACE)
        target_include_directories(
            psql_bm25s_icu
            INTERFACE
            ${PSQL_BM25S_ICU_INCLUDE_DIR}
        )
        target_link_libraries(
            psql_bm25s_icu
            INTERFACE
            ${PSQL_BM25S_ICU_I18N_LIB}
            ${PSQL_BM25S_ICU_UC_LIB}
            ${PSQL_BM25S_ICU_DATA_LIB}
        )
        set(PSQL_BM25S_ICU_TARGETS psql_bm25s_icu)
    endif()
endif()

add_library(
    psql_bm25s_core
    STATIC
    src/psql_bm25s_core.c
    src/psql_bm25s_query.c
    src/psql_bm25s_stem.c
    src/psql_bm25s_storage.c
    src/psql_bm25s_text.c
)
target_include_directories(psql_bm25s_core PUBLIC src)
target_compile_options(
    psql_bm25s_core
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
if(NOT WIN32)
    target_link_libraries(psql_bm25s_core PUBLIC m)
endif()
target_link_libraries(psql_bm25s_core PUBLIC ${PSQL_BM25S_ICU_TARGETS})

add_executable(psql_bm25s_unit_tests tests/test_main.c)
target_link_libraries(psql_bm25s_unit_tests PRIVATE psql_bm25s_core)
target_include_directories(psql_bm25s_unit_tests PRIVATE src)

add_executable(psql_bm25s_simd_bench tests/benchmark_simd.c)
target_link_libraries(psql_bm25s_simd_bench PRIVATE psql_bm25s_core)
target_include_directories(psql_bm25s_simd_bench PRIVATE src)

enable_testing()
add_test(NAME psql_bm25s_unit_tests COMMAND psql_bm25s_unit_tests)
