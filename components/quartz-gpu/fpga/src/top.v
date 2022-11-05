`timescale 1ns/1ps
`include "ili.v"

module top (
	input  wire      clk_in,
	output wire      irq_n,
	
	input  wire      spi_mosi,
	output wire      spi_miso,
	input  wire      spi_clk,
	input  wire      spi_cs_n,
	
	output wire[7:0] pmod,
	output wire      red_n,
	output wire      green_n,
	output wire      blue_n,
	
	inout  wire[3:0] ram_io,
	output wire      ram_clk,
	output wire      ram_cs_n,
	
	output wire      lcd_rs,
	output wire      lcd_wr_n,
	input  wire      lcd_cs_n,
	output wire[7:0] lcd_d,
	input  wire      lcd_mode,
	input  wire      lcd_rst_n,
	input  wire      lcd_fmark
);
	
	wire [7:0] lcd_d_tmp;
	assign     lcd_d = lcd_mode ? lcd_d_tmp : 'bz;
	
	wire pix_clk;
	
	ili9341 ili_driver(
		clk_in,
		!cld_rst_n,
		lcd_d_tmp,
		lcd_wr_n,
		lcd_rs,
		lcd_fmark,
		pix_clk,
		0
	);
	
	// Commands.
	localparam cmd_status = 'h01;
	localparam cmd_rgbled = 'h02;
	
	reg[7:0] spi_recv_data;
	reg[7:0] spi_tx_data;
	
	assign spi_miso = spi_cs_n ? 'bz : spi_tx_data[7];
	
	assign red_n   = !resp_rdy;
	assign green_n = spi_cs_n;
	assign blue_n  = !lcd_mode;
	
	assign pmod[0]   = spi_cs_n;
	assign pmod[1]   = spi_clk;
	assign pmod[2]   = spi_mosi;
	assign pmod[3]   = spi_tx_data[7];
	assign pmod[4:7] = 0;
	
	// Sample buffer.
	reg[7:0]   sample_buf[15:0];
	reg[3:0]   sample_index;
	
	initial begin
		spi_bit       = 0;
		spi_byte_idx  = 0;
		spi_tx_sum    = 'hcc;
		clkdiv        = 1;
		spi_cs_n_last = 1;
		resp_rdy      = 0;
		
		// Transmit buffer.
		spi_tx_avl    = 8;
		
		// Hello status flag.
		hello_reg     = 0;
		// Receive error flag.
		status_err_rx = 0;
	end
	
	// Hello status flag.
	reg [1:0] hello_reg;
	wire      status_hello = hello_reg == 1;
	
	// Receive error flag.
	reg       status_err_rx;
	
	// Architecture ID.
	assign spi_tx_buf[0] = 1;
	// Architecture revision.
	assign spi_tx_buf[1] = 1;
	// Task capacity.
	assign spi_tx_buf[2] = 'h0a;
	assign spi_tx_buf[3] = 'h00;
	// Task usage.
	assign spi_tx_buf[4] = 'h00;
	assign spi_tx_buf[5] = 'h00;
	// Status flags.
	assign spi_tx_buf[6] = { 4'b0, status_hello, 3'b0 };
	assign spi_tx_buf[7] = { 7'b0, status_err_rx };
	
	always @(posedge clk_in) begin
		// TODO
	end
	
	// SPI Send.
	reg [7:0]  spi_tx_avl;
	wire[7:0]  spi_tx_buf[7:0];
	reg [2:0]  spi_bit;
	reg [7:0]  spi_tx_next;
	reg [7:0]  spi_byte_idx;
	reg [7:0]  spi_tx_sum;
	
	// SPI Recv.
	reg [7:0]  spi_rx_sum;
	reg [7:0]  spi_rx_avl;
	reg [7:0]  spi_rx_buf[31:0];
	reg        spi_rx_trigger;
	
	always @(posedge spi_clk, posedge spi_cs_n) begin
		if (spi_cs_n) begin
			// Prepare send state.
			spi_bit      <= 0;
			spi_tx_data  <= spi_tx_avl;
			spi_tx_sum   <= 'hcc ^ spi_tx_avl;
			spi_byte_idx <= -1;
			
		end else begin
			
			// Prepate bytes to send.
			if (spi_tx_avl != spi_byte_idx) begin
				// If there's data left:
				if (spi_bit == 4) begin
					// Increment send counter.
					spi_byte_idx <= spi_byte_idx + 1;
				end else if (spi_bit == 5) begin
					// Pick the next byte to send.
					spi_tx_next  <= spi_tx_buf[spi_byte_idx];
				end else if (spi_bit == 6) begin
					// Compute checksum.
					spi_tx_sum   <= spi_tx_sum ^ spi_tx_next;
				end
			end else begin
				if (spi_bit == 5) begin
					// If there's no more data, send checksum.
					spi_tx_next <= spi_tx_sum;
				end
			end
			
			// Select bit to send.
			if (spi_bit == 7) begin
				spi_tx_data <= spi_tx_next;
			end else begin
				spi_tx_data <= spi_tx_data << 1;
			end
			
			// Record received bits.
			if (spi_bit == 7) begin
				spi_rx_avl  <= spi_rx_avl + 1;
				spi_rx_sum  <= spi_rx_sum ^ { spi_recv_data[6:0], spi_mosi };
				spi_rx_buf[spi_byte_idx] <= { spi_recv_data[6:0], spi_mosi };
			end
			spi_recv_data <= { spi_recv_data[6:0], spi_mosi };
			
			// Increment bit position to keep track of bytes received.
			spi_bit <= spi_bit + 1;
		end
	end
	
	// READY TO RESPOND thing.
	wire   resp_rdy_s = spi_rx_trigger;
	wire   resp_rdy_r = !spi_cs_n;
	reg    resp_rdy;
	assign irq_n      = !resp_rdy;
	
	always @(posedge resp_rdy_s, posedge resp_rdy_r) begin
		if (resp_rdy_r) begin
			resp_rdy <= 0;
		end else if (resp_rdy_s) begin
			resp_rdy <= 1;
		end
	end
	
	reg spi_cs_n_last;
	always @(posedge clk_in) begin
		if (spi_rx_trigger) begin
			spi_rx_trigger <= 0;
		end else if (spi_cs_n && !spi_cs_n_last) begin
			spi_rx_trigger <= 1;
			
			// Hello logic.
			if (spi_rx_buf[0] == cmd_status) begin
				if (hello_reg != 2) begin
					hello_reg <= hello_reg + 1;
				end
			end
		end
		
		spi_cs_n_last <= spi_cs_n;
		resp_rdy_init <= 1;
	end
	
endmodule
