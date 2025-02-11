cmake_minimum_required(VERSION 3.20)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
  set(TOOLCHAIN_PATH ${CMAKE_SOURCE_DIR}/arm-toolchain/linux-x86_64/gcc-arm-none-eabi-10.3-2021.10)
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  set(TOOLCHAIN_PATH ${CMAKE_SOURCE_DIR}/arm-toolchain/mac-x86_64/gcc-arm-none-eabi-10.3-2021.10)
else()

endif()

set(LD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/da16200_sdk/core/bsp/ldscripts")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target arm-unknown-none-eabi -nodefaultlibs -nostartfiles -fuse-ld=${TOOLCHAIN_PATH}/arm-none-eabi/bin/ld -L ${LD_PATH} -L${TOOLCHAIN_PATH}/arm-none-eabi/lib/arm/v5te/hard -L${TOOLCHAIN_PATH}/lib/gcc/arm-none-eabi/10.3.1/arm/v5te/hard")
include_directories(${TOOLCHAIN_PATH}/arm-none-eabi/include)
link_directories("${TOOLCHAIN_PATH}/lib/gcc/arm-none-eabi/10.3.1/arm/v5te/hard")

set(CMAKE_SYSROOT "${TOOLCHAIN_PATH}/arm-none-eabi")
set(CMAKE_LIBRARY_PATH "${TOOLCHAIN_PATH}/arm-none-eabi/lib/arm/v5te/hard")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${TOOLCHAIN_PATH}/arm-none-eabi/lib/arm/v5te/hard -L${TOOLCHAIN_PATH}/lib/gcc/arm-none-eabi/10.3.1/arm/v5te/hard")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/da16200_sdk/cmake)
set(CMAKE_TOOLCHAIN_FILE da16200_sdk/cmake/da16200-toolchain.cmake)

project(neuralert C ASM)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

add_subdirectory(neuralert)

set(MAIN_TARGET ${NEURALERT_TARGET})
set(MAIN_TARGET_NAME ${NEURALERT_TARGET_NAME})
set(MAIN_TARGET_INCLUDE ${NEURALERT_TARGET_INCLUDE})

add_subdirectory(da16200_sdk)
target_link_libraries(${MAIN_TARGET} DA16200_SDK_MAIN c m gcc nosys)
