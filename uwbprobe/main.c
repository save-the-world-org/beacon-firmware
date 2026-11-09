// uwbprobe: a first firmware for the Qorvo DWM3001CDK (nRF52833 + DW3110 UWB radio + LIS2DH12).
//
// No SDK and no libc: registers by address, built with Apple clang, linked with GNU ld.
// It talks over the J-Link's USB serial port (UART0: TX P0.19, RX P0.15, 115200 8N1).
// Pins come from Zephyr's board files (boards/qorvo/decawave_dwm3001cdk).

#include <stdint.h>

#define REG(a) (*(volatile uint32_t *)(uintptr_t)(a))

/* ---------- timing: TIMER1 free-running at 1 MHz, reset ----------
 * Not the DWT cycle counter: its enable bit lives in DEMCR, which a debugger clears when it
 * detaches, and every delay would then wait forever. */

#define TIMER1    0x40009000u
#define SCB_AIRCR 0xE000ED0Cu

static void timer_init(void) {
    REG(TIMER1 + 0x504) = 0; /* MODE: timer */
    REG(TIMER1 + 0x508) = 3; /* BITMODE: 32 bit */
    REG(TIMER1 + 0x510) = 4; /* PRESCALER: 16 MHz / 2^4 = 1 MHz */
    REG(TIMER1 + 0x00C) = 1; /* TASKS_CLEAR */
    REG(TIMER1 + 0x000) = 1; /* TASKS_START */
}
static inline uint32_t micros(void) { /* wraps every 71 minutes */
    REG(TIMER1 + 0x040) = 1;    /* TASKS_CAPTURE[0] */
    return REG(TIMER1 + 0x540); /* CC[0] */
}
static void delay_us(uint32_t us) {
    uint32_t start = micros();
    while ((uint32_t)(micros() - start) < us) {
    }
}
static void delay_ms(uint32_t ms) {
    while (ms--) delay_us(1000);
}

static uint32_t ms_count, ms_mark;
static uint32_t millis(void) { /* call at least every 71 minutes */
    uint32_t steps = (uint32_t)(micros() - ms_mark) / 1000u;
    ms_count += steps;
    ms_mark += steps * 1000u;
    return ms_count;
}

/* ---------- clock ---------- */

#define CLOCK 0x40000000u
static int hfxo_start(void) {
    REG(CLOCK + 0x100) = 0; /* EVENTS_HFCLKSTARTED */
    REG(CLOCK + 0x000) = 1; /* TASKS_HFCLKSTART */
    uint32_t start = micros();
    while (!REG(CLOCK + 0x100)) {
        if ((uint32_t)(micros() - start) > 20000) return 0;
    }
    return 1;
}
static int hfxo_running(void) {
    uint32_t s = REG(CLOCK + 0x40C); /* HFCLKSTAT */
    return (s & 1u) && (s & (1u << 16));
}

/* ---------- GPIO ---------- */

#define P0           0x50000000u
#define P1           0x50000300u
#define GPIO_OUT     0x504
#define GPIO_OUTSET  0x508
#define GPIO_OUTCLR  0x50C
#define GPIO_IN      0x510
#define GPIO_PIN_CNF 0x700

#define PIN(port, n) (((port) << 5) | (n))

#define CNF_INPUT        0x00000000u /* input, no pull */
#define CNF_INPUT_PULLUP 0x0000000Cu
#define CNF_OUTPUT       0x00000001u
#define CNF_OD_PULLUP    0x0000060Du /* output, standard 0 / disconnect 1, pull-up: open drain */

static inline uint32_t port_base(uint32_t p) { return (p >> 5) ? P1 : P0; }
static inline uint32_t pin_bit(uint32_t p) { return 1u << (p & 31); }
static void pin_cfg(uint32_t p, uint32_t cnf) { REG(port_base(p) + GPIO_PIN_CNF + 4 * (p & 31)) = cnf; }
static inline void pin_set(uint32_t p) { REG(port_base(p) + GPIO_OUTSET) = pin_bit(p); }
static inline void pin_clr(uint32_t p) { REG(port_base(p) + GPIO_OUTCLR) = pin_bit(p); }
static inline int pin_read(uint32_t p) { return (REG(port_base(p) + GPIO_IN) & pin_bit(p)) != 0; }

/* LEDs are active low. */
static const uint8_t led_pins[4] = {PIN(0, 4), PIN(0, 14), PIN(0, 22), PIN(0, 5)};
static const char *const led_names[4] = {"D9 green", "D12 red", "D11 red", "D10 blue"};
static void led(int i, int on) {
    if (on)
        pin_clr(led_pins[i]);
    else
        pin_set(led_pins[i]);
}

#define BUTTON PIN(0, 2) /* SW2, active low; SW1 is P0.18, nRESET only when UICR says so */

/* ---------- UART0 to the J-Link's virtual COM port ---------- */

#define UART0  0x40002000u
#define TX_PIN PIN(0, 19)
#define RX_PIN PIN(0, 15)

static void uart_init(void) {
    pin_set(TX_PIN);
    pin_cfg(TX_PIN, CNF_OUTPUT);
    pin_cfg(RX_PIN, CNF_INPUT_PULLUP);
    REG(UART0 + 0x508) = 0xFFFFFFFFu; /* PSEL.RTS: unused */
    REG(UART0 + 0x510) = 0xFFFFFFFFu; /* PSEL.CTS: unused */
    REG(UART0 + 0x50C) = TX_PIN;      /* PSEL.TXD */
    REG(UART0 + 0x514) = RX_PIN;      /* PSEL.RXD */
    REG(UART0 + 0x524) = 0x01D7E000u; /* BAUDRATE 115200 */
    REG(UART0 + 0x56C) = 0;           /* CONFIG: no parity, no flow control */
    REG(UART0 + 0x500) = 4;           /* ENABLE */
    REG(UART0 + 0x11C) = 0;           /* EVENTS_TXDRDY */
    REG(UART0 + 0x108) = 0;           /* EVENTS_RXDRDY */
    REG(UART0 + 0x008) = 1;           /* TASKS_STARTTX */
    REG(UART0 + 0x000) = 1;           /* TASKS_STARTRX */
}

static void uart_putc(char c) {
    REG(UART0 + 0x11C) = 0;
    REG(UART0 + 0x51C) = (uint8_t)c;
    uint32_t start = micros();
    while (!REG(UART0 + 0x11C)) {
        if ((uint32_t)(micros() - start) > 1000) break; /* a byte takes 87 us */
    }
}

static int uart_getc(void) {
    if (REG(UART0 + 0x124)) { /* EVENTS_ERROR: overrun, framing, break */
        REG(UART0 + 0x124) = 0;
        REG(UART0 + 0x480) = REG(UART0 + 0x480); /* ERRORSRC: write 1 to clear */
    }
    if (!REG(UART0 + 0x108)) return -1;
    REG(UART0 + 0x108) = 0;
    return (int)(REG(UART0 + 0x518) & 0xFF);
}

static void put(const char *s) {
    for (; *s; s++) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s);
    }
}
static void put_hex(uint32_t v, int digits) {
    for (int i = digits - 1; i >= 0; i--) uart_putc("0123456789ABCDEF"[(v >> (4 * i)) & 0xF]);
}
static void put_udec(uint32_t v) {
    char b[10];
    int n = 0;
    do {
        b[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (n) uart_putc(b[--n]);
}
/* Sign, then the number right-aligned in width columns. */
static void put_sdec(int32_t v, int width) {
    uint32_t u = v < 0 ? (uint32_t)(-(v + 1)) + 1 : (uint32_t)v;
    int digits = 1;
    for (uint32_t t = u; t >= 10; t /= 10) digits++;
    for (int i = digits + 1; i < width; i++) uart_putc(' ');
    uart_putc(v < 0 ? '-' : '+');
    put_udec(u);
}

/* ---------- SPI to the DW3110 (bit-banged, mode 0, MSB first, ~250 kHz) ---------- */

#define DW_SCK  PIN(0, 3)
#define DW_MOSI PIN(0, 8)
#define DW_MISO PIN(0, 29)
#define DW_CS   PIN(1, 6)

static void spi_init(void) {
    pin_set(DW_CS);
    pin_cfg(DW_CS, CNF_OUTPUT);
    pin_clr(DW_SCK);
    pin_cfg(DW_SCK, CNF_OUTPUT);
    pin_clr(DW_MOSI);
    pin_cfg(DW_MOSI, CNF_OUTPUT);
    pin_cfg(DW_MISO, CNF_INPUT);
}

static uint8_t spi_xfer(uint8_t out) {
    uint8_t in = 0;
    for (int i = 7; i >= 0; i--) {
        if (out & (1u << i))
            pin_set(DW_MOSI);
        else
            pin_clr(DW_MOSI);
        delay_us(1);
        pin_set(DW_SCK);
        delay_us(1);
        in = (uint8_t)((in << 1) | pin_read(DW_MISO));
        pin_clr(DW_SCK);
    }
    return in;
}

/* DW3000 SPI header: bit 7 write, bit 6 extended address, bits 5..1 register file, then the
 * sub-address over bit 0 and the next octet's bits 7..2. */
static uint32_t dw_read32(uint8_t file, uint8_t offset) {
    uint8_t hdr0, hdr1 = 0;
    int n;
    if (offset == 0) {
        hdr0 = (uint8_t)((file << 1) & 0x3E);
        n = 1;
    } else {
        hdr0 = (uint8_t)(0x40 | ((file << 1) & 0x3E) | ((offset >> 6) & 1));
        hdr1 = (uint8_t)(offset << 2);
        n = 2;
    }
    pin_clr(DW_CS);
    delay_us(2);
    spi_xfer(hdr0);
    if (n == 2) spi_xfer(hdr1);
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v |= (uint32_t)spi_xfer(0) << (8 * i);
    pin_set(DW_CS);
    delay_us(2);
    return v;
}

/* ---------- I2C to the LIS2DH12 (bit-banged open drain, ~100 kHz) ---------- */

#define I2C_SDA    PIN(0, 24)
#define I2C_SCL    PIN(1, 4)
#define ACCEL_ADDR 0x19

static void i2c_init(void) {
    pin_set(I2C_SDA);
    pin_set(I2C_SCL);
    pin_cfg(I2C_SDA, CNF_OD_PULLUP);
    pin_cfg(I2C_SCL, CNF_OD_PULLUP);
}
static void sda(int v) {
    if (v)
        pin_set(I2C_SDA);
    else
        pin_clr(I2C_SDA);
}
static void scl(int v) {
    if (v)
        pin_set(I2C_SCL);
    else
        pin_clr(I2C_SCL);
}
#define I2C_DELAY() delay_us(5)
static void i2c_start(void) {
    sda(1);
    scl(1);
    I2C_DELAY();
    sda(0);
    I2C_DELAY();
    scl(0);
    I2C_DELAY();
}
static void i2c_stop(void) {
    sda(0);
    I2C_DELAY();
    scl(1);
    I2C_DELAY();
    sda(1);
    I2C_DELAY();
}
static int i2c_write_byte(uint8_t b) { /* 1 when acknowledged */
    for (int i = 7; i >= 0; i--) {
        sda((b >> i) & 1);
        I2C_DELAY();
        scl(1);
        I2C_DELAY();
        scl(0);
    }
    sda(1);
    I2C_DELAY();
    scl(1);
    I2C_DELAY();
    int ack = !pin_read(I2C_SDA);
    scl(0);
    I2C_DELAY();
    return ack;
}
static uint8_t i2c_read_byte(int ack) {
    uint8_t b = 0;
    sda(1);
    for (int i = 7; i >= 0; i--) {
        I2C_DELAY();
        scl(1);
        I2C_DELAY();
        b = (uint8_t)((b << 1) | pin_read(I2C_SDA));
        scl(0);
    }
    sda(!ack);
    I2C_DELAY();
    scl(1);
    I2C_DELAY();
    scl(0);
    sda(1);
    I2C_DELAY();
    return b;
}
static int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, int n) {
    i2c_start();
    if (!i2c_write_byte((uint8_t)(addr << 1)) || !i2c_write_byte(reg)) {
        i2c_stop();
        return 0;
    }
    i2c_start(); /* repeated start */
    if (!i2c_write_byte((uint8_t)((addr << 1) | 1))) {
        i2c_stop();
        return 0;
    }
    for (int i = 0; i < n; i++) buf[i] = i2c_read_byte(i < n - 1);
    i2c_stop();
    return 1;
}
static int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    i2c_start();
    int ok = i2c_write_byte((uint8_t)(addr << 1)) && i2c_write_byte(reg) && i2c_write_byte(val);
    i2c_stop();
    return ok;
}

static int accel_on;
static int accel_enable(void) {
    accel_on = i2c_write_reg(ACCEL_ADDR, 0x23, 0x80)    /* CTRL_REG4: block data update, +-2 g */
               && i2c_write_reg(ACCEL_ADDR, 0x20, 0x57); /* CTRL_REG1: 100 Hz, normal mode, X Y Z */
    return accel_on;
}
static int accel_read_mg(int32_t xyz[3]) {
    uint8_t b[6];
    if (!i2c_read_reg(ACCEL_ADDR, 0x28 | 0x80, b, 6)) return 0; /* OUT_X_L, auto-increment */
    for (int i = 0; i < 3; i++) {
        int16_t raw = (int16_t)(b[2 * i] | (b[2 * i + 1] << 8));
        xyz[i] = (raw >> 6) * 4; /* normal mode: 10 bits, 4 mg a digit at +-2 g */
    }
    return 1;
}

/* ---------- die temperature ---------- */

#define TEMP 0x4000C000u
static int32_t temp_quarters(void) {
    REG(TEMP + 0x100) = 0; /* EVENTS_DATARDY */
    REG(TEMP + 0x000) = 1; /* TASKS_START */
    uint32_t start = micros();
    while (!REG(TEMP + 0x100)) {
        if ((uint32_t)(micros() - start) > 10000) break;
    }
    int32_t t = (int32_t)REG(TEMP + 0x508);
    REG(TEMP + 0x100) = 0;
    REG(TEMP + 0x004) = 1; /* TASKS_STOP */
    return t;
}

/* ---------- reports ---------- */

#define FICR 0x10000000u

static void show_uwb(void) {
    uint32_t id = dw_read32(0x00, 0x00); /* DEV_ID */
    put("  uwb radio  DEV_ID 0x");
    put_hex(id, 8);
    if ((id >> 16) == 0xDECA) {
        put(" (Qorvo DW3000 family: model ");
        put_hex((id >> 8) & 0xFF, 2);
        put(", version ");
        put_hex((id >> 4) & 0xF, 1);
        put(", revision ");
        put_hex(id & 0xF, 1);
        put(")\n");
    } else {
        put(" (no answer: a DW3000 says 0xDECA03xx)\n");
    }
}

static void show_accel_id(void) {
    uint8_t who = 0;
    put("  accel      ");
    if (i2c_read_reg(ACCEL_ADDR, 0x0F, &who, 1)) {
        put("I2C 0x19 WHO_AM_I 0x");
        put_hex(who, 2);
        put(who == 0x33 ? " (LIS2DH12)" : " (not a LIS2DH12)");
        put(accel_on ? ", on at 100 Hz\n" : ", off\n");
    } else {
        put("no answer at I2C 0x19\n");
    }
}

static void show_id(void) {
    uint32_t variant = REG(FICR + 0x104);
    put("  chip       nRF52833 variant ");
    for (int i = 3; i >= 0; i--) uart_putc((char)((variant >> (8 * i)) & 0xFF));
    put(", package 0x");
    put_hex(REG(FICR + 0x108), 4);
    put(", ");
    put_udec(REG(FICR + 0x110));
    put(" KB flash, ");
    put_udec(REG(FICR + 0x10C));
    put(" KB RAM\n");

    put("  device id  ");
    put_hex(REG(FICR + 0x064), 8);
    put_hex(REG(FICR + 0x060), 8);
    put("\n");

    uint32_t a0 = REG(FICR + 0x0A4), a1 = REG(FICR + 0x0A8);
    uint32_t bytes[6] = {((a1 >> 8) & 0xFF) | 0xC0, a1 & 0xFF, a0 >> 24, (a0 >> 16) & 0xFF, (a0 >> 8) & 0xFF,
                         a0 & 0xFF};
    put("  ble addr   ");
    for (int i = 0; i < 6; i++) {
        put_hex(bytes[i], 2);
        if (i < 5) uart_putc(':');
    }
    put((REG(FICR + 0x0A0) & 1) ? " (random static)\n" : " (public)\n");

    put("  clock      64 MHz, ");
    put(hfxo_running() ? "from the 32 MHz crystal\n" : "from the internal RC oscillator\n");

    show_uwb();
    show_accel_id();
}

static void show_temp(void) {
    int32_t q = temp_quarters();
    put("  die temp   ");
    if (q < 0) {
        uart_putc('-');
        q = -q;
    }
    put_udec((uint32_t)q / 4);
    uart_putc('.');
    put((const char *[]){"00", "25", "50", "75"}[q % 4]);
    put(" C\n");
}

static void show_accel(void) {
    int32_t v[3];
    if (!accel_read_mg(v)) {
        put("  the accelerometer did not answer\n");
        return;
    }
    put("  accel  x ");
    put_sdec(v[0], 6);
    put("  y ");
    put_sdec(v[1], 6);
    put("  z ");
    put_sdec(v[2], 6);
    put("  mg\n");
}

/* ---------- shell ---------- */

static int heartbeat = 1;

static int streq(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static int split(char *s, char *argv[], int max) {
    int n = 0;
    while (*s && n < max) {
        while (*s == ' ') *s++ = 0;
        if (!*s) break;
        argv[n++] = s;
        while (*s && *s != ' ') s++;
    }
    return n;
}

static void cmd_help(void) {
    put("commands:\n"
        "  id               chip, radio and sensor IDs\n"
        "  led N on|off     N: 0 D9 green, 1 D12 red, 2 D11 red, 3 D10 blue, all\n"
        "  uwb              read the DW3110's DEV_ID over SPI\n"
        "  accel            read the accelerometer once, in mg\n"
        "  watch            stream the accelerometer until a key is pressed\n"
        "  temp             nRF52833 die temperature\n"
        "  uptime           milliseconds since reset\n"
        "  reset            restart the nRF52833\n");
}

static void cmd_led(int argc, char *argv[]) {
    if (argc != 3 || !(streq(argv[2], "on") || streq(argv[2], "off"))) {
        put("  usage: led 0|1|2|3|all on|off\n");
        return;
    }
    int on = streq(argv[2], "on");
    int first, last;
    if (streq(argv[1], "all")) {
        first = 0;
        last = 3;
    } else if (argv[1][0] >= '0' && argv[1][0] <= '3' && argv[1][1] == 0) {
        first = last = argv[1][0] - '0';
    } else {
        put("  no LED ");
        put(argv[1]);
        put("\n");
        return;
    }
    if (heartbeat && first == 0) {
        heartbeat = 0;
        put("  (heartbeat on D9 stopped)\n");
    }
    for (int i = first; i <= last; i++) {
        led(i, on);
        put("  ");
        put(led_names[i]);
        put(on ? " on\n" : " off\n");
    }
}

static void cmd_watch(void) {
    put("  10 readings a second; press any key to stop\n");
    for (;;) {
        show_accel();
        for (int i = 0; i < 10; i++) {
            delay_ms(10);
            if (uart_getc() >= 0) return;
        }
        millis();
    }
}

static void run(char *line) {
    char *argv[4];
    int argc = split(line, argv, 4);
    if (argc == 0) return;
    if (streq(argv[0], "help") || streq(argv[0], "?"))
        cmd_help();
    else if (streq(argv[0], "id"))
        show_id();
    else if (streq(argv[0], "led"))
        cmd_led(argc, argv);
    else if (streq(argv[0], "uwb"))
        show_uwb();
    else if (streq(argv[0], "accel"))
        show_accel();
    else if (streq(argv[0], "watch"))
        cmd_watch();
    else if (streq(argv[0], "temp"))
        show_temp();
    else if (streq(argv[0], "uptime")) {
        put("  ");
        put_udec(millis());
        put(" ms\n");
    } else if (streq(argv[0], "reset")) {
        put("  restarting\n");
        delay_ms(5);
        REG(SCB_AIRCR) = 0x05FA0004u; /* SYSRESETREQ */
        for (;;) {
        }
    } else {
        put("  unknown command: ");
        put(argv[0]);
        put(" (try help)\n");
    }
}

static char line[64];
static int line_len;
static void prompt(void) {
    put("uwbprobe> ");
    for (int i = 0; i < line_len; i++) uart_putc(line[i]);
}

int main(void) {
    timer_init();
    hfxo_start();
    for (int i = 0; i < 4; i++) {
        pin_set(led_pins[i]);
        pin_cfg(led_pins[i], CNF_OUTPUT);
    }
    pin_cfg(BUTTON, CNF_INPUT_PULLUP);
    uart_init();
    spi_init();
    i2c_init();
    accel_enable();

    for (int round = 0; round < 2; round++) {
        for (int i = 0; i < 4; i++) {
            led(i, 1);
            delay_ms(90);
            led(i, 0);
        }
    }

    put("\n\nuwbprobe 0.1 on the DWM3001CDK, over the J-Link serial port\n");
    show_id();
    show_temp();
    put("type help\n");
    prompt();

    uint32_t hb_next = 0;
    int hb_on = 0, last_cr = 0;
    int btn_stable = 1, btn_last = 1;
    uint32_t btn_since = 0;
    for (;;) {
        uint32_t t = millis();
        if (heartbeat && (int32_t)(t - hb_next) >= 0) {
            hb_on = !hb_on;
            led(0, hb_on);
            hb_next = t + (hb_on ? 60 : 940);
        }

        int b = pin_read(BUTTON);
        if (b != btn_last) {
            btn_last = b;
            btn_since = t;
        } else if (b != btn_stable && t - btn_since >= 30) {
            btn_stable = b;
            led(3, !b);
            put(b ? "\n  button SW2 up\n" : "\n  button SW2 down\n");
            prompt();
        }

        int c = uart_getc();
        if (c < 0) continue;
        if (c == '\r' || c == '\n') {
            if (c == '\n' && last_cr) {
                last_cr = 0;
                continue;
            }
            last_cr = c == '\r';
            put("\n");
            line[line_len] = 0;
            run(line);
            line_len = 0;
            prompt();
        } else {
            last_cr = 0;
            if (c == 0x7F || c == 0x08) {
                if (line_len) {
                    line_len--;
                    put("\b \b");
                }
            } else if (c >= ' ' && c < 0x7F && line_len < (int)sizeof line - 1) {
                line[line_len++] = (char)c;
                uart_putc((char)c);
            }
        }
    }
}

/* ---------- startup ---------- */

extern uint32_t _sidata[], _sdata[], _edata[], _sbss[], _ebss[], _estack[];

void Reset_Handler(void) {
    volatile uint32_t *src = _sidata, *dst = _sdata;
    while (dst < _edata) *dst++ = *src++;
    for (dst = _sbss; dst < _ebss;) *dst++ = 0;
    main();
    for (;;) {
    }
}

void Default_Handler(void) {
    for (;;) {
    }
}

/* A fault blinks both red LEDs fast. */
void HardFault_Handler(void) {
    pin_cfg(led_pins[1], CNF_OUTPUT);
    pin_cfg(led_pins[2], CNF_OUTPUT);
    for (;;) {
        led(1, 1);
        led(2, 1);
        for (volatile uint32_t i = 0; i < 800000; i++) {
        }
        led(1, 0);
        led(2, 0);
        for (volatile uint32_t i = 0; i < 800000; i++) {
        }
    }
}

typedef void (*vector_t)(void);

__attribute__((section(".isr_vector"), used)) const vector_t vectors[16 + 48] = {
    (vector_t)(uintptr_t)_estack,
    Reset_Handler,
    Default_Handler,   /* NMI */
    HardFault_Handler, /* HardFault */
    Default_Handler,   /* MemManage */
    Default_Handler,   /* BusFault */
    Default_Handler,   /* UsageFault */
    0,
    0,
    0,
    0,
    Default_Handler, /* SVCall */
    Default_Handler, /* DebugMonitor */
    0,
    Default_Handler, /* PendSV */
    Default_Handler, /* SysTick */
    [16 ... 63] = Default_Handler,
};

/* The compiler may call these; volatile keeps it from turning them into calls to themselves. */
void *memset(void *d, int c, __SIZE_TYPE__ n) {
    volatile uint8_t *p = d;
    while (n--) *p++ = (uint8_t)c;
    return d;
}
void *memcpy(void *d, const void *s, __SIZE_TYPE__ n) {
    volatile uint8_t *p = d;
    const volatile uint8_t *q = s;
    while (n--) *p++ = *q++;
    return d;
}
void __aeabi_memset(void *d, __SIZE_TYPE__ n, int c) { memset(d, c, n); }
void __aeabi_memset4(void *d, __SIZE_TYPE__ n, int c) { memset(d, c, n); }
void __aeabi_memclr(void *d, __SIZE_TYPE__ n) { memset(d, 0, n); }
void __aeabi_memclr4(void *d, __SIZE_TYPE__ n) { memset(d, 0, n); }
void __aeabi_memcpy(void *d, const void *s, __SIZE_TYPE__ n) { memcpy(d, s, n); }
void __aeabi_memcpy4(void *d, const void *s, __SIZE_TYPE__ n) { memcpy(d, s, n); }
