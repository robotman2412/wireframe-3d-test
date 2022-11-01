
# Base ESP-IDF component
idf_component_register(
	SRCS
		"src/quartz.c"
	INCLUDE_DIRS "src" 
	REQUIRES pax-graphics esp_rom mch2022-bsp
)
