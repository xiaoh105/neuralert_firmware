/**
 ****************************************************************************************
 *
 * @file user_atcmd.c
 *
 * @brief Entry point for User written AT-CMD
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

#include "user_atcmd.h"

#if defined ( __SUPPORT_ATCMD__ )

#undef    __TEST_USER_AT_CMD__

extern int atq_result;

/* Internal functions */
#if defined (  __TEST_USER_AT_CMD__ )
int user_atcmd_test(int argc, char* argv[]);
int user_atcmd_test_1(int argc, char* argv[]);
#endif  // __TEST_USER_AT_CMD__

/* AT COMMANDS */
static const command_t user_at_cmd[] =
{
  /* cmd,    function,    arg_count,    delimit,    format,    brief */

#ifdef    __TEST_USER_AT_CMD__
  { (char *)"USER_TEST",   user_atcmd_test,   0, (char *)NULL,                      "User test at-cmd." },
  { (char *)"USER_TEST_1", user_atcmd_test_1, 2, (char *)"<command>=<arg1>,<arg2>", "User test at-cmd." },
#endif    /* __TEST_USER_AT_CMD__ */

  { NULL, NULL, 0, NULL, NULL }
};

static const command_t *user_at_cmd_table = user_at_cmd;


atcmd_error_code user_atcmd_help(char *cmd)
{
    const command_t* cmd_ptr;
    atcmd_error_code result_code = AT_CMD_ERR_CMD_OK;

    if (cmd == NULL) {
        PRINTF_ATCMD("\r\n\r\n=== User AT-CMD ======\r\n\r\n");

        for (cmd_ptr = user_at_cmd_table; cmd_ptr->name != NULL; cmd_ptr++) {
            if (cmd_ptr->format != NULL) {
                PRINTF_ATCMD("    %s%s%s\r\n",
                            cmd_ptr->name,
                            (cmd_ptr->arg_count > 0 ? DELIMIT_FIRST : DELIMIT),
                            cmd_ptr->format);
            } else {
                PRINTF_ATCMD("    %s\r\n", cmd_ptr->name);
            }

            if (cmd_ptr->brief != NULL) {
                PRINTF_ATCMD("        - %s\r\n", cmd_ptr->brief);
            } else {
                PRINTF_ATCMD("        - No example for %s\r\n", cmd_ptr->name);
            }
        }
    } else {
        result_code = AT_CMD_ERR_UNKNOWN_CMD;
        for (cmd_ptr = user_at_cmd_table; cmd_ptr->name != NULL; cmd_ptr++) {
            if (strcasecmp(cmd, cmd_ptr->name) == 0) {
                result_code = AT_CMD_ERR_CMD_OK;

                if (cmd_ptr->format == NULL && cmd_ptr->brief == NULL) {
                    PRINTF_ATCMD("\r\n        - No example for %s\r\n", cmd);
                } else {
                    PRINTF_ATCMD("\r\n    %s%s%s\r\n",
                                cmd_ptr->name,
                                (cmd_ptr->arg_count > 0 ? DELIMIT_FIRST : DELIMIT),
                                (cmd_ptr->format != NULL) ? cmd_ptr->format : "");

                    if (cmd_ptr->brief != NULL) {
                        PRINTF_ATCMD("          - %s\r\n", cmd_ptr->brief);
                    }
                }
            }
        }
    }

    return result_code;
}

/* Register User AT-CMD call-back function */
void reg_user_atcmd_help_cb(void)
{
#if defined ( __SUPPORT_USER_CMD_HELP__ )
    user_atcmd_help_cb_fn = user_atcmd_help;
#endif    // __SUPPORT_USER_CMD_HELP__
}


atcmd_error_code user_atcmd_parser(char *line)
{
    const command_t *cmd_ptr = NULL;
    atcmd_error_code result_code = AT_CMD_ERR_UNKNOWN_CMD;
    char    *params[MAX_PARAMS];
    int    param_cnt = 0;
    int    cmd_len = strlen(line);
    char    *json_param = NULL;
    char    *tmp_line = NULL;

    if (strstr(line, ATCMD_JSON_IDENTITY)) {
        char *tmp = NULL;

        json_param = pvPortMalloc(ATCMD_JSON_MAX_LENGTH);
        if (json_param == NULL) {
            PRINTF("Failed to allocate memory(L%d)\n", __LINE__);
            return AT_CMD_ERR_UNKNOWN;
        }

        tmp = strstr(line, ATCMD_JSON_IDENTITY) + 1;
        strcpy(json_param, tmp);
    }

    /* To check if first character is ',' or not */
    tmp_line = pvPortMalloc(cmd_len + 1);
    if (tmp_line == NULL) {
        PRINTF("Failed to allocate memory(L%d)\n", __LINE__);
        return AT_CMD_ERR_UNKNOWN;
    }
    memset(tmp_line, 0, cmd_len + 1);
    strncpy(tmp_line, line, cmd_len);

    if (strchr(line, '=') == NULL) {
        params[param_cnt++] = strtok(line, DELIMIT);
    } else {
        params[param_cnt++] = strtok(line, DELIMIT_FIRST);
    }

    for (cmd_ptr = user_at_cmd_table; cmd_ptr->name != NULL; cmd_ptr++) {
        if (strcasecmp(params[0], cmd_ptr->name) == 0) {
            if (cmd_ptr->name != NULL) {
                break;
            }
        }
    }

    if (cmd_ptr->name == NULL) {
        result_code = AT_CMD_ERR_UNKNOWN_CMD;
        goto user_atcmd_result;
    } else {
        /* To check if first character is ',' or not */
        /* Command length + '=' */
        if (tmp_line[strlen(params[0]) + 1] == ',') {
            result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
            goto user_atcmd_result;
        }

        while (((params[param_cnt] = strtok(NULL, DELIMIT_COMMA)) != NULL)) {
            if (params[param_cnt][0] == '\'') {
                char *tmp_ptr;

                // Restore delimit-character
                params[param_cnt][strlen(params[param_cnt])] = ',';

                if (strncmp(params[param_cnt], "',", 2) == 0) {
                    // First argument : AT+XXX=',aaaaa','bbbb'
                    if (param_cnt == 1) {
                        if ((tmp_ptr = strstr(&params[param_cnt][1], "',")) != NULL) {
                            params[param_cnt] = params[param_cnt] + 1;
                            strtok(tmp_ptr, DELIMIT_COMMA);
                            *tmp_ptr = '\0';
                            *(tmp_ptr + 1) = '\0';
                        }
                    } else {
                        if (params[param_cnt][strlen(params[param_cnt]) - 1] == '\'') {
                            tmp_ptr = params[param_cnt] + strlen(params[param_cnt]) - 1;
                            params[param_cnt] = params[param_cnt] + 1;
                            strtok(tmp_ptr, "'");
                            *tmp_ptr = '\0';
                        } else {
                            result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                            goto user_atcmd_result;
                        }
                    }
                } else if ((tmp_ptr = strstr(params[param_cnt], "',")) != NULL) {
                    params[param_cnt] = params[param_cnt] + 1;
                    strtok(tmp_ptr, DELIMIT_COMMA);
                    *tmp_ptr = '\0';
                    *(tmp_ptr + 1) = '\0';
                } else if (params[param_cnt][strlen(params[param_cnt]) - 1] == '\'') {
                    tmp_ptr = params[param_cnt] + strlen(params[param_cnt]) - 1;
                    params[param_cnt] = params[param_cnt] + 1;
                    strtok(tmp_ptr, "'");
                    *tmp_ptr = '\0';
                } else {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto user_atcmd_result;
                }
            } else if (json_param && params[param_cnt][0] == '{') {
                strcpy(params[param_cnt], json_param);
                break;
            } else {
                if (strstr(params[param_cnt], "'") != NULL) {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto user_atcmd_result;
                }
            }

            param_cnt++;

            if (param_cnt > (MAX_PARAMS - 1)) {
                result_code = AT_CMD_ERR_TOO_MANY_ARGS;
                goto user_atcmd_result;
            }
        }

        if (param_cnt > 1 && cmd_len - strlen(params[0]) - 1 > TX_PAYLOAD_MAX_SIZE) {
            result_code = AT_CMD_ERR_CMD_OK;
            goto user_atcmd_result;
        }
    }

    if (param_cnt - 1 > cmd_ptr->arg_count) {
        result_code = AT_CMD_ERR_TOO_MANY_ARGS;
        goto user_atcmd_result;
    }

    /* run command */
    result_code = (atcmd_error_code)cmd_ptr->command(param_cnt, params);

    /*
     *    ERR_CMD_OK              =  0,
     *    ERR_UNKNOWN_CMD         = -1,
     *    ERR_INSUFFICENT_ARGS    = -2,
     *    ERR_TOO_MANY_ARGS       = -3,
     *    ERR_WRONG_ARGUMENTS     = -4,
     *    ERR_NOT_SUPPORTED       = -5,
     *    ERR_NOT_CONNECTED       = -6,
     *    ERR_NO_RESULT           = -7,
     *    ERR_TOO_LONG_RESULT     = -8,
     *    ERR_INSUFFICENT_CONFIG  = -9,
     *    ERR_TIMEOUT             = -10,
     *    ERR_NVR_WRITE           = -11,
     *    ERR_RTM_WRITE           = -12,
     *    ERR_UNKNOWN             = -99
     */

user_atcmd_result :
    if (atq_result == 1) {
        if (result_code == AT_CMD_ERR_CMD_OK) {
            PRINTF_ATCMD("\r\nOK\r\n");
        } else {
            PRINTF_ATCMD("\r\nERROR:%d\r\n", result_code);
        }
    }

    /* To check if first character is ',' or not */
    if (tmp_line != NULL) {
        vPortFree(tmp_line);
    }

    if (json_param != NULL) {
        vPortFree(json_param);
    }

    return result_code;
}

/*****************************************************************************
 * User AT-CMD functions
 *****************************************************************************/

#if defined ( __TEST_USER_AT_CMD__ )
int user_atcmd_test(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    atcmd_error_code result = AT_CMD_ERR_CMD_OK;

    PRINTF("\n\n----- User AT-CMD Test Command arrived ...\n");

    return result;
}

int user_atcmd_test_1(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);

    if (argc < 3) {
        PRINTF("[%s] Wrong arguments ...\n", __func__);
        return AT_CMD_ERR_INSUFFICENT_ARGS;
    }

    PRINTF("\n\n----- User AT-CMD Test Command arrived ...\n");
    PRINTF("%s=%s,%s\n", argv[0], argv[1], argv[2]);

    return AT_CMD_ERR_CMD_OK;
}

#endif /* __TEST_USER_AT_CMD__ */

#else

#include "da16x_types.h"

void setAtCmdErrorCode(int errorCodeInt, char *errorCodeString)
{
    DA16X_UNUSED_ARG(errorCodeInt);
    DA16X_UNUSED_ARG(errorCodeString);
    return;
}

void atCmdRcvCmd(char *atCmdCmd,
                char *atCmdRpcId,
                char *atCmdCmdStatus,
                char *atCmdCmdParam)
{
    DA16X_UNUSED_ARG(atCmdCmd);
    DA16X_UNUSED_ARG(atCmdRpcId);
    DA16X_UNUSED_ARG(atCmdCmdStatus);
    DA16X_UNUSED_ARG(atCmdCmdParam);
    return;
}

#endif /* __SUPPORT_ATCMD__ */

/* EOF */
