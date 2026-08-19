#############################################################################################################################
# file:  tasks/muxer/CMakeLists.txt
# brief: Main data router inside the project
# 
# author : l.heywang <leonard.heywang@proton.me>
# date : 20/02/2026
#############################################################################################################################

# Set sources
set(SOURCES 
	src/task_muxer.cpp
)

# Creating the library
add_library(MUXER STATIC ${SOURCES})

# Link against some libraries
target_link_libraries(MUXER THREADX_OS LOGGER)

# Adding the include folder
target_include_directories(MUXER PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/include/
)