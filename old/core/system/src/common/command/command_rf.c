/**
 ****************************************************************************************
 *
 * @file command_rf.c
 *
 * @brief RF console commands
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

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "clib.h"

#include "da16x_types.h"
#include "command_rf.h"
#include "oal.h"
#include "da16x_system.h"
#include "rf_meas_api.h"


#define REG_PL_WR(addr, value)       (*(volatile uint32_t *)(addr)) = (value)
#define REG_PL_RD(addr)              (*(volatile uint32_t *)(addr))

#undef OTP_TEST


//-----------------------------------------------------------------------
// Command NET-List
//-----------------------------------------------------------------------

static void cmd_tuner_verify(int argc, char *argv[]);
static void cmd_dpd_modeling(int argc, char* argv[]);

#if 0 /* remove Unused declaration of functions */
extern void dpd_modeling_data_save();
extern int dpd_modeling_wait_for_done();
extern void dpd_send_test_vec(uint32_t rate, uint32_t power);

extern void fc9000_dpd_pre_distorter(uint8_t profile);

extern void hal_machw_reset(void);
extern void rxl_reset(void);
extern void txl_reset(void);
#endif

extern void phy_rc_rf_reg_write(uint16_t addr, uint16_t value);
extern void phy_rc_rf_reg_read(uint16_t addr, uint16_t* value);

extern unsigned char cmd_lmac_tx_flag;
extern unsigned char cmd_rf_flag;

//-----------------------------------------------------------------------
// Internal Functions
//-----------------------------------------------------------------------
#ifdef OTP_TEST
static void otp_test(int argc, char* argv[]);
#define MEM_BYTE_READ(addr, data)		*data = *((volatile UCHAR *)(addr))
#define MEM_BYTE_WRITE(addr, data)		*((volatile UCHAR *)(addr)) = data
static void otp_test(int argc, char* argv[])
{
    int status;
    uint8_t wdata, rdata;
    uint32_t cnt=1, i, addr;
    uint32_t cmd;
    uint16_t wtemp;

    if (!cmd_rf_flag) {
        return ;
    }

    RTC_IOCTL(RTC_GET_LDO_CONTROL_REG, &wtemp);
    wtemp |= LDO_IP3_ENABLE(1);
    RTC_IOCTL(RTC_SET_LDO_CONTROL_REG, &wtemp);

    if (DRIVER_STRCMP(argv[1], "write") == 0) {
        addr = htoi(argv[2]);
        wdata = htoi(argv[3]);

        otp_mem_create();	//OTP PIN CTRL

        status  = otp_mem_write(addr, &wdata, 1);
        if (status != OTP_OK) {
            PRINTF("OTP ERROR =%X\n", status);
        }
    } else if ( DRIVER_STRCMP(argv[1], "read") == 0) {
        addr = htoi(argv[2]);
        if (argc == 4)
            cnt = atoi(argv[3]);

        otp_mem_create();	//OTP PIN CTRL
        for (i = 0; i < cnt; i++) {
            status  = otp_mem_read(addr+i, &rdata, 1);
            if (status != OTP_OK) {
                PRINTF("OTP ERROR =%X\n", status);
            }
            PRINTF("addr=0x%X, data=0x%X\n", addr+i,rdata);
        }

        otp_mem_close();

    } else if ( DRIVER_STRCMP(argv[1], "lock") == 0) {
        otp_mem_create();//OTP PIN CTRL

        cmd = OTP_CMD_LOCK;

        _sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);
        ASIC_DBG_TRIGGER(0xb102);

		//busy wait for done signal
        for (i = 0; i < BUSY_WAIT_TIME_OUT; i++) {
            SYSUSLEEP(1);

            MEM_BYTE_READ(OTP_ADDR_CMD, &cmd);

            if ((cmd & 0x40) == 0x40) {
                ASIC_DBG_TRIGGER(0xb120);

                PRINTF("OTP LOCK OK\n");
                break;
            } else if ((cmd&0x20)==0x20) {

            }
        }

        if (i == BUSY_WAIT_TIME_OUT) {	//check for time out
            PRINTF("OTP LOCK FAIL\n");
        }

        otp_mem_close();
    } else if ( DRIVER_STRCMP(argv[1], "bc") == 0) {
        uint32_t sysclock;
        uint32_t otpclk=20*MHz, current_status, otpcrtl_0;

        _sys_clock_read( &sysclock, sizeof(uint32_t));
        otpclk = sysclock;

        for (i = 20000000; i < 122500000; i+= 2500000) {
            otpclk = (UINT32)i;

            _sys_clock_write( &otpclk, sizeof(UINT32));

            if (i != otpclk)
                continue;

            PRINTF("clock %d\n", otpclk);

            do {
                if ((otpclk >= (15*MHz)) && (otpclk <= (30*MHz))) {
                    otpcrtl_0 = 1.5*otpclk;

                    if ( (otpcrtl_0 % 1000000) > 0)
                        otpcrtl_0 += 1000000;

                    otpcrtl_0 = otpcrtl_0/MHz + 37;

                    REG_PL_WR(0x500012D0, otpcrtl_0);
                    break;
                }

                otpclk=otpclk/2;
			} while (otpclk>=15*MHz);

            PRINTF("OTP clock = %d\n", otpclk);
            otp_mem_create();
            cmd = OTP_CMD_TEST_1;

            _sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);

            ASIC_DBG_TRIGGER(0xb102);

			//busy wait for done signal
            while (pdTRUE) {
                SYSUSLEEP(1);
                MEM_BYTE_READ(OTP_ADDR_CMD, &cmd);

                current_status=(cmd >> 5) &0x3;

                if (current_status == 1) {	//fail
                    ASIC_DBG_TRIGGER(0xb130);
                    PRINTF("FAIL status 1 \n");
                    break;
                } else if (current_status == 2) {	//success
                    ASIC_DBG_TRIGGER(0xb120);
                    PRINTF("BC Success\n");
                    break;
                } else if (current_status == 3) {
                    PRINTF("FAIL status 3 \n");
                    ASIC_DBG_TRIGGER(0xb140);
                    break;
                }
                ASIC_DBG_TRIGGER(0xb110);
                ASIC_DBG_TRIGGER(cmd);
            }

			//check for time out
            if (i == BUSY_WAIT_TIME_OUT) {
                ASIC_DBG_TRIGGER(0xb150);
            }

            otp_mem_close();
        }

        _sys_clock_write( &sysclock, sizeof(uint32_t));

    } else if ( DRIVER_STRCMP(argv[1], "tw") == 0) {
#define	FC9000_OTPROM_ADDRESS(x) (FC9000_OTPROM_BASE|((FC9000_OTPROM_SIZE-1)&(x<<3)))
        UINT32 sysclock, bakclock;
        uint32_t otp_addr=0;
        uint8_t wval=0xff;

        _sys_clock_read( &bakclock , sizeof(UINT32));

        for (i = 20000000; i < 122500000; i+= 2500000) {
            sysclock = i;

            _sys_clock_write( &sysclock , sizeof(UINT32));

            if (i != sysclock)
                continue;

            PRINTF("clock %d\n", sysclock);

            otp_mem_create();

            cmd = OTP_CMD_TEST_3;
            _sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);

            status = _sys_otp_write(FC9000_OTPROM_ADDRESS(otp_addr++), &wval, 1);
            if (status != OTP_OK){
                PRINTF("Write test = %x\n",status);
            } else {
                PRINTF("Write test OK\n",status);
            }
            otp_mem_close();
        }

        _sys_clock_write( &bakclock , sizeof(UINT32));
    } else if ( DRIVER_STRCMP(argv[1], "td") == 0) {
        UINT32 sysclock, bakclock;
        uint32_t current_status;

        _sys_clock_read( &bakclock, sizeof(UINT32));

        for (i = 20000000; i < 122500000; i+= 2500000) {
            sysclock = i;
            _sys_clock_write( &sysclock , sizeof(UINT32));
            if (i != sysclock)
                continue;
            PRINTF("clock %d\n", sysclock);

            otp_mem_create();

            //TEST DEC
            cmd = OTP_CMD_TEST_2;
            _sys_otp_ioctl(OTP_IOCTL_CMD_WRITE,&cmd);

			//busy wait for done signal
            while (pdTRUE) {
                SYSUSLEEP(10);
                MEM_BYTE_READ(OTP_ADDR_CMD, &cmd);

                current_status=(cmd >> 5) &0x3;

                if (current_status==1) {	//fail
                    PRINTF("TEST DEC FAIL\n");
                    break;
                } else if (current_status==2) {	//success
                    PRINTF("TEST DEC OK\n");
                    break;
                } else if (current_status==3) {
                    PRINTF("TEST DEC FAIL\n");
                    break;
                }
            }

            otp_mem_close();
        }
        _sys_clock_write(&bakclock,sizeof(UINT32));
    }

    RTC_IOCTL(RTC_GET_LDO_CONTROL_REG, &wtemp);
    wtemp &= (~LDO_IP3_ENABLE(1));
    RTC_IOCTL(RTC_SET_LDO_CONTROL_REG, &wtemp);
}
#endif //OTP_TEST

//-----------------------------------------------------------------------
// External Functions
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Command Functions
//-----------------------------------------------------------------------
struct phy_cmd {
    const char *cmd;
    int (*handler)(int argc, char *argv[]);
    char ** (*completion)(const char *str, int pos);
    //enum phy_ext_cmd_flags flags;
    const char *usage;
};

#if 0	// Not support in FreeRTOS SDK
void cmd_exe_phycmd(int argc, char* argv[])
{
    struct phy_cmd *cmd, *match = NULL;
    struct phy_cmd *phy_commands;
    int ret;

    RAMLIB_SYMBOL_TYPE    *ramlib;

    ramlib = (RAMLIB_SYMBOL_TYPE *)DA16X_RAMSYM_BASE;

    phy_commands = (struct phy_cmd *)(ramlib->phy_commands);

    argv = &argv[1];
    argc--;
    if (argc == 0)
        return ;

    int count = 0;

    cmd = (struct phy_cmd *)phy_commands;

    while (cmd->cmd) {
        if (!strcmp(argv[0],cmd->cmd)) {
            count=1;
            match=cmd;
            break;
        }

        ++cmd;
    }

    if (count == 0) {
        PRINTF("Unknown command '%s'\n",argv[0]);
    } else {
        ret = match->handler(argc, argv);
    }
}
#endif	// 0

void cmd_tuner_read(int argc, char *argv[])
{
    uint16_t addr;
    uint16_t data;
    uint32_t length=1, i;

    if (!cmd_rf_flag) {
        return ;
    }

    if ( argc!=2 && argc !=3){
        PRINTF("useage :ird [0xaddr] [0xlength]\n");
        return ;
    }
    addr = (uint16_t)htoi(argv[1]);
    if (argc==3)
        length = htoi(argv[2]);
    for (i=addr; i<length+addr; i++)
    {
        phy_rc_rf_reg_read((uint16_t)i, &data);
        PRINTF("Addr 0x%02X Data 0x%04X \n", i, data);
    }
}

void cmd_tuner_write(int argc, char *argv[])
{
    uint16_t addr;
    uint16_t data;

    if (!cmd_rf_flag) {
        return ;
    }

    if (argc!=3){
        PRINTF("useage :iwr [0xaddr] [0xdata]\n");
        return;
    }
    addr = (uint16_t)htoi(argv[1]);
    data = (uint16_t)htoi(argv[2]);
    phy_rc_rf_reg_write(addr, data);
}

#if 0
extern struct phy_cfg_tag *phy_param;
static void cmd_set_phy_param(int argc, char *argv[])
{
    if (argc!=2){
        PRINTF("phy_param [mode]\n");
        return;
    }
    if (phy_param == NULL)
        phy_param = (struct phy_cfg_tag*) APP_MALLOC( sizeof(struct phy_cfg_tag) );

    phy_param->parameters[0] = htoi(argv[1]);
}
#endif

#if 0
static void cmd_phy_init(int argc, char* argv[])
{
    int iter = 1 , i;
    if (argc == 2)
        iter = atoi(argv[1]);
    if (phy_param == NULL) {
        char*argv2[] = {"phy_param","1"};
        cmd_set_phy_param(2, argv2);
    }
    for (i = 0; i<iter; i++)
    {
        phy_init(phy_param);
    }
}
#endif

#define FC9000_DPD_COEF_CNT 128
#define MAX_PROFILE 6

#if 1  /* DPD COEFF use DPM */
#define RETMEM_DPD_COEFF     0x00180800
#define RETMEM_DPD_GAIN_OUT3            0x001803E0
#define RETMEM_DPD_LUT_SCALER1        0x001803E4
#else
//static uint32_t dpd_coef[MAX_PROFILE][FC9000_DPD_COEF_CNT];
//static uint32_t dpd_gain_out3[MAX_PROFILE], dpd_lut_scaler1[MAX_PROFILE], dpd_lut_scaler2[MAX_PROFILE];
#endif

void dpd_recover_maxim()
{
    //BB setting
    REG_PL_WR(0x60C00908, 0x3);
    REG_PL_WR(0x60c500c0, 0x180);
    //end of BB
    phy_rc_rf_reg_write(0xe0,0x0004);
    phy_rc_rf_reg_write(0x02,0x0110);
    phy_rc_rf_reg_write(0x03,0x0000);
    phy_rc_rf_reg_write(0x04,0x0000);
    phy_rc_rf_reg_write(0x07,0x0100);
    phy_rc_rf_reg_write(0x09,0x0003);
    phy_rc_rf_reg_write(0x3c,0x116b);
    phy_rc_rf_reg_write(0x08,0x0000);
    //phy_rc_rf_reg_write(0x9a,0x4002);
    phy_rc_rf_reg_write(0xd6,0x4860);
    phy_rc_rf_reg_write(0xd7,0x02cf);
    phy_rc_rf_reg_write(0xda,0x0001);
    phy_rc_rf_reg_write(0xda,0x0000);
}

struct txp_s{
    u8 rfch;
    u32 rate;
    u32 power;
    u32 power2;
    u32 len;
};

extern SemaphoreHandle_t txp_lock;
extern TaskHandle_t test_thread_type;
extern void txp_thread(void* thread_input);
extern int txp_on_off;

void dpd_modeling_tx_gen(uint8_t flag, uint32_t rate, uint32_t length)
{
    if (flag == 1) {
		if (cmd_lmac_tx_flag) {
			txp_on_off= 1;
		}
        struct txp_s param;
        param.rfch = 0;
        param.rate = rate;  // MCS0, MCS7
        param.power = 1;
		param.power2 = 1;
        param.len = length;  // length
        PRINTF("TX ON.\n");

        //REG_PL_WR(0x50001304, 0x59597e);
        //REG_PL_WR(0x50001360, 0x595959);
        REG_PL_WR(0x60C00908, 0x2);
        REG_PL_WR(0x60c500c0, 0x1e0);
        //REG_PL_WR(0x50001300, 0x100);

        txp_lock = xSemaphoreCreateMutex();
		if (cmd_lmac_tx_flag) {
			xTaskCreate( txp_thread,
					"TestThread",
					512,
					( void * )(&param),
					OS_TASK_PRIORITY_SYSTEM+9,
					&test_thread_type );
		}
        return;
    }
    else if (flag == 0) {
		if (cmd_lmac_tx_flag) {
			txp_on_off = 0;
		}
        PRINTF("TX off.\n");
		if (cmd_lmac_tx_flag) {
			if (test_thread_type) {
				TaskHandle_t l_task = NULL;
				l_task = test_thread_type;
				test_thread_type = NULL;
				vTaskDelete(l_task);
			}
		}
        vSemaphoreDelete(&txp_lock);
    }

}

#define DPD_CONT_TX_MODE
void fc9000_dpd_modeling()
{
#if 0	// Non-used code : compile warning ...
    int vec = 7;
    int trial = 1;
    UINT add_length = 100;
    UINT cont_tx = 1;
    UINT rfch = 0;
#else
    UINT rate = 0x1007;
    UINT power = 1;
#endif	// 0

#if 1	// Becuase of compile warning ...
#ifdef DPD_CONT_TX_MODE
	extern void dpd_send_test_vec(uint32_t rate, uint32_t power);
	extern int dpd_modeling_wait_for_done();
	extern  void hal_machw_reset(void );
	extern void rxl_reset(void);
	extern void txl_reset(void);
#endif
	extern void dpd_modeling_data_save();
#endif	// 1

    //dpd_modeling_mode_init(0,0,0);
#ifdef DPD_CONT_TX_MODE
    dpd_send_test_vec(rate, power);
#else
    dpd_modeling_tx_gen(1, 0x1007, 0xf000);
#endif
    dpd_modeling_wait_for_done(0);
    REG_PL_WR(0x60c0f280,0x18010b); //DAC OUT = DPD_PUT
    dpd_recover_maxim();

#ifdef DPD_CONT_TX_MODE
  hal_machw_reset();
  rxl_reset();
  txl_reset();
 // nxmac_next_state_setf(HW_ACTIVE);
#endif

    //dpd_modeling_tx_gen(0, 0, 0);
    dpd_modeling_data_save(0);
}



static void cmd_dpd_modeling(int argc, char* argv[])
{
#if 1
    uint32_t rate=0x1007, length=0xf000;
    int flag = 0;
    int step = 0;

	extern void fc9000_dpd_pre_distorter(uint8_t profile);
	extern int dpd_modeling_wait_for_done();
	extern void dpd_modeling_data_save();

    if (!cmd_rf_flag) {
        return ;
    }

    if (argc == 2){
        flag = atoi(argv[1]);

        if (flag==0)
            fc9000_dpd_modeling();
        else if (flag == 1)
            fc9000_dpd_pre_distorter(0);
    }
    if (argc >= 3){
        flag = atoi(argv[1]);
        step = (int)htoi(argv[2]);
        if (step & 1){
            if (argc == 8)
                rate = htoi(argv[7]);
            if (argc == 9) {
                rate = htoi(argv[7]);
                length = htoi(argv[8]);
            }
            PRINTF("tx gen rate   = 0x%x\n", rate);
            PRINTF("tx gen length = 0x%x\n", length );

            dpd_modeling_tx_gen(1, rate, length);
            //OAL_MSLEEP(30);
            if (argc >=4)
            	vTaskDelay((uint16_t)atoi(argv[3]));
        }
        if (step & 2)
        {
#if 0	// Non-used code : compile warning ...
            uint8_t manual_mode=0;
#endif	// 0
            uint16_t param1=0, param2=0;
            if (argc>=7)
            {
#if 0	// Non-used code : compile warning ...
				manual_mode = atoi(argv[4]);
#endif	// 0
                param1 = (uint16_t)htoi(argv[5]);
                param2 = (uint16_t)htoi(argv[6]);
                PRINTF("MANUAL mode on,0x03=%04X, 0x06=%04X\n",param1, param2);
            }
            //dpd_modeling_mode_init(manual_mode,param1,param2);
        }
        if (step & 4)
#if 0	// Non-used code : compile warning ...
            dpd_modeling_wait_for_done(0);
#else
            dpd_modeling_wait_for_done();
#endif	// 0
        if (step & 8)
            REG_PL_WR(0x60c0f280,0x48010b); //TimingIn=0
        if (step & 16)
            dpd_recover_maxim();
        if (step & 32)
            dpd_modeling_tx_gen(0, 0, 0);
        if (step & 64 )
            dpd_modeling_data_save(0);
    }
#endif
}

#define RF_REG_NUM 73
static void cmd_tuner_verify(int argc, char *argv[])
{
	DA16X_UNUSED_ARG(argc);

#if 1
    uint8_t addr;
    uint16_t wdata, rdata;
    int i;
    int iter=1;

    if (!cmd_rf_flag) {
        return ;
    }

    addr = (uint8_t)htoi(argv[1]);
    iter = (int)htoi(argv[2]);
    if (addr == 0){
        uint8_t check = 0;
        uint8_t addr_table[RF_REG_NUM] ={
            0x04, 0x07, 0x09, 0x39, 0x3a, 0x3b, 0x3e, 0x3f, 0x40,
            0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a,
            0x4b, 0x4c, 0x4d, 0x4e, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c,
            0x5d, 0x5e, 0x62, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
            0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x72, 0x76,
            0x77, 0x78, 0x79, 0x7d, 0x7e, 0x7f, 0x80, 0x82, 0x83, 0x84,
            0x85, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x97,
            0x98, 0x99, 0x9b, 0x9c, 0x9d, 0x9e, 0xcc, 0xcf
        };

        while (iter--) {
            phy_rc_rf_reg_write(0x00, 0xff);
            for (i = 0; i < RF_REG_NUM; i++){
                addr = addr_table[i];

                PRINTF("\naddr = 0x%02X",addr);

                for (wdata = 0; wdata < 0xffff; wdata++) {
                    phy_rc_rf_reg_write(addr, wdata);
                    phy_rc_rf_reg_read(addr, &rdata);

                    if (wdata!=rdata){
                        PRINTF("Mismatch Data :  addr[0x%X] write [0x%X] : read [0x%X]\n",addr, wdata, rdata);
                        check = 1;
                        break;
                    }
                    else{
                        //PRINTF(".");
                    }
                }
                PRINTF(": %s",check?"FAIL":"OK");
                check = 0;
            }
        }
    } else {
        for (i = 0; i < iter; i++) {
            phy_rc_rf_reg_write((uint16_t)addr, (uint16_t)i);
            phy_rc_rf_reg_read(addr, &rdata);

            if (i!=rdata) {
                PRINTF("Mismatch Data :  addr[0x%X] write [0x%X] : read [0x%X]\n",addr, i, rdata);
            } else {
                PRINTF(".");
            }
        }
    }
#endif
}

const COMMAND_TREE_TYPE	cmd_rf_list[] = {
  { "RF",			CMD_MY_NODE,	cmd_rf_list,	NULL,		"RF "			},		// Head

  { "-------",		CMD_FUNC_NODE,	NULL,	NULL,					"--------------------------------"} ,

  { "dpd",			CMD_FUNC_NODE,	NULL,		&cmd_dpd_modeling,			"dpd [0/1]"					},
  { "ird",			CMD_FUNC_NODE,	NULL,		&cmd_tuner_read,			"tuner read cmd"			},
  { "iwr",			CMD_FUNC_NODE,	NULL,		&cmd_tuner_write,			"tuner write cmd"			},
  { "iverify",		CMD_FUNC_NODE,	NULL,		&cmd_tuner_verify,			"tuner write cmd"			},
#ifdef OTP_TEST
  { "otp",			CMD_FUNC_NODE,	NULL,		&otp_test,					"otp test"					},
#endif //OTP_TEST

#if 0	// Not support in FreeRTOS SDK
  { "phy",			CMD_FUNC_NODE,	NULL,		&cmd_exe_phycmd,			"excute phy and RF cmd"		},
  { "phy_param",	CMD_FUNC_NODE,	NULL,		&cmd_set_phy_param,			"calmode cmd"				},
  { "phy_init",		CMD_FUNC_NODE,	NULL,		&cmd_phy_init,				"phy_init cmd"				},
#endif	// 0

#if 0	// Not supported yet in FreeRTOS version
  { "btcoex",		CMD_FUNC_NODE,	NULL,		&cmd_rf_meas_btcoex,		"Measureing BTCOEX"			},
#endif	// 0

#if 0	// Move to command_root_cmd.c
  { "MonRxTaRssi",	CMD_FUNC_NODE,	NULL,		&cmd_rf_meas_MonRxTaRssi,	"Measureing MonRxTaRssi"	},
  { "MonRxTaCfo",	CMD_FUNC_NODE,	NULL,		&cmd_rf_meas_MonRxTaCfo,	"Measureing MonRxTaCfo"		},
#endif	// 0

  { NULL,			CMD_NULL_NODE,	NULL,	NULL,				NULL }						// Tail
};


/* EOF */
