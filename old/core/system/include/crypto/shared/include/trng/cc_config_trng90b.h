/****************************************************************************
* The confidential and proprietary information contained in this file may    *
* only be used by a person authorised under and to the extent permitted      *
* by a subsisting licensing agreement from ARM Limited or its affiliates.    *
*   (C) COPYRIGHT [2001-2018] ARM Limited or its affiliates.         *
*       ALL RIGHTS RESERVED                          *
* This entire notice must be reproduced on all copies of this file           *
* and copies of this file may only be made by a person if such person is     *
* permitted to do so under the terms of a subsisting license agreement       *
* from ARM Limited or its affiliates.                        *
*****************************************************************************/

#ifndef _CC_CONFIG_TRNG90B_H
#define _CC_CONFIG_TRNG90B_H

/*
This file should be updated according to the characterization process.
*/

/*** For Startup Tests ***/
// amount of bytes for the startup test = 528 (at least 4096 bits (NIST SP 800-90B (2nd Draft) 4.3.12) = 22 EHRs = 4224 bits)
#define CC_CONFIG_TRNG90B_AMOUNT_OF_BYTES_STARTUP              528



#endif  // _CC_CONFIG_TRNG90B_H
