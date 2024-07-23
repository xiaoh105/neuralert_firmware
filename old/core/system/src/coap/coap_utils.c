/**
 ****************************************************************************************
 *
 * @file coap_utils.c
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
#include "coap_utils.h"

static void *internal_coap_calloc(size_t n, size_t size)
{
	void *buf = NULL;
	size_t buflen = (n * size);

	buf = pvPortMalloc(buflen);
	if (buf) {
		memset(buf, 0x00, buflen);
	}

	return buf;
}

static void internal_crypto_free(void *f)
{
	if (f == NULL) {
		return;
	}

	vPortFree(f);
}

void *(*coap_calloc)(size_t n, size_t size) = internal_coap_calloc;
void (*coap_free)(void *ptr) = internal_crypto_free;

int coap_client_tolower(int val)
{
	return (val >= 'A' && val <= 'Z') ? (val + 32) : (val);
}

void coap_set_calloc_free(void *(*calloc_func)(size_t, size_t),
						  void (*free_func)(void *))
{
	coap_calloc = calloc_func;
	coap_free = free_func;
}

void coap_print_hexdump(const unsigned char *title, const unsigned char *buffer,
						size_t len)
{
	if (title) {
		PRINTF("=====%s(len:%ld)=====\r\n", title, len);
	}

	if (len > 0) {
		unsigned int hex_index = 0;
		unsigned int char_index = 0;

		PRINTF("%04X: ", hex_index);

		for (hex_index = 0 ; hex_index < len ; hex_index++) {

			PRINTF("%02X ", buffer[hex_index]);

			if ((hex_index + 1) % 16 == 0) {
				PRINTF("\t");

				for (char_index = (hex_index - 15) ; char_index <= hex_index ; char_index++) {
					if ((buffer[char_index] > 0x1F) && (buffer[char_index] < 0x7F)) {
						PRINTF("%c", buffer[char_index]);
					} else {
						PRINTF(".");
					}
				}

				PRINTF("\r\n");

				if ((hex_index + 1) < len) {
					PRINTF("%04X: ", hex_index + 1);
				}
			}
		}

		hex_index--;

		if ((hex_index + 1) % 16) {
			char_index = hex_index;

			while ((hex_index + 1) % 16) {
				PRINTF("   ");
				hex_index++;
			}

			PRINTF("\t");

			hex_index = char_index;

			for (char_index = (hex_index - (hex_index % 16)) ; char_index <= hex_index ;
				 char_index++) {
				if ((buffer[char_index] > 0x1F) && (buffer[char_index] < 0x7F)) {
					PRINTF("%c", buffer[char_index]);
				} else {
					PRINTF(".");
				}
			}

			PRINTF("\r\n");
		}
	}

	return ;
}

void coap_set_in_bit_array(uint8_t *arr, int pos, int len, int val)
{
	int index = 0;

	for (index = pos + len - 1 ; index >= pos ; index--) {
		arr[index] = (uint8_t)(val % 2);
		val = val / 2;
	}
}

uint8_t coap_get_from_bit_array(uint8_t *arr, int pos, int len)
{
	int ret = 0;
	int index = 0;

	for (index = pos + len - 1 ; index >= pos ; index--) {
		if (arr[index] != 0) {
			ret = (int)(ret + pow_long(2, ((pos + len - 1) - index)));
		}
	}

	return (uint8_t)ret;
}

int coap_get_byte_from_bits(uint8_t *arr)
{
	int ret = 0;
	int index = 0;

	for (index = 7 ; index >= 0 ; index--) {
		if (arr[index] != 0) {
			ret = (int)(ret + pow_long(2, (7 - index)));
		}
	}

	return ret;
}

void coap_get_bits_from_byte(uint8_t *arr, int value)
{
	int index = 0;
	memset(arr, 0, 8);

	for (index = 7 ; index >= 0 ; index--) {
		arr[index] = (uint8_t)(value % 2);
		value = value / 2;
	}
}

void coap_print_header(coap_header_t *header)
{
	PRINTF("Header:\r\n");
	PRINTF("\t* Version: 0x%02X\r\n", header->version);
	PRINTF("\t* Type: 0x%02X\r\n", header->type);
	PRINTF("\t* Token length: 0x%02X\r\n", header->token_len);
	PRINTF("\t* State code: 0x%02X\r\n", header->code);
	PRINTF("\t* Msg-Id: 0x%02X%02X\r\n", header->msg_id[0], header->msg_id[1]);
}

int coap_parse_header(coap_header_t *header, const uint8_t *buf, size_t buflen)
{
	if (buflen < 4) {
		return COAP_ERR_HEADER_TOO_SHORT;
	}

	header->version = (buf[0] & 0xC0) >> 6;

	if (header->version != 1) {
		return COAP_ERR_VERSION_NOT_1;
	}

	header->type = (buf[0] & 0x30) >> 4;
	header->token_len = buf[0] & 0x0F;
	header->code = buf[1];
	header->msg_id[0] = buf[2];
	header->msg_id[1] = buf[3];

	return 0;
}

int coap_parse_token(coap_buffer_t *token,
					 const coap_header_t *header,
					 const uint8_t *buf,
					 size_t buflen)
{
	if (header->token_len == 0) {
		token->p = NULL;
		token->len = 0;
		return 0;
	} else {
		if (header->token_len <= 8) {
			if (4U + header->token_len > buflen) {
				return COAP_ERR_TOKEN_TOO_SHORT;    // token bigger than packet
			}

			token->p = buf + 4; // past header
			token->len = header->token_len;
			return 0;
		}
	}

	return COAP_ERR_TOKEN_TOO_SHORT;
}

int coap_parse_option(coap_option_t *opt,
					  uint16_t *run_delta,
					  const uint8_t **buf,
					  size_t buflen)
{
	const uint8_t *pos = *buf;
	uint8_t headlen = 1;
	uint16_t len, cur_delta;

	if (buflen < headlen) {
		return COAP_ERR_OPTION_TOO_SHORT_FOR_HEADER;
	}

	cur_delta = (pos[0] & 0xF0) >> 4;
	len = pos[0] & 0x0F;

	if (cur_delta == 13) {
		headlen++;

		if (buflen < headlen) {
			return COAP_ERR_OPTION_TOO_SHORT_FOR_HEADER;
		}

		cur_delta = pos[1] + 13;
		pos++;
	} else if (cur_delta == 14) {
		headlen += 2;

		if (buflen < headlen) {
			return COAP_ERR_OPTION_TOO_SHORT_FOR_HEADER;
		}

		cur_delta = (uint16_t)(((pos[1] << 8) | pos[2]) + 269);
		pos += 2;
	} else if (cur_delta == 15) {
		return COAP_ERR_OPTION_DELTA_INVALID;
	}

	if (len == 13) {
		headlen++;

		if (buflen < headlen) {
			return COAP_ERR_OPTION_TOO_SHORT_FOR_HEADER;
		}

		len = pos[1] + 13;
		pos++;
	} else if (len == 14) {
		headlen += 2;

		if (buflen < headlen) {
			return COAP_ERR_OPTION_TOO_SHORT_FOR_HEADER;
		}

		len = (uint16_t)(((pos[1] << 8) | pos[2]) + 269);
		pos += 2;
	} else if (len == 15) {
		return COAP_ERR_OPTION_LEN_INVALID;
	}

	if ((pos + 1 + len) > (*buf + buflen)) {
		return COAP_ERR_OPTION_TOO_BIG;
	}

	//PRINTF("option num=%d\r\n", cur_delta + *run_delta);
	opt->num = (uint8_t)(cur_delta + *run_delta);
	opt->buf.p = pos + 1;
	opt->buf.len = len;

	// advance buf
	*buf = pos + 1 + len;
	*run_delta += cur_delta;

	return 0;
}

int coap_parse_option_and_payload(coap_option_t *opt,
								  uint8_t *num_opt,
								  coap_buffer_t *payload,
								  const coap_header_t *hdr,
								  const uint8_t *buf,
								  size_t buflen)
{
	size_t idx = 0;
	uint16_t delta = 0;
	const uint8_t *pos = buf + 4 + hdr->token_len;
	const uint8_t *end = buf + buflen;
	int retval;

	if (pos > end) {
		return COAP_ERR_OPTION_OVERFLOW;
	}

	while ((idx < *num_opt) && (pos < end) && (*pos != 0xFF)) {
		if (0 != (retval = coap_parse_option(&opt[idx], &delta, &pos, (size_t)(end - pos)))) {
			return retval;
		}

		idx++;
	}

	*num_opt = (uint8_t)idx;

	// payload marker(0xFF)
	if (pos + 1 < end && *pos == 0xFF) {
		payload->p = pos + 1;
		payload->len = (size_t)(end - (pos + 1));
	} else {
		payload->p = NULL;
		payload->len = 0;
	}

	return COAP_ERR_SUCCESS;
}

void coap_print_options(coap_option_t *opts, size_t num_opt)
{
	size_t i;
	PRINTF(" Options:\r\n");

	for (i = 0 ; i < num_opt ; i++) {
		PRINTF(" #%d. Option:\r\n", i + 1);
		coap_print_hexdump(NULL, opts[i].buf.p, opts[i].buf.len);
	}
}

void coap_print_packet(coap_packet_t *packet)
{
	coap_print_header(&packet->header);
	coap_print_options(packet->opts, packet->numopts);
	coap_print_hexdump((const unsigned char *)"Payload", packet->payload.p, packet->payload.len);
	PRINTF("\r\n");
}

void coap_print_option_num(int num)
{
	switch (num) {
		case COAP_OPTION_IF_MATCH:
			PRINTF("IF-MATCH");
			break;
		case COAP_OPTION_URI_HOST:
			PRINTF("URI_HOST");
			break;
		case COAP_OPTION_ETAG:
			PRINTF("ETAG");
			break;
		case COAP_OPTION_IF_NONE_MATCH:
			PRINTF("IF_NONE_MATCH");
			break;
		case COAP_OPTION_OBSERVE:
			PRINTF("OBSERVE");
			break;
		case COAP_OPTION_URI_PORT:
			PRINTF("URI_PORT");
			break;
		case COAP_OPTION_LOCATION_PATH:
			PRINTF("LOCATION_PATH");
			break;
		case COAP_OPTION_URI_PATH:
			PRINTF("URI_PATH");
			break;
		case COAP_OPTION_CONTENT_FORMAT:
			PRINTF("CONTENT_FORMAT");
			break;
		case COAP_OPTION_MAX_AGE:
			PRINTF("MAX_AGE");
			break;
		case COAP_OPTION_URI_QUERY:
			PRINTF("URI_QUERY");
			break;
		case COAP_OPTION_ACCEPT:
			PRINTF("ACCEPT");
			break;
		case COAP_OPTION_LOCATION_QUERY:
			PRINTF("LOCATION_QUERY");
			break;
		case COAP_OPTION_PROXY_URI:
			PRINTF("PROXY_URI");
			break;
		case COAP_OPTION_PROXY_SCHEME:
			PRINTF("PROXY_SCHEME");
			break;
		case COAP_OPTION_BLOCK_1:
			PRINTF("BLOCK_1");
			break;
		case COAP_OPTION_BLOCK_2:
			PRINTF("BLOCK_2");
			break;
		case COAP_OPTION_SIZE_1:
			PRINTF("SIZE_1");
			break;
		case COAP_OPTION_SIZE_2:
			PRINTF("SIZE_2");
			break;
		default:
			PRINTF("%d", num);
			break;
	}
}

void coap_print_content_format(int num)
{
	switch (num) {
		case COAP_CONTENT_TYPE_TEXT_PLAIN:
			PRINTF("text/plain");
			break;
		case COAP_CONTENT_TYPE_APPLICATION_LINKFORMAT:
			PRINTF("application/link-format");
			break;
		default:
			PRINTF("%d", num);
			break;
	}
}

void coap_print_response(coap_packet_t *packet, int show_opt)
{
	if (packet->header.type == COAP_MSG_TYPE_ACK && packet->payload.len == 0) {
		PRINTF("\r\n--ACK Received for separate response packet--\r\n");
		return;
	}

	if (packet->header.type != COAP_MSG_TYPE_ACK && packet->payload.len > 0) {
		PRINTF("\r\n--Payload Received for separate response packet--\r\n");
	}

	switch (packet->header.code) {
		case COAP_RESP_CODE_CREATED:
			PRINTF("\r\nContent Created\r\n");
			break;
		case COAP_RESP_CODE_DELETED:
			PRINTF("\r\nContent Delete\r\n");
			break;
		case COAP_RESP_CODE_VALID:
			PRINTF("\r\nContent Valid\r\n");
			break;
		case COAP_RESP_CODE_CONTINUE:
			PRINTF("\r\nContent Continue\r\n");
			break;
		case COAP_RESP_CODE_CHANGED:
			PRINTF("\r\nContent Changed\r\n");
			break;
		case COAP_RESP_CODE_UNAUTHORIZED:
			PRINTF("\r\nCoAP Error : UNAUTHORIZED\r\n");
			break;
		case COAP_RESP_CODE_BAD_OPTION:
			PRINTF("\r\nCoAP Error : BAD_OPTION\r\n");
			break;
		case COAP_RESP_CODE_FORBIDDEN:
			PRINTF("\r\nCoAP Error : FORBIDDEN\r\n");
			break;
		case COAP_RESP_CODE_NOT_FOUND:
			PRINTF("\r\nCoAP Error : NOT_FOUND\r\n");
			break;
		case COAP_RESP_CODE_METHOD_NOT_ALLOWED:
			PRINTF("\r\nCoAP Error : NOT_ALLOWED\r\n");
			break;
		case COAP_RESP_CODE_NOT_ACCEPTABLE:
			PRINTF("\r\nCoAP Error : NOT_ACCEPTABLE\r\n");
			break;
		case COAP_RESP_CODE_PRECONDITION_FAILED:
			PRINTF("\r\nCoAP Error : PRECONDITION_FAILED\r\n");
			break;
		case COAP_RESP_CODE_REQUEST_ENTITY_TOO_LARGE:
			PRINTF("\r\nCoAP Error : ENTITY_TOO_LARGE\r\n");
			break;
		case COAP_RESP_CODE_UNSUPPORTED_CONTENT_FORMAT:
			PRINTF("\r\nCoAP Error : UNSUPPORTED_CONTENT_FORMAT\r\n");
			break;
		case COAP_RESP_CODE_CONTENT:
			if (show_opt) {
				PRINTF("\r\nOptions -");
			}

			for (int i = 0 ; i < packet->numopts ; i++) {
				if (show_opt) {
					PRINTF ("\r\n  Option Count %d", i + 1);
					PRINTF ("\r\n  Option Type : ");
					coap_print_option_num( packet->opts[i].num );
					PRINTF ("\r\n ");
				}

				if (packet->opts[i].num == COAP_OPTION_CONTENT_FORMAT) {
					if (show_opt) {
						PRINTF ("\r\n\r\n  Content Format : ");

						if (packet->opts[i].buf.len == 0) {
							coap_print_content_format(0);
						} else if (packet->opts[i].buf.len != 0) {
							coap_print_content_format(packet->opts[i].buf.p[0]);
						}
					}
				} else if ((packet->opts[i].num == COAP_OPTION_BLOCK_1)
						   || (packet->opts[i].num == COAP_OPTION_BLOCK_2)) {
					uint32_t num = 0;
					uint8_t more = 0;
					uint8_t szx = 0;

					coap_get_block_info(&(packet->opts[i]), &num, &more, &szx);

					if (show_opt) {
						PRINTF("  NUM:%d, M: %d, SZX: %d\r\n", num, more, szx);
					}
				} else if (packet->opts[i].num == COAP_OPTION_OBSERVE) {
					if (show_opt) {
						PRINTF("Opt Length : %ld", (packet->opts[i].buf.p[0] & 0x0F));
					}
				} else if ((packet->opts[i].num == COAP_OPTION_SIZE_1)
						   || (packet->opts[i].num == COAP_OPTION_SIZE_2)) {
					if (show_opt) {
						int block_size = 0;

						for (unsigned int idx = 0 ; idx < packet->opts[i].buf.len ; idx++) {
							block_size =
								((block_size << 8) | packet->opts[i].buf.p[idx]);
						}

						PRINTF("  Block Size : %d", block_size);
					}
				} else {
					if (show_opt) {
						coap_print_hexdump((const unsigned char *)"\r\n Option Data :",
										   packet->opts[i].buf.p,
										   packet->opts[i].buf.len);
					}
				}

				if (show_opt) {
					PRINTF ("\r\n");
				}
			}
			break;
		default:
			coap_print_header(&packet->header);
			break;
	}

	if (packet->payload.len > 0) {
		if (show_opt || packet->header.code != COAP_RESP_CODE_CONTENT) {
			PRINTF("\r\nPayload -");
			PRINTF("\r\n  Length - %d\r\n", packet->payload.len);
		}
	} else if (packet->payload_raw.len > 0) {
		if (show_opt || packet->header.code != COAP_RESP_CODE_CONTENT) {
			PRINTF("\r\nPayload -");
			PRINTF("\r\n  Length - %d\r\n", packet->payload_raw.len);
		}
	} else {
		PRINTF("\r\nNo Payload.\r\n");
	}

	return ;
}

void coap_get_block_info(coap_option_t *opt,
						 uint32_t *num, uint8_t *next, uint8_t *szx)
{
	int total_len = 0;
	uint8_t *arr = NULL;

	if (!opt || opt->buf.len == 0) {
		return ;
	}

	total_len = (int)(opt->buf.len * 8);

	arr = coap_calloc(1, opt->buf.len * 8);
	if (arr == NULL) {
		PRINTF("[%s]failed to allocate memory\r\n", __func__);
		return ;
	}

	for (unsigned int j = 0 ; j < opt->buf.len ; j++) {
		coap_set_in_bit_array(arr, (int)(j * 8), 8, opt->buf.p[j]);
	}

	*num = coap_get_from_bit_array(arr, 0, total_len - 4);
	*next = coap_get_from_bit_array(arr, total_len - 4, 1);
	*szx = coap_get_from_bit_array(arr, total_len - 3, 3);

	coap_free(arr);

	return ;
}

int coap_parse_raw_data(coap_packet_t *packet, const uint8_t *buf, size_t buflen)
{
	int ret = 0;

	ret = coap_parse_header(&packet->header, buf, buflen);
	if (ret) {
		return ret;
	}

	ret = coap_parse_token(&packet->token, &packet->header, buf, buflen);
	if (ret) {
		return ret;
	}

	packet->numopts = MAXOPT;

	ret = coap_parse_option_and_payload(packet->opts, &(packet->numopts),
										&(packet->payload), &packet->header,
										buf, buflen);
	if (ret) {
		return ret;
	}

	return 0;
}

const coap_option_t *coap_search_options(const coap_packet_t *packet,
										 uint8_t num, uint8_t *count)
{
	int idx;
	const coap_option_t *opt = NULL;
	*count = 0;

	for (idx = 0 ; idx < packet->numopts ; idx++) {
		if (packet->opts[idx].num == num) {
			if (opt == NULL) {
				opt = &packet->opts[idx];
			}

			(*count)++;
		} else {
			if (opt) {
				break;
			}
		}
	}

	return opt;
}

int coap_generate_packet(uint8_t *buf, size_t *buflen, const coap_packet_t *packet)
{
	size_t opts_len = 0;
	size_t i;
	uint8_t *pos = NULL;

	uint32_t last_option_number = 0;
	uint32_t option_delta = 0;
	uint8_t option_delta_nibble = 0;

	uint32_t option_length = 0;
	uint8_t option_length_nibble = 0;

	uint8_t byte_buf[32] = {0x00,};

	// construct coap header
	if (*buflen < (4U + packet->header.token_len)) {
		return COAP_ERR_BUFFER_TOO_SMALL;
	}

	buf[0] = (uint8_t)((packet->header.version & 0x03) << 6);
	buf[0] = (uint8_t)(buf[0] | ((packet->header.type & 0x03) << 4));
	buf[0] = (uint8_t)(buf[0] | (packet->header.token_len & 0x0F));
	buf[1] = packet->header.code;
	buf[2] = packet->header.msg_id[0];
	buf[3] = packet->header.msg_id[1];

	// write token
	pos = buf + 4;

	if ((packet->header.token_len > 0)
		&& (packet->header.token_len != packet->token.len)) {
		return COAP_ERR_NOT_SUPPORTED;
	}

	if (packet->header.token_len > 0) {
		memcpy(pos, packet->token.p, packet->header.token_len);
	}

	// write options
	pos += packet->header.token_len;

	for (i = 0 ; i < packet->numopts ; i++) {
		// write 4-bit option delta
		option_delta = packet->opts[i].num - last_option_number;
		coap_get_option_nibble(option_delta, &option_delta_nibble);
		coap_set_in_bit_array(byte_buf, 0, 4, option_delta_nibble);

		// write 4-bit option length
		option_length = packet->opts[i].buf.len;
		coap_get_option_nibble(option_length, &option_length_nibble);
		coap_set_in_bit_array(byte_buf, 4, 4, option_length_nibble);

		*pos++ = (uint8_t)coap_get_byte_from_bits(byte_buf);

		// write extended option delta field (0 - 2 bytes)
		if (option_delta_nibble == 13) {
			coap_set_in_bit_array(byte_buf, 0, 8, (int)(option_delta - 13));
			*pos++ = (uint8_t)coap_get_byte_from_bits(byte_buf);
		} else if (option_delta_nibble == 14) {
			coap_set_in_bit_array(byte_buf, 0, 16, (int)(option_delta - 269));
			*pos++ = (uint8_t)coap_get_byte_from_bits(byte_buf);
			*pos++ = (uint8_t)coap_get_byte_from_bits(byte_buf + 8);
		}

		// write extended option length field (0 - 2 bytes)
		if (option_length_nibble == 13) {
			coap_set_in_bit_array(byte_buf, 0, 8, (int)(option_length - 13));
			*pos++ = (uint8_t)coap_get_byte_from_bits(byte_buf);
		} else if (option_length_nibble == 14) {
			coap_set_in_bit_array(byte_buf, 0, 16, (int)(option_length - 269));
			*pos++ = (uint8_t)coap_get_byte_from_bits(byte_buf);
			*pos++ = (uint8_t)coap_get_byte_from_bits(byte_buf + 8);
		}

		// write option value
		if (option_length) {
			memcpy(pos, packet->opts[i].buf.p, option_length);
			pos += option_length;
		}

		last_option_number = packet->opts[i].num;
	}

	// number of bytes used by options
	opts_len = (size_t)((pos - buf) - 4);

	if (packet->copy_mode && packet->payload_raw.len > 0) {
		if (*buflen < 4 + 1 + packet->payload_raw.len + opts_len) {
			return COAP_ERR_BUFFER_TOO_SMALL;
		}

		//add payload marker(0xFF)
		buf[4 + opts_len] = 0xFF;

		memcpy(buf + 5 + opts_len, packet->payload_raw.p, packet->payload_raw.len);
		*buflen = opts_len + 5 + packet->payload_raw.len;
	} else if (packet->payload.len > 0) {
		if (*buflen < 4 + 1 + packet->payload.len + opts_len) {
			return COAP_ERR_BUFFER_TOO_SMALL;
		}

		//add payload marker(0xFF)
		buf[4 + opts_len] = 0xFF;

		memcpy(buf + 5 + opts_len, packet->payload.p, packet->payload.len);
		*buflen = opts_len + 5 + packet->payload.len;
	} else {
		*buflen = opts_len + 4;
	}

	return COAP_ERR_SUCCESS;
}

int coap_get_option_nibble(uint32_t value, uint8_t *nibble)
{
	if (value < 13) {
		*nibble = (0xFF & value);
	} else if (value <= 0xFF + 13) {
		*nibble = 13;
	} else if (value <= 0xFFFF + 269) {
		*nibble = 14;
	} else {
		return -1;
	}

	return 0;
}

int coap_generate_response(coap_rw_buffer_t *scratch,
						   coap_packet_t *packet,
						   const uint8_t *content,
						   size_t content_len,
						   uint8_t msgid_hi,
						   uint8_t msgid_lo,
						   const coap_buffer_t *token,
						   coap_resp_code_t rsp_code,
						   coap_content_type_t content_type,
						   coap_header_t req_hdr)
{
	packet->header.version = 0x01;

	if (req_hdr.type == COAP_MSG_TYPE_CON) {
		packet->header.type = COAP_MSG_TYPE_ACK;
	} else {
		packet->header.type = COAP_MSG_TYPE_NONCON;
	}

	packet->header.token_len = 0;
	packet->header.code = rsp_code;
	packet->header.msg_id[0] = msgid_hi;
	packet->header.msg_id[1] = msgid_lo;
	packet->numopts = 1;

	// add token
	if (token) {
		packet->header.token_len = (uint8_t)(token->len);
		packet->token = *token;
	}

	packet->opts[0].num = COAP_OPTION_CONTENT_FORMAT;
	packet->opts[0].buf.p = scratch->p;

	if (scratch->len < 2) {
		return COAP_ERR_BUFFER_TOO_SMALL;
	}

	scratch->p[0] = (uint8_t)(((uint16_t)content_type & 0xFF00) >> 8);
	scratch->p[1] = (uint8_t)((uint16_t)content_type & 0x00FF);
	packet->opts[0].buf.len = 2;
	packet->payload.p = content;
	packet->payload.len = content_len;

	return 0;
}

int coap_generate_seperate_response(coap_rw_buffer_t *scratch,
									coap_packet_t *packet,
									const uint8_t *content,
									size_t content_len,
									uint8_t msgid_hi,
									uint8_t msgid_lo,
									const coap_buffer_t *token,
									coap_resp_code_t rsp_code,
									coap_content_type_t content_type,
									coap_header_t req_hdr)
{
	DA16X_UNUSED_ARG(req_hdr);

	packet->header.version = 0x01;
	packet->header.type = COAP_MSG_TYPE_CON;
	packet->header.token_len = 0;
	packet->header.code = rsp_code;
	packet->header.msg_id[0] = msgid_hi;
	packet->header.msg_id[1] = msgid_lo;
	packet->numopts = 1;

	// add token
	if (token) {
		packet->header.token_len = (uint8_t)(token->len);
		packet->token = *token;
	}

	packet->opts[0].num = COAP_OPTION_CONTENT_FORMAT;
	packet->opts[0].buf.p = scratch->p;

	if (scratch->len < 2) {
		return COAP_ERR_BUFFER_TOO_SMALL;
	}

	scratch->p[0] = (uint8_t)(((uint16_t)content_type & 0xFF00) >> 8);
	scratch->p[1] = (uint8_t)((uint16_t)content_type & 0x00FF);
	packet->opts[0].buf.len = 2;
	packet->payload.p = content;
	packet->payload.len = content_len;

	return 0;
}

int coap_set_bytes_splitted_by_block(coap_packet_t *packet,
									 int block_num, int block_szx, uint8_t *block_more)
{
	unsigned int block_size = (unsigned int)pow_long(2, block_szx + 4);

	unsigned int start_idx = 0;
	int add_fact = 0;

	int ret = 0;

	start_idx = ((unsigned int)block_num * block_size);

	if (start_idx >= packet->payload.len) {
		packet->payload.len = 0;
		return 0;
	}

	if (packet->payload.len < start_idx + block_size) {
		add_fact = (int)(packet->payload.len - start_idx);
		*block_more = 0;
	} else {
		add_fact = (int)block_size;
		*block_more = 1;
	}

	ret = coap_set_rw_buffer(&packet->payload_raw,
							 (const char *)(packet->payload.p + start_idx),
							 (size_t)add_fact);

	if (ret < 0) {
		PRINTF("[%s]failed to copy payload(%d)\r\n", __func__, ret);
		return -1;
	}

	packet->copy_mode = 1;

	return 0;
}

int coap_set_block_payload(int blockno,
						   int szx,
						   int block_opt_len,
						   uint8_t *data,
						   int pos,
						   uint8_t next)
{
	int total_len = block_opt_len * 8;
	uint8_t *arr = NULL;
	int idx = 0;

	arr = coap_calloc(1, (size_t)total_len);

	if (arr == NULL) {
		return 0;
	}

	coap_set_in_bit_array(arr, 0, total_len - 4, blockno);
	coap_set_in_bit_array(arr, total_len - 4, 1, next);
	coap_set_in_bit_array(arr, total_len - 3, 3, szx);

	for (idx = 0 ; idx < block_opt_len ; idx++) {
		data[pos++] = coap_get_from_bit_array(arr, idx * 8, 8);
	}

	coap_free(arr);

	return pos;
}

int coap_get_block_opt_len(int blockno)
{
	int block_opt_len = 0;

	if (blockno <= 15) {
		block_opt_len = 1;
	} else if (blockno > 15 && blockno <= (255 + 15)) {
		block_opt_len = 2;
	} else {
		block_opt_len = 2 + ((blockno - (255 + 15)) % 255) + 1;
	}

	return block_opt_len;
}

int coap_set_rw_buffer(coap_rw_buffer_t *buffer,
					   const char *data, const size_t len)
{
	uint8_t *buf = NULL;
	size_t buflen = buffer->len + len;

	if (data == NULL || len < 1) {
		return 0;
	}

	buf = coap_calloc(1, buflen + 1);
	if (!buf) {
		PRINTF("[%s]failed to allocate memory\r\n", __func__);
		return -1;
	}

	if (buffer->len) {
		memcpy(buf, buffer->p, buffer->len);
	}

	memcpy(buf + buffer->len, data, len);
	buf[buflen] = '\0';

	if (buffer->p) {
		coap_free(buffer->p);
	}

	buffer->p = buf;
	buffer->len = buflen;

	return (int)buflen;
}

void coap_free_rw_buffer(coap_rw_buffer_t *data)
{
	if (data->p != NULL) {
		coap_free(data->p);
	}

	data->p = NULL;
	data->len = 0;

	return ;
}

int coap_copy_rw_packet(coap_rw_packet_t *dst, coap_packet_t *src)
{
	int opt_idx = 0;

	if (!dst || !src) {
		return -1;
	}

	//Copy header
	memcpy(&dst->header, &src->header, sizeof(coap_header_t));

	//Copy Token
	if (dst->token.len) {
		coap_free_rw_buffer(&dst->token);
	}

	coap_set_rw_buffer(&dst->token, (const char *)src->token.p, src->token.len);

	//Copy Options
	if (dst->numopts) {
		for (opt_idx = 0 ; opt_idx < dst->numopts ; opt_idx++) {
			dst->opts[opt_idx].num = 0;
			coap_free_rw_buffer(&dst->opts[opt_idx].buf);
		}

		dst->numopts = 0;
	}

	dst->numopts = src->numopts;

	for (opt_idx = 0 ; opt_idx < dst->numopts ; opt_idx++) {
		dst->opts[opt_idx].num = src->opts[opt_idx].num;
		coap_set_rw_buffer(&dst->opts[opt_idx].buf,
						   (const char *)src->opts[opt_idx].buf.p,
						   src->opts[opt_idx].buf.len);
	}

	//Copy Payload
	if (dst->payload.len) {
		coap_free_rw_buffer(&dst->payload);
	}

	coap_set_rw_buffer(&dst->payload,
					   (const char *)src->payload.p, src->payload.len);

	return 0;
}

void coap_clear_rw_packet(coap_rw_packet_t *packet)
{
	if (packet == NULL) {
		return ;
	}

	coap_free_rw_buffer(&packet->token);

	for (int idx =  0 ; idx < packet->numopts ; idx++) {
		coap_free_rw_buffer(&packet->opts[idx].buf);
	}

	coap_free_rw_buffer(&packet->payload);

	memset(packet, 0x00, sizeof(coap_rw_packet_t));

	return ;
}

/* EOF */
