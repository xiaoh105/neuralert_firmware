Dynamic Power Management (DPM) {#dpm}
============================================================

## Overview

VirtualZero is a synthesis of breakthrough ultra-low power technologies which enables extremely low power operation in the DA16200 SoC. VirtualZero shuts down every micro element of the chip that is not in use, which allows a near zero level of power consumption when DA16200 SoC does not actively transmit or receive data. Such low power operation can deliver a year or more of battery life depending on the application. VirtualZero also enables ultra-low power operation to transmit and receive data when the SoC needs to be awake to exchange information with other devices. Advanced algorithms enable to stay asleep until the exact moment required to wake up to transmit or receive. DPM (Dynamic Power Management) Manager APIs make it easy to develop a DPM application. \n\n
DPM Manager is developed for users to easily develop DPM applications. It has a simple interface; all that users need to do is to write the necessary callback functions and register them to DPM Manager, which then takes care of all the DPM relevant jobs internally. \n\n
