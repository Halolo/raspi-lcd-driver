cmake_minimum_required(VERSION 2.8)
project(raspi-lcd-control)

include_directories(
    "${PROJECT_SOURCE_DIR}")

# lcd lib
add_library( raspi-lcd-control SHARED
             lcd.c )
 
# test program
add_executable(	test-lcd
                test.c )

target_link_libraries(test-lcd raspi-lcd-control ${ITK_LIBRARIES})