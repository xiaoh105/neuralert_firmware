/**
 ****************************************************************************************
 *
 * @file tcp_client_sample.c
 *
 * @brief TCP Client sample code
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


#if defined ( __TCP_CLIENT_SAMPLE__ )

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"
#include "lwip/sockets.h"
#include "sample_defs.h"

#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#undef	TCP_CLIENT_SAMPLE_ENABLED_HEXDUMP

#define TCP_CLIENT_SAMPLE_DEF_PORT				10192
#define TCP_CLIENT_SAMPLE_DEF_BUF_SIZE			(1024 * 1)
#define TCP_CLIENT_SAMPLE_DEF_SERVER_IP_ADDR	"192.168.0.11"
#define TCP_CLIENT_SAMPLE_DEF_SERVER_PORT		TCP_CLI_TEST_PORT

//function properties
void tcp_client_sample_hex_dump(const char *title, unsigned char *buf, size_t len);
void tcp_client_sample(void *param);

//bodies
void tcp_client_sample_hex_dump(const char *title, unsigned char *buf, size_t len)
{
#if defined (TCP_CLIENT_SAMPLE_ENABLED_HEXDUMP)
    extern void hex_dump(unsigned char *data, unsigned int length);

    if (len) {
        PRINTF("%s(%ld)\n", title, len);
        hex_dump(buf, len);
    }
#else
    DA16X_UNUSED_ARG(title);
    DA16X_UNUSED_ARG(buf);
    DA16X_UNUSED_ARG(len);
#endif // (TCP_CLIENT_SAMPLE_ENABLED_HEXDUMP)

    return ;
}

void tcp_client_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    int ret = 0;
    int socket_fd;
    struct sockaddr_in local_addr;
    struct sockaddr_in srv_addr;

    int len = 0;
    char data_buffer[TCP_CLIENT_SAMPLE_DEF_BUF_SIZE] = {0x00,};

    PRINTF("[%s] Start of TCP Client sample\r\n", __func__);

    memset(&local_addr, 0x00, sizeof(struct sockaddr_in));
    memset(&srv_addr, 0x00, sizeof(struct sockaddr_in));

    socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        PRINTF("[%s] Failed to create socket\r\n", __func__);
        goto end_of_task;
    }

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(TCP_CLIENT_SAMPLE_DEF_PORT);

    ret = bind(socket_fd, (struct sockaddr *)&local_addr,
               sizeof(struct sockaddr_in));
    if (ret == -1) {
        PRINTF("[%s] Failed to bind socket\r\n", __func__);

        close(socket_fd);
        goto end_of_task;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = inet_addr(TCP_CLIENT_SAMPLE_DEF_SERVER_IP_ADDR);
    srv_addr.sin_port = htons(TCP_CLIENT_SAMPLE_DEF_SERVER_PORT);

    ret = connect(socket_fd, (struct sockaddr_in *)&srv_addr,
                  sizeof(struct sockaddr_in));
    if (ret < 0) {
        PRINTF("[%s] Failed to establish TCP session\r\n", __func__);

        close(socket_fd);
        goto end_of_task;
    }

    while (1) {
        memset(data_buffer, 0x00, sizeof(data_buffer));

        PRINTF("< Read from server: ");

        len = recv(socket_fd, data_buffer, sizeof(data_buffer), 0);
        if (len <= 0) {
            PRINTF("[%s] Failed to receive data(%d)\r\n", __func__, len);
            break;
        }

        data_buffer[len] = '\0';

        PRINTF("%d bytes read\r\n", len);

        tcp_client_sample_hex_dump("Received data", (unsigned char *) data_buffer, len);

        PRINTF("> Write to server: ");

        len = send(socket_fd, data_buffer, len, 0);
        if (len <= 0) {
            PRINTF("[%s] Failed to send data\r\n", __func__);
            break;
        }

        PRINTF("%d bytes written\r\n", len);

        tcp_client_sample_hex_dump("Sent data", (unsigned char *)data_buffer, len);
    }

    close(socket_fd);

end_of_task:

    PRINTF("[%s] End of TCP Client sample\r\n", __func__);

    vTaskDelete(NULL);

    return ;
}
#endif // ( __TCP_CLIENT_SAMPLE__ )

/* EOF */
