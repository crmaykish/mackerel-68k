`timescale 1ns / 1ps

module irq_encoder_tb;

reg irq1 = 0;
reg irq2 = 0;
reg irq3 = 0;
reg irq4 = 0;
reg irq5 = 0;
reg irq6 = 0;
reg irq7 = 0;
reg irq8 = 0;

wire ipl0, ipl1, ipl2;

initial begin
    $dumpfile("irq_encoder.vcd");
    $dumpvars(0, irq_encoder_tb);

    irq1 = 1;

    #100

    irq2 = 1;

    #100

    irq5 = 1;
    irq6 = 1;

    #100

    irq1 = 0;
    irq2 = 0;
    irq3 = 0;
    irq4 = 0;
    irq5 = 0;
    irq6 = 0;
    irq7 = 0;

    #100 $finish;
end

irq_encoder e1(
    irq1,
    irq2,
    irq3,
    irq4,
    irq5,
    irq6,
    irq7,
    ipl0,
    ipl1,
    ipl2
);

endmodule
