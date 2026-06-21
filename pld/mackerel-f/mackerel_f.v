// Mackerel-F Top Level SoC Module
module mackerel_f (
    input clk_27, // 27 MHz oscillator, pin 4

    output [5:0] led, // 6 onboard LEDs

    input uart_rx,
    output uart_tx,

    // SD card
    output cs_spi_sd,
    output sd_mosi,
    output sd_sck,
    input sd_miso,

    // NIC is on a second SPI master
    output cs_spi_nic,
    output nic_mosi,
    output nic_sck,
    input nic_miso,
    input nic_int,

    // SDRAM (Tang Nano 20k in-package SiP; pins auto-routed by name, no .cst)
    output O_sdram_clk,
    output O_sdram_cke,
    output O_sdram_cs_n,
    output O_sdram_cas_n,
    output O_sdram_ras_n,
    output O_sdram_wen_n,
    output [3:0]  O_sdram_dqm,
    output [10:0] O_sdram_addr,
    output [1:0]  O_sdram_ba,
    inout  [31:0] IO_sdram_dq
);

    // PLL: 27 MHz oscillator -> 75.6 MHz SoC clock (+ phase-shifted SDRAM chip clock)
    wire clk_soc;
    wire clk_soc_ps;
    wire pll_lock;
    pll soc_pll(.clk_in(clk_27), .clk_out(clk_soc), .clk_out_ps(clk_soc_ps), .locked(pll_lock));

    // Reset
    reg [23:0] rst = 24'd0;
    wire rst_cpu = ~rst[23];

    always @(posedge clk_soc) begin
        if (!pll_lock) rst <= 24'd0;   // power-on reset, held until the PLL locks
        else if (rst_cpu) rst <= rst + 1'b1;
    end

    // Two-phase clock: fx68k enables on alternate clk_soc cycles -> CPU = clk_soc/2 = 37.8 MHz
    reg phase = 1'b0;
    always @(posedge clk_soc) phase <= ~phase;
    wire enPhi1 = (phase == 1'b1);
    wire enPhi2 = (phase == 1'b0);

    // System Buses
    wire [23:1] ADDR_BUS;
    wire [15:0] DATA_BUS_OUT;
    reg [15:0] DATA_BUS_IN = 16'h0;

    // Control Lines
    wire RWn, ASn, LDSn, UDSn, FC0, FC1, FC2;
    wire DTACKn;
    wire BERRn;
    wire VPAn;
    wire IPL0n, IPL1n, IPL2n;

    // CPU Core
    fx68k cpu(
        .extReset(rst_cpu),
        .pwrUp(rst_cpu),
        .HALTn(1'b1),

        .clk(clk_soc),
        .enPhi1(enPhi1),
        .enPhi2(enPhi2),

        .eab(ADDR_BUS),
        .iEdb(DATA_BUS_IN),
        .oEdb(DATA_BUS_OUT),

        .eRWn(RWn),
        .ASn(ASn),
        .LDSn(LDSn),
        .UDSn(UDSn),

        .DTACKn(DTACKn),

        .FC0(FC0),
        .FC1(FC1),
        .FC2(FC2),

        .IPL0n(IPL0n),
        .IPL1n(IPL1n),
        .IPL2n(IPL2n),

        .VPAn(VPAn),
        .BERRn(BERRn),
        .BRn(1'b1),
        .BGACKn(1'b1)
    );

    // BOOT Signal (Map ROM to 0x0000 temporarily on reset)
    wire BOOT;
    boot_signal bs(~rst_cpu, ASn, BOOT);

    // Memory Map
    //  SDRAM        0x000000-0x7FFFFF  8 MB system RAM (sdram.v adapter + sdram_nano.v controller)
    //  (unmapped)   0x800000-0xFEFFFF  Nothing - BERR will trigger
    //  BSRAM        0xFF0000-0xFF7FFF  32 KB onboard BSRAM
    //  ROM          0xFF8000-0xFFF7FF  30 KB
    //  Peripherals  0xFFF800-0xFFFFFF  2 KB    8 slots x 256 B:
    //                 slot 0  0xFFF800  GPIO
    //                 slot 1  0xFFF900  UART
    //                 slot 2  0xFFFA00  Timer (system tick)
    //                 slot 3-7          reserved
    wire [23:0] address = {ADDR_BUS, 1'b0};   // ADDR_BUS has no bit 0, add it for convenient math

    wire in_periph = &address[23:11];                 // top 2 KB
    wire in_rom    = (&address[23:15]) && ~in_periph;  // top 32 KB minus top 2 KB
    wire in_ram    = ~(&address[23:15]);               // everything below 0xFF8000
    wire in_sdram  = in_ram && ~address[23];           // low 8 MB (0x000000-0x7FFFFF) - SDRAM system RAM
    wire in_bsram  = (address[23:15] == 9'h1FE);       // 0xFF0000-0xFF7FFF - BSRAM
    wire [2:0] periph_sel = address[10:8];             // 1 of 8 peripheral slots

    // CPU space (FC = 111): the 68000 drives this only for interrupt-acknowledge.
    // Gate the peripheral decode on it so an IACK (address 0xFFFFFx) can't alias
    // slot 7, and terminate the IACK with VPAn (autovector) instead of DTACK.
    wire cpu_space = FC0 && FC1 && FC2;
    wire iack      = cpu_space && ~ASn;

    // Boot shadow maps ROM into low memory until BOOT asserts, so the CPU fetches
    // the reset vector from ROM at 0x0; afterward SDRAM owns 0x0.
    wire cs_rom_n    = ~(~ASn && (~BOOT || in_rom));
    wire cs_sdram_n  = ~(~ASn &&  BOOT && in_sdram);
    wire cs_bsram_n  = ~(~ASn &&  BOOT && in_bsram);
    wire cs_periph_n = ~(~ASn &&  BOOT && in_periph && ~cpu_space);

    wire cs_gpio_n  = ~(~cs_periph_n && periph_sel == 3'd0);
    wire cs_uart_n  = ~(~cs_periph_n && periph_sel == 3'd1);
    wire cs_timer_n = ~(~cs_periph_n && periph_sel == 3'd2);
    wire cs_spi_n   = ~(~cs_periph_n && periph_sel == 3'd3);
    wire cs_spi2_n  = ~(~cs_periph_n && periph_sel == 3'd4);
    wire cs_intc_n  = ~(~cs_periph_n && periph_sel == 3'd5);

    // ROM (16K x 16 = 32 KB)
    reg [15:0] rom [0:16383];
    reg [15:0] rom_out;
    // Preload the ROM with a hex file
    initial $readmemh("rom.hex", rom);
    always @(posedge clk_soc) rom_out <= rom[ADDR_BUS[14:1]];

    // BSRAM (16k x 16 = 32 KB)
    reg [7:0] bsram_hi [0:16383];
    reg [7:0] bsram_lo [0:16383];
    reg [15:0] bsram_out;
    always @(posedge clk_soc) begin
        if (~cs_bsram_n && ~RWn && ~UDSn) bsram_hi[ADDR_BUS[14:1]] <= DATA_BUS_OUT[15:8];
        if (~cs_bsram_n && ~RWn && ~LDSn) bsram_lo[ADDR_BUS[14:1]] <= DATA_BUS_OUT[7:0];
        bsram_out <= {bsram_hi[ADDR_BUS[14:1]], bsram_lo[ADDR_BUS[14:1]]};
    end


    // GPIO
    reg [7:0] gpio = 8'b0;
    always @(posedge clk_soc) begin
        // GPIO writes
        if (~cs_gpio_n && ~RWn && ~UDSn) gpio <= DATA_BUS_OUT[15:8];
    end

    // Simple interrupt masking register
    reg [7:0] intc = 8'b0;
    always @(posedge clk_soc) begin
        if (~cs_intc_n && ~RWn && ~UDSn) intc <= DATA_BUS_OUT[15:8];
    end

    // UART
    wire irq_uart;
    wire dtack_uart;
    wire [7:0] uart_dout;

    uart u(
        .clk(clk_soc),
        .rst_n(~rst_cpu),

        .cs_n(cs_uart_n),
        .reg_addr(ADDR_BUS[3:1]),
        .rwn(RWn),
        .ds_n(UDSn),
        .data_in(DATA_BUS_OUT[15:8]),
        .data_out(uart_dout),
        .dtack_n(dtack_uart),
        .irq(irq_uart),

        .rx(uart_rx),
        .tx(uart_tx)
    );

    // Timer (slot 2 @ 0xFFFA00) -- programmable system-tick interrupt source.
    // CLK_HZ defaults to 75.6 MHz (= clk_soc), so the tick rates divide exactly.
    wire       irq_timer;
    wire       dtack_timer;
    wire [7:0] timer_dout;

    timer tmr(
        .clk(clk_soc),
        .rst_n(~rst_cpu),

        .cs_n(cs_timer_n),
        .reg_addr(ADDR_BUS[3:1]),
        .rwn(RWn),
        .ds_n(UDSn),
        .data_in(DATA_BUS_OUT[15:8]),
        .data_out(timer_dout),
        .dtack_n(dtack_timer),
        .irq(irq_timer)
    );

    // SPI Master
    wire irq_spi;
    wire dtack_spi;
    wire [7:0] spi_dout;
    wire [7:0] spi_data_in = ~LDSn ? DATA_BUS_OUT[7:0] : DATA_BUS_OUT[15:8];

    spi sp(
        .clk(clk_soc),
        .rst_n(~rst_cpu),

        .cs_n(cs_spi_n),
        .reg_addr(ADDR_BUS[4:2]),
        .rwn(RWn),
        .ds_n(UDSn),
        .data_in(spi_data_in),
        .data_out(spi_dout),
        .dtack_n(dtack_spi),
        .irq(irq_spi),

        .mosi(sd_mosi),
        .sck(sd_sck),
        .miso(sd_miso)
    );

    // SPI Master 2 (NIC)
    wire irq_spi2;
    wire dtack_spi2;
    wire [7:0] spi2_dout;

    spi sp2(
        .clk(clk_soc),
        .rst_n(~rst_cpu),

        .cs_n(cs_spi2_n),
        .reg_addr(ADDR_BUS[4:2]),
        .rwn(RWn),
        .ds_n(UDSn),
        .data_in(spi_data_in),
        .data_out(spi2_dout),
        .dtack_n(dtack_spi2),
        .irq(irq_spi2),

        .mosi(nic_mosi),
        .sck(nic_sck),
        .miso(nic_miso)
    );

    // NIC interrupt
    wire irq_nic = ~nic_int & intc[4];

    // Interrupt map (mirrors Mackerel-30's irq_encoder): priority-encode the
    // sources onto the 68000 IPL pins. Timer = level 6, UART = level 5, NIC = level 4.
    irq_encoder ie(
        .irq1(1'b0),
        .irq2(1'b0),
        .irq3(1'b0),
        .irq4(irq_nic),     // W5500 -> IPL 4
        .irq5(irq_uart),    // UART  -> IPL 5
        .irq6(irq_timer),   // Timer -> IPL 6
        .irq7(1'b0),
        .ipl0_n(IPL0n),
        .ipl1_n(IPL1n),
        .ipl2_n(IPL2n)
    );

    // Autovector every interrupt-acknowledge cycle: VPAn asserted during CPU
    // space -> the CPU takes autovector 24 + level (timer = 30, UART = 29).
    assign VPAn = ~iack;

    // SDRAM Read Cache
    wire [15:0] sdram_dout;       // CPU-side read data (from the cache)
    wire        dtack_sdram;      // CPU-side DTACK (from the cache)
    wire        cache_mem_cs_n;   // cache -> adapter chip-select
    wire [15:0] mem_rdata_w;      // adapter -> cache: selected 16-bit word
    wire [31:0] mem_rdata32_w;    // adapter -> cache: full 32-bit line
    wire        mem_dtack_w;      // adapter -> cache: DTACK

    sdram_cache cache(
        .clk(clk_soc),
        .rst_n(pll_lock),
        .cs_n(cs_sdram_n),
        .addr(ADDR_BUS[22:1]),
        .rwn(RWn),
        .udsn(UDSn),
        .ldsn(LDSn),
        .rdata(sdram_dout),
        .dtack_n(dtack_sdram),
        .mem_cs_n(cache_mem_cs_n),
        .mem_rdata(mem_rdata_w),
        .mem_rdata32(mem_rdata32_w),
        .mem_dtack_n(mem_dtack_w)
    );

    sdram_controller s(
        .clk(clk_soc),
        .clk_ps(clk_soc_ps),
        .rst_n(pll_lock),          // inits independently of the CPU reset counter

        .cs_n(cache_mem_cs_n),
        .addr(ADDR_BUS[22:1]),
        .rwn(RWn),
        .udsn(UDSn),
        .ldsn(LDSn),
        .wdata(DATA_BUS_OUT),
        .rdata(mem_rdata_w),
        .rdata32(mem_rdata32_w),
        .dtack_n(mem_dtack_w),
        .init_done(),              // reserved: gate reset until SDRAM init done (when SDRAM is system RAM)

        .O_sdram_clk(O_sdram_clk),
        .O_sdram_cke(O_sdram_cke),
        .O_sdram_cs_n(O_sdram_cs_n),
        .O_sdram_cas_n(O_sdram_cas_n),
        .O_sdram_ras_n(O_sdram_ras_n),
        .O_sdram_wen_n(O_sdram_wen_n),
        .O_sdram_dqm(O_sdram_dqm),
        .O_sdram_addr(O_sdram_addr),
        .O_sdram_ba(O_sdram_ba),
        .IO_sdram_dq(IO_sdram_dq)
    );

    // DTACK Generation:
    //   No wait states for ROM and GPIO
    //   SDRAM, UART, timer, and SPI controller all provide their own DTACK
    //   Any other access keeps DTACK de-asserted so BERR will trigger
    assign DTACKn = iack        ? 1'b1 :
                    ~cs_rom_n   ? 1'b0 :
                    ~cs_bsram_n ? 1'b0 :
                    ~cs_gpio_n  ? 1'b0 :
                    ~cs_sdram_n ? dtack_sdram :
                    ~cs_uart_n  ? dtack_uart  :
                    ~cs_timer_n ? dtack_timer :
                    ~cs_spi_n   ? dtack_spi   :
                    ~cs_spi2_n  ? dtack_spi2  :
                    ~cs_intc_n  ? 1'b0        :
                                  1'b1;

    // Bus Watchdog - if a bus cycles fails to DTACK within a reasonable time (4096 clock cycles), assert BERR
    bus_watchdog #(.TIMEOUT(4096)) wd(
        .clk(clk_soc),
        .rst_n(~rst_cpu),
        .as_n(ASn),
        .dtack_n(DTACKn),
        .vpa_n(VPAn),
        .berr_n(BERRn)
    );

    // Databus Input Mux - map the correct memory/peripheral data bus to the CPU on read cycles
    always @(*) begin
        if (~cs_rom_n) DATA_BUS_IN = rom_out;
        else if (~cs_bsram_n) DATA_BUS_IN = bsram_out;
        else if (~cs_sdram_n) DATA_BUS_IN = sdram_dout;
        else if (~cs_gpio_n) DATA_BUS_IN = {gpio, 8'h00};        // Pad 8-bit peripherals with LDS of 0
        else if (~cs_uart_n) DATA_BUS_IN = {uart_dout, 8'h00};   // "
        else if (~cs_timer_n) DATA_BUS_IN = {timer_dout, 8'h00}; // "
        else if (~cs_spi_n) DATA_BUS_IN = {spi_dout, 8'h00};     // "
        else if (~cs_spi2_n) DATA_BUS_IN = {spi2_dout, 8'h00};   // "
        else if (~cs_intc_n) DATA_BUS_IN = {intc, 8'h00};        // "
        else DATA_BUS_IN = 16'h0000;
    end

    // Debug LEDs (active low)
    assign led = ~gpio[5:0];

    // Use the GPIO register as SPI chip-selects
    assign cs_spi_sd = ~gpio[6];        // SD Card
    assign cs_spi_nic = ~gpio[7];       // Network interface (W5500)

endmodule
