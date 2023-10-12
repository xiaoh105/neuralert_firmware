AT-Commands {#atcmd}
============================================================

## Overview

Configuration and control of the DA16200 is provided through an ASCII based command string called "AT Command". "AT Command" is a standard that was originally defined by Hayes Microcomputer for controlling smart modems and is widely used in many products. 
AT is an abbreviation of "Attention", which means to take note of or fix one's sight upon something. An example of an AT Command is "ATZ" which instructs a modem to become initialized and return to a state with no command input.
An AT Command has a very simple structure consisting of a prefix "AT" concatenated with a command string. This is a very convenient method for sending a series of commands over a serial interface such as a UART. Commands may consist of capital letters, lowercase letters, spaces and some special characters.

## Features

- Support AT-CMD with UART1, SDIO, and SPI
- Most of the functions supported by DA16200 are supported by AT-CMD.
- Send the TCP(including MQTT), UDP messages
- Support the event notification function from the system (e.g. Wi-Fi Connection, Disconnection)

## Command Types

- Basic Function
- Network Function
- Wi-Fi Function
- Advanced Function
    - MQTT
    - HTTP Client
    - HTTP Server
    - OTA
    - Zeroconf
    - Provision
- Transfer Function
    - TCP Server
    - TCP Client
    - UDP Session
    - TLS Client
- RF Test Function
- System and Peripheral Function

## APIs

- Start AT-Commands system \n
> @ref start_atcmd \n
