/*
 * XSPI.h
 *
 * Project: XSPI
 * Copyright (c) 2014 Shelby Merrick
 * http://www.forkineye.com
 *
 *  This program is provided free for you to use in any way that you wish,
 *  subject to the laws and regulations where you are using it.  Due diligence
 *  is strongly suggested before using this code.  Please give credit where due.
 *
 *  The Author makes no warranty of any kind, express or implied, with regard
 *  to this program or the documentation contained in this document.  The
 *  Author shall not be liable in any event for incidental or consequential
 *  damages in connection with, or arising out of, the furnishing, performance
 *  or use of these programs.
 *
 */ 

#ifndef XSPI_H_
#define XSPI_H_

#include "../config.h"
#include <avr/io.h>
#include <stdbool.h>

/*! \brief Structure which defines items needed for SPI control.
 *  \param usart    Pointer to the SPI module.
 *  \param port     Pointer to the port on which the SPI module resides.
 *  \param mosi_pin MOSI pin number.
 *  \param miso_pin MISO pin number.
 *  \param sck_pin  SCK pin number.
 *  \param ss_pin   SS pin number.
 */
typedef struct {
    SPI_t   *spi;
    PORT_t  *port;
    uint8_t mosi_pin;
    uint8_t miso_pin;
    uint8_t sck_pin;
    uint8_t ss_pin;
} xspi_config_t;

/************************************************************************/
/* Normal hardware SPI stuff                                            */
/************************************************************************/

/*! \brief SPI Master initialization function.
 *  \param port         Pointer to the port on which this SPI module resides.
 *  \param spi          Pointer to SPI_t module structure.
 *  \param mode         Clock and polarity mode for SPI.
 *  \param lsb          Set to true for LSB data, false for MSB.
 *  \param prescaler    SPI clock prescaler value.
 *  \param clk2x        Enables SPI clock double-speed.
 */
static inline void xspi_master_init(xspi_config_t *config, SPI_MODE_t mode, bool lsb, SPI_PRESCALER_t prescaler, bool clk2x) {
    config->port->DIRSET = (1 << config->mosi_pin) | (1 << config->sck_pin) | (1 << config->ss_pin);
    config->spi->CTRL = SPI_ENABLE_bm |	SPI_MASTER_bm | mode | prescaler |
            (clk2x ? SPI_CLK2X_bm : 0) | (lsb ? SPI_DORD_bm : 0);
}

/*! \brief SPI Slave initialization function.
 *  \param port Pointer to the port on which this SPI module resides.
 *  \param spi  Pointer to XSPI_t configuration structure.
 *  \param mode Clock and polarity mode for SPI.
 *  \param lsb  Set to true for LSB data, false for MSB.
 */
static inline void xspi_slave_init(xspi_config_t *config, SPI_MODE_t mode, bool lsb) {
    config->port->DIRSET = (1 << config->miso_pin);
    config->port->DIRCLR = (1 << config->sck_pin) | (1 << config->ss_pin);
    config->spi->CTRL = SPI_ENABLE_bm | mode | (lsb ? SPI_DORD_bm : 0);
}

/*! \brief Blocking call that sends and returns a single byte.
 *  \param spi  Pointer to SPI_t module structure.
 *  \return     Single byte read from SPI.
 */
static inline uint8_t xspi_transfer_byte(SPI_t *spi, uint8_t val) {
    spi->DATA = val;
    while(!(spi->STATUS & SPI_IF_bm));
    return spi->DATA;
}

/*! \brief Sends a packet, ignoring any returned SPI data.
 *  \param spi  Pointer to SPI_t module structure.
 *  \param data Pointer to the data being sent.
 *  \param len  Length in bytes of the data being sent.
 */
static inline void xspi_send_packet(SPI_t *spi, uint8_t *data, uint8_t len) {
    while (len--) {
        spi->DATA = *data++;
        while(!(spi->STATUS & SPI_IF_bm));
    }
}

/*! \brief Retrieves a packet of data via SPI
 *  \param spi  Pointer to SPI_t module structure.
 *  \param data Pointer to a buffer to store the retrieved data.
 *  \param len  Size of the buffer in bytes.
 */
static inline void xspi_get_packet(SPI_t *spi, uint8_t *data, uint8_t len) {
    while (len--) {
        spi->DATA = 0xFF;
        while(!(spi->STATUS & SPI_IF_bm));
        *data++ = spi->DATA;
    }
}

/************************************************************************/
/* USART specific SPI stuff                                             */
/************************************************************************/
#define USART_UDORD_bm USART_CHSIZE2_bm
#define USART_UCPHA_bm USART_CHSIZE1_bm
#define SERIAL_SPI_UBBRVAL(Baud)    ((Baud < (F_CPU / 2)) ? ((F_CPU / (2 * Baud)) - 1) : 0)

/*! \brief SPI Master USART initialization function.
 *  \param port         Pointer to the port on which this SPI module resides.
 *  \param usart        Pointer to USART_t module structure.
 *  \param mode         Clock and polarity mode for SPI.
 *  \param lsb          Set to true for LSB data, false for MSB.
 *  \param prescaler    SPI clock prescaler value.
 *  \param clk2x        Enables SPI clock double-speed.
 */
static inline void xspi_usart_master_init(PORT_t *port, USART_t *usart, SPI_MODE_t mode, uint32_t baudrate) {
    uint16_t baudval = SERIAL_SPI_UBBRVAL(baudrate);
	
    usart->BAUDCTRLB = (baudval >> 8);
    usart->BAUDCTRLA = (baudval & 0xFF);
    usart->CTRLC = (USART_CMODE_MSPI_gc | mode);
    usart->CTRLB = (USART_RXEN_bm | USART_TXEN_bm); 	
}

/*! \brief Blocking call that sends and returns a single byte.
 *  \param usart    Pointer to USART_t module structure.
 *  \param val      Byte to send.
 *  \return         Single byte read from SPI.
 */
static inline uint8_t xspi_usart_transfer_byte(USART_t *usart, uint8_t val) {
    usart->DATA = val;
    while(!(usart->STATUS & USART_TXCIF_bm));
    usart->STATUS = USART_TXCIF_bm;
    return usart->DATA;
}

/*! \brief Blocking call that sends a single buffered byte.
 *  \param usart    Pointer to USART_t module structure.
 *  \param val      Byte to send.
 */
static inline void xspi_usart_send_byte(USART_t *usart, uint8_t val) {
    while (!(usart->STATUS & USART_DREIF_bm));
    usart->DATA = val;
}

/*! \brief Blocking call that retrieves a single byte.
 *  \param usart    Pointer to USART_t module structure.
 *  \return         Next byte from the UART
 */
static inline uint8_t xspi_usart_get_byte(USART_t *usart) {
    return xspi_usart_transfer_byte(usart, 0xFF);
}

/*! \brief Sends a packet of data via UART in Master SPI mode.
 *  \param usart    Pointer to USART_t module structure.
 *  \param data     Pointer to a buffer to store the retrieved data.
 *  \param len      Size of the buffer in bytes.
 */
static inline void xspi_usart_send_packet(USART_t *usart, uint8_t *data, uint8_t len) {
    while (len--)
    xspi_usart_send_byte(usart, *data++);
}

/*! \brief Retrieves a packet of data via UART in Master SPI mode.
 *  \param usart    Pointer to USART_t module structure.
 *  \param data     Pointer to a buffer to store the retrieved data.
 *  \param len      Size of the buffer in bytes.
 */
static inline void xspi_usart_get_packet(USART_t *usart, uint8_t *data, uint8_t len) {
    while (len--)
    *data++ = xspi_usart_get_byte(usart);
}

#endif /* XSPI_H_ */