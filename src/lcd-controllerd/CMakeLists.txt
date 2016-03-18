include_directories("${PROJECT_SOURCE_DIR}/liblcd")
 
# lcd control service
add_executable(	lcd-controllerd
                lcd-controllerd.c )

target_link_libraries(lcd-controllerd lcd ${ITK_LIBRARIES})

install(TARGETS lcd-controllerd
	RUNTIME DESTINATION bin)