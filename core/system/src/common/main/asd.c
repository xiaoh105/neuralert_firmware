/**
 ****************************************************************************************
 *
 * @file asd.c
 *
 * @brief Application Specific Device
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

#include "sdk_type.h"

#include "common_def.h"
#include "da16x_types.h"

#include "da16x_system.h"
#include "asd.h"


int asd_enabled;

#define REG_ASD_WR(addr, value)       (*(volatile UINT32 *)(addr)) = (value)
#define REG_ASD_RD(addr)              (*(volatile UINT32 *)(addr))

#define FUNC_OUT_EN0         0x4001003c
#define RF_SW_OUTSEL0        0x40010fc8
#define FCI_FEM_CTRL         0x60c036f0
#define DataOut_Set          0x40010010
#define DataOut_REG          0x40010004
typedef enum 
{
    ASD_DIABLE,
    ASD_AUTO_ENABLE,
    ASD_ANTENNA_PORT_1,
    ASD_ANTENNA_PORT_2,
    ASD_UNKNOW
} ASD_SELECT;

 // #define MEM_BYTE_WRITE11(addr, data)    *((volatile UINT8 *)(addr)) = (data)

void asd_init_antenna(int port)
{
    UINT32 reg_val;
    UINT32 data_val;

    if (port == 1) {
        PRINTF("[%s], ASD Auto Enabled \n", __func__);
    } else {
        PRINTF("[%s], port = %d\n", __func__,port - 1);
    }

    if (port == ASD_AUTO_ENABLE) {
        REG_ASD_WR(FUNC_OUT_EN0,0x0300); // Function out enabled RF_SW1,2
        REG_ASD_WR(RF_SW_OUTSEL0,0x10); // port selection of the RF_SW2 
        REG_ASD_WR(FCI_FEM_CTRL,0x10); // Antenna Diversity Enabled

    } else {
        REG_ASD_WR(FUNC_OUT_EN0,0x00); // Function out disable RF_SW1,2

        reg_val = REG_ASD_RD(DataOut_Set);
        data_val = reg_val | 0x01;
        REG_ASD_WR(DataOut_Set,data_val); // GPIO Data output enable set

        if (port == ASD_ANTENNA_PORT_1) {
            reg_val = REG_ASD_RD(DataOut_REG);
            data_val = reg_val | 0x01;
            REG_ASD_WR(DataOut_REG,data_val); // GPIO Output value
        } else if (port == ASD_ANTENNA_PORT_2) {
            reg_val = REG_ASD_RD(DataOut_REG);
            data_val = reg_val &(~0x01);
            REG_ASD_WR(DataOut_REG,data_val); // GPIO Output value
        }
    }
}


int  asd_set_ant_port(int port)
{
    int save_port = port + 1; // port 1 --> nvram 2, port 2 --> nvarm 3
    
    // Read ASD flag in the NVRAM, if failed, disable ASD(after factory reset)
    if (write_nvram_int(ENV_ASD_ENABLED, save_port) == -1) {
        PRINTF("[%s] Faild to set ASD port = %d \n", __func__, port);
       return -1;
    } else {
        asd_enabled = save_port;
        asd_init_antenna(save_port);
    }

    return 0;
}

void asd_init(void)
{
    // Read ASD flag in the NVRAM, if failed, disable ASD(after factory reset)
    if (read_nvram_int(ENV_ASD_ENABLED, &asd_enabled) == -1) {
        asd_enabled = 0;
        write_nvram_int(ENV_ASD_ENABLED, 0);
    } 
    
    /* Check dpm wake up/run mode */
    if (asd_enabled) {
        asd_init_antenna(asd_enabled);
    }
}

int asd_enable(int ant_type)
{
     if (write_nvram_int(ENV_ASD_ENABLED, ant_type) == -1) {
        return -1;
    } else {
        asd_init_antenna(ant_type);
        asd_enabled = ant_type;
        return 0;
    }
}

int asd_disable(void)
{
    if (write_nvram_int(ENV_ASD_ENABLED, ASD_DIABLE) == -1) {
        return -1;
    } else {
        REG_ASD_WR(FCI_FEM_CTRL,0x00); // Antenna Diversity disabled
        return 0;
    }
}

int get_asd_enabled(void)
{
    if (read_nvram_int(ENV_ASD_ENABLED, &asd_enabled) == -1) {
        PRINTF("[%s] Faild to read ASD flag , set disabled.\n", __func__);
        asd_enabled = 0;
        write_nvram_int(ENV_ASD_ENABLED, 0);
    }

    return asd_enabled;
}


/* EOF */
