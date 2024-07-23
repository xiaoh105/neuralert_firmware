/**
 ****************************************************************************************
 *
 * @file le_byteshift.h
 *
 * @brief Header file
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

#ifndef _DIW_UNALIGNED_LE_BYTESHIFT_H
#define _DIW_UNALIGNED_LE_BYTESHIFT_H

static inline u16 __diw_get_unaligned_le16(const u8 *b)
{
	return b[0] | b[1] << 8;
}

static inline u32 __diw_get_unaligned_le32(const u8 *b)
{
	return b[0] | b[1] << 8 | b[2] << 16 | b[3] << 24;
}

static inline void __diw_put_unaligned_le16(u16 val, u8 *b)
{
	*b++ = val;
	*b++ = val >> 8;
}

static inline void __diw_put_unaligned_le32(u32 val, u8 *b)
{
	__diw_put_unaligned_le16(val >> 16, b + 2);
	__diw_put_unaligned_le16(val, b);
}

static inline u16 diw_get_unaligned_le16(const void *b)
{
	return __diw_get_unaligned_le16((const u8 *)b);
}

static inline u32 diw_get_unaligned_le32(const void *b)
{
	return __diw_get_unaligned_le32((const u8 *)b);
}

static inline void diw_put_unaligned_le32(u32 val, void *b)
{
	__diw_put_unaligned_le32(val, b);
}

#ifdef FEATURE_UMAC_MESH
static inline void put_unaligned_le16(u16 val, void *p)
{
	__diw_put_unaligned_le16(val, p);
}
#endif /* FEATURE_UMAC_MESH */
#endif /* _DIW_UNALIGNED_LE_BYTESHIFT_H */
