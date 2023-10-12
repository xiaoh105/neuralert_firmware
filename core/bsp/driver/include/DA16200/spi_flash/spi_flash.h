/**
 ****************************************************************************************
 *
 * @file spi_flash.h
 *
 * @brief API functions related to reading, writing, erasing, memory mapping for data in the external flash with SPI single I/O.
 *
 * Copyright (c) 2016-2022 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */


#ifndef CUSTOMER_APP_INCLUDE_USER_MAIN_SPI_FLASH_H_
#define CUSTOMER_APP_INCLUDE_USER_MAIN_SPI_FLASH_H_


/**
 * @brief Structure to describe a SPI flash chip connected to the system.
 */
typedef struct {
	/**
	 * spi instance
	 */
	HANDLE spi;

	/**
	 * spi clock
	 */
	UINT32 spi_clock;

	/**
	 * spi chip select
	 */
	UINT32 spi_cs;

} spi_flash_t;

/**
 * @brief This function create SPI Flash handler.
 * @param[in] spi_clock Clock of SPI
 * @param[in] spi_cs Chip Select of SPI
 * @return Handler of SPI Flash Chip
 */
HANDLE spi_flash_open(UINT32 spi_clock, UINT32 spi_cs);

/**
 * @brief Read flash ID using RDID SPI flash command.
 * @param[in] handler Pointer to identify flash chip. Should be obtained using spi_flash_open.
 * @param[in] rx_buf Pointer to a buffer where the data will be read.
 * @return zero - false, non-zero - data length
 */
int spi_flash_read_id(HANDLE handler, UINT8 *rx_buf);

/**
 * @brief Read Data from SPI Flash Chip
 * @param[in] handler Pointer to identify flash chip. Should be obtained using spi_flash_open.
 * @param[in] address Address on flash to read from.
 * @param[in] rx_buf Pointer to a buffer where the data will be read.
 * @param[in] rx_len Length of data
 * @return zero - false, non-zero - data length
 */
int spi_flash_read(HANDLE handler, UINT32 address, UINT8 *rx_buf,
		UINT32 rx_len);

/**
 * @brief Write Data from SPI Flash Chip
 * @param[in] handler Pointer to identify flash chip. Should be obtained using spi_flash_open.
 * @param[in] address Address on flash to write.
 * @param[in] tx_buf Pointer to a buffer where the data will be write.
 * @param[in] tx_len Length of data
 * @return zero - false, non-zero - data length
 */
int spi_flash_write(HANDLE handler, UINT32 address, UINT8 *tx_buf,
		UINT32 tx_len);

/**
 * @brief Erase 4K Sector Data from SPI Flash Chip
 * @param[in] handler Pointer to identify flash chip. Should be obtained using spi_flash_open.
 * @param[in] address Address on flash to erase.
 * @return zero - false, one - success
 */
int spi_flash_erase(HANDLE handler, UINT32 address);

/**
 * @brief Close SPI Flash Chip Handler
 * @param[in] handler Pointer to identify flash chip. Should be obtained using spi_flash_open.
  * @return zero - false, one - success
 */
int spi_flash_close(HANDLE handler);

#endif /* CUSTOMER_APP_INCLUDE_USER_MAIN_SPI_FLASH_H_ */
