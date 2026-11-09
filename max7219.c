#include "max7219.h"
#include "font.h"

// DEBUG
#include "../AVR_Atmega328p_Core/UART.h"
#include "../NumToStr/nts.h"
#include <stdint.h>
#include <string.h>
// END DEBUG

/*-------------------HAL LEVEL-----------------*/
/**
 * @brief function to send 1 byte via SPI
 * @param data -> data byte
 * @retval max7219_status_t
 */
max7219_status_t MAX7219_SPI_TX(uint8_t data) {
    uint32_t cnt = 0;
    SPDR = data;
    while (!(SPSR & (1 << SPIF))) {
        // emergency exit
        if (cnt++ > MAX7219_SPI_TIMEOUT) {
            return MAX7219_TO;
        }
    };
    return MAX7219_OK;
}

/**
 * @brief function to send buffer via SPI
 * @param data_buf[in] -> data buffer pointer
 * @param size -> size of data buffer
 * @retval max7219_status_t
 */
max7219_status_t MAX7219_SPI_TX_Buf(const uint8_t *data_buf, uint8_t size) {
    MAX7219_MACR_CHECK_PTR(data_buf, return MAX7219_EP);
    uint32_t cnt;
    for (uint8_t i = 0; i < size; i++) {
        cnt = 0;
        SPDR = data_buf[i];
        while (!(SPSR & (1 << SPIF))) {
            // emergency exit
            if (cnt++ > MAX7219_SPI_TIMEOUT) {
                return MAX7219_TO;
            }
        };
    }
    return MAX7219_OK;
}

/*-----------------DRIVER LEVEL----------------*/
/**
 * @brief function to write MAX7219 register data
 * @param reg -> MAX7219 register
 * @param data -> data to write
 * @param struc [in/out] -> max7219 configure structure
 * @retval max7219_status_t
 */
static max7219_status_t MAX7219_WriteReg(uint8_t reg, uint8_t data,
                                         max7219_t *struc) {
    MAX7219_MACR_CHECK_PTR(struc, return MAX7219_EP);
    max7219_status_t err;
    uint8_t frame[] = {reg, data};

    MAX7219_PIN_0(*struc->cs_port, struc->cs_pin);
    err = MAX7219_SPI_TX_Buf(frame, 2);
    MAX7219_PIN_1(*struc->cs_port, struc->cs_pin);
    return err;
}

/**
 * @brief function to initialize MAX7219
 * @param struc [in/out] -> max7219 configure structure
 * @retval max7219_status_t
 */
max7219_status_t MAX7219_Init(max7219_t *struc) {
    MAX7219_MACR_CHECK_PTR(struc, return MAX7219_EP);
    max7219_status_t err;

    // max7219 doesnt suppurt more that 7 symbols
    if (MAX7219_SYMB_QUANT > 7) {
        return MAX7219_SE;
    }
    // scan limit ->number of digits 6 digits
    err |= MAX7219_WriteReg(MAX7219_REG_SCAN_LIMIT, MAX7219_SYMB_QUANT, struc);
    // no decode, use own font
    err |= MAX7219_WriteReg(MAX7219_REG_DECODE, 0, struc);
    // normal mode, Shut down mode OFF
    err |= MAX7219_WriteReg(MAX7219_REG_SHUTDOWN, 1, struc);
    // normal  mode, test mode OFF
    err |= MAX7219_WriteReg(MAX7219_REG_DISPLAY_TEST, 0, struc);
    err |= MAX7219_ClearDisp(struc);
    err |= MAX7219_SetBright(struc);
    return err;
}

/**
 * @brief function to change dislpay brightness
 * brighness is a number from 0 to 8
 * @param struc [in/out] -> max7219 configure structure
 * @retval max7219_status_t
 */
max7219_status_t MAX7219_SetBright(max7219_t *struc) {
    MAX7219_MACR_CHECK_PTR(struc, return MAX7219_EP);
    if (struc->bright > 8) {
        struc->bright = 8;
    }
    return MAX7219_WriteReg(MAX7219_REG_INTENSITY, struc->bright, struc);
}

/**
 * @brief function to flush the buffer to the display
 * @param struc [in/out] -> max7219 configure structure
 * @retval max7219_status_t
 */
max7219_status_t MAX7219_Flush(max7219_t *struc) {
    MAX7219_MACR_CHECK_PTR(struc, return MAX7219_EP);
    uint8_t bar = 0;
    max7219_status_t err;

    if (struc->bar_rate > 6) {
        struc->bar_rate = 6;
    }
    for (uint8_t i = 0; i < struc->bar_rate; i++) {
        bar |= 2 | 2 << i;
    }
    err |= MAX7219_WriteReg(6, struc->buf[4], struc);
    err |= MAX7219_WriteReg(5, struc->buf[3], struc);
    err |= MAX7219_WriteReg(4, struc->buf[2], struc);
    err |= MAX7219_WriteReg(3, struc->buf[1], struc);
    err |= MAX7219_WriteReg(2, struc->buf[0], struc);
    err |= MAX7219_WriteReg(1, bar, struc);

    // write dot
    if (struc->dot_loc > MAX7219_SYMB_QUANT - 1) {
        struc->dot_loc = MAX7219_SYMB_QUANT - 1;
    }
    err |= MAX7219_WriteReg(struc->dot_loc, 0x80, struc);

    return err;
}

/**
 * @brief clear display
 * @param struc [in/out] -> max7219 configure structure
 * @retval max7219_status_t
 */
max7219_status_t MAX7219_ClearDisp(max7219_t *struc) {
    MAX7219_MACR_CHECK_PTR(struc, return MAX7219_EP);
    max7219_status_t err;

    err |= MAX7219_WriteReg(6, 0, struc);
    err |= MAX7219_WriteReg(5, 0, struc);
    err |= MAX7219_WriteReg(4, 0, struc);
    err |= MAX7219_WriteReg(3, 0, struc);
    err |= MAX7219_WriteReg(2, 0, struc);
    return err;
}

/**
 * @brief clear display buffer
 * @param struc [in/out] -> max7219 configure structure
 * @retval max7219_status_t
 */
max7219_status_t MAX7219_ClearBuf(max7219_t *struc) {
    MAX7219_MACR_CHECK_PTR(struc, return MAX7219_EP);

    for (uint8_t i = 0; i < MAX7219_SYMB_QUANT; i++) {
        struc->buf[i] = 0;
    }
    return MAX7219_OK;
}

/**
 * @brief fill the driver buffer with the given string
 * @param struc [in/out] -> max7219 configure structure
 * @param str pointer to the string
 * @retval max7219_status_t
 */
max7219_status_t MAX7219_PushStr(max7219_t *struc, const uint8_t *str) {
    MAX7219_MACR_CHECK_PTR(struc, return MAX7219_EP);
    MAX7219_MACR_CHECK_PTR(str, return MAX7219_EP);

    uint8_t font_ptr;

    // convert ANSI to the font table
    for (uint8_t i = 0; i < MAX7219_SYMB_QUANT; i++) {
        font_ptr = str[i] - 32;
        if (font_ptr > 58) {
            return MAX7219_SE;
        }
#ifdef MAX7219_ARCH_AVR
        struc->buf[i] = pgm_read_byte(&sev_seg_font[font_ptr]);
#elif MAX7219_ARCH_STM32
        struc->buf[i] = sev_seg_font[font_ptr];
#endif
    }
    return MAX7219_OK;
}

/**
 * @brief function to push runnig string to the output buffer
 * @param struc [in/out] -> max7219 configure structure
 * @param rstruct [in/out] -> runnig string structure
 * @retval max7219_status_t
 */
max7219_status_t MAX7219_RunStr(max7219_t *struc, max7219_run_str_t *rs_struc) {
    MAX7219_MACR_CHECK_PTR(struc, return MAX7219_EP);
    MAX7219_MACR_CHECK_PTR(rs_struc, return MAX7219_EP);

    // await
    if (rs_struc->shift_fl) {
        rs_struc->shift_fl = 0;
    } else {
        return MAX7219_OK;
    }

    static uint8_t step;                         // shift steps
    uint8_t len = strlen((char *)rs_struc->str); // string length
    uint8_t total_len; // total length of string + symbols
    total_len = len + MAX7219_SYMB_QUANT * 2 + 1;
    uint8_t font_ptr; // font pointer
    uint8_t iter;     // array iterator

    // stack overflow protection
    if (total_len > MAX7219_RUN_STR_MAX_LEN) {
        return MAX7219_SE;
    }

    uint8_t buf[len + MAX7219_SYMB_QUANT * 2 + 1];

    // new running string start procedure
    if (rs_struc->start_fl) {
        step = 0;
        rs_struc->start_fl = 0;
        rs_struc->finish_fl = 0;
    }

    // fill the buffer with epmty fileds from right and left sides
    for (uint8_t i = 0; i < MAX7219_SYMB_QUANT; i++) {
        buf[iter++] = ' ';
    }
    for (uint8_t i = 0; i < len; i++) {
        buf[iter++] = rs_struc->str[i];
    }
    for (uint8_t i = 0; i < MAX7219_SYMB_QUANT; i++) {
        buf[iter++] = ' ';
    }

    // slice the buffer to fit in the display
    if ((++step) + MAX7219_SYMB_QUANT < total_len) {
        // NO BUS FAULT - NO WRITE. ONLY READ (buf + step)
        memcpy(struc->buf, buf + step, MAX7219_SYMB_QUANT);
    } else {
        rs_struc->finish_fl = 1;
    }

    // convert ANSI to the font table
    for (uint8_t i = 0; i < MAX7219_SYMB_QUANT; i++) {
        font_ptr = struc->buf[i] - 32;
        if (font_ptr > 58) {
            return MAX7219_SE;
        }
#ifdef MAX7219_ARCH_AVR
        struc->buf[i] = pgm_read_byte(&sev_seg_font[font_ptr]);
#elif MAX7219_ARCH_STM32
        struc->buf[i] = sev_seg_font[font_ptr];
#endif
    }

    return MAX7219_OK;
}

/**
 * @brief locate the dot at the output buffer
 * @param struc [in/out] -> max7219 configure structure
 * @retval max7219_status_t
 */
inline max7219_status_t MAX7219_LocateDot(max7219_t *struc, uint8_t dot_loc) {
    MAX7219_MACR_CHECK_PTR(struc, return MAX7219_EP);

    struc->dot_loc = dot_loc;

    return MAX7219_OK;
}