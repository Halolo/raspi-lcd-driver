include_directories( "${PROJECT_SOURCE_DIR}/lcd-controllerd"
                     "${PROJECT_SOURCE_DIR}/liblcd" )
 
# lcd control client app
add_executable(	lcd-controllerd-cli
                lcd-controllerd-cli.c )

target_link_libraries(lcd-controllerd-cli ${ITK_LIBRARIES})

install(TARGETS lcd-controllerd-cli
	RUNTIME DESTINATION bin)

install(FILES sample/linux.bmp DESTINATION /usr/local/share/lcd-controllerd-cli)