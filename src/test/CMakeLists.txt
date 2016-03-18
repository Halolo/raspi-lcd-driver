include_directories("${PROJECT_SOURCE_DIR}/liblcd")
 
# test program
add_executable(	test-lcd
                test.c )

target_link_libraries(test-lcd lcd ${ITK_LIBRARIES})

install(TARGETS test-lcd
	RUNTIME DESTINATION bin)

install(FILES linux.bmp DESTINATION /usr/local/share/test-lcd)