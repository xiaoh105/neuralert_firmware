/**
 ****************************************************************************************
 *
 * @file coap_utils.h
 *
 * @brief CoAP common utility
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
#ifndef	__COAP_UTILS_H__
#define	__COAP_UTILS_H__

#include "sdk_type.h"
#include "coap_common.h"
#include "common_def.h"

extern void *(*coap_calloc)(size_t n, size_t size);
extern void (*coap_free)(void *ptr);

/**
 ****************************************************************************************
 * @brief Convert uppercase letter to lowercase
 * @param[in] val            Character to be converted, casted to an int.
 *
 * @return The lowercase equivalent to val, if such value exists, or val (unchanged) otherwise.
 ****************************************************************************************
 */
int coap_client_tolower(int val);

/**
 ****************************************************************************************
 * @brief Set memory-management functions.
 * @param[in] calloc_func    The calloc function implementation
 * @param[in] free_func      The free function implementation
 ****************************************************************************************
 */
void coap_set_calloc_free(void *(*calloc_func)(size_t, size_t), void (*free_func)(void *));

/**
 ****************************************************************************************
 * @brief Print Hexdump logs.
 * @param[in] title          Title
 * @param[in] buffer         Pointer of data
 * @param[in] len            Length of data
 ****************************************************************************************
 */
void coap_print_hexdump(const unsigned char *title, const unsigned char *buffer, size_t len);

/**
 ****************************************************************************************
 * @brief Set data to bit arrary.
 * @param[in] arr            Bit array
 * @param[in] pos            Position of array
 * @param[in] len            Length of array
 * @param[in] val            Data
 ****************************************************************************************
 */
void coap_set_in_bit_array(uint8_t *arr, int pos, int len, int val);

/**
 ****************************************************************************************
 * @brief Get data from bit array.
 * @param[in] arr            Bit array
 * @param[in] pos            Position of array
 * @param[in] len            Length of array
 * @return Returns data.
 ****************************************************************************************
 */
uint8_t coap_get_from_bit_array(uint8_t *arr, int pos, int len);

/**
 ****************************************************************************************
 * @brief Get byte from bit array.
 * @param[in] arr            Bit array
 * @return Returns byte.
 ****************************************************************************************
 */
int coap_get_byte_from_bits(uint8_t *arr);

/**
 ****************************************************************************************
 * @brief Get bits from byte array.
 * @param[out] arr           Bit array
 * @param[in] value          Data
 ****************************************************************************************
 */
void coap_get_bits_from_byte(uint8_t *arr, int value);

/**
 ****************************************************************************************
 * @brief Print CoAP header.
 * @param[in] header         Pointer of CoAP header
 ****************************************************************************************
 */
void coap_print_header(coap_header_t *header);

/**
 ****************************************************************************************
 * @brief Parse CoAP header.
 * @param[out] header        Pointer of CoAP header
 * @param[in] buf            Pointer of data
 * @param[in] buflen         Length of data
 * @return 0 on success.
 ****************************************************************************************
 */
int coap_parse_header(coap_header_t *header, const uint8_t *buf, size_t buflen);

/**
 ****************************************************************************************
 * @brief Parse CoAP token
 * @param[out] token         Parsed token
 * @param[in] header         Pointer of CoAP header to indicate token
 * @param[in] buf            Pointer of data
 * @param[in] buflen         Length of data
 * @return 0 on success
 ****************************************************************************************
 */
int coap_parse_token(coap_buffer_t *token,
					 const coap_header_t *header,
					 const uint8_t *buf,
					 size_t buflen);

/**
 ****************************************************************************************
 * @brief Parse CoAP option.
 * @param[out] opt           Parsed option
 * @param[out] run_delta     Pointer of running delta value
 * @param[in] buf            Pointer of data
 * @param[in] buflen         Length of data
 * @return 0 on success.
 ****************************************************************************************
 */
int coap_parse_option(coap_option_t *opt,
					 uint16_t *run_delta,
					 const uint8_t **buf,
					 size_t buflen);

/**
 ****************************************************************************************
 * @brief Parse CoAP option and payload.
 * @param[out] opt           Parsed options
 * @param[out] num_opt       Number of parsed options
 * @param[out] payload       Parsed payload
 * @param[in] hdr            Pointer of CoAP header to indicate token
 * @param[in] buf            Pointer of data
 * @param[in] buflen         Length of data
 * @return 0 on success.
 ****************************************************************************************
 */
int coap_parse_option_and_payload(coap_option_t *opt,
								  uint8_t *num_opt,
								  coap_buffer_t *payload,
								  const coap_header_t *hdr,
								  const uint8_t *buf,
								  size_t buflen);

/**
 ****************************************************************************************
 * @brief Print CoAP option.
 * @param[in] opts           Pointer of CoAP option
 * @param[in] numopt         Number of CoAP option
 ****************************************************************************************
 */
void coap_print_options(coap_option_t *opts, size_t num_opt);

/**
 ****************************************************************************************
 * @brief Print CoAP packet.
 * @param[in] packet         Pointer of CoAP packet
 ****************************************************************************************
 */
void coap_print_packet(coap_packet_t *packet);

/**
 ****************************************************************************************
 * @brief Print script of CoAP option number.
 * @param[in] num            CoAP option number
 ****************************************************************************************
 */
void coap_print_option_num(int num);

/**
 ****************************************************************************************
 * @brief Print script of CoAP content format.
 * @param[in] num            CoAP content format
 ****************************************************************************************
 */
void coap_print_content_format(int num);

/**
 ****************************************************************************************
 * @brief Print CoAP response.
 * @param[in] packet         Pointer of CoAP response
 * @param[in] show_opt       Option of detilas
 ****************************************************************************************
 */
void coap_print_response(coap_packet_t *packet, int show_opt);

/**
 ****************************************************************************************
 * @brief Get CoAP block option information.
 * @param[in] opt            CoAP block option
 * @param[out] num           Block number
 * @param[out] next          Block more flag
 * @param[out] szx           Block szx
 ****************************************************************************************
 */
void coap_get_block_info(coap_option_t *opt,
						 uint32_t *num, uint8_t *next, uint8_t *szx);

/**
 ****************************************************************************************
 * @brief Parse CoAP packet.
 * @param[out] packet        Parsed CoAP packet
 * @param[in] buf            Pointer of raw data
 * @param[in] buflen         Length of raw data
 * @return 0 on success.
 ****************************************************************************************
 */
int coap_parse_raw_data(coap_packet_t *packet, const uint8_t *buf, size_t buflen);

/**
 ****************************************************************************************
 * @brief Search CoAP option.
 * @param[in] packet         Pointer of CoAP packet
 * @param[in] num            CoAP option number to find
 * @param[out] count         Position of CoAP option
 * @return Returns pointer of CoAP option in the packet.
 ****************************************************************************************
 */
const coap_option_t *coap_search_options(const coap_packet_t *packet,
										 uint8_t num,
										 uint8_t *count);

/**
 ****************************************************************************************
 * @brief Generate CoAP packet.
 * @param[out] buf           Pointer of buffer to build CoAP packet
 * @param[out] buflen        Length of buffer length
 * @param[in] packet         CoAP packet to build
 * @return 0 on success.
 ****************************************************************************************
 */
int coap_generate_packet(uint8_t *buf, size_t *buflen, const coap_packet_t *packet);

/**
 ****************************************************************************************
 * @brief Get CoAP option nibble.
 * @param[in]  value         CoAP option
 * @param[out] nibble        Nibble
 * @return 0 on success.
 ****************************************************************************************
 */
int coap_get_option_nibble(uint32_t value, uint8_t *nibble);

/**
 ****************************************************************************************
 * @brief Generate CoAP response.
 * @param[out] scratch       Pointer of scratch
 * @param[out] packet        Pointer of CoAP packet
 * @param[in] content        Pointer of content
 * @param[in] content_len    Length of content
 * @param[in] msgid_hi       Message-ID
 * @param[in] msgid_lo       Message-ID
 * @param[in] token          Pointer of token
 * @param[in] rspcode        CoAP response code
 * @param[in] content_type   CoAP content type
 * @param[in] req_hdr        Pointer of CoAP request header
 * @return 0 on success.
 ****************************************************************************************
 */
int coap_generate_response(coap_rw_buffer_t *scratch,
						   coap_packet_t *packet,
						   const uint8_t *content,
						   size_t content_len,
						   uint8_t msgid_hi,
						   uint8_t msgid_lo,
						   const coap_buffer_t *token,
						   coap_resp_code_t rsp_code,
						   coap_content_type_t content_type,
						   coap_header_t req_hdr);

/**
 ****************************************************************************************
 * @brief Generate sperate CoAP response.
 * @param[out] scratch       Pointer of scratch
 * @param[out] packet        Pointer of CoAP packet
 * @param[in] content        Pointer of content
 * @param[in] content_len    Length of content
 * @param[in] msgid_hi       Message-ID
 * @param[in] msgid_lo       Message-ID
 * @param[in] token          Pointer of token
 * @param[in] rsp_code       CoAP response code
 * @param[in] content_type   CoAP content type
 * @param[in] req_hdr        Pointer of CoAP request header
 * @return 0 on success.
 ****************************************************************************************
 */
int coap_generate_seperate_response(coap_rw_buffer_t *scratch,
									coap_packet_t *packet,
									const uint8_t *content,
									size_t content_len,
									uint8_t msgid_hi,
									uint8_t msgid_lo,
									const coap_buffer_t *token,
									coap_resp_code_t rsp_code,
									coap_content_type_t content_type,
									coap_header_t req_hdr);

/**
 ****************************************************************************************
 * @brief Set bytes for splitted CoAP block option.
 * @param[in] packet         Pointer of CoAP packet
 * @param[in] block_num      Number of block option
 * @param[in] block_szx      Szx of block option
 * @param[in] block_more     More flag of block option
 * @return 0 on success.
 ****************************************************************************************
 */
int coap_set_bytes_splitted_by_block(coap_packet_t *packet,
									 int block_num,
									 int block_szx,
									 uint8_t *block_more);

/**
 ****************************************************************************************
 * @brief Set payload for splitted CoAP block option.
 * @param[in] blockno        Number of block option 
 * @param[in] szx            Szx of block option
 * @param[in] block_opt_len  Length of block option
 * @param[in] data           Payload
 * @param[in] pos            Position of payload
 * @param[out] next          More flag of block option
 * @return 0 on success.
 ****************************************************************************************
 */
int coap_set_block_payload(int blockno, int szx,
						   int block_opt_len,
						   uint8_t *data,
						   int pos,
						   uint8_t next);

/**
 ****************************************************************************************
 * @brief Get length of CoAP block option.
 * @param[in] blockno        Number of block option
 * @return Returns length of CoAP block option.
 ****************************************************************************************
 */
int coap_get_block_opt_len(int blockno);

/**
 ****************************************************************************************
 * @brief Set CoAP buffer(Read/Write).
 * @param[in] buffer         Pointer of CoAP buffer 
 * @param[in] data           Pointer of data
 * @param[in] len            Length of data
 * @return Returns length of CoAP buffer.
 ****************************************************************************************
 */
int coap_set_rw_buffer(coap_rw_buffer_t *buffer,
					   const char *data, const size_t len);

/**
 ****************************************************************************************
 * @brief Free CoAP buffer(Read/Write).
 * @param[in] data           Pointer of CoAP buffer to free
 ****************************************************************************************
 */
void coap_free_rw_buffer(coap_rw_buffer_t *data);

/**
 ****************************************************************************************
 * @brief Copy CoAP packet(Read/Write).
 * @param[out] dst           Destination of CoAP packet
 * @param[in] src            Source of CoAP packet to copy
 * @return 0 on success.
 ****************************************************************************************
 */
int coap_copy_rw_packet(coap_rw_packet_t *dst, coap_packet_t *src);

/**
 ****************************************************************************************
 * @brief Clear CoAP packet(Read/Write).
 * @param[in] packet         Pointer of CoAP packet to clear
 ****************************************************************************************
 */
void coap_clear_rw_packet(coap_rw_packet_t *packet);

#endif // (__COAP_UTILS_H__)

/* EOF */
