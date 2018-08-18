/* tcp_perf Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "wifi_tcp.h"
#include "wifi_udp.h"

/*socket*/
static int server_socket = 0;
static struct sockaddr_in server_addr;
static struct sockaddr_in client_addr;
static unsigned int socklen = sizeof(client_addr);
static int connect_socket = 0;
bool g_rxtx_need_restart = false;

int g_total_data = 0;

#if EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO

int g_total_pack = 0;
int g_send_success = 0;
int g_send_fail = 0;
int g_delay_classify[5] = {0};

#endif /*EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO*/

//send data
void send_data(void *pvParameters)
{
    int len = 0;
    char *databuff = (char *)malloc(EXAMPLE_DEFAULT_PKTSIZE * sizeof(char));
    memset(databuff, EXAMPLE_PACK_BYTE_IS, EXAMPLE_DEFAULT_PKTSIZE);
    vTaskDelay(100 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "start sending...");

    while (1)
    {
        int to_write = EXAMPLE_DEFAULT_PKTSIZE;

        while (to_write > 0)
        {
            len = send(connect_socket, databuff + (EXAMPLE_DEFAULT_PKTSIZE - to_write), to_write, 0);
            if (len > 0)
            {
                g_total_data += len;
                to_write -= len;
            }
            else
            {
                int err = get_socket_error_code(connect_socket);

                if (err != ENOMEM)
                {
                    show_socket_error_reason("send_data", connect_socket);
                    break;
                }
            }
        }

#if EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO
        gettimeofday(&tv_finish, NULL);
#endif /*EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO*/
        if (g_total_data > 0)
        {
#if EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO
            g_send_success++;
            send_delay_ms = (tv_finish.tv_sec - tv_start.tv_sec) * 1000 + (tv_finish.tv_usec - tv_start.tv_usec) / 1000;
            if (send_delay_ms < 30)
            {
                g_delay_classify[0]++;
            }
            else if (send_delay_ms < 100)
            {
                g_delay_classify[1]++;
            }
            else if (send_delay_ms < 300)
            {
                g_delay_classify[2]++;
            }
            else if (send_delay_ms < 1000)
            {
                g_delay_classify[3]++;
            }
            else
            {
                g_delay_classify[4]++;
            }
#endif /*EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO*/
        }
        else
        {
            break;
        }
    }
    g_rxtx_need_restart = true;
    free(databuff);
    vTaskDelete(NULL);
}

//receive data
void recv_data(void *pvParameters)
{
    int len = 0;

    char databuff[1024];
    int bReuseaddr=1;

    while (1)
    {
    	//每次接收都要清空接收数组
        memset(databuff, 0x00, sizeof(databuff));
        len = recv(connect_socket, databuff, sizeof(databuff), 0);
        g_rxtx_need_restart = false;
        if (len > 0) {
            g_total_data += len;
            //打印接收到的数组
            ESP_LOGI(TAG, "recvData: %s\n", databuff);
            //原路返回，不指定某个客户端
            //send(connect_socket, databuff, sizeof(databuff), 0);
        }
        else {
            show_socket_error_reason("recv_data", connect_socket);
            //closesocket(connect_socket);
            setsockopt(connect_socket,SOL_SOCKET ,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(int));
            g_rxtx_need_restart = true;
            break;
        }
    }

    close_tcp_socket();
    g_rxtx_need_restart = true;
    vTaskDelete(NULL);
}

esp_err_t create_tcp_server(bool isCreatServer)
{

    if (isCreatServer)
    {
        ESP_LOGI(TAG, "server socket....,port=%d", TCP_PORT);
        server_socket = socket(AF_INET, SOCK_STREAM, 0);

        if (server_socket < 0)
        {
            show_socket_error_reason("create_server", server_socket);
            return ESP_FAIL;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(TCP_PORT);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            show_socket_error_reason("bind_server", server_socket);
            close(server_socket);
            return ESP_FAIL;
        }
    }

    if (listen(server_socket, 5) < 0)
    {
        show_socket_error_reason("listen_server", server_socket);
        close(server_socket);
        return ESP_FAIL;
    }

    connect_socket = accept(server_socket, (struct sockaddr *)&client_addr, &socklen);

    if (connect_socket < 0)
    {
        show_socket_error_reason("accept_server", connect_socket);
        close(server_socket);
        return ESP_FAIL;
    }

    /*connection established锛宯ow can send/recv*/
    ESP_LOGI(TAG, "tcp connection established!");
    return ESP_OK;
}

int check_working_socket()
{
    int ret;
#if EXAMPLE_ESP_TCP_MODE_SERVER
    ESP_LOGD(TAG, "check server_socket");
    ret = get_socket_error_code(server_socket);
    if (ret != 0)
    {
        ESP_LOGW(TAG, "server socket error %d %s", ret, strerror(ret));
    }
    if (ret == ECONNRESET)
    {
        return ret;
    }
#endif
    ESP_LOGD(TAG, "check connect_socket");
    ret = get_socket_error_code(connect_socket);
    if (ret != 0)
    {
        ESP_LOGW(TAG, "connect socket error %d %s", ret, strerror(ret));
    }
    if (ret != 0)
    {
        return ret;
    }
    return 0;
}

int show_socket_error_reason(const char *str, int socket)
{
    int err = get_socket_error_code(socket);

    if (err != 0)
    {
        ESP_LOGW(TAG, "%s socket error %d %s", str, err, strerror(err));
    }

    return err;
}

int send_buff_with_tcp(char *databuff, int length)
{
	int result;
	result = send(connect_socket, databuff, length, 0);

	return result;
}

void close_tcp_socket()
{
    close(connect_socket);
    close(server_socket);
}
