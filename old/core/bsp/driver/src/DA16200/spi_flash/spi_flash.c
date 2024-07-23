/*
 * spi_flash.c
 *
 *  Created on: 2021. 12. 6.
 *      Author: shong
 */

#include "da16x_system.h"
#include "spi_flash/spi_flash.h"

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define SPI_FLASH_RDID_CMD	0x9F
#define SPI_FLASH_WREN_CMD	0x06
#define SPI_FLASH_RDSR_CMD	0x05
#define SPI_FLASH_SER_CMD	0x20
#define SPI_FLASH_PP_CMD	0x02
#define SPI_FLASH_READ_CMD	0x03


#define GET_BIT(reg, loc)	((reg) & ((0x1) << (loc)))
//#define SPI_FLASH_PRINT(...)	PRINTF(__VA_ARGS__)
#define SPI_FLASH_PRINT(...)


#undef  SPI_FLASH_PECYCLE_GUARD   

#define SPI_FLASH_LOCK(handler, ioctldata, boolval)      {                      \
                ioctldata[0] = boolval;                                         \
                ioctldata[1] = portMAX_DELAY;                                   \
                ioctldata[2] = handler->spi_cs;                                 \
                SPI_IOCTL(handler->spi,SPI_SET_LOCK, ioctldata);                \
        }


static  UINT32 _spi_instance = 0;

HANDLE spi_flash_open(UINT32 spi_clock, UINT32 spi_cs) {
	UINT32 ioctldata[4];
	spi_flash_t *spi_flash;

	spi_flash = (spi_flash_t*) pvPortMalloc(sizeof(spi_flash_t));

	if (spi_flash == NULL) {
		return NULL;
	}

	spi_flash->spi = SPI_CREATE(SPI_UNIT_0);

	if (spi_flash->spi == NULL) {
                vPortFree(spi_flash);
		return NULL;
	}

	spi_flash->spi_clock = spi_clock;
	spi_flash->spi_cs = spi_cs;


        if( _spi_instance == 0 ) {      
        	ioctldata[0] = (1 * MBYTE);
        	SPI_IOCTL(spi_flash->spi, SPI_SET_MAX_LENGTH, ioctldata);

        	SPI_FLASH_PRINT("spi_cs: %d\n", spi_flash->spi_cs);

        	_sys_clock_read(ioctldata, sizeof(UINT32));
        	SPI_FLASH_PRINT("Core Clk - %d Hz\n", ioctldata[0]);

        	SPI_IOCTL(spi_flash->spi, SPI_SET_CORECLOCK, ioctldata);

        	SPI_FLASH_PRINT("spi_clock: %d\n", spi_flash->spi_clock);
        	ioctldata[0] = spi_flash->spi_clock * MHz;

        	SPI_FLASH_PRINT("Set SPI Clk - %d Hz\n", ioctldata[0]);
        	SPI_IOCTL(spi_flash->spi, SPI_SET_SPEED, ioctldata);

        	ioctldata[0] = SPI_TYPE_MOTOROLA_O0H0;
        	SPI_IOCTL(spi_flash->spi, SPI_SET_FORMAT, ioctldata);

        	ioctldata[0] = SPI_DMA_MP0_BST(8) | SPI_DMA_MP0_IDLE(1)
        	| SPI_DMA_MP0_HSIZE(XHSIZE_DWORD)
        	| SPI_DMA_MP0_AI(SPI_ADDR_INCR)
        	;
        	SPI_IOCTL(spi_flash->spi, SPI_SET_DMA_CFG, ioctldata);

        	SPI_INIT(spi_flash->spi);

        	if (SPI_IOCTL(spi_flash->spi, SPI_GET_SPEED, ioctldata) == TRUE) {
        		SPI_FLASH_PRINT("SPI - %d Hz\n", ioctldata[0]);
        		if ((spi_flash->spi_clock * MHz) < ioctldata[0]) {
        			SPI_FLASH_PRINT("SPI - %d Hz (wrong)!!\n", ioctldata[0]);
        			return NULL;
        		} else {
        			SPI_FLASH_PRINT("SPI - %d Hz\n", ioctldata[0]);
        		}
        	}

        	SPI_IOCTL(spi_flash->spi, SPI_SET_DMAMODE, NULL);

        	ioctldata[0] = spi_flash->spi_cs;
        	ioctldata[1] = 4; // 4-wire
        	SPI_IOCTL(spi_flash->spi, SPI_SET_WIRE, (VOID*) ioctldata);
        }

        _spi_instance++;
        
	return spi_flash;
}

int spi_flash_close(HANDLE handler) {
	spi_flash_t *spi_flash;

	if (handler == NULL) {
		return FALSE;
	}

	spi_flash = (spi_flash_t*) handler;

	if (spi_flash->spi != NULL) {
		SPI_CLOSE(spi_flash->spi);
		spi_flash->spi = NULL;
	}

        if( _spi_instance != 0 ){
                _spi_instance--;
        }
	vPortFree(spi_flash);
	return TRUE;
}

int spi_flash_read_id(HANDLE handler, UINT8 *rx_buf) {
	spi_flash_t *spi_flash;
	UINT32 busctrl[3];
        int status;

	if (handler == NULL) {
		return FALSE;
	}

	spi_flash = (spi_flash_t*) handler;

        SPI_FLASH_LOCK(spi_flash, busctrl, TRUE);
	/*
	 * READ PRODUCT IDENTIFICATION BY JEDEC ID OPERATION (RDJDID, 9FH)
	 */
	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-0-0-1] 9Fh\n");
	busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
			| SPI_SPI_PHASE_ADDR_3BYTE | SPI_SET_SPI_DUMMY_CYCLE(0);
	busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
			SPI_BUS_TYPE(0,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
			SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
			);
	SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);
	status = SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_RDID_CMD, 0x00, 0x00, NULL, 0,
			rx_buf, 3);
        
        SPI_FLASH_LOCK(spi_flash, busctrl, FALSE);
        
        return status;
}

int spi_flash_erase(HANDLE handler, UINT32 address) {
	spi_flash_t *spi_flash;
	UINT32 busctrl[3];
	INT32 status;
	UINT32 addrbyte = 3;
	UINT16 rsdr = 0;

	if (handler == NULL) {
		return FALSE;
	}

	spi_flash = (spi_flash_t*) handler;

        SPI_FLASH_LOCK(spi_flash, busctrl, TRUE);
	/*
	 * READ STATUS REGISTER OPERATION (RDSR, 05H)
	 */
	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-0-0-1] 03h, RDSR\n");
	while (1) {
		busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
				| ((addrbyte == 3) ?
						SPI_SPI_PHASE_ADDR_3BYTE : SPI_SPI_PHASE_ADDR_4BYTE)
				| SPI_SET_SPI_DUMMY_CYCLE(0);
		busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(0,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
				);
		SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);

		SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_RDSR_CMD, 0x00, 0x00, NULL, 0, &rsdr,
				1);
		SPI_FLASH_PRINT("RDSR: 0x%x\n", rsdr);
		if (GET_BIT(rsdr, 0) == 0x0)
			break;

		SYSUSLEEP(1000);
	}

	/*
	 * WRITE ENABLE OPERATION (WREN, 06H)
	 */
	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-0-0-0] 06h, WREN\n");
	busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
			| SPI_SPI_PHASE_ADDR_3BYTE | SPI_SET_SPI_DUMMY_CYCLE(0);
	busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
			SPI_BUS_TYPE(0,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
			SPI_BUS_TYPE(0,0,SPI_BUS_SPI) // DATA
			);
	SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);
	status = SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_WREN_CMD, 0x00, 0x00, NULL, 0,
			NULL, 0);
	SPI_FLASH_PRINT("status: %d\n", status);

	/*
	 * READ STATUS REGISTER OPERATION (RDSR, 05H)
	 */
	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-0-0-1] 03h, RDSR\n");
	while (1) {
		busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
				| ((addrbyte == 3) ?
						SPI_SPI_PHASE_ADDR_3BYTE : SPI_SPI_PHASE_ADDR_4BYTE)
				| SPI_SET_SPI_DUMMY_CYCLE(0);
		busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(0,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
				);
		SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);

		SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_RDSR_CMD, 0x00, 0x00, NULL, 0, &rsdr,
				1);
		SPI_FLASH_PRINT("RDSR: 0x%x\n", rsdr);
		if (GET_BIT(rsdr, 1) == 0x2)
			break;

		SYSUSLEEP(1000);
	}

	/*
	 * SECTOR ERASE OPERATION (SER, D7H/20H)
	 */
	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-1-0-0] 20h, SER\n");
	busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
			| SPI_SPI_PHASE_ADDR_3BYTE | SPI_SET_SPI_DUMMY_CYCLE(0);
	busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
			SPI_BUS_TYPE(1,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
			SPI_BUS_TYPE(0,0,SPI_BUS_SPI) // DATA
			);
	SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);
	status = SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_SER_CMD, address, 0x00, NULL, 0,
			NULL, 0);
	SPI_FLASH_PRINT("status: %d\n", status);

#ifdef  SPI_FLASH_PECYCLE_GUARD
        // Waiting until ERASE-op is complete.
	/*
	 * READ STATUS REGISTER OPERATION (RDSR, 05H)
	 */
	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-0-0-1] 03h, RDSR\n");
	while (1) {
		busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
				| ((addrbyte == 3) ?
						SPI_SPI_PHASE_ADDR_3BYTE : SPI_SPI_PHASE_ADDR_4BYTE)
				| SPI_SET_SPI_DUMMY_CYCLE(0);
		busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(0,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
				);
		SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);

		SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_RDSR_CMD, 0x00, 0x00, NULL, 0, &rsdr,
				1);
		SPI_FLASH_PRINT("RDSR: 0x%x\n", rsdr);
		if (GET_BIT(rsdr, 0) == 0x0)
			break;

		SYSUSLEEP(2000);
	}
#endif //SPI_FLASH_PECYCLE_GUARD

        SPI_FLASH_LOCK(spi_flash, busctrl, FALSE);

	return TRUE;
}

int spi_flash_write(HANDLE handler, UINT32 address, UINT8 *tx_buf,
		UINT32 tx_len) {
	spi_flash_t *spi_flash;
	UINT32 busctrl[3];
	INT32 status;
	UINT32 addrbyte = 3;
	UINT16 rsdr = 0;

	if (handler == NULL) {
		return FALSE;
	}

	spi_flash = (spi_flash_t*) handler;

        SPI_FLASH_LOCK(spi_flash, busctrl, TRUE);
	/*
	 * READ STATUS REGISTER OPERATION (RDSR, 05H)
	 */
	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-0-0-1] 03h, RDSR\n");
	while (1) {
		busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
				| ((addrbyte == 3) ?
						SPI_SPI_PHASE_ADDR_3BYTE : SPI_SPI_PHASE_ADDR_4BYTE)
				| SPI_SET_SPI_DUMMY_CYCLE(0);
		busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(0,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
				);
		SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);

		SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_RDSR_CMD, 0x00, 0x00, NULL, 0, &rsdr,
				1);
		SPI_FLASH_PRINT("RDSR: 0x%x\n", rsdr);
		if (GET_BIT(rsdr, 0) == 0x0)
			break;
		
                SYSUSLEEP(1000);
	}

	/*
	 * WRITE ENABLE OPERATION (WREN, 06H)
	 */
	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-0-0-0] 06h, WREN\n");
	busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
			| SPI_SPI_PHASE_ADDR_3BYTE | SPI_SET_SPI_DUMMY_CYCLE(0);
	busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
			SPI_BUS_TYPE(0,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
			SPI_BUS_TYPE(0,0,SPI_BUS_SPI) // DATA
			);
	SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);
	status = SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_WREN_CMD, 0x00, 0x00, NULL, 0,
			NULL, 0);
	SPI_FLASH_PRINT("status: %d\n", status);

	/*
	 * READ STATUS REGISTER OPERATION (RDSR, 05H)
	 */
	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-0-0-1] 03h, RDSR\n");
	while (1) {
		busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
				| ((addrbyte == 3) ?
						SPI_SPI_PHASE_ADDR_3BYTE : SPI_SPI_PHASE_ADDR_4BYTE)
				| SPI_SET_SPI_DUMMY_CYCLE(0);
		busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(0,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
				);
		SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);

		SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_RDSR_CMD, 0x00, 0x00, NULL, 0, &rsdr,
				1);
		SPI_FLASH_PRINT("RDSR: 0x%x\n", rsdr);
		if (GET_BIT(rsdr, 1) == 0x2)
			break;
		SYSUSLEEP(1000);
	}

	/*
	 * PAGE PROGRAM OPERATION (PP, 02H)
	 */
	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-1-0-1] 02h, addr %dB\n", addrbyte);
	busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
			| ((addrbyte == 3) ?
					SPI_SPI_PHASE_ADDR_3BYTE : SPI_SPI_PHASE_ADDR_4BYTE)
			| SPI_SET_SPI_DUMMY_CYCLE(0);
	busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
			SPI_BUS_TYPE(1,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
			SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
			);
	SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);

	status = SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_PP_CMD, address, 0x00, tx_buf,
			tx_len, NULL, 0);

#ifdef  SPI_FLASH_PECYCLE_GUARD
        // Waiting until PROGRAM-op is complete.
	/*
	 * READ STATUS REGISTER OPERATION (RDSR, 05H)
	 */
	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-0-0-1] 03h, RDSR\n");
	while (1) {
		busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
				| ((addrbyte == 3) ?
						SPI_SPI_PHASE_ADDR_3BYTE : SPI_SPI_PHASE_ADDR_4BYTE)
				| SPI_SET_SPI_DUMMY_CYCLE(0);
		busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(0,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
				);
		SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);

		SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_RDSR_CMD, 0x00, 0x00, NULL, 0, &rsdr,
				1);
		SPI_FLASH_PRINT("RDSR: 0x%x\n", rsdr);
		if (GET_BIT(rsdr, 0) == 0x0)
			break;

		SYSUSLEEP(1000);
	}
#endif  //SPI_FLASH_PECYCLE_GUARD
        
        SPI_FLASH_LOCK(spi_flash, busctrl, FALSE);

	return status;
}

int spi_flash_read(HANDLE handler, UINT32 address, UINT8 *rx_buf, UINT32 rx_len) {
	spi_flash_t *spi_flash;
	UINT32 busctrl[3];
	INT32 status;
	UINT32 addrbyte = 3;
	UINT16 rsdr = 0;

	if (handler == NULL) {
		return FALSE;
	}

	spi_flash = (spi_flash_t*) handler;

        SPI_FLASH_LOCK(spi_flash, busctrl, TRUE);

	/*
	 * READ STATUS REGISTER OPERATION (RDSR, 05H)
	 */
	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-0-0-1] 03h, RDSR\n");
	while (1) {
		busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
				| ((addrbyte == 3) ?
						SPI_SPI_PHASE_ADDR_3BYTE : SPI_SPI_PHASE_ADDR_4BYTE)
				| SPI_SET_SPI_DUMMY_CYCLE(0);
		busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(0,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
				SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
				);
		SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);

		SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_RDSR_CMD, 0x00, 0x00, NULL, 0, &rsdr,
				1);
		SPI_FLASH_PRINT("RDSR: 0x%x\n", rsdr);
		if (GET_BIT(rsdr, 0) == 0x0)
			break;
		SYSUSLEEP(1000);
	}

	SPI_FLASH_PRINT(">>>>>>\n[SPI  1-1-0-1] 03h, addr %dB\n", addrbyte);
	busctrl[0] = SPI_SPI_TIMEOUT_EN | SPI_SPI_PHASE_CMD_1BYTE
			| ((addrbyte == 3) ?
					SPI_SPI_PHASE_ADDR_3BYTE : SPI_SPI_PHASE_ADDR_4BYTE)
			| SPI_SET_SPI_DUMMY_CYCLE(0);
	busctrl[1] = SPI_SET_SPI_BUS_TYPE(SPI_BUS_TYPE(1,0,SPI_BUS_SPI),
			SPI_BUS_TYPE(1,0,SPI_BUS_SPI), SPI_BUS_TYPE(0,0,SPI_BUS_SPI),
			SPI_BUS_TYPE(1,0,SPI_BUS_SPI) // DATA
			);
	SPI_IOCTL(spi_flash->spi, SPI_SET_BUSCONTROL, busctrl);
	status = SPI_SFLASH_TRANSMIT(spi_flash->spi, SPI_FLASH_READ_CMD, address, 0x00, NULL, 0,
			rx_buf, rx_len);

        SPI_FLASH_LOCK(spi_flash, busctrl, FALSE);

	return status;
}

