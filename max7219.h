#ifndef _MAX7219_H_
#define _MAX7219_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*-------------------NOTES-------------------*/
/*
My display has bar graph so I initialized 6 symbols
but use 5 for printing. For this reason "bar varible"
added in max7219_t.
Init,Flush,Clear funtions must be rewrite if you use
normal display

HOW TO USE:
1) choose architecture
2) define SPI transmit function
3) define CS push/pull macros
4) define CS pin/port types in max7219_t
5) define symbol quantity
6) map your own font.h
Your code:
1) create and fill max7219_t_t struct
2) initialize SPI/GPIO periphery
3) example:
    max7219_t max = {
        .buf =
            {
                0,
            },
        .cs_port = &PORTB,
        .cs_pin = PB2,
        .bar_rate = 0, // display with a bar graph
        .bright = 0,
    };
    max7219_status_t err = 0;
    err |= MAX7219_Init(&max);
    uint8_t test_str[] = "HELLO!";
    err |= MAX7219_PushStr(test_str, &max);
    err |= MAX7219_Flush(&max);
    err |= MAX7219_ClearBuf(&max);
    if(err != MAX7219_OK) printf("err: %d\n", err);
*/
#ifdef __cplusplus
extern "C" {
#endif

/*-------------------DEFINES-----------------*/
// ARCHITECTURE
#define MAX7219_ARCH_AVR
// #define MAX7219_ARCH_STM32

// RUN STRING
#define MAX7219_RUN_STR_MAX_LEN 50U

// HAL LEVEL
///////////////////////////////////////////////
// spi
#define MAX7219_MACR_SPI_TX(data) MAX7219_SPI_TX(data)
#define MAX7219_MACR_SPI_TX_BUF(data, size)                                    \
    MAX7219_SPI_TX_BUF(data, size, struc)
#define MAX7219_PIN_1(port, pin) port |= (1 << (pin))
#define MAX7219_PIN_0(port, pin) port &= ~(1 << (pin))

// DRIVER LEVEL
///////////////////////////////////////////////
#define MAX7219_SYMB_QUANT 5U
#define MAX7219_SPI_TIMEOUT 1000000U

// MAX7219 registers
#define MAX7219_REG_DECODE 0x09       // "decode mode" register
#define MAX7219_REG_INTENSITY 0x0A    // "intensity" register
#define MAX7219_REG_SCAN_LIMIT 0x0B   // "scan limit" register
#define MAX7219_REG_SHUTDOWN 0x0C     // "shutdown" register
#define MAX7219_REG_DISPLAY_TEST 0x0F // "display test" register

// MACROS
#define MAX7219_MACR_CHECK_PTR(ptr, act)                                       \
    do {                                                                       \
        if ((ptr) == 0) {                                                      \
            act;                                                               \
        }                                                                      \
    } while (0)

/*------------------TYPES---------------------*/

/**
 * @brief max7219 status enum
 * @param MAX7219_OK all in ok
 * @param MAX7219_TO error -> SPI timeout reached
 * @param MAX7219_EP error -> empty pointer
 * @param MAX7219_SE error -> symbol quantity
 */
typedef enum {
    MAX7219_OK = 0,
    MAX7219_TO = 0b1,
    MAX7219_EP = 0b01,
    MAX7219_SE = 0b001
} max7219_status_t;

/**
 * @brief max7219 struct
 * @param cs_port chip select port pointer
 * @param cs_pin chip select pin number
 * @param buf display buffer
 * @param bar_rate display bar rating
 * @param bright brightness variable
 * @param dot_loc location of the dot in the display
 */
typedef struct max7219_s {
    volatile uint8_t *cs_port;
    uint8_t cs_pin;
    uint8_t buf[MAX7219_SYMB_QUANT];
    uint8_t bar_rate;
    uint8_t bright;
    uint8_t dot_loc;
} max7219_t;

/**
 * @brief max7219 run string
 * @param str pointer to a string, ends with '\0'
 * @param shift_fl flag to shift string
 * @param start_fl flag to start new string pushing
 * @param finish_fl function finished a string
 */
typedef struct max7219_run_str_s {
    uint8_t *str;
    bool shift_fl;
    bool start_fl;
    bool finish_fl;
} max7219_run_str_t;

/*------------------FUNCTIONS------------------*/
// HAL LEVEL
max7219_status_t MAX7219_SPI_TX(uint8_t data);
max7219_status_t MAX7219_SPI_TX_Buf(const uint8_t *data_buf, uint8_t size);

// DRIVER LEVEL
max7219_status_t MAX7219_Init(max7219_t *struc);
max7219_status_t MAX7219_SetBright(max7219_t *struc);
max7219_status_t MAX7219_Flush(max7219_t *struc);
max7219_status_t MAX7219_ClearDisp(max7219_t *struc);
max7219_status_t MAX7219_ClearBuf(max7219_t *struc);
max7219_status_t MAX7219_PushStr(max7219_t *struc, const uint8_t *str);
max7219_status_t MAX7219_RunStr(max7219_t *struc, max7219_run_str_t *rs_struc);
inline max7219_status_t MAX7219_LocateDot(max7219_t *struc, uint8_t dot_loc);

#ifdef __cplusplus
}
#endif

#endif