# CMake toolchain for the Apollo 4 microcontroller
#
# Author: TU Delft Sustainable Systems Laboratory
# License: MIT License

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(DEVICE "apollo4b" CACHE STRING "")
set(CPU "cortex-m4" CACHE STRING "")

set(OUTPUT_SUFFIX ".elf" CACHE STRING "")
set(LINKER_SCRIPT "${PROJECT_SOURCE_DIR}/config/${DEVICE}.ld")

string(TOUPPER ${DEVICE} DEVICE_DEFINE)
set(DEVICE_DEFINE "PART_${DEVICE_DEFINE}")

set(CMAKE_C_COMPILER    "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER  "arm-none-eabi-g++")
set(CMAKE_AR            "arm-none-eabi-ar")
set(CMAKE_LINKER        "arm-none-eabi-ld")
set(CMAKE_NM            "arm-none-eabi-nm")
set(CMAKE_OBJDUMP       "arm-none-eabi-objdump")
set(CMAKE_STRIP         "arm-none-eabi-strip")
set(CMAKE_RANLIB        "arm-none-eabi-ranlib")
set(CMAKE_SIZE          "arm-none-eabi-size")

# General compiler flags
add_compile_options(
    -mthumb
    -mcpu=${CPU}
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -mthumb
    -ffunction-sections -fdata-sections -fomit-frame-pointer
    -MMD -MP -std=c99 -g
    -Wall -Wextra -Wno-unused-parameter -Wshadow -Wno-expansion-to-defined  #-Wundef
    -Wno-unused-variable -Wno-unused-function -Wno-sign-compare -Wno-implicit-fallthrough -Wno-shadow -Wno-old-style-declaration -Wno-missing-field-initializers -Wno-cpp -fno-exceptions
    -DPART_${DEVICE}
    -DAM_${DEVICE_DEFINE}
    -DAM_PACKAGE_BGA
    -Dgcc
    )

# Device linker flags
add_link_options(
    -mthumb
    -mcpu=${CPU}
  #  -specs=nano.specs 
  #  -specs=nosys.specs
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -nostartfiles -static 
    -Wl,--gc-sections,--entry,Reset_Handler,--print-memory-usage
    -Wl,-Map=${PROJECT_NAME}.map
)

link_libraries(
    gcc
    c
    m
    nosys
    )
    
    
