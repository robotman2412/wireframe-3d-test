
module top (
    input  wire      clk_in,
    input  wire      irq_n,
    
    input  wire      spi_mosi,
    output wire      spi_miso,
    input  wire      spi_clk,
    input  wire      spi_cs_n,
    
    input  wire[7:0] pmod,
    output wire      red_n,
    output wire      green_n,
    output wire      blue_n,
    
    inout  wire[3:0] ram_io,
    output wire      ram_clk,
    output wire      ram_cs_n
);
    
    reg[7:0] spi_recv;
    reg[7:0] spi_send;
    
    assign spi_miso = spi_cs_n ? 'bz : spi_send[7];
    
    assign red_n   = 1;
    assign green_n = spi_cs_n;
    assign blue_n  = 1;
    
    // Sample buffer.
    reg[7:0]   sample_buf[15:0];
    reg[3:0]   sample_index;
    
    initial begin
        spi_cs_n_last = 1;
        spi_bit       = 0;
        spi_byte      = 0;
        clkdiv        = 1;
        last_cmd = cmd_slow;
        
        // Debug buffer.
        spi_tx_avl <= 9;
        spi_tx_buf[3] = 1;
        spi_tx_buf[4] = 1;
        spi_tx_buf[5] = 0;
        spi_tx_buf[6] = 0;
        spi_tx_buf[7] = 'h04;
        spi_tx_buf[8] = 'h00;
    end
    
    always @(posedge clk_in) begin
        // TODO
    end
    
    // SPI Send.
    reg[7:0]  spi_tx_avl;
    reg[7:0]  spi_tx_buf[7:0];
    reg       spi_cs_n_last;
    reg[2:0]  spi_bit;
    reg[7:0]  spi_byte;
    always @(negedge spi_clk | spi_cs_n) begin
        if (spi_cs_n_last && !spi_cs_n) begin
            // Prepare send data.
            spi_bit  <= 0;
            spi_byte <= 0;
            // First byte: Idle.
            spi_send <= 00;
        end else if (!spi_clk) begin
            if (spi_bit == 7) begin
                // Check BUF has DATA.
                if (spi_tx_avl != spi_byte) begin
                    // Send more bytes.
                    spi_byte <= spi_byte + 1;
                    spi_send <= spi_tx_buf[spi_byte];
                end
                
            end else begin
                // Move the send data.
                spi_send[7:1] <= spi_send[6:0];
                spi_send[0]   <= 0;
            end
            
            // Increment bit position to keep track of bytes received.
            spi_bit <= spi_bit + 1;
        end
        spi_cs_n_last = spi_cs_n;
    end
    
    // SPI Recv.
    always @(posedge spi_clk) begin
        if (spi_cs_n == 0) begin
            // Receive data.
            spi_recv[7:1] <= spi_recv[6:0];
            spi_recv[0]   <= spi_mosi;
        end
    end
    
endmodule
