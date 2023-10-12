Command Line Interface (CLI) {#cli}
============================================================

## Overview

The simplest way to control the DA16200 is the CLI. DA16200 supports CLI with UART0. User can control Wi-Fi function, network configuration, or start/stop an application and so on. In addition, user can add CLI command.

## How to use

Users can add command to control DA16200 \n
1. Add a struct member to cmd_user_list[] in the `user_command.c` \n
{ [command], CMD_FUNC_NODE(fixed), NULL(fixed), &[function name], [description] } \n
~~~{.c}
{ "set_config", CMD_FUNC_NODE, NULL, &cmd_set_configuration, "Set DA16200 Configuration" }
~~~
2. Make a new function to execute the command ([function name]) \n
~~~{.c}
void cmd_set_configuration(int argc, char *argv[])
{
    ...
}
~~~
3. Execute the command on the UART0 window. \n
ex) [/DA16200] # user.set_config