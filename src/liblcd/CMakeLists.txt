# lcd lib
add_library( lcd SHARED
             lcd.c )

install(TARGETS lcd
	LIBRARY DESTINATION lib)