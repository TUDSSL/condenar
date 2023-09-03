# License TBD

project(console VERSION 1.0.0 LANGUAGES C ASM)

# Options
set(DEBUG 0)

#--------------------------------------------------------------------------------------------------
#     Configuration
#--------------------------------------------------------------------------------------------------
# set all dependencies everything here in these folders .c and .s is compiled and included
    set(DEPENDENCIES
        config
        platforms
        libs/Kernel
        libs/Kernel/Helpers
        libs/Kernel/Helpers/Fonts
        libs/Kernel/PowerDownRecovery
        libs/Kernel/Helpers/Screen
        libs/Kernel/Helpers/Screen/COM50
        libs/Kernel/Helpers/Screen/SharpBW
        libs/MainMenu
        libs/Settings
        libs/DownloadMode
        libs/TurnOff
        libs/TestMode
        games/Doom
        games/Doom/src
        games/Gameboy
        games/Gameboy/GBA
        games/Gameboy/GBA/emulator
        games/Gameboy/GBA/emulator/apu
        games/Gameboy/GBA/emulator/common
        games/Gameboy/GBA/emulator/gba
        games/Gameboy/GBA/emulator/glib
        games/Gameboy/GBC
        games/Gameboy/GBC/emulator
        third-party/Segger_RTT
        # DOOM
        third-party/Doom/include
        )


# Setup a list of source directories for the glob
foreach(dep ${DEPENDENCIES})
    list(APPEND DEP_SOURCES "${CMAKE_SOURCE_DIR}/${dep}/*.[cs]")
    list(APPEND DEP_SOURCES "${CMAKE_SOURCE_DIR}/${dep}/*.cpp")
endforeach()

#include the SDK files and sources
include(${CMAKE_SOURCE_DIR}/config/ambiq-sdk.cmake)

# List source files to be compiled
file(GLOB SOURCES
    "${PROJECT_SOURCE_DIR}/*.[cs]"
    ${DEP_SOURCES}
    )

# Add executable target
add_executable(${PROJECT_NAME} ${SOURCES})

# Add in Doom as a library
include(${CMAKE_SOURCE_DIR}/config/doom.cmake)

#--------------------------------------------------------------------------------------------------
#     Compilation flags
#--------------------------------------------------------------------------------------------------

# Defines (-D)
target_compile_definitions(${PROJECT_NAME}
    PRIVATE HWREV=2
    PRIVATE USE_TSI_MALLOC
    PRIVATE BAREMETAL
    PRIVATE AM_UTIL_FAULTISR_PRINT
)


# Compiler options for this project
if(DEBUG)
target_compile_options(${PROJECT_NAME}
    PRIVATE -O0
)
else()
target_compile_definitions(${PROJECT_NAME}
    PRIVATE AM_HAL_DISABLE_API_VALIDATION
)
target_compile_options(${PROJECT_NAME}
    PRIVATE -O3
)
#set_property(TARGET console PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Include directories of libraries and externals
foreach(dep ${DEPENDENCIES})
    target_include_directories(${PROJECT_NAME}
        PRIVATE ${CMAKE_SOURCE_DIR}/${dep}/
        PRIVATE ${PROJECT_SOURCE_DIR}
        )
endforeach()

# Linker options for this project
target_link_options(${PROJECT_NAME}
    PRIVATE -L${CMAKE_SOURCE_DIR}/config
    PRIVATE -T ${LINKER_SCRIPT}
    #PRIVATE -march=armv7e-m
    #PRIVATE -flto
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE DOOM
)

# Print size of binary after linking
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}${OUTPUT_SUFFIX}
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}${OUTPUT_SUFFIX} ${PROJECT_NAME}.hex
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_OBJDUMP} -S ${PROJECT_NAME}.elf > ${PROJECT_NAME}.lst
)

# Change target suffix to have <name>.out
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ${OUTPUT_SUFFIX})



# Install executable in bin
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
