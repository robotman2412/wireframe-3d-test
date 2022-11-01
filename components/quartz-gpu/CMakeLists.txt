idf_component_register(
	INCLUDE_DIRS "src"
	
	SRCS
		"src/quartz.c"
		
	EMBED_FILES
		./fpga/build-tmp/quartz.bin
	
	REQUIRES
		pax-graphics
		esp_rom
		mch2022-bsp
		mch2022-rp2040
		spi-ice40
		spi-ili9341
)
