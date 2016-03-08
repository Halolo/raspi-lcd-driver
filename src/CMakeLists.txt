cmake_minimum_required(VERSION 2.8)
project(raspi-lcd-control)

# add the binary tree to the search path for include files
include_directories(
    "${PROJECT_SOURCE_DIR}")
 
# add the executable
add_executable(	raspi-lcd-control
                lcd.c )

target_link_libraries(raspi-lcd-control ${ITK_LIBRARIES})
