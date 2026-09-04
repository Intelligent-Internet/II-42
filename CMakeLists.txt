cmake_minimum_required(VERSION 3.20)

project(ii42 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(ICU QUIET COMPONENTS i18n uc data)
if(ICU_FOUND)
    set(II42_ICU_TARGETS ICU::i18n ICU::uc)
    if(TARGET ICU::data)
        list(APPEND II42_ICU_TARGETS ICU::data)
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
        set(II42_ICU_TARGETS PkgConfig::ICU_PKG)
    else()
        set(
            II42_ICU_PREFIX_HINTS
            /opt/homebrew/opt/icu4c@78
            /opt/homebrew/opt/icu4c@77
            /opt/homebrew/opt/icu4c
            /usr/local/opt/icu4c
        )
        find_path(
            II42_ICU_INCLUDE_DIR
            unicode/ubrk.h
            PATH_SUFFIXES include
            PATHS ${II42_ICU_PREFIX_HINTS}
        )
        find_library(
            II42_ICU_I18N_LIB
            NAMES icui18n
            PATH_SUFFIXES lib
            PATHS ${II42_ICU_PREFIX_HINTS}
        )
        find_library(
            II42_ICU_UC_LIB
            NAMES icuuc
            PATH_SUFFIXES lib
            PATHS ${II42_ICU_PREFIX_HINTS}
        )
        find_library(
            II42_ICU_DATA_LIB
            NAMES icudata
            PATH_SUFFIXES lib
            PATHS ${II42_ICU_PREFIX_HINTS}
        )
        if(
            NOT II42_ICU_INCLUDE_DIR
            OR NOT II42_ICU_I18N_LIB
            OR NOT II42_ICU_UC_LIB
            OR NOT II42_ICU_DATA_LIB
        )
            message(FATAL_ERROR 'Unable to locate ICU headers and libraries')
        endif()

        add_library(ii42_icu INTERFACE)
        target_include_directories(
            ii42_icu
            INTERFACE
            ${II42_ICU_INCLUDE_DIR}
        )
        target_link_libraries(
            ii42_icu
            INTERFACE
            ${II42_ICU_I18N_LIB}
            ${II42_ICU_UC_LIB}
            ${II42_ICU_DATA_LIB}
        )
        set(II42_ICU_TARGETS ii42_icu)
    endif()
endif()

add_library(
    ii42_core
    STATIC
    src/ii42_block_ranges.c
    src/ii42_core.c
    src/ii42_document_tid_lookup.c
    src/ii42_document_cow.c
    src/ii42_initial_fold_stream.c
    src/ii42_lexicon_cow.c
    src/ii42_prefix_cow.c
    src/ii42_posting_heat.c
    src/ii42_query.c
    src/ii42_semantic_accelerator.c
    src/ii42_semantic_accelerator_builder.c
    src/ii42_semantic_accelerator_directory.c
    src/ii42_semantic_forward.c
    src/ii42_semantic_forward_bound.c
    src/ii42_semantic_impact_frontier.c
    src/ii42_scope.c
    src/ii42_weighted_space_saving.c
    src/ii42_semantic_bmp.c
    src/ii42_segments.c
    src/ii42_term_cow.c
    src/ii42_stem.c
    src/ii42_storage.c
    src/ii42_text.c
)
target_include_directories(ii42_core PUBLIC src)
target_compile_options(
    ii42_core
    PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
if(NOT WIN32)
    target_link_libraries(ii42_core PUBLIC m)
endif()
target_link_libraries(ii42_core PUBLIC ${II42_ICU_TARGETS})

add_executable(ii42_unit_tests tests/test_main.c)
target_link_libraries(ii42_unit_tests PRIVATE ii42_core)
target_include_directories(ii42_unit_tests PRIVATE src)

add_executable(ii42_simd_bench tests/benchmark_simd.c)
target_link_libraries(ii42_simd_bench PRIVATE ii42_core)
target_include_directories(ii42_simd_bench PRIVATE src)

add_executable(ii42_extent_bench tests/benchmark_extents.c)
target_link_libraries(ii42_extent_bench PRIVATE ii42_core)
target_include_directories(ii42_extent_bench PRIVATE src)

add_executable(
    ii42_semantic_bmp_bench
    tests/benchmark_semantic_bmp.c
)
target_link_libraries(ii42_semantic_bmp_bench PRIVATE ii42_core)
target_include_directories(ii42_semantic_bmp_bench PRIVATE src)

enable_testing()
add_test(NAME ii42_unit_tests COMMAND ii42_unit_tests)
