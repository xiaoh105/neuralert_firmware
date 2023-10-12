PTIM SDK {#ptim_sdk}
============================================================

## Overview

PTIM is a light-weight application whose main mission is to maintain the connection to an AP during power off state of DA16200. It wakes up periodically and check the beacon from the AP and process some simple tasks which will be described in later sections. RTC and RTM should be used for PTIM operations so that PTIM runs in sleep mode 3 of DA16200.

PTIM has the following features:

- PTIM keeps the connection with AP during sleep in sleep mode 3
- PTIM keeps the server connection of MAC/UDP/TCP protocols
- PTIM’s boot time is faster than a full boot because the image size is small
  and the image is loaded from RTM, not Sflash
- PTIM is designed to consume the power as less as possible

## PTIM Status

When the user runs PTIM by ptim_execute() API, it returns its result after running. PTIM goes to sleep or full-boot depending on this return value. PTIM goes to full-boot after getting the return value shown in Table 1 except for FAST_FROM for which PTIM goes to sleep. If necessary, the user can change the PTIM status.

Table 1: PTIM number, status, and description overview

| Number      | Status     | Description                                                     |
|-------------|------------|-----------------------------------------------------------------|
| UC          | 0x00000001 | There is UC data to be processed by Full-Boot.                  |
| BCMC        | 0x00000002 | There is broadcast/multicast data to be processed by Full-Boot. |
| BCN CHANGED | 0x00000004 | The beacon has changed from previous beacon.                    |
| NO BCN      | 0x00000008 | PTIM couldn’t receive the beacon of the connected AP.           |
| FROM FAST   | 0x00000010 | The last state was PTIM.                                        |
| NO ACK      | 0x00000020 | PTIM transmits data, but No ACK for it from AP.                 |
| DEAUTH      | 0x00000800 | The station was disassociated from AP.                          |
| FROM FULL   | 0x00002000 | The last state was Full-Boot.                                   |

## PTIM Functions

Supported functions by PTIM in order to keep the connection with an AP and process data from the AP are described in the following sections. Take note that most APs are verified for its functional compatibility with the PTIM Dialog designed. However, there may be some errant APs that may cause some functional problems with PTIM.

### TIM

There is TIM field in the beacon frame indicting UC or BC/MC data buffered in AP. PTIM application receives a beacon frame and parses it to determine if there were UC or BC/MC data to receive. When there is UC data, PTIM sends a PS_POLL to receive it from AP.

### BCMC

BCMC of PTIM is the function that receives BC/MC data buffered by the AP. The maximum number of BC/MC packets can be set by bcmc_max configuration. The maximum waiting time for receiving it can be set by bcmc_timeout_tu configuration.

### PS-POLL

When an UC packet is buffered in AP (indicated by TIM field in a beacon frame already described), a PS_POLL packet is transmitted by PTIM to receive it.

### AUTO ARP (Autonomous ARP)

Auto ARP (Autonomous ARP) of PTIM is a function that transmits autonomously ARP-REPLY to AP. Some APs transmit a broadcast packet called ARP request to the connected stations to check if they are still there; BC/MC data are transmitted at a fixed time called DTIM from AP. Normal stations in power saving state wakes up periodically at DTIM time to receive BC/MC data from AP. However, in DPM, DA16200 may not receive the BC/MC data at DTIM time because it could remain in the sleep state at that time. To emulate that DA16200 has received it, PTIM sends a ARP reply packet to AP to maintain the connection.

### ARP-REQ

ARP-REQ of PTIM is a function that transmits autonomously ARP-REQUEST to AP. For some reason, AP transmits a deauthentication or disassociation frame to a connected station when AP wants to disconnect the stations. However, some APs does not send any packet even when they want to disconnect the stations. To confirm the connection, PTIM sends a frame called ARP request to the AP. When the AP is still there, it will respond to this frame with an ARP reply. In this way, PTIM can confirm if it is still connected to the AP.

### ARP-REPLY

ARP-REPLY of PTIM is a function that transmits ARP-REPLY frame to a peer that has sent an ARP-REQUEST frame. PTIM does not need to wake up to full-boot to transmit the ARP-REPLY frame.

### UDPH (UDP hole-punch)

UDPH of PTIM is a function that transmits autonomously a UDP packet called UDP hole punch (only UDP header without payload) or a user defined UDP packet to keep the UDP port in AP or router. If the UDP protocol is used instead of UDP hole-punch, some APs or routers may release the UDP port causing DA16200 to disconnect from the UDP server.

### TCP ACK

TCP ACK of PTIM is a function that transmits autonomously TCP ACK frame to keep the TCP port. If the TCP protocol is used instead of TCP ACK, some APs or routers may release the TCP port causing DA16200 to disconnect from the TCP server.

### DeauthChk (Deauthentication Check)

DeauthChk function is for waiting for the deauthentication or disassociation frame from AP. When an AP wants to disconnect the stations, it normally transmits a deauthentication or disassociation frame to the stations.

Deauthentication check is done in DA16200 as follows:

- Sends a KA packet to the AP
- Check if a deauthentication or disassociation packet is transmitted from the AP

DeauthChk waits for this deathentication or disassociation frame for up to the configured time duration called chk_timeout_tu. See ptim_set_deauthchk API section in this document for more details.

## APIs

- \ref ptim.h "PTIM Control APIs"
- \ref ptim_config.h "PTIM Configuration APIs"
- \ref ptim_helper.h "PTIM Helper APIs"
- \ref ptim_samples "PTIM Samples"
