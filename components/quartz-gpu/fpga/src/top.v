
// module top (
//     input  wire      clk_in,
//     input  wire      irq_n,
    
//     input  wire      spi_mosi,
//     output wire      spi_miso,
//     input  wire      spi_clk,
//     input  wire      spi_cs_n,
    
//     input  wire[7:0] pmod,
//     output wire      red_n,
//     output wire      green_n,
//     output wire      blue_n,
    
//     inout  wire[3:0] ram_io,
//     output wire      ram_clk,
//     output wire      ram_cs_n
// );
    
//     // Slow command.
//     localparam cmd_slow  = 0;
//     // Fast command.
//     localparam cmd_fast  = 1;
//     // Dump to RAM mode.
//     localparam cmd_ramst = 2;
//     // Read from RAM mode.
//     localparam cmd_ramld = 3;
    
//     reg[1:0] last_cmd;
    
//     reg      fast_cap_clk;
//     reg[15:0] clkdiv;
//     reg[7:0] spi_recv;
//     reg[7:0] spi_send;
    
//     assign spi_miso = spi_cs_n ? 'bz : spi_send[7];
    
//     assign red_n   = last_cmd != cmd_fast && last_cmd != cmd_ramst;
//     assign green_n = last_cmd != cmd_slow && last_cmd != cmd_ramst;
//     assign blue_n  = last_cmd != cmd_ramld;
    
//     // Sample buffer.
//     reg[7:0]   sample_buf[15:0];
//     reg[3:0]   sample_index;
    
//     initial begin
//         spi_cs_n_last = 1;
//         spi_bit       = 0;
//         clkdiv        = 1;
//         last_cmd = cmd_slow;
//     end
    
//     wire cap_clk = (last_cmd == cmd_fast) ? fast_cap_clk : !spi_cs_n;
//     always @(posedge cap_clk) begin
//         // Capture sample at every SPI transfer.
//         sample_buf[sample_index] <= pmod;
//         sample_index <= sample_index + 1;
//     end
    
//     always @(posedge clk_in) begin
//         // Dead simple clk div.
//         if (clkdiv == 0) begin
//             // clkdiv <= 1367;
//             clkdiv <= 500;
//             fast_cap_clk <= ~fast_cap_clk;
//         end else if (clkdiv) begin
//             clkdiv <= clkdiv - 1;
//         end
//     end
    
//     always @(negedge spi_cs_n) begin
//         // Capture command.
//         last_cmd <= spi_recv;
//     end
    
//     // SPI Send.
//     reg       spi_cs_n_last;
//     reg[2:0]  spi_bit;
//     // reg[7:0]  spi_byte;
//     reg[3:0]  sample_read;// = sample_index+1+spi_byte;
//     wire      sck = spi_clk;
//     always @(posedge sck | spi_cs_n) begin
//         if (spi_cs_n) begin
//             // Prepare send data.
//             spi_bit  <= 0;
//             // spi_byte <= 0;
//             sample_read <= 0;
//             // First byte: Capture length.
//             spi_send <= sample_index;
//         end else if (sck) begin
//             if (spi_bit == 7) begin
//                 // More bytes: Sample data.
//                 spi_send <= sample_buf[sample_read];
                
//                 // spi_byte <= spi_byte + 1;
//                 sample_read <= sample_read + 1;
//             end else begin
//                 // Move the send data.
//                 spi_send[7:1] <= spi_send[6:0];
//                 spi_send[0]   <= 0;
//             end
//             spi_bit <= spi_bit + 1;
//         end
//         spi_cs_n_last = spi_cs_n;
//     end
    
//     // SPI Recv.
//     always @(posedge spi_clk) begin
//         if (spi_cs_n == 0) begin
//             // Receive data.
//             spi_recv[7:1] <= spi_recv[6:0];
//             spi_recv[0]   <= spi_mosi;
//         end
//     end
    
// endmodule
