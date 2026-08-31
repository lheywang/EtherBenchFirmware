#############################################################################################################################
# file:  tasks/programmer/CMakeLists.txt
# brief: CMSIS programmer, in between others programming function.
# 
# author : l.heywang <leonard.heywang@proton.me>
# date : 20/02/2026
#############################################################################################################################

# Set sources
set(SOURCES 
	src/task_programmer.cpp
)

# Add the programming classes sources
list(APPEND SOURCES
    src/ProbeController/ProbeController.cpp
    src/ProtocolController/ARMCoreSightSWDEngine.cpp
    src/ProtocolController/JTAGTAPEngine.cpp
    src/ProtocolController/MicrochipICSPEngine.cpp
    src/ProtocolController/RawSPIEngine.cpp
    src/TargetController/ARMTargetController.cpp
    src/TargetController/RISCVTargetController.cpp
)

# Add the
list(APPEND SOURCES 
    src/servers/CMSIS/CmsisDAPServer.cpp
    src/servers/GDB/GdbRspServer.cpp
    src/servers/xSVF/xSVFServer.cpp
    src/servers/XVC/XillinxXVCServer.cpp
)

# Creating the library
add_library(PROGRAMMER STATIC ${SOURCES})

target_link_libraries(PROGRAMMER HAL THREADX)

# Adding the include folder
target_include_directories(PROGRAMMER PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/include/
  	${CMAKE_CURRENT_SOURCE_DIR}/CMSIS-DAP/include/
)