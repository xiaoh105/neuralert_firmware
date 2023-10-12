/**
 ****************************************************************************************
 *
 * @file atcmd_peripheral.c
 *
 * @brief AT-Command Peripheral device test
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


#include "da16x_system.h"
#include "atcmd.h"
#include "hal.h"
#include "clib.h"

#if defined (__SUPPORT_PERI_CMD__)
#include "atcmd_peripheral.h"
#include "da16200_ioconfig.h"
#endif

#if defined (__SUPPORT_LMAC_RF_CMD__)
#include "pwm.h"
#include "sys_i2s.h"
#include "sys_i2c.h"
#include "sys_image.h"
#include "adc.h"


/******************************************************************************
 * Function Name : atcmd_testmode
 * Discription : atcmd_testmode
 * Arguments : argc, argv[]
 * Return Value : result_code
 *****************************************************************************/
int atcmd_testmode(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);

    int status = AT_CMD_ERR_CMD_OK;
    char result_str[128];

    memset(result_str, 0, 128);

    if (strcasecmp(argv[0] + 5, "RFNOINIT") == 0) {
        if (argv[1][0] != '?') {
            /* AT+TMRFNOINIT=x */
            UINT init_flag = 1;

            if (argv[1][0] == '1') {
                init_flag = 0;
            }

            set_wlaninit(init_flag);
        } else {
            /* AT+TMRFNOINIT=? */
            sprintf(result_str, "%u", !get_wlaninit());
        }
    } else if (strcasecmp(argv[0] + 5, "LMACINIT") == 0) {
        if (argv[1][0] != '?') {
            /* AT+TMLMACINIT */
            extern int lmacinit(void);
            status = lmacinit();
        } else {

        }
    } else {
        status = AT_CMD_ERR_UNKNOWN_CMD;
    }

    /* Response Message */
    if (strlen(result_str) > 0) {
        PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, result_str);
    }

    return status;
}



static void temp_cal_off(void)
{
/*
    hpi_fc9000_write(0x3F,0x0606);
    hpi_fc9000_write(0x40,0x0606);
    hpi_fc9000_write(0x41,0x0606);
    hpi_fc9000_write(0x42,0x0606);
    hpi_fc9000_write(0x43,0x0606);
    hpi_fc9000_write(0x44,0x0606);
    hpi_fc9000_write(0x45,0x0606);
    hpi_fc9000_write(0x46,0x0606);
    hpi_fc9000_write(0x4F,0);
    hpi_fc9000_write(0x50,0);
    hpi_fc9000_write(0x51,0);
    hpi_fc9000_write(0x52,0);
    hpi_fc9000_write(0x53,0);
    hpi_fc9000_write(0x54,0);
    hpi_fc9000_write(0x55,0);
    hpi_fc9000_write(0x56,0);
*/
}

extern void get_per(UINT32 reset, UINT32 *pass, UINT32 *fcs, UINT32 *phy, UINT32 *overflow);
/* RF Test cmd */
int atcmd_rftest(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);

    int status = ERR_OK;

    if (strcasecmp(argv[0] + 9, "START") == 0) {
        /* AT+TMRFNOINIT=x */
        extern void cmd_lmac_ops_start(int argc, char *argv[]);
        extern int lmacinit(void);

        status = lmacinit();
        cmd_lmac_ops_start(0, NULL);

        /* MAC HW Active Clock Gating OFF  */
        (*((volatile UINT *)(0x60b0004c))) = 0x200cf41;
        temp_cal_off();

////// For RF TEST Mode Register ...
#define MEM_LONG_WRITE(addr, data) *((volatile UINT *)(addr)) = data

        MEM_LONG_WRITE(0x60f008fc, 0x0808);
        MEM_LONG_WRITE(0x60f00900, 0x0808);
        MEM_LONG_WRITE(0x60f00904, 0x0808);
        MEM_LONG_WRITE(0x60f00908, 0x0808);
        MEM_LONG_WRITE(0x60f0090c, 0x0808);
        MEM_LONG_WRITE(0x60f00910, 0x0808);
        MEM_LONG_WRITE(0x60f00914, 0x0808);
        MEM_LONG_WRITE(0x60f00918, 0x0808);

        MEM_LONG_WRITE(0x60f0093c, 0x0000);
        MEM_LONG_WRITE(0x60f00940, 0x0000);
        MEM_LONG_WRITE(0x60f00944, 0x0000);
        MEM_LONG_WRITE(0x60f00948, 0x0000);
        MEM_LONG_WRITE(0x60f0094c, 0x0000);
        MEM_LONG_WRITE(0x60f00950, 0x0000);
        MEM_LONG_WRITE(0x60f00954, 0x0000);
        MEM_LONG_WRITE(0x60f00958, 0x0000);
////////////////////////////////////////////////////////////
    } else if (strcasecmp(argv[0] + 9, "STOP") == 0) {
        /* AT+TMLMACINIT */
        extern void cmd_lmac_ops_stop(int argc, char *argv[]);
        cmd_lmac_ops_stop(0, NULL);
    } else {
        status = AT_CMD_ERR_UNKNOWN_CMD;
    }

    if (status == ERR_OK) {
        return AT_CMD_ERR_CMD_OK;
    }

    return AT_CMD_ERR_UNKNOWN_CMD;
}

extern int    txp_on_off;
extern void    cmd_lmac_rftx(int argc, char *argv[]);
int atcmd_rftx(int argc, char *argv[])
{
    int status = ERR_OK;

    if (strcasecmp(argv[0] + 5, "TX") == 0) {
        cmd_lmac_rftx(argc, argv);
    } else if (strcasecmp(argv[0] + 5, "TXSTOP") == 0) {
        txp_on_off = 0;
    } else {
        status = AT_CMD_ERR_UNKNOWN_CMD;
    }

    if (status == ERR_OK) {
        return AT_CMD_ERR_CMD_OK;
    }

    return AT_CMD_ERR_UNKNOWN_CMD;
}

extern void    cmd_lmac_per(int argc, char *argv[]);
extern int    set_rf_channel(unsigned int freq);
//extern void    nxmac_rx_cntrl_set(uint32_t value);
extern int    set_device_mac(char *str_mac);
extern unsigned int htoi(char *s);
int atcmd_rfrxtest(int argc, char *argv[])
{
    int Ch = 0;
#if 0
    int BW = 0;
    int ant = 0;
#endif    /* 0 */
    unsigned int FilterFrameType = 0;
    int per_argc = 2;
    char *str_mac = NULL;
    char *per_argv[2] = {"per", "start"};

    switch ( argc ) {
    case 6:
        /* fall through */
    case 5: {
        str_mac = argv[4];
    }
        /* fall through */
    case 4: {
        FilterFrameType = htoi(argv[3]);
        //nxmac_rx_cntrl_set(FilterFrameType);
        (*(volatile uint32_t *)(0x60B00060)) = (FilterFrameType);
    }
        /* fall through */
    case 3:
        /* fall through */
    case 2: {
        Ch = atoi(argv[1]);
    } break;

    default: {
    } break;

    } // End of Switch

    if (str_mac) {
        if (set_device_mac(str_mac)) {
            return AT_CMD_ERR_UNKNOWN_CMD;
        }
    }

    if (Ch) {
        ATCMD_DBG("Set RF Channel Frequency %d\n", Ch);
        set_rf_channel(Ch);
    }

    cmd_lmac_per(per_argc, per_argv);
    return AT_CMD_ERR_CMD_OK;
}

int atcmd_rfrxstop(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    int per_argc = 2;
    char *per_argv[2] = {"per", "stop"};

    cmd_lmac_per(per_argc, per_argv);
    return AT_CMD_ERR_CMD_OK;
}

extern void cmd_lmac_rfcwtest(int argc, char *argv[]);
extern void cmd_lmac_rfcwstop(int argc, char *argv[]);
extern void set_rf_cw_pll();
extern void reset_rf_cw_pll();
int atcmd_rfcwttest(int argc, char *argv[])
{
    int status = ERR_OK;

    if (strcasecmp(argv[0] + 7, "TEST") == 0) {
        set_rf_cw_pll();
        cmd_lmac_rfcwtest(argc, argv);
    } else if (strcasecmp(argv[0] + 7, "STOP") == 0) {
        cmd_lmac_rfcwstop(argc, argv);
        reset_rf_cw_pll();
    } else {
        status = AT_CMD_ERR_UNKNOWN_CMD;
    }

    return status;
}

extern void get_per(UINT32 reset, UINT32 *pass, UINT32 *fcs, UINT32 *phy, UINT32 *overflow);
int atcmd_rf_per(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);

    int status = ERR_OK;

    if (strcasecmp(argv[0] + 5, "PER") == 0) {
        UINT32 pass, fcs, phy, overflow;
        get_per(0, &pass, &fcs, &phy, &overflow);
        PRINTF_ATCMD("\r\n%d %d %d %d\r\n", pass, fcs, phy, overflow);
    } else if (strcasecmp(argv[0] + 5, "PERRESET") == 0) {
        PRINTF_ATCMD("\r\nRESET PER COUNT !\r\n");
        get_per(1, NULL, NULL, NULL, NULL);
    } else {
        status = AT_CMD_ERR_UNKNOWN_CMD;
    }

    if (status == ERR_OK) {
        return AT_CMD_ERR_CMD_OK;
    }

    return AT_CMD_ERR_UNKNOWN_CMD;
}

extern void tx_cont_start(int argc, char *argv[]);
extern void tx_cont_stop();
int atcmd_rf_cont(int argc, char *argv[])
{
    int status = ERR_OK;

    if (strcasecmp(argv[0] + 9, "START") == 0) {
        tx_cont_start(argc, argv);
    } else if (strcasecmp(argv[0] + 9, "STOP") == 0) {
        tx_cont_stop();
    } else {
        status = AT_CMD_ERR_UNKNOWN_CMD;
    }

    if (status == ERR_OK) {
        return AT_CMD_ERR_CMD_OK;
    }


    return AT_CMD_ERR_UNKNOWN_CMD;
}

int atcmd_rf_chan(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);

    int status;
    int freq = atoi(argv[1]);

    status = set_rf_channel(freq);
    if (status == ERR_OK) {
        return AT_CMD_ERR_CMD_OK;
    }

    return AT_CMD_ERR_UNKNOWN_CMD;
}

#else

int atcmd_rf_chan(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    return -1;    // AT_CMD_ERR_UNKNOWN_CMD
}

int atcmd_rftx(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    return -1;    // AT_CMD_ERR_UNKNOWN_CMD
}

int atcmd_rftest(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    return -1;    // AT_CMD_ERR_UNKNOWN_CMD
}
#endif /* __SUPPORT_LMAC_RF_CMD__ */


#if defined (__SUPPORT_PERI_CMD__)
int atcmd_spiconf(int argc, char *argv[])
{
#define SPI_SLAVE_REG        0x50080240
    int cpol = 0, cpha = 0;

    if (argc != 3) {
        //PRINTF_HOST("Usage : %s <clock polarity>, <clock phage>\n");
        return AT_CMD_ERR_WRONG_ARGUMENTS;
    }

    cpol = ctoi(argv[1]);    /* bit[7] */
    cpha = ctoi(argv[2]);   /* bit[6] */

    (*((volatile UINT *)(SPI_SLAVE_REG))) = (*((volatile UINT *)SPI_SLAVE_REG) & 0xffffff3f);
    (*((volatile UINT *)(SPI_SLAVE_REG))) = (*((volatile UINT *)SPI_SLAVE_REG) | (cpol << 7) | (cpha << 6));

    return AT_CMD_ERR_CMD_OK;
}

int atcmd_xtal(int argc, char *argv[])
{
    extern void dpm_set_otp_xtal40_offset(unsigned long otp_xtal40_offset);
    UINT value = 0;

    if (strcasecmp(argv[0] + 7, "WR") == 0) {
        if (argc > 1) {
            value = htoi(argv[1]);
        }

        if (value > 127) {
            ATCMD_DBG("maximum value is 127\n");
            return AT_CMD_ERR_WRONG_ARGUMENTS;
        }

        DA16200_COMMON->XTAL40Mhz &= 0xffff80ff;
        DA16200_COMMON->XTAL40Mhz |= value << 8;

        dpm_set_otp_xtal40_offset(value);
    } else if (strcasecmp(argv[0] + 7, "RD") == 0) {
        PRINTF_ATCMD("\r\n0x%02x\r\n", (DA16200_COMMON->XTAL40Mhz & 0x7f00) >> 8);
    }

    return AT_CMD_ERR_CMD_OK;
}

extern void    DA16X_SecureBoot_OTPLock(UINT32 mode);
int atcmd_uotp(int argc, char *argv[])
{
    unsigned char wdata[256]={0,}, rdata[256];
    unsigned int addr, cnt = 1, i;
    int status = OTP_OK;

    // at_otp read
    otp_mem_create();    //OTP PIN CTRL
    otp_mem_lock_read((0x3ffc >> 2), (UINT32 *)rdata);
    otp_mem_close();

    //    *(unsigned int*)(0x50001288) &=0xfb;//interrupt diable
    if (strcasecmp(argv[0] + 7, "RDASC") == 0) {
        if (argc > 1) {
            if(strlen(argv[1]) > 8) {
                return AT_CMD_ERR_INSUFFICENT_ARGS;
            }
            addr = htoi(argv[1]);
        } else {
            addr = 0;
        }

        if (argc > 2) {
            cnt = htoi(argv[2]);
        }

        if (cnt > 128) {
            ATCMD_DBG("maximum length is 128\n");
            return AT_CMD_ERR_INSUFFICENT_ARGS;
        }

        if (addr > 0x7fc) {
            ATCMD_DBG("addr range < 0x800\n");
            return AT_CMD_ERR_INSUFFICENT_ARGS;
        }

        otp_mem_create();//OTP PIN CTRL

        for (i = 0; i <= (cnt - 1) / 4; i++) {
            status = otp_mem_read((addr / 4) + (i), (UINT32 *)(&rdata[i * 4]));
            if (status != OTP_OK) {
                ATCMD_DBG("OTP ERROR =%X\n", status);
                return AT_CMD_ERR_UNKNOWN_CMD;
            }

            ATCMD_DBG("addr=0x%X, data=0x%X\n", addr + i, rdata[i]);
        }

        otp_mem_close();
        PRINTF_ATCMD("\r\n");

        for (i = 0; i < cnt; i++) {
            PRINTF_ATCMD("%02x", rdata[i]);
        }

        PRINTF_ATCMD("\r");
    }
    // at+otp write
    else if (strcasecmp(argv[0] + 7, "WRASC") == 0) {
        char temp[3];
        temp[2] = 0;

        if (argc < 4) {
            ATCMD_DBG("at+otp write [addr] [cnt] [data]\n");
        }

        if(strlen(argv[1]) > 8) {
            return AT_CMD_ERR_INSUFFICENT_ARGS;
        }

        addr = htoi(argv[1]);
        cnt = htoi(argv[2]);

        if (cnt > 128) {
            ATCMD_DBG("maximum length is 128\n");
        }

        if (addr > 0x7fc) {
            ATCMD_DBG("addr range < 0x800\n");
            return AT_CMD_ERR_INSUFFICENT_ARGS;
        }

        for (i = 0; i < cnt; i++) {
            temp[0] = (unsigned char)(*(argv[3] + (i * 2)));
            temp[1] = (unsigned char)(*(argv[3] + (i * 2 + 1)));

            wdata[i] = htoi(temp);
        }

        DA16X_SecureBoot_OTPLock(1);
        otp_mem_create();    //OTP PIN CTRL

        for (i = 0; i <= (cnt - 1) / 4; i++) {
            status = otp_mem_write((addr / 4) + (i), *(((UINT32 *)wdata) + i));
            if (status != OTP_OK) {
                ATCMD_DBG("OTP ERROR =%X\n", status);
                DA16X_SecureBoot_OTPLock(0);
                return AT_CMD_ERR_UNKNOWN_CMD;
            }
        }

        DA16X_SecureBoot_OTPLock(0);
    }    // at+otp blank

    return AT_CMD_ERR_CMD_OK;
}

int atcmd_get_uversion(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    char str[256];

    strcat(str, (char *)ramlib_get_logo());
    strcat(str, "OS");
    strcat(str, __DATE__);
    strcat(str, " ");
    strcat(str, __TIME__);

    PUTS_ATCMD(str, strlen(str));

    return AT_CMD_ERR_CMD_OK;
}


int atcmd_set_TXPower(int argc, char *argv[])
{
    unsigned char    gmode_txrf_proc, txpga_gmode_cal;
    unsigned short    temp;

    if (argc > 2) {
        gmode_txrf_proc = htoi(argv[1]);
        txpga_gmode_cal = htoi(argv[2]);

        if ((gmode_txrf_proc > 0x7f) || (txpga_gmode_cal > 0x3)) {
            return AT_CMD_ERR_UNKNOWN_CMD;
        }

        phy_rc_rf_reg_read(0x0261, &temp);
        phy_rc_rf_reg_write(0x0261, (temp & 0xFF80) | (gmode_txrf_proc & 0x7f));

        phy_rc_rf_reg_read(0x0263, &temp);
        phy_rc_rf_reg_write(0x0263, (temp & 0xCFFF) | (txpga_gmode_cal << 12));

        return AT_CMD_ERR_CMD_OK;
    }

    return AT_CMD_ERR_INSUFFICENT_ARGS;
}

#define FLASH_READ_BURST_LENGTH        (4096)


extern HANDLE    flash_image_open(UINT32 mode, UINT32 imghdr_offset, VOID *locker);

int atcmd_flash_dump(int argc, char *argv[])
{
    unsigned char *data = NULL;
    unsigned int  addr, length, numBlocks, i;
    HANDLE        handler;

    if (argc == 3) {

        if(strlen(argv[1]) > 8) {
            return AT_CMD_ERR_INSUFFICENT_ARGS;
        }

        addr = htoi(argv[1]);
        length = ctoi(argv[2]);

        data = (unsigned char *)ATCMD_MALLOC(FLASH_READ_BURST_LENGTH);
        if (data == NULL) {
            return AT_CMD_ERR_UNKNOWN_CMD;
        }

        if (addr > 0x3fffff) {
            ATCMD_DBG("addr range < 0x400000\n");
            return AT_CMD_ERR_INSUFFICENT_ARGS;
        }

        handler = flash_image_open((sizeof(UINT32) * 8), (UINT32)addr, (VOID *)&da16x_environ_lock);

        numBlocks = length / FLASH_READ_BURST_LENGTH;

        PRINTF_ATCMD("\r\n");

        for (i = 0; i < numBlocks; i++) {
            flash_image_read(handler, addr, (void *)data, FLASH_READ_BURST_LENGTH);
            length    -= FLASH_READ_BURST_LENGTH;
            addr    += FLASH_READ_BURST_LENGTH;

            PUTS_ATCMD((char *)data, FLASH_READ_BURST_LENGTH);
        }

        if (length > 0) {
            flash_image_read(handler, addr, (void *)data, length );
            PUTS_ATCMD((char *)data, length);
        }

        flash_image_close(handler);
        ATCMD_FREE(data);
        return AT_CMD_ERR_CMD_OK;
    }

    return AT_CMD_ERR_INSUFFICENT_ARGS;
}

int atcmd_gpio_test(int argc, char* argv[])
{
    static DA16X_IO_ALL_PINMUX pinmux_all;
    static HANDLE * _gpio[3];
    UINT32 port, pins, direction, out_level;
    UINT16 data;

    if ( (strcasecmp(argv[0]+7, "START") == 0)    ) {
        if (argc == 4) {
            port = htoi(argv[1]);
            pins = htoi(argv[2]);
            direction = ctoi(argv[3]);

            if (port != GPIO_UNIT_A && port != GPIO_UNIT_C) {
                return AT_CMD_ERR_INSUFFICENT_ARGS;
            }

            if (pins & (UINT32)(0xffff<<16)) {
                return AT_CMD_ERR_INSUFFICENT_ARGS;
            }

            if (direction > 1) {
                return AT_CMD_ERR_INSUFFICENT_ARGS;
            }

            if ((port == GPIO_UNIT_A) && (pins & 0x3)) {
                _da16x_io_pinmux(PIN_AMUX, AMUX_GPIO);
            }
            if ((port == GPIO_UNIT_A) && (pins & 0xc)) {
                _da16x_io_pinmux(PIN_BMUX, BMUX_GPIO);
            }
            if ((port == GPIO_UNIT_A) && (pins & 0x30)) {
                _da16x_io_pinmux(PIN_CMUX, CMUX_GPIO);
            }
            if ((port == GPIO_UNIT_A) && (pins & 0xc0)) {
                _da16x_io_pinmux(PIN_DMUX, DMUX_GPIO);
            }
            if ((port == GPIO_UNIT_A) && (pins & 0x300)) {
                _da16x_io_pinmux(PIN_EMUX, EMUX_GPIO);
            }
            if ((port == GPIO_UNIT_A) && (pins & 0xc00)) {
                _da16x_io_pinmux(PIN_FMUX, FMUX_GPIO);
            }
            if ((port == GPIO_UNIT_A) && (pins & 0x8000)) {
                _da16x_io_pinmux(PIN_HMUX, HMUX_ADGPIO);
            }
            if ((port == GPIO_UNIT_C) && (pins & 0x1c0)) {
                _da16x_io_pinmux(PIN_UMUX, UMUX_GPIO);
            }

            _gpio[port] = GPIO_CREATE(port);
            GPIO_INIT(_gpio[port]);

            if (direction) {
                /* output */
                GPIO_IOCTL(_gpio[port], GPIO_SET_OUTPUT, &pins);
            } else {
                /* input */
                GPIO_IOCTL(_gpio[port], GPIO_SET_INPUT, &pins);
            }

            return AT_CMD_ERR_CMD_OK;
        }

        return AT_CMD_ERR_INSUFFICENT_ARGS;
    } else if ( (strcasecmp(argv[0]+7, "RD") == 0) ) {
        if (argc == 3) {
            port = htoi(argv[1]);
            pins = htoi(argv[2]);

            if (port != GPIO_UNIT_A && port != GPIO_UNIT_C) {
                return AT_CMD_ERR_INSUFFICENT_ARGS;
            }

            if (pins & (UINT32)(0xffff<<16)) {
                return AT_CMD_ERR_INSUFFICENT_ARGS;
            }

            GPIO_READ(_gpio[port], pins, &data, sizeof(data));
            PRINTF_ATCMD("\r\n0x%04x\r\n",data);
            return AT_CMD_ERR_CMD_OK;
        }

        return AT_CMD_ERR_INSUFFICENT_ARGS;
    } else if ( (strcasecmp(argv[0]+7, "WR") == 0) ) {
        if (argc == 4) {
            port = htoi(argv[1]);
            pins = htoi(argv[2]);
            out_level = htoi(argv[3]);

            if (port != GPIO_UNIT_A && port != GPIO_UNIT_C) {
                return AT_CMD_ERR_INSUFFICENT_ARGS;
            }

            if (pins & (UINT32)(0xffff<<16)) {
                return AT_CMD_ERR_INSUFFICENT_ARGS;
            }

            if (out_level > 1) {
                return AT_CMD_ERR_INSUFFICENT_ARGS;
            }

            if (out_level) {
                /* output high */
                data = 0xffff;
            } else {
                /* output low */
                data = 0;
            }

            GPIO_WRITE(_gpio[port], pins, &data, sizeof(data));
            return AT_CMD_ERR_CMD_OK;
        }

        return AT_CMD_ERR_INSUFFICENT_ARGS;
    } else if ( (strcasecmp(argv[0]+3, "SAVE_PININFO") == 0) ) {
        _da16x_io_get_all_pinmux(&pinmux_all);

        return AT_CMD_ERR_CMD_OK;
    } else if ( (strcasecmp(argv[0]+3, "RESTORE_PININFO") == 0) ) {
        _da16x_io_set_all_pinmux(&pinmux_all);

        return AT_CMD_ERR_CMD_OK;
    } else {
        return AT_CMD_ERR_NOT_SUPPORTED;
    }
}

#define ATCMD_US2CLK(us)   ((((unsigned long long ) us) << 9ULL) / 15625ULL)
#define MAX_USEC			0x1FFFFF
int atcmd_sleep_ms(int argc, char *argv[])
{
    UINT32    time, ioctldata;
    extern void     RTC_INTO_POWER_DOWN(UINT32 flag, unsigned long long period);

    if (argc == 2) {
        time = ctoi(argv[1]);
        if (time < 1 || time > MAX_USEC)
        	return AT_CMD_ERR_COMMON_ARG_RANGE;

        RTC_READY_POWER_DOWN(1);

        /* disable brown out black out */
        RTC_IOCTL(RTC_GET_RTC_CONTROL_REG, &ioctldata);
        ioctldata &= (UINT32)(~(BROWN_OUT_INT_ENABLE(1) | BLACK_OUT_INT_ENABLE(1)));
        RTC_IOCTL(RTC_SET_RTC_CONTROL_REG, &ioctldata);

        RTC_IOCTL(RTC_GET_BOR_CIRCUIT, &ioctldata);
        ioctldata &= (UINT32)(~(0x300));
        RTC_IOCTL(RTC_SET_BOR_CIRCUIT, &ioctldata);

        RTC_SET_RETENTION_FLAG();
        da16x_abmsecure->c_abm_req_start = 2;
        RTC_INTO_POWER_DOWN(5, ATCMD_US2CLK(time * 1000));
    }

    return AT_CMD_ERR_WRONG_ARGUMENTS;
}
#endif    // __SUPPORT_PERI_CMD__

/* EOF */
