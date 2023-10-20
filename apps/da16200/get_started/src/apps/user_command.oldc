/**
 ****************************************************************************************
 *
 * @file user_command.c
 *
 * @brief Console command specified by customer
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

#include "user_command.h"


#include "assert.h"
#include "clib.h"
struct phy_chn_info
{
    uint32_t info1;
    uint32_t info2;
};

UCHAR user_power_level[14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
UCHAR user_power_level_dsss[14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* External global functions */
extern int fc80211_set_tx_power_table(int ifindex, unsigned int *one_regcode_start_addr, unsigned int *one_regcode_start_addr_dsss);
extern int fc80211_set_tx_power_grade_idx(int ifindex, int grade_idx , int grade_idx_dsss );
extern void phy_get_channel(struct phy_chn_info *info, uint8_t index);


int freq_to_channel(unsigned char band, unsigned short freq)
{
    int channel = 0;

    // 2.4.GHz
    if (band == 0) {
        // Check if frequency is in the expected range
        if ((freq < 2412) || (freq > 2484)) {
            PRINTF("Wrong channel\n");
            return (channel);
        }

        // Compute the channel number
        if (freq == 2484) {
            channel = 14;
        } else {
            channel = (freq - 2407) / 5;
        }
    }
    // 5 GHz
    else if (band == 1) {
        assert(0);

        // Check if frequency is in the expected range
        if ((freq < 5005) || (freq > 5825)) {
            PRINTF("Wrong channel\n");
            return (channel);
        }

        // Compute the channel number
        channel = (freq - 5000) / 5;
    }

    return (channel);
}

int get_current_freq_ch(void)
{
    struct phy_chn_info phy_info;
    unsigned char band;
    unsigned short freq;
    int chan;
#define    PHY_PRIM    0

    phy_get_channel(&phy_info, PHY_PRIM);
    band = phy_info.info1 & 0xFF;
    freq = phy_info.info2 & 0xffff;

    chan = freq_to_channel(band, freq);

    PRINTF("BAND:            %d\n"
           "center_freq:    %d MHz\n"
           "chan:    %d \n",    band, freq, chan);
    return chan;
}
const COMMAND_TREE_TYPE    cmd_user_list[] = {
  { "user",        CMD_MY_NODE,    cmd_user_list, NULL,             "User cmd "                        },    // Head

  { "-------",     CMD_FUNC_NODE,  NULL,          NULL,             "--------------------------------" },
  { "testcmd",     CMD_FUNC_NODE,  NULL,          &cmd_test,        "testcmd [option]"                 },
#if defined(__COAP_CLIENT_SAMPLE__)
  { "coap_client", CMD_FUNC_NODE,  NULL,          &cmd_coap_client, "CoAP Client"                      },
#endif /* __COAP_CLIENT_SAMPLE__ */
  { "utxpwr",      CMD_FUNC_NODE,  NULL,          &cmd_txpwr,       "testcmd [option]"                 },

  { NULL, CMD_NULL_NODE,  NULL, NULL, NULL         }    // Tail
};

//
//-----------------------------------------------------------------------
// Internal Functions
//-----------------------------------------------------------------------

void cmd_test(int argc, char *argv[])
{    
    if (argc < 2) {
        PRINTF("Usage: testcmd [option]\n   ex) testcmd test\n\n");
        return;
    }

    PRINTF("\n### TEST CMD : %s ###\n\n", argv[1]);
}

void cmd_txpwr(int argc, char *argv[])
{    
    int i;
    int channel, pwr_mode, power, power_dsss;

    if (argc != 30) {
        goto error;
    }

    pwr_mode = (int)ctoi(argv[1]);
    PRINTF("TX PWR : [%s] ", (pwr_mode)?"AP":"STA");
    PRINTF("\n OFDM \n");
    for (i = 0; i < 14; i++) {
        user_power_level[i] = (UCHAR)htoi(argv[i + 2]);

        if (user_power_level[i] > 0x0f) {
            PRINTF("..................\n");
            goto error;
        } else {
            PRINTF("CH%02d[0x%x] ", i + 1, user_power_level[i]);
        }

        if (i == 6)
            PRINTF("\n");
    }

    PRINTF("\n DSSS \n");
    for (i = 14; i < 28; i++) {
        user_power_level_dsss[i - 14] = (UCHAR)htoi(argv[i + 2]);

        if (user_power_level_dsss[i - 14] > 0x0f) {
            PRINTF("..................\n");
            goto error;
        } else {
            PRINTF("CH%02d[0x%x] ", i + 1, user_power_level_dsss[i - 14]);
        }

        if (i == 20)
            PRINTF("\n");
    }

    PRINTF("\n");
    channel = get_current_freq_ch();
    power = user_power_level[channel-1];
    power_dsss = user_power_level_dsss[channel-1];

    fc80211_set_tx_power_table(pwr_mode, (unsigned int *)&user_power_level, (unsigned int *)&user_power_level_dsss);
    fc80211_set_tx_power_grade_idx(pwr_mode, power, power_dsss);

    PRINTF("TX PWR set channel[%d], pwr [0x%x]\n", channel, power);

    return;


error:
    PRINTF("\n\tUsage : utxpwr mode [CH1 CH2 ... CH13 CH14] [CH1 CH2 ... CH13 CH14 [DSSS]]\n");
    PRINTF("\t\tCH    0~f\n");
    PRINTF("\t\tex) utxpwr 0 1 1 1 1 1 1 1 1 1 1 1 f f f 2 3 3 3 3 3 3 3 3 3 1 f f f\n");

    return;
}

#if defined(__COAP_CLIENT_SAMPLE__)
void cmd_coap_client(int argc, char *argv[])
{
    extern void coap_client_sample_cmd(int argc, char *argv[]);

    coap_client_sample_cmd(argc, argv);

    return;
}
#endif /* __COAP_CLIENT_SAMPLE__ */

/* EOF */
