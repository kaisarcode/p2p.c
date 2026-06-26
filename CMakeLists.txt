# rp2p - RedP2P
# KaisarCode - CMake build configuration
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.14)
project(rp2p LANGUAGES C)
set(CMAKE_C_STANDARD 11)

if(NOT DEFINED RP2P_BUILD_VERSION)
    set(RP2P_BUILD_VERSION 0)
endif()

option(RP2P_BUILD_TESTS "Build rp2p tests" OFF)

set(RP2P_TEST_EXTERNAL_ARTIFACTS OFF)
if(RP2P_BUILD_TESTS AND RP2P_TEST_SHARED_LIBRARY)
    set(RP2P_TEST_EXTERNAL_ARTIFACTS ON)
endif()

if(NOT MSVC)
    add_compile_options(-O3 -Wall -Wextra -Werror)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        add_compile_options(-Wno-unused-command-line-argument)
    endif()
    add_definitions(-D_GNU_SOURCE)
else()
    add_compile_options(/O2 /W3 /WX)
endif()

# Shared library
set(RP2P_SOURCES
    src/librp2p.c
    lib/monocypher/monocypher.c
)

set_source_files_properties(src/librp2p.c PROPERTIES
    COMPILE_DEFINITIONS RP2P_BUILD_VERSION=${RP2P_BUILD_VERSION}
)

if(NOT RP2P_TEST_EXTERNAL_ARTIFACTS)
    add_library(rp2p_shared SHARED ${RP2P_SOURCES})
    set_target_properties(rp2p_shared PROPERTIES OUTPUT_NAME "rp2p")
    target_include_directories(rp2p_shared PRIVATE src lib/monocypher)

    # Static library
    add_library(rp2p_static STATIC ${RP2P_SOURCES})
    set_target_properties(rp2p_static PROPERTIES OUTPUT_NAME "rp2p")
    target_include_directories(rp2p_static PRIVATE src lib/monocypher)

    # Binary: rp2p
    add_executable(rp2p src/rp2p.c ${RP2P_SOURCES})
    target_include_directories(rp2p PRIVATE src lib/monocypher)

    if(UNIX AND NOT ANDROID AND NOT APPLE)
        target_link_libraries(rp2p_shared pthread m)
        target_link_libraries(rp2p_static pthread m)
        target_link_libraries(rp2p pthread m)
    elseif(MINGW)
        target_link_libraries(rp2p_shared ws2_32 bcrypt)
        target_link_libraries(rp2p_static ws2_32 bcrypt)
        target_link_libraries(rp2p ws2_32 bcrypt)
    elseif(MSVC)
        target_link_libraries(rp2p_shared ws2_32 bcrypt)
        target_link_libraries(rp2p_static ws2_32 bcrypt)
        target_link_libraries(rp2p ws2_32 bcrypt)
    endif()
endif()

if(RP2P_BUILD_TESTS)
    enable_testing()

    if(RP2P_TEST_SHARED_LIBRARY)
        add_library(rp2p_test_shared SHARED IMPORTED)
        set_target_properties(rp2p_test_shared PROPERTIES
            IMPORTED_LOCATION "${RP2P_TEST_SHARED_LIBRARY}"
        )
        if(RP2P_TEST_IMPORT_LIBRARY)
            set_target_properties(rp2p_test_shared PROPERTIES
                IMPORTED_IMPLIB "${RP2P_TEST_IMPORT_LIBRARY}"
            )
        endif()
    else()
        set(RP2P_TEST_SHARED_TARGET rp2p_shared)
    endif()

    if(TARGET rp2p_test_shared)
        set(RP2P_TEST_SHARED_TARGET rp2p_test_shared)
    endif()

    add_executable(rp2p_contract_test src/test.c)
    target_include_directories(rp2p_contract_test PRIVATE src)
    target_link_libraries(rp2p_contract_test PRIVATE ${RP2P_TEST_SHARED_TARGET})

    if(UNIX AND NOT ANDROID AND NOT APPLE)
        target_link_libraries(rp2p_contract_test PRIVATE pthread m)
    elseif(MINGW)
        target_link_libraries(rp2p_contract_test PRIVATE ws2_32 bcrypt)
    elseif(MSVC)
        target_link_libraries(rp2p_contract_test PRIVATE ws2_32 bcrypt)
    endif()

    add_custom_command(TARGET rp2p_contract_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${RP2P_TEST_SHARED_TARGET}>
            $<TARGET_FILE_DIR:rp2p_contract_test>
    )

    add_test(NAME library-version-strerror COMMAND rp2p_contract_test version-strerror)
    add_test(NAME library-validators COMMAND rp2p_contract_test validators)
    add_test(NAME library-options COMMAND rp2p_contract_test options)
    add_test(NAME library-open-close COMMAND rp2p_contract_test open-close)
    add_test(NAME library-stop COMMAND rp2p_contract_test stop)
    add_test(NAME library-setters COMMAND rp2p_contract_test setters)
    add_test(NAME library-signals COMMAND rp2p_contract_test signals)
    add_test(NAME library-multictx COMMAND rp2p_contract_test multictx)
    add_test(NAME library-index-catalog COMMAND rp2p_contract_test index-catalog)
    add_test(NAME library-tcp-tunnel COMMAND rp2p_contract_test tcp-tunnel)
    add_test(NAME library-udp-tunnel COMMAND rp2p_contract_test udp-tunnel)
    add_test(NAME library-index-tcp-only COMMAND rp2p_contract_test index-tcp-only)
    add_test(NAME library-control-catalog COMMAND rp2p_contract_test control-catalog)
    add_test(NAME library-protocol-vectors COMMAND rp2p_contract_test protocol-vectors)
    add_test(NAME library-tcp-concurrent COMMAND rp2p_contract_test tcp-concurrent)
    add_test(NAME library-tcp-reconnect COMMAND rp2p_contract_test tcp-reconnect)
    add_test(NAME library-tcp-large COMMAND rp2p_contract_test tcp-large)
    add_test(NAME library-tcp-loss COMMAND rp2p_contract_test tcp-loss)
    add_test(NAME library-tcp-reorder COMMAND rp2p_contract_test tcp-reorder)
    add_test(NAME library-udp-large COMMAND rp2p_contract_test udp-large)
    add_test(NAME library-peer-disappearance COMMAND rp2p_contract_test peer-disappearance)
    add_test(NAME library-failure-cases COMMAND rp2p_contract_test failure-cases)
    add_test(NAME library-auth-vip-capacity COMMAND rp2p_contract_test auth-vip-capacity)
    add_test(NAME library-multictx-stop COMMAND rp2p_contract_test multictx-stop)
    set_tests_properties(library-index-catalog PROPERTIES TIMEOUT 30)
    set_tests_properties(library-tcp-tunnel PROPERTIES TIMEOUT 45)
    set_tests_properties(library-udp-tunnel PROPERTIES TIMEOUT 45)
    set_tests_properties(library-control-catalog PROPERTIES TIMEOUT 30)
    set_tests_properties(library-protocol-vectors PROPERTIES TIMEOUT 30)
    set_tests_properties(library-tcp-concurrent PROPERTIES TIMEOUT 45)
    set_tests_properties(library-tcp-reconnect PROPERTIES TIMEOUT 45)
    set_tests_properties(library-tcp-large PROPERTIES TIMEOUT 60)
    set_tests_properties(library-tcp-loss PROPERTIES TIMEOUT 60)
    set_tests_properties(library-tcp-reorder PROPERTIES TIMEOUT 60)
    set_tests_properties(library-udp-large PROPERTIES TIMEOUT 45)
    set_tests_properties(library-peer-disappearance PROPERTIES TIMEOUT 45)
    set_tests_properties(library-failure-cases PROPERTIES TIMEOUT 30)
    set_tests_properties(library-auth-vip-capacity PROPERTIES TIMEOUT 30)
    set_tests_properties(library-multictx-stop PROPERTIES TIMEOUT 30)

    if(RP2P_TEST_CLI)
        add_executable(rp2p_test_cli IMPORTED)
        set_target_properties(rp2p_test_cli PROPERTIES
            IMPORTED_LOCATION "${RP2P_TEST_CLI}"
        )
        set(RP2P_TEST_CLI_COMMAND rp2p_test_cli)
    else()
        set(RP2P_TEST_CLI_COMMAND rp2p)
    endif()

    add_test(NAME cli-help COMMAND ${RP2P_TEST_CLI_COMMAND} --help)
    add_test(NAME cli-version COMMAND ${RP2P_TEST_CLI_COMMAND} --version)
endif()
