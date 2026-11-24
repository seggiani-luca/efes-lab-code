#define PROJECT 5

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

/*
 * UART device registers (16 bit):
 * OFFSET R/W REG
 * -------------------------------------
 * 0      RO  RXDATA (least sig. 8 bits)
 * 1      WO  TXDATA (least sig. 8 bits)
 * 2      RW  STATUS (see docs)
 * 3      RW  CONTROL (see docs)
 * 4      RW  DIVISOR (16 bits)
 */

// default clock frequency
#define F_CK (50 * 1000 * 1000)

// baud rate
#define BAUD_RATE 115200

// UART device 0 base
#define UART_0_BASE 0x8001060

// UART device 0 registers
#define UART_RXDATA  0
#define UART_TXDATA  1
#define UART_STATUS  2
#define UART_CONTROL 3
#define UART_DIVISOR 4

// UART device status bits
#define UART_STATUS_PARITY_ERROR       (1U << 0)
#define UART_STATUS_FRAME_ERROR        (1U << 1)
#define UART_STATUS_BREAK_ERROR        (1U << 2)
#define UART_STATUS_RX_OVERFLOW_ERROR  (1U << 3)
#define UART_STATUS_TX_OVERFLOW_ERROR  (1U << 4)
#define UART_STATUS_TX_EMPTY           (1U << 5)
#define UART_STATUS_TX_READY           (1U << 6)
#define UART_STATUS_RX_READY           (1U << 7)
#define UART_STATUS_ERROR              (1U << 8)

// UART control register bits
#define UART_CONTROL_IRQEN_PARITY      (1U << 0)
#define UART_CONTROL_IRQEN_FRAME       (1U << 1)
#define UART_CONTROL_IRQEN_BREAK       (1U << 2)
#define UART_CONTROL_IRQEN_RX_OVERFLOW (1U << 3)
#define UART_CONTROL_IRQEN_TX_OVERFLOW (1U << 4)
#define UART_CONTROL_IRQEN_TX_EMPTY    (1U << 5)
#define UART_CONTROL_IRQEN_TX_READY    (1U << 6)
#define UART_CONTROL_IRQEN_RX_READY    (1U << 7)
#define UART_CONTROL_IRQEN_ERROR       (1U << 8)

// macro to read register via direct memory access
#define READ_REG(base, offset, bits) (         \
    (*((volatile int *)(base) + (offset))) &   \
    ((bits) == 16 ? 0xFFFF : ((1U << (bits)) - 1)) \
)

// macro to write register via direct memory access
#define WRITE_REG(base, offset, value) (       \
    *((volatile int *)(base) + (offset)) = (value) \
)

// macro to get divisor value from baud rate
#define DIVISOR(f_ck, rate) (((f_ck) / (rate)) - 1)

// dumps all UART registers to console
// (basically the body of project 1)
void dump_uart(void) {
    int rxdata = READ_REG(UART_0_BASE, UART_RXDATA, 8);
    int status = READ_REG(UART_0_BASE, UART_STATUS, 9);
    int control = READ_REG(UART_0_BASE, UART_CONTROL, 9);
    int divisor = READ_REG(UART_0_BASE, UART_DIVISOR, 16);

    printf("RXDATA  [0x%02X]: 0x%04X\n", UART_RXDATA, rxdata);
    printf("STATUS  [0x%02X]: 0x%04X\n", UART_STATUS, status);
    printf("CONTROL [0x%02X]: 0x%04X\n", UART_CONTROL, control);
    printf("DIVISOR [0x%02X]: 0x%04X\n", UART_DIVISOR, divisor);

    printf("\nSTATUS:\n");
    printf("  PARITY_ERROR:      %d\n", !!(status & UART_STATUS_PARITY_ERROR));
    printf("  FRAME_ERROR:       %d\n", !!(status & UART_STATUS_FRAME_ERROR));
    printf("  BREAK_ERROR:       %d\n", !!(status & UART_STATUS_BREAK_ERROR));
    printf("  RX_OVERFLOW_ERROR: %d\n", !!(status & UART_STATUS_RX_OVERFLOW_ERROR));
    printf("  TX_OVERFLOW_ERROR: %d\n", !!(status & UART_STATUS_TX_OVERFLOW_ERROR));
    printf("  TX_EMPTY:          %d\n", !!(status & UART_STATUS_TX_EMPTY));
    printf("  TX_READY:          %d\n", !!(status & UART_STATUS_TX_READY));
    printf("  RX_READY:          %d\n", !!(status & UART_STATUS_RX_READY));
    printf("  ERROR:             %d\n", !!(status & UART_STATUS_ERROR));

    printf("\nCONTROL:\n");
    printf("  IRQEN_PARITY:      %d\n", !!(control & UART_CONTROL_IRQEN_PARITY));
    printf("  IRQEN_FRAME:       %d\n", !!(control & UART_CONTROL_IRQEN_FRAME));
    printf("  IRQEN_BREAK:       %d\n", !!(control & UART_CONTROL_IRQEN_BREAK));
    printf("  IRQEN_RX_OVERFLOW: %d\n", !!(control & UART_CONTROL_IRQEN_RX_OVERFLOW));
    printf("  IRQEN_TX_OVERFLOW: %d\n", !!(control & UART_CONTROL_IRQEN_TX_OVERFLOW));
    printf("  IRQEN_TX_EMPTY:    %d\n", !!(control & UART_CONTROL_IRQEN_TX_EMPTY));
    printf("  IRQEN_TX_READY:    %d\n", !!(control & UART_CONTROL_IRQEN_TX_READY));
    printf("  IRQEN_RX_READY:    %d\n", !!(control & UART_CONTROL_IRQEN_RX_READY));
    printf("  IRQEN_ERROR:       %d\n", !!(control & UART_CONTROL_IRQEN_ERROR));

    printf("\n");
}

void dump_status(int status) {
    printf("\nSTATUS:\n");
    printf("  PARITY_ERROR:      %d\n", !!(status & UART_STATUS_PARITY_ERROR));
    printf("  FRAME_ERROR:       %d\n", !!(status & UART_STATUS_FRAME_ERROR));
    printf("  BREAK_ERROR:       %d\n", !!(status & UART_STATUS_BREAK_ERROR));
    printf("  RX_OVERFLOW_ERROR: %d\n", !!(status & UART_STATUS_RX_OVERFLOW_ERROR));
    printf("  TX_OVERFLOW_ERROR: %d\n", !!(status & UART_STATUS_TX_OVERFLOW_ERROR));
    printf("  TX_EMPTY:          %d\n", !!(status & UART_STATUS_TX_EMPTY));
    printf("  TX_READY:          %d\n", !!(status & UART_STATUS_TX_READY));
    printf("  RX_READY:          %d\n", !!(status & UART_STATUS_RX_READY));
    printf("  ERROR:             %d\n", !!(status & UART_STATUS_ERROR));

    printf("\n");
}

#if PROJECT == 1

/*
 * Read the value of the registers:
 * - RXDATA
 * - STATUS
 * - CONTROL
 * - DIVISOR
 * Of the UART device. TXDATA is not needed as it is write-only, reading has no
 * meaning.
 */
int main(void) {
    dump_uart();

    return 0;
}

#elif PROJECT == 2

int main(void) {
    printf("Default config:\n");
    dump_uart();

    printf("Testing with baud rate = 115200\n");
    int baud_rate = 115200;
    int divisor = DIVISOR(F_CK, baud_rate);
    WRITE_REG(UART_0_BASE, UART_DIVISOR, divisor);
    dump_uart();

    printf("Testing with baud rate = 2400\n");
    baud_rate = 2400;
    divisor = DIVISOR(F_CK, baud_rate);
    WRITE_REG(UART_0_BASE, UART_DIVISOR, divisor);
    dump_uart();

    return 0;
}

#elif PROJECT == 3

// number of consecutive characters
#define NUM_CHARS 3

// decides if the program waits until ready between each transmit
#define POLL_CHARS 0

int main(void) {
	DBG_LO();

	// clear status
    WRITE_REG(UART_0_BASE, UART_STATUS, 0);

    // array for status
    int status_arr[2];

    // configure baud rate
    int divisor = DIVISOR(F_CK, BAUD_RATE);
    WRITE_REG(UART_0_BASE, UART_DIVISOR, divisor);

    // wait until ready
    while (!(READ_REG(UART_0_BASE, UART_STATUS, 9) & UART_STATUS_TX_READY))
        ;

    // send character
    char c = 'a';
    status_arr[0] = READ_REG(UART_0_BASE, UART_STATUS, 9);
    for (int i = 0; i < NUM_CHARS; i++) {
    	DBG_HI();
        WRITE_REG(UART_0_BASE, UART_TXDATA, c++);
    	DBG_LO();

        // wait until ready
        if (POLL_CHARS && (i != NUM_CHARS - 1)) {
            while (!(READ_REG(UART_0_BASE, UART_STATUS, 9) & UART_STATUS_TX_READY))
                ;
        }
    }

    status_arr[1] = READ_REG(UART_0_BASE, UART_STATUS, 9);

    printf("STATUS BEFORE [0x%02X]: 0x%04X\n", UART_STATUS, status_arr[0]);
    dump_status(status_arr[0]);
    printf("STATUS AFTER [0x%02X]: 0x%04X\n", UART_STATUS, status_arr[1]);
    dump_status(status_arr[1]);

    return 0;
}

#elif PROJECT == 4

#define TEST_STRING \
    "My name is what? My name is who? My name is - chika chika - Slim Shady"

int main(void) {
	DBG_LO();

    char str[] = TEST_STRING;
    char *c = str;

	// clear status
    WRITE_REG(UART_0_BASE, UART_STATUS, 0);

    // configure baud rate
    int divisor = DIVISOR(F_CK, BAUD_RATE);
    WRITE_REG(UART_0_BASE, UART_DIVISOR, divisor);

    // start timer
    alt_timestamp_start();
    alt_u32 t_start = (alt_u32)alt_timestamp();
    DBG_HI();

    // print test string
    while (*c) {
        // wait until ready
        while (!(READ_REG(UART_0_BASE, UART_STATUS, 9) & UART_STATUS_TX_READY))
            ;

        // feed char to TX
        WRITE_REG(UART_0_BASE, UART_TXDATA, *c++);
    }

    // wait for final transmission to complete
    while (!(READ_REG(UART_0_BASE, UART_STATUS, 9) & UART_STATUS_TX_EMPTY))
        ;

    // stop timer
    DBG_LO();
    alt_u32 t_stop = (alt_u32)alt_timestamp();
    alt_u32 delta = t_stop - t_start;
    printf("Transmission took %lu ms\n",
           (unsigned long)(delta * 1000ULL / alt_timestamp_freq()));

    return 0;
}

#elif PROJECT == 5

// decides if the program waits until ready between each transmit
#define POLL_CHARS 1

// character echo on STDOUT delay
#define DELAY_MS 0

// added to correct DIVISOR value
#define DIVISOR_SKEW 65

int main(void) {
	DBG_LO();

    // clear interrupts (might not be needed)
    WRITE_REG(UART_0_BASE, UART_CONTROL, 0x00);

	// clear status
    WRITE_REG(UART_0_BASE, UART_STATUS, 0);

    // configure baud rate
    int divisor = DIVISOR(F_CK, BAUD_RATE) + DIVISOR_SKEW;
    WRITE_REG(UART_0_BASE, UART_DIVISOR, divisor);

    // array for status
    int status_arr[2];

    // start timer
    alt_timestamp_start();

    // get test chars
    for (;;) {
        int status = READ_REG(UART_0_BASE, UART_STATUS, 9);

        // clear RX overflow error bit if set
        if (status & UART_STATUS_RX_OVERFLOW_ERROR) {
            WRITE_REG(UART_0_BASE, UART_STATUS, 0);
        }

        // clear parity error bit if set
        if (status & UART_STATUS_PARITY_ERROR) {
            WRITE_REG(UART_0_BASE, UART_STATUS, 0);
        }

        // wait until ready
        if (POLL_CHARS) {
            while (!(READ_REG(UART_0_BASE, UART_STATUS, 9) & UART_STATUS_RX_READY))
                ;
        }

        // get char from RX
        DBG_HI();

        status_arr[0] = READ_REG(UART_0_BASE, UART_STATUS, 9);
        char c = READ_REG(UART_0_BASE, UART_RXDATA, 8);
        status_arr[1] = READ_REG(UART_0_BASE, UART_STATUS, 9);

        // start delay
        if (DELAY_MS > 0) {
            alt_u32 t_start = (alt_u32)alt_timestamp();
            alt_u32 delay_ticks = (alt_u32)(
                (unsigned long long)DELAY_MS * alt_timestamp_freq() / 1000
            );

            while ((alt_u32)(alt_timestamp() - t_start) < delay_ticks)
                ;
        }


    	DBG_LO();

        printf("STATUS BEFORE [0x%02X]: 0x%04X\n", UART_STATUS, status_arr[0]);
        printf("STATUS AFTER [0x%02X]: 0x%04X\n", UART_STATUS, status_arr[1]);
        if(status_arr[1] & UART_STATUS_PARITY_ERROR) printf("Parity error detected\n");
        printf("Received character: %c\n", c);
    }

    return 0;
}

#elif PROJECT == 8

// size of receive buffer
#define BUF_SIZE 64

int main(void) {
    // clear interrupts (might not be needed)
    WRITE_REG(UART_0_BASE, UART_CONTROL, 0x00);

	// clear status
    WRITE_REG(UART_0_BASE, UART_STATUS, 0);

    // configure baud rate
    int divisor = DIVISOR(F_CK, BAUD_RATE);
    WRITE_REG(UART_0_BASE, UART_DIVISOR, divisor);

    // setup buffer
    char s[BUF_SIZE];
    for(;;) {
    	memset(s, 0, BUF_SIZE); // will be NULL terminated no matter what

        // get test chars
        for (int i = 0; i < BUF_SIZE - 1; i++) {
            // wait until ready
            while (!(READ_REG(UART_0_BASE, UART_STATUS, 9) & UART_STATUS_RX_READY))
                ;

            // get char from RX
            s[i] = READ_REG(UART_0_BASE, UART_RXDATA, 8);

            if (s[i] == '\0' || s[i] == '\n' || s[i] == '\r') {
                break;
            }
        }

        printf("Received string: %s\n", s);
    }

    return 0;
}

#elif PROJECT == 10

int main(void) {
    /* Left as bonus */
    return 0;
}

#else
#error "PROJECT must be 1, 2, 3, 4, 5, 8, or 10"
#endif
