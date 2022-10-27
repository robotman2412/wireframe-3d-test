idf_component_register(
	SRCS
		"src/wf3d.c"
		"src/matrix3.c"
		"src/obj.c"
	INCLUDE_DIRS "src" 
	REQUIRES pax-graphics esp_rom
)
