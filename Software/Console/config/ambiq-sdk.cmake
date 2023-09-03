
SET(SDK_DEPENDENCIES
  # SDK HAL + BSP
  third-party/AmbiqSuite_R4.3.0/mcu/apollo4b/hal/mcu
  third-party/AmbiqSuite_R4.3.0/mcu/apollo4b/hal
)
        
 # Setup a list of source directories for the glob
foreach(dep ${SDK_DEPENDENCIES})
  list(APPEND DEP_SOURCES "${CMAKE_SOURCE_DIR}/${dep}/*.[cs]")
endforeach()

# Compile some specific sources to avoid including whole SDK
list(APPEND DEP_SOURCES 
  "${CMAKE_SOURCE_DIR}/third-party/AmbiqSuite_R4.3.0/utils/am_util_id.c"
  "${CMAKE_SOURCE_DIR}/third-party/AmbiqSuite_R4.3.0/utils/am_util_delay.c"
  "${CMAKE_SOURCE_DIR}/third-party/AmbiqSuite_R4.3.0/utils/am_util_stdio.c"
  "${CMAKE_SOURCE_DIR}/third-party/AmbiqSuite_R4.3.0/utils/am_util_faultisr.c"
  "${CMAKE_SOURCE_DIR}/third-party/AmbiqSuite_R4.3.0/boards/apollo4b_evb/bsp/am_bsp.c"
  "${CMAKE_SOURCE_DIR}/third-party/AmbiqSuite_R4.3.0/boards/apollo4b_evb/bsp/am_bsp_pins.c"
)

# Include the SDK dependencies
foreach(dep ${SDK_DEPENDENCIES})
  include_directories(${PROJECT_NAME}
     PRIVATE ${CMAKE_SOURCE_DIR}/${dep}/
  )
endforeach()

# Add some additional include only directories.
include_directories(${PROJECT_NAME}
  PRIVATE ${CMAKE_SOURCE_DIR}/third-party/AmbiqSuite_R4.3.0/CMSIS/ARM/Include
  PRIVATE ${CMAKE_SOURCE_DIR}/third-party/AmbiqSuite_R4.3.0/CMSIS/AmbiqMicro/Include
  PRIVATE ${CMAKE_SOURCE_DIR}/third-party/AmbiqSuite_R4.3.0/utils
  PRIVATE ${CMAKE_SOURCE_DIR}/third-party/AmbiqSuite_R4.3.0/devices
  PRIVATE ${CMAKE_SOURCE_DIR}/third-party/AmbiqSuite_R4.3.0/mcu/apollo4b
  PRIVATE ${CMAKE_SOURCE_DIR}/third-party/AmbiqSuite_R4.3.0/boards/apollo4b_evb/bsp
)
  

