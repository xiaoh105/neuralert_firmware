/**
 ****************************************************************************************
 *
 * @file tcp_server_sample.c
 *
 * @brief TCP Server sample code
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


#if defined(__TCP_SERVER_SAMPLE__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"
#include "lwip/sockets.h"
#include "sample_defs.h"

#undef	TCP_SERVER_SAMPLE_ENABLED_HEXDUMP

#define	TCP_SERVER_SAMPLE_DEF_BUF_SIZE		(1024 * 1)
#define	TCP_SERVER_SAMPLE_DEF_PORT			TCP_SVR_TEST_PORT
#define	TCP_SERVER_SAMPLE_DEF_TIMEOUT		100
#define	TCP_SERVER_SAMPLE_BACKLOG			1

//function properties
void tcp_server_sample_hex_dump(const char *title, unsigned char *buf, size_t len);
void tcp_server_sample(void *params);

void tcp_server_sample_hex_dump(const char *title, unsigned char *buf, size_t len)
{
#if defined (TCP_SERVER_SAMPLE_ENABLED_HEXDUMP)
    extern void hex_dump(unsigned char *data, unsigned int length);

    if (len) {
        PRINTF("%s(%ld)\n", title, len);
        hex_dump(buf, len);
    }
#else
    DA16X_UNUSED_ARG(title);
    DA16X_UNUSED_ARG(buf);
    DA16X_UNUSED_ARG(len);
#endif // (TCP_SERVER_SAMPLE_ENABLED_HEXDUMP)
    return ;
}

void tcp_server_sample(void *param)
{
    DA16X_UNUSED_ARG(param);

    int ret = 0;
    int listen_sock = -1;
    int client_sock = -1;

    struct sockaddr_in server_addr;

    struct sockaddr_in client_addr;
    int client_addrlen = sizeof(struct sockaddr_in);

    int len = 0;
    char data_buffer[TCP_SERVER_SAMPLE_DEF_BUF_SIZE] = {0x00,};

    memset(&server_addr, 0x00, sizeof(struct sockaddr_in));
    memset(&client_addr, 0x00, sizeof(struct sockaddr_in));

    PRINTF("[%s] Start of TCP Server sample\r\n", __func__);

    listen_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        PRINTF("[%s] Failed to create listen socket\r\n", __func__);
        goto end_of_task;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(TCP_SERVER_SAMPLE_DEF_PORT);

    ret = bind(listen_sock, (struct sockaddr *)&server_addr,
               sizeof(struct sockaddr_in));
    if (ret == -1) {
        PRINTF("[%s] Failed to bind socket\r\n", __func__);
        goto end_of_task;
    }

    ret = listen(listen_sock, TCP_SERVER_SAMPLE_BACKLOG);
    if (ret != 0) {
        PRINTF("[%s] Failed to listen socket of tcp server(%d)\r\n",
               __func__, ret);
        goto end_of_task;
    }

    while (1) {
        client_sock = -1;
        memset(&client_addr, 0x00, sizeof(struct sockaddr_in));
        client_addrlen = sizeof(struct sockaddr_in);

        client_sock = accept(listen_sock, (struct sockaddr *)&client_addr,
                             (socklen_t *)&client_addrlen);
        if (client_sock < 0) {
            continue;
        }

        PRINTF("Connected client(%d.%d.%d.%d:%d)\r\n",
               (ntohl(client_addr.sin_addr.s_addr) >> 24) & 0xFF,
               (ntohl(client_addr.sin_addr.s_addr) >> 16) & 0xFF,
               (ntohl(client_addr.sin_addr.s_addr) >>  8) & 0xFF,
               (ntohl(client_addr.sin_addr.s_addr)      ) & 0xFF,
               (ntohs(client_addr.sin_port)));

        while (1) {
            memset(data_buffer, 0x00, sizeof(data_buffer));

            PRINTF("< Read from client: ");

            len = recv(client_sock, data_buffer, sizeof(data_buffer), 0);
            if (len <= 0) {
                PRINTF("[%s] Failed to receive data(%d)\r\n", __func__, len);
                break;
            }

            data_buffer[len] = '\0';

            PRINTF("%d bytes read\r\n", len);

            tcp_server_sample_hex_dump("Received data", (unsigned char *)data_buffer, len);

            PRINTF("> Write to client: ");

            len = send(client_sock, data_buffer, len, 0);
            if (len <= 0) {
                PRINTF("[%s] Failed to send data\r\n", __func__);
                break;
            }

            PRINTF("%d bytes written\r\n", len);

            tcp_server_sample_hex_dump("Sent data", (unsigned char *)data_buffer, len);
        }

        close(client_sock);

        PRINTF("Disconnected client\r\n");
    }

end_of_task:

    PRINTF("[%s] End of TCP Server sample\r\n", __func__);

    close(listen_sock);
    close(client_sock);

    vTaskDelete(NULL);

    return ;
}
#endif // (__TCP_SERVER_SAMPLE__)
