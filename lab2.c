#define PROJECT 2

#include <stdio.h>
#include "system.h"
#include "alt_types.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_timestamp.h"

// debug pin toggle (1 to enable, 0 to disable)
#define DEBUG_PIN 1

// debug macros
#if DEBUG_PIN
#define DBG_HI() IOWR_ALTERA_AVALON_PIO_DATA(NIOS_HEADER_CONN_BASE, 1)
#define DBG_LO() IOWR_ALTERA_AVALON_PIO_DATA(NIOS_HEADER_CONN_BASE, 0)
#else
#define DBG_HI()
#define DBG_LO()
#endif

// macro to wait until timestamp reaches target tick count
#define WAIT_UNTIL(t) while ((alt_u32)alt_timestamp() < (t)) { }

// software UART configuration
#define BAUD_RATE 4800  // baud rate of UART
#define NBIT      8     // number of UART bits
#define PARITY    0     // parity bit enable 
#define NSTOPBIT  1     // number of stop bits

// PIO read / write macros
#define RX_READ()   (IORD_ALTERA_AVALON_PIO_DATA(NIOS_UARTRX_BASE) & 0x01)
#define TX_WRITE(v) IOWR_ALTERA_AVALON_PIO_DATA(NIOS_UARTTX_BASE, (v))

#if PROJECT == 1

int main(void) {
    printf("Hello from Nios II! Lab 2 is alive.\n");

	for(;;)
		;

    return 0;
}

#elif PROJECT == 2

// delay for project 2 square wave toggling (ticks)
#define PROJ_2_DELAY (10 * 1000)

int main(void) {
    alt_u32 t = 0;
    alt_timestamp_start();

    while (1) {
        DBG_HI();

        t += PROJ_2_DELAY;
        WAIT_UNTIL(t);

        DBG_LO();

        t += PROJ_2_DELAY;
        WAIT_UNTIL(t);
    }

    return 0;
}

#elif PROJECT == 4 || PROJECT == 9

// sample a single UART RX bit at bit time t
#define RX_BIT(i) do {                          \
    t += tbit;                                  \
    WAIT_UNTIL(t);                              \
    DBG_HI();                                   \
    if (RX_READ()) {                            \
        data |= (1U << (i));                    \
    }                                           \
    DBG_LO();                                   \
} while (0)

static alt_u32 tbit;
static alt_u32 thalf;

// receives a single UART frame via software bit-banging
static int uart_rx(unsigned int *out) {
    unsigned int data = 0;
    alt_u32 t;
    int err = 0;

    // wait for idle line (high) then wait for falling edge of START bit
    while (RX_READ() == 0) {
    }
    while (RX_READ() != 0) {
    }
    alt_timestamp_start();

    // sample START bit at half-bit time to verify validity
    t = thalf;
    WAIT_UNTIL(t);
    DBG_HI();
    if (RX_READ() != 0) {
        DBG_LO();
        return 1;
    }
    DBG_LO();

    // sample data bits
    for (int i = 0; i < NBIT; i++) {
        RX_BIT(i);
    }

#if PARITY != 0
    // check parity bit
    {
        unsigned int p, ones = 0, d = data;
        t += tbit;
        WAIT_UNTIL(t);
        DBG_HI();
        p = RX_READ();
        DBG_LO();
        while (d) {
            ones += d & 1;
            d >>= 1;
        }
        ones += p;
#if PARITY == 1
        if ((ones & 1) == 0) {
            err = 2;
        }
#else
        if ((ones & 1) != 0) {
            err = 2;
        }
#endif
    }
#endif

    // check STOP bit
    t += tbit;
    WAIT_UNTIL(t);
    DBG_HI();
    if (RX_READ() == 0) {
        err = 3;
    }
    DBG_LO();

#if NSTOPBIT >= 2
    // check second STOP bit
    t += tbit;
    WAIT_UNTIL(t);
    if (RX_READ() == 0) {
        err = 3;
    }
#endif

    *out = data;
    return err;
}

#if PROJECT == 9
// transmits a single character via software bit-banging
static void uart_tx(unsigned int c) {
    alt_u32 t = 0;
    int i, ones = 0;

    alt_timestamp_start();

    // send START bit
    TX_WRITE(0);

    // send data bits
    for (i = 0; i < NBIT; i++) {
        t += tbit;
        WAIT_UNTIL(t);
        TX_WRITE((c >> i) & 1);
        ones += (c >> i) & 1;
    }

#if PARITY != 0
    // send parity bit
    t += tbit;
    WAIT_UNTIL(t);
#if PARITY == 1
    TX_WRITE((ones & 1) ? 0 : 1);
#else
    TX_WRITE((ones & 1) ? 1 : 0);
#endif
#endif

    // send STOP bit(s)
    t += tbit;
    WAIT_UNTIL(t);
    TX_WRITE(1);
    t += tbit * NSTOPBIT;
    WAIT_UNTIL(t);
}
#endif

int main(void) {
    alt_u32 freq;
    unsigned int c;
    int err;
    static const char *msg[] = {
        "OK",
        "START error (glitch)",
        "PARITY error",
        "FRAMING error (STOP low)"
    };

#if PROJECT == 9
    TX_WRITE(1);
#endif
    DBG_LO();

    if (alt_timestamp_start() < 0) {
        printf("ERROR: no timestamp timer! Set timestamp_timer = timer_0 in BSP Editor.\n");
        while (1) {
        }
    }
    freq = alt_timestamp_freq();
    tbit = (freq + BAUD_RATE / 2) / BAUD_RATE;
    thalf = tbit / 2;

    printf("SW UART RX: %d baud, %d bit, parity %d, %d stop\n",
           BAUD_RATE, NBIT, PARITY, NSTOPBIT);
    printf("timer = %lu ticks/s, 1 bit = %lu ticks\n",
           (unsigned long)freq, (unsigned long)tbit);

    while (1) {
        err = uart_rx(&c);
        if (err == 0) {
            printf("got '%c'  ASCII %3u  (0x%02X)\n",
                   (c >= 32 && c < 127) ? (char)c : '.', c, c);
        } else {
            printf("%s, data = 0x%02X\n", msg[err], c);
        }
#if PROJECT == 9
        if (err == 0) {
            uart_tx(c & ((1U << NBIT) - 1));
        }
#endif
    }

    return 0;
}

#else
#error "PROJECT must be 1, 2, 4, or 9"
#endif
