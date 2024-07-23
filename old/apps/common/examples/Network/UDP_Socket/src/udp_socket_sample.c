/**
 ****************************************************************************************
 *
 * @file udp_socket_sample.c
 *
 * @brief UDP Socket sample code
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


#if defined(__UDP_SOCKET_SAMPLE__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"
#include "lwip/sockets.h"
#include "sample_defs.h"

#undef	UDP_SOCKET_SAMPLE_ENABLED_HEXDUMP

#define UDP_SOCKET_SAMPLE_PEER_PORT			UDP_CLI_TEST_PORT
#define UDP_SOCKET_SAMPLE_BUF_SIZE			(1024 * 1)
#define UDP_SOCKET_SAMPLE_DEF_TIMEOUT		100

//function properties
void udp_socket_sample_hex_dump(const char *title, unsigned char *buf, size_t len);
void udp_socket_sample(void *param);

void udp_socket_sample_hex_dump(const char *title, unsigned char *buf, size_t len)
{
#if defined (UDP_SOCKET_SAMPLE_ENABLED_HEXDUMP)
    extern void hex_dump(unsigned char *data, unsigned int length);

    if (len) {
        PRINTF("%s(%ld)\n", title, len);
        hex_dump(buf, len);
    }
#else
    DA16X_UNUSED_ARG(title);
    DA16X_UNUSED_ARG(buf);
    DA16X_UNUSED_ARG(len);
#endif // (UDP_SOCKET_SAMPLE_ENABLED_HEXDUMP)

    return ;
}

void udp_socket_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    int sock;

    struct sockaddr_in local_addr;
    struct sockaddr_in peer_addr;
    int addr_len;
    int ret;
    int optval = 1;

    int len;
    char data_buffer[UDP_SOCKET_SAMPLE_BUF_SIZE] = {0x00,};

    PRINTF("[%s] Start of UDP Socket sample\r\n", __func__);

    memset(&local_addr, 0x00, sizeof(local_addr));
    memset(&peer_addr, 0x00, sizeof(peer_addr));

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        PRINTF("[%s] Failed to create socket\r\n", __func__);
        goto end_of_task;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(UDP_SOCKET_SAMPLE_PEER_PORT);

    ret = bind(sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        PRINTF("[%s] Failed to bind socket(%d)\r\n", __func__, ret);
        goto end_of_task;
    }

    while (1) {
        memset(&peer_addr, 0x00, sizeof(struct sockaddr_in));
        memset(data_buffer, 0x00, sizeof(data_buffer));

        PRINTF("< Read from peer: ");

        ret = recvfrom(sock, data_buffer, sizeof(data_buffer), 0,
                       (struct sockaddr *)&peer_addr, (socklen_t *)&addr_len);
        if (ret > 0) {
            len = ret;

            PRINTF("%d bytes read(%d.%d.%d.%d:%d)\r\n", len,
                   (ntohl(peer_addr.sin_addr.s_addr) >> 24) & 0xff,
                   (ntohl(peer_addr.sin_addr.s_addr) >> 16) & 0xff,
                   (ntohl(peer_addr.sin_addr.s_addr) >>  8) & 0xff,
                   (ntohl(peer_addr.sin_addr.s_addr)      ) & 0xff,
                   (ntohs(peer_addr.sin_port)));

            udp_socket_sample_hex_dump("Received data", (unsigned char *)data_buffer, len);

            PRINTF("> Write to peer: ");

            ret = sendto(sock, data_buffer, len, 0,
                         (struct sockaddr *)&peer_addr, addr_len);
            if (ret < 0) {
                PRINTF("[%s] Failed to send data\r\n", __func__);
                break;
            }

            PRINTF("%d bytes written(%d.%d.%d.%d:%d)\r\n", len,
                   (ntohl(peer_addr.sin_addr.s_addr) >> 24) & 0xff,
                   (ntohl(peer_addr.sin_addr.s_addr) >> 16) & 0xff,
                   (ntohl(peer_addr.sin_addr.s_addr) >>  8) & 0xff,
                   (ntohl(peer_addr.sin_addr.s_addr)      ) & 0xff,
                   (ntohs(peer_addr.sin_port)));

            udp_socket_sample_hex_dump("Sent data", (unsigned char *)data_buffer, len);
        }
    }

end_of_task:

    close(sock);

    PRINTF("[%s] End of UDP Socket sample\r\n", __func__);

    vTaskDelete(NULL);

    return ;
}
#endif // (__UDP_SOCKET_SAMPLE__)
