# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 2.8...3.18)
project(socket_api_sample_wasm_app)

message(CHECK_START "Detecting WASI-SDK at /opt/wasi-sdk")
if(NOT (DEFINED WASI_SDK_DIR OR DEFINED CACHE{WASI_SDK_DIR}))
  find_path(WASI_SDK_PARENT
    wasi-sdk
    PATHS /opt
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
  )
  if(WASI_SDK_PARENT)
    set(WASI_SDK_DIR ${WASI_SDK_PARENT}/wasi-sdk)
  endif()
endif()
if(WASI_SDK_DIR)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
endif()

if(NOT EXISTS ${WASI_SDK_DIR})
  message(FATAL_ERROR "Please install WASI-SDK under /opt/wasi-sdk")
endif()

message(CHECK_START "Detecting WASI_TOOLCHAIN_FILE at ${WASI_SDK_DIR}")
find_file(WASI_TOOLCHAIN_FILE
  wasi-sdk.cmake
  PATHS "${WASI_SDK_DIR}/share/cmake"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
if(WASI_TOOLCHAIN_FILE)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
endif()

if(WASI_TOOLCHAIN_FILE-NOTFOUND)
  message(FATAL_ERROR "Can not find wasi-sdk.cmake under ${WASI_SDK_DIR}")
endif()

message(CHECK_START "Detecting WASI_SYS_ROOT at ${WASI_SDK_DIR}")
find_path(WASI_SYS_ROOT
  wasi-sysroot
  PATHS "${WASI_SDK_DIR}/share"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
if(WASI_SYS_ROOT)
  message(CHECK_PASS "found")
  set(WASI_SYS_ROOT ${WASI_SYS_ROOT}/wasi-sysroot)
else()
  message(CHECK_FAIL "not found")
endif()

if(WASI_SYS_ROOT-NOTFOUND)
  message(FATAL_ERROR "Can not find wasi-sysroot/ under ${WASI_SDK_DIR}")
endif()

message(STATUS "WASI_SDK_DIR is ${WASI_SDK_DIR}")
message(STATUS "WASI_TOOLCHAIN_FILE is ${WASI_TOOLCHAIN_FILE}")
message(STATUS "WASI_SYS_ROOT is ${WASI_SYS_ROOT}")


include(CMakePrintHelpers)
message(CHECK_START "Detecting WABT")
if(NOT (DEFINED WABT_DIR OR DEFINED CACHE{WABT_DIR}))
  find_path(WABT_DIR
    wabt
    PATHS /opt
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
  )
  if(DEFINED WABT_DIR)
    set(WABT_DIR ${WABT_DIR}/wabt)
  endif()
endif()
if(WABT_DIR)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
endif()

message(CHECK_START "Detecting WASM_OBJDUMP at ${WABT_DIR}")
find_program(WASM_OBJDUMP
  wasm-objdump
  PATHS "${WABT_DIR}/bin"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
if(WASM_OBJDUMP)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
endif()

set(SRC ${CMAKE_CURRENT_SOURCE_DIR})

include(${CMAKE_CURRENT_SOURCE_DIR}/../../../core/iwasm/libraries/lib-socket/lib_socket_wasi.cmake)
# include(${CMAKE_CURRENT_SOURCE_DIR}/../../../core/iwasm/libraries/lib-pthread/lib_pthread.cmake)

#######################################
## Build curl
#######################################

include(ExternalProject)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}) 

# We're building curl ourselves
set(CURL_LIB_TYPE STATIC)

# The cURL library name always starts with "libcurl", regardless of OS conventions, but the
# suffix is platform-dependent
set(CURL_LIB_SUFFIX ${CMAKE_${CURL_LIB_TYPE}_LIBRARY_SUFFIX})

SET(CURL_INSTALL_DIR ${CMAKE_BINARY_DIR}/thirdparty)

set(CURL_CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CURL_INSTALL_DIR}
        -DCMAKE_INSTALL_LIBDIR=${CURL_INSTALL_DIR}/lib
        -DBUILD_CURL_EXE=OFF
        -DBUILD_TESTING=OFF
        -DCURL_DISABLE_TESTS=ON
        -DHTTP_ONLY=ON
        -DENABLE_IPV6=OFF
        -DUSE_TLS_SRP=OFF
        -DSSL_ENABLED=OFF
        -DHAVE_LIBZ=OFF
        -DCURL_DISABLE_CRYPTO_AUTH=ON
        -DENABLE_MANUAL=OFF
        -DCURL_USE_LIBSSH2=OFF
        -DCMAKE_USE_LIBSSH2=OFF
        -DCMAKE_USE_OPENSSL=OFF
        -DCURL_USE_LIBSSH=OFF
        -DCMAKE_USE_OPENSSL=OFF
        -DCURL_ENABLE_SSL=OFF
        -DUSE_OPENSSL=OFF
        -DCURL_USE_LIBPSL=OFF
        -DZLIB_FOUND=OFF
        -DCURL_DISABLE_OPENSSL_AUTO_LOAD_CONFIG=OFF
        -DHAVE_POLL_FINE_EXITCODE=0
        -DCURL_CA_BUNDLE=none
        -DCURL_CA_PATH=none
        -DWASI_SDK_PREFIX=${WASI_SDK_DIR}
        -DCMAKE_TOOLCHAIN_FILE=${WASI_TOOLCHAIN_FILE}
        -DCMAKE_SYSROOT=${WASI_SYS_ROOT}
        -DCMAKE_DEBUG_POSTFIX=
        ${PLATFORM_CURL_CMAKE_ARGS}
)

set(CURL_CMAKE_ARGS ${CURL_CMAKE_ARGS} -DBUILD_SHARED_LIBS=OFF)

set(CURL_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/curl")
set(CURL_INCLUDE ${CURL_INSTALL_DIR}/include)
set(CURL_LIBRARY ${CURL_INSTALL_DIR}/lib/libcurl${CURL_LIB_SUFFIX})

set(CURL_CMAKE_ARGS ${CURL_CMAKE_ARGS} -DWITHOUT_LIBIDN2=ON)

add_definitions(-I${CMAKE_CURRENT_SOURCE_DIR}/../../../core/iwasm/libraries/lib-socket/inc/wasi_socket_ext.h)

ExternalProject_Add(
        Libs_Curl
        DEPENDS ${CURL_EXTERNAL_DEPS}
        SOURCE_DIR "${CMAKE_SOURCE_DIR}/curl"
        BINARY_DIR "${CURL_BIN_DIR}"
        INSTALL_DIR "${CURL_INSTALL_DIR}"
        LIST_SEPARATOR |
        CONFIGURE_COMMAND ${CMAKE_COMMAND} ${CURL_CMAKE_ARGS} ${CMAKE_CURRENT_SOURCE_DIR}/curl
        BUILD_COMMAND "${CMAKE_COMMAND}" --build "${CURL_BIN_DIR}"
        TEST_COMMAND ""
        INSTALL_COMMAND  "${CMAKE_COMMAND}" --build "${CURL_BIN_DIR}" --target install
        BUILD_BYPRODUCTS "${CURL_LIBRARY}"
)

file(MAKE_DIRECTORY ${CURL_INCLUDE})

set(CURL_INCLUDE_DIR ${CURL_INSTALL_DIR}/include)
add_library(libcurltargettag OBJECT ${CURL_INSTALL_DIR}/lib/libcurl.a)
set_target_properties(libcurltargettag PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CURL_INCLUDE}
                                         IMPORTED_LINK_LANGUAGES "C"
                                         IMPORTED_LOCATION ${CURL_LIBRARY})
set_target_properties(libcurltargettag PROPERTIES INTERFACE_COMPILE_DEFINITIONS CURL_STATICLIB)
add_dependencies(libcurltargettag Libs_Curl)


message("CURL_LIBRARY=${CURL_LIBRARY}")
message("CURL_INCLUDE=${CURL_INCLUDE}")

#######################################
## End build curl
#######################################


function(COMPILE_WITH_CLANG SOURCE_FILE)
  get_filename_component(FILE_NAME ${SOURCE_FILE} NAME_WLE)

  set(WASM_MODULE ${FILE_NAME}.wasm)

  set(MAIN_TARGET_NAME MODULE_${FILE_NAME})

  add_executable(${MAIN_TARGET_NAME} ${SOURCE_FILE})
  set_target_properties(${MAIN_TARGET_NAME} PROPERTIES OUTPUT_NAME ${WASM_MODULE})
  include_directories(${CURL_INCLUDE_DIR})
  target_include_directories(${MAIN_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
  target_compile_options(${MAIN_TARGET_NAME} INTERFACE -pthread)
  target_link_libraries(${MAIN_TARGET_NAME} socket_wasi_ext ${CURL_INSTALL_DIR}/lib/libcurl.a)

  target_link_options(${MAIN_TARGET_NAME} PRIVATE
    LINKER:--export=__heap_base
    LINKER:--export=__data_end
    LINKER:--shared-memory,--max-memory=196608
    LINKER:--no-check-features
    LINKER:--allow-undefined
  )

  message(STATUS "Linking ${CURL_INCLUDE_DIR} ${CURL_LIBRARIES}")

  if(EXISTS ${WASM_OBJDUMP})
    message(STATUS "Dumping ${WASM_MODULE}...")
    set(WASM_DUMP ${WASM_MODULE}.dump)
    set(DUMP_TARGET_NAME DUMP_${FILE_NAME})

    add_custom_command(OUTPUT ${WASM_DUMP}
      COMMAND ${WASM_OBJDUMP} -dx ${WASM_MODULE} > ${WASM_DUMP}
      COMMENT "Dumping ${WASM_MODULE}..."
      DEPENDS ${MAIN_TARGET_NAME}
    )

    add_custom_target(${DUMP_TARGET_NAME} ALL
      DEPENDS ${WASM_DUMP}
    )
  endif()
endfunction()

compile_with_clang(tcp_server.c)
compile_with_clang(tcp_client.c)
compile_with_clang(send_recv.c)
compile_with_clang(addr_resolve.c)
compile_with_clang(socket_opts.c)
compile_with_clang(udp_client.c)
compile_with_clang(udp_server.c)
compile_with_clang(multicast_client.c)
compile_with_clang(multicast_server.c)
compile_with_clang(timeout_client.c)
compile_with_clang(timeout_server.c)
compile_with_clang(curl_example.c)
