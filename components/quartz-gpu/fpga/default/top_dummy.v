
module top (
    input  wire      spi_cs_n,
    
    output wire      red_n,
    output wire      green_n,
    output wire      blue_n
);
    
    assign red_n   = 1;
    assign green_n = spi_cs_n;
    assign blue_n  = 1;
    
endmodule
