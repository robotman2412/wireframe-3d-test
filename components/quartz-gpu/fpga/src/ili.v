

module ili9341(
	input  wire       clk,
	input  wire       rst,
	output wire[7:0]  lcd_data,
	output wire       lcd_we_n,
	output wire       lcd_rs,
	input  wire       lcd_fmark,
	output wire       pix_clk,
	input  wire[15:0] pix_data
);
	
	localparam state_rst   = 0;
	localparam state_init  = 1;
	localparam state_idle  = 2;
	localparam state_write = 3;
	
	reg[3:0] state;
	
	reg      lcd_rs_reg;
	reg      lcd_we_reg;
	reg[7:0] lcd_data_reg;
	
	assign lcd_we_n = !(clk & lcd_we_reg);
	assign lcd_rs   = lcd_rs_reg;
	assign lcd_data = lcd_data_reg;
	
	localparam init_len = 101;
	reg[6:0]   init_counter;
	reg[8:0]   init_sequence[0:100];
	
	initial begin
		init_counter = 0;
		
		init_sequence[7'h00] = 9'h0ef;
		init_sequence[7'h01] = 9'h103;
		init_sequence[7'h02] = 9'h180;
		init_sequence[7'h03] = 9'h102;
		init_sequence[7'h04] = 9'h0cf; // Power control B
		init_sequence[7'h05] = 9'h100;
		init_sequence[7'h06] = 9'h1c1;
		init_sequence[7'h07] = 9'h130;
		init_sequence[7'h08] = 9'h0ed; // Power on sequence control
		init_sequence[7'h09] = 9'h164;
		init_sequence[7'h0a] = 9'h103;
		init_sequence[7'h0b] = 9'h112;
		init_sequence[7'h0c] = 9'h181;
		init_sequence[7'h0d] = 9'h0e8; // Driver timing control A
		init_sequence[7'h0e] = 9'h185;
		init_sequence[7'h0f] = 9'h100;
		init_sequence[7'h10] = 9'h178;
		init_sequence[7'h11] = 9'h0cb; // Power control A
		init_sequence[7'h12] = 9'h139;
		init_sequence[7'h13] = 9'h12c;
		init_sequence[7'h14] = 9'h100;
		init_sequence[7'h15] = 9'h134;
		init_sequence[7'h16] = 9'h102;
		init_sequence[7'h17] = 9'h0f7; // Pump ratio control
		init_sequence[7'h18] = 9'h120;
		init_sequence[7'h19] = 9'h0ea; // Driver timing control B
		init_sequence[7'h1a] = 9'h100;
		init_sequence[7'h1b] = 9'h100;
		init_sequence[7'h1c] = 9'h0c0; // Power control 1
		init_sequence[7'h1d] = 9'h123;
		init_sequence[7'h1e] = 9'h0c1; // Power control 2
		init_sequence[7'h1f] = 9'h110;
		init_sequence[7'h20] = 9'h0c5; // VCOM Control 1
		init_sequence[7'h21] = 9'h13e;
		init_sequence[7'h22] = 9'h128;
		init_sequence[7'h23] = 9'h0c7; // VCOM Control 2
		init_sequence[7'h24] = 9'h186;
		init_sequence[7'h25] = 9'h036; // Memory Access Control
		init_sequence[7'h26] = 9'h148;
		init_sequence[7'h27] = 9'h03a;
		init_sequence[7'h28] = 9'h155;
		init_sequence[7'h29] = 9'h0b1;
		init_sequence[7'h2a] = 9'h100;
		init_sequence[7'h2b] = 9'h118;
		init_sequence[7'h2c] = 9'h0b6;
		init_sequence[7'h2d] = 9'h108;
		init_sequence[7'h2e] = 9'h182;
		init_sequence[7'h2f] = 9'h127;
		init_sequence[7'h30] = 9'h0f2;
		init_sequence[7'h31] = 9'h100;
		init_sequence[7'h32] = 9'h026;
		init_sequence[7'h33] = 9'h101;
		init_sequence[7'h34] = 9'h0e0;
		init_sequence[7'h35] = 9'h10f;
		init_sequence[7'h36] = 9'h131;
		init_sequence[7'h37] = 9'h12b;
		init_sequence[7'h38] = 9'h10c;
		init_sequence[7'h39] = 9'h10e;
		init_sequence[7'h3a] = 9'h108;
		init_sequence[7'h3b] = 9'h14e;
		init_sequence[7'h3c] = 9'h1f1;
		init_sequence[7'h3d] = 9'h137;
		init_sequence[7'h3e] = 9'h107;
		init_sequence[7'h3f] = 9'h110;
		init_sequence[7'h40] = 9'h103;
		init_sequence[7'h41] = 9'h10e;
		init_sequence[7'h42] = 9'h109;
		init_sequence[7'h43] = 9'h100;
		init_sequence[7'h44] = 9'h0e1;
		init_sequence[7'h45] = 9'h100;
		init_sequence[7'h46] = 9'h10e;
		init_sequence[7'h47] = 9'h114;
		init_sequence[7'h48] = 9'h103;
		init_sequence[7'h49] = 9'h111;
		init_sequence[7'h4a] = 9'h107;
		init_sequence[7'h4b] = 9'h131;
		init_sequence[7'h4c] = 9'h1c1;
		init_sequence[7'h4d] = 9'h148;
		init_sequence[7'h4e] = 9'h108;
		init_sequence[7'h4f] = 9'h10f;
		init_sequence[7'h50] = 9'h10c;
		init_sequence[7'h51] = 9'h131;
		init_sequence[7'h52] = 9'h136;
		init_sequence[7'h53] = 9'h10f;
		init_sequence[7'h54] = 9'h011; // Sleep Out
		init_sequence[7'h55] = 9'h029; // Display ON
		init_sequence[7'h56] = 9'h036; // Memory Access Control
		init_sequence[7'h57] = 9'h108; //   Select BGR color filter
		init_sequence[7'h58] = 9'h02a; // Column Address Set
		init_sequence[7'h59] = 9'h100; //   Start column [15:8]
		init_sequence[7'h5a] = 9'h100; //   Start column [7:0]
		init_sequence[7'h5b] = 9'h100; //   End column [15:8]
		init_sequence[7'h5c] = 9'h1ef; //   End column [7:0]
		init_sequence[7'h5d] = 9'h02b; // Page Address Set
		init_sequence[7'h5e] = 9'h100; //   Start position [15:8]
		init_sequence[7'h5f] = 9'h100; //   Start position [7:0]
		init_sequence[7'h60] = 9'h101; //   End position   [15:8]
		init_sequence[7'h61] = 9'h13f; //   End position   [7:0]
		init_sequence[7'h62] = 9'h035; // Tearing Effect Line ON
		init_sequence[7'h63] = 9'h000; //   V-Blanking information only (set to 01h for both V-Blanking and H-Blanking information
		init_sequence[7'h64] = 9'h02c; // Memory Write
	end
	
	always @(negedge clk) begin
		if (rst) begin
			// The screen driver reset edition.
			lcd_we_reg   <= 0;
			
		end else if (state == state_rst) begin
			// Prepare init sequence things.
			lcd_we_reg   <= 1;
			init_counter <= 1;
			state        <= state_init;
			
			lcd_rs_reg   <= init_sequence[0][8];
			lcd_data_reg <= init_sequence[0][7:0];
			
		end else if (state == state_init) begin
			// Prepare next BOIT of init data.
			lcd_rs_reg   <= init_sequence[init_counter][8];
			lcd_data_reg <= init_sequence[init_counter][7:0];
			init_counter <= init_counter + 1;
			
			// Check length.
			if (init_counter == init_len) begin
				state    <= state_idle;
			end
			
		end else if (state == state_idle) begin
			// Nothing happens.
			lcd_we_reg   <= 0;
			
		end
	end
	
endmodule
