
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/fpga/build-tmp/quartz.bin")
	# Build with new shiny quartz.bin
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
else()
	# Build with default quartz.bin
	idf_component_register(
		INCLUDE_DIRS "src"
		
		SRCS
			"src/quartz.c"
			
		EMBED_FILES
			./fpga/default/quartz.bin
		
		REQUIRES
			pax-graphics
			esp_rom
			mch2022-bsp
			mch2022-rp2040
			spi-ice40
			spi-ili9341
	)
endif()
