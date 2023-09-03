
add_library(DOOM)

# Add all the other include dependencies here for now
foreach(dep ${DEPENDENCIES})
  target_include_directories(DOOM
    PUBLIC ${CMAKE_SOURCE_DIR}/${dep}/
        )
endforeach()
  
# Glob all the doom sources 
file(GLOB DOOM_SOURCES    
  "${CMAKE_SOURCE_DIR}/third-party/Doom/source/*.[cs]"
)

target_sources(DOOM PRIVATE
  ${DOOM_SOURCES}
)

# Compile them in O0
target_compile_options(DOOM
PRIVATE -O0
)