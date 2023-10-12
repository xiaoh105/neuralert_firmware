Configuration with Non-Volatile Random-Access Memory (NVRAM) {#nvram}
============================================================

## Overview

Abbreviation of Non-Volatile Random Access Memory, a type of memory that retains its contents when power is turned off. One type of NVRAM is SRAM that is made non-volatile by connecting it to a constant power source such as a battery. Another type of NVRAM uses EEPROMchips to save its contents when power is turned off. In this case, NVRAM is composed of a combination of SRAM and EEPROM chips. \n
DA16200 has an NVRAM area on the flash memory to store system data and user data. NVRAM has various system configuration parameters to control the Wi-Fi function. \n\n

## DA16200 Configuration

User can use APIs to set/get NVRAM parameters. \n
- Set string value: int da16x_set_config_str(int name, char *value) \n
- Set number value: int da16x_set_config_int(int name, int value) \n
- Get string value: int da16x_get_config_str(int name, char *value) \n
- Get number value: int da16x_get_config_int(int name, int *value) \n
(Return: If succeeded return 0, if failed return an error code)\n
\n
[name] list configurable are written in `common_config.h`. (@ref DA16X_CONF_STR / @ref DA16X_CONF_INT)

## NVRAM parameter addition for User

Users can make APIs to set/get parameter to/from NVRAM. \n
1. Add a parameter name to @ref DA16X_USER_CONF_STR or @ref DA16X_USER_CONF_INT (enum) in the `user_nvram_cmd_table.h` \n
2. Add a struct member to user_config_str_with_nvram_name[] or user_config_int_with_nvram_name[] in the `user_nvram_cmd_table.c` \n
String value: { [parameter name], [NVRAM name], [Max length of the value] } \n
Number value: { [parameter name], [NVRAM name], [Min value], [Max value], [Default value] } \n
3. User can set/get DA16200 parameters with the configuration APIs.\n

#### Example

1. Add a parameter name, DA16X_CONF_STR_DEST_IP, to @ref DA16X_USER_CONF_STR in the `user_nvram_cmd_table.h` \n
~~~{.c}
typedef enum {
    DA16X_CONF_STR_USER_START = DA16X_CONF_STR_MAX,
    ...
    DA16X_CONF_STR_DEST_IP,

    DA16X_CONF_STR_FINAL_MAX
} DA16X_USER_CONF_STR;
~~~
2. Add a struct member, DA16X_CONF_STR_DEST_IP, to @ref user_config_str_with_nvram_name in the `user_nvram_cmd_table.c` \n
~~~{.c}
const user_conf_str user_config_str_with_nvram_name[] =
{
    ...
    {DA16X_CONF_STR_DEST_IP, "USER_DEST_IP", 16},

    { 0, "", 0 }
};
~~~
3. Set and Get the parameter by calling API.
~~~{.c}
{
    da16x_set_config_str(DA16X_CONF_STR_DEST_IP, "192.168.0.1");

    da16x_get_config_str(DA16X_CONF_STR_DEST_IP, name);
}
~~~
