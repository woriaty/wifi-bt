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
#include "uart.h"
#include "cmd.h"

/*socket*/
static int server_socket = 0;
static struct sockaddr_in server_addr;
static struct sockaddr_in client_addr;
static unsigned int socklen = sizeof(client_addr);
static int connect_socket = 0;
bool g_rxtx_need_restart = false;

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

int tcp_cmd_send(int port, const char *buff, int len)
{
	send(port, buff, len, 0);
	return 0;
}

struct cmd_ops tcp_cmd_ops = {
	.cmd_send = tcp_cmd_send,
};

//receive data
void recv_data(void *pvParameters)
{
    int len = 0;
    char databuff[100];
    char *hi = "*HELLO*\n\r";

    send(connect_socket, hi, strlen(hi), 0);
    //send(connect_socket, prompt, strlen(prompt), 0);
    while (1)
    {
    	//每次接收都要清空接收数组
        memset(databuff, 0x00, sizeof(databuff));
        len = recv(connect_socket, databuff, sizeof(databuff), 0);
        g_rxtx_need_restart = false;
        if (len > 0) {
            //打印接收到的数组
            ESP_LOGI(TAG, "recvData: %s", databuff);
            tcp_cmd_ops.port = connect_socket;
            cmd_cli(&tcp_cmd_ops, databuff, len);
            len = 0;
        }
        else {
            show_socket_error_reason("recv_data", connect_socket);
            //g_rxtx_need_restart = true;
            break;
        }
        vTaskDelay(300 / portTICK_RATE_MS);
    }

    close_tcp_socket();
    //allow the bind address reuse
    //setsockopt(connect_socket,SOL_SOCKET ,SO_REUSEADDR,(const int*)&bReuseaddr,sizeof(int));
    //setsockopt(server_socket,SOL_SOCKET ,SO_REUSEADDR,(const int*)&bReuseaddr,sizeof(int));
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

//this task establish a TCP connection and receive data from TCP
void tcp_conn(void *pvParameters)
{
	struct sys_data *wifi_bt_data = pvParameters;
	g_rxtx_need_restart = false;
    ESP_LOGI(TAG, "task tcp_conn...");

    /*wating for connecting to AP*/
    xEventGroupWaitBits(tcp_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    TaskHandle_t tx_rx_task = NULL;
    wifi_bt_data->wifi_tcp_enabled = ENABLE;
    xEventGroupSetBits(uart_event_group, UART_CONNECTED_BIT);

    ESP_LOGI(TAG, "tcp_server will start after 3s...");
    vTaskDelay(3000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "create_tcp_server.");
    int socket_ret = create_tcp_server(true);
    if (socket_ret == ESP_FAIL) {
    	ESP_LOGI(TAG, "create tcp socket error,stop...");
        //continue;
    	goto tcp_fail;
    }
    else {
        ESP_LOGI(TAG, "create tcp socket succeed...");
    }

    if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task)) {
        ESP_LOGI(TAG, "Recv task create fail!");
    }
    else {
        ESP_LOGI(TAG, "Recv task create succeed!");
    }

    while (1) {
        vTaskDelay(3000 / portTICK_RATE_MS);

        if (g_rxtx_need_restart) {
        	ESP_LOGE(TAG, "tcp server send or receive task encoutner error, need to restart...");

        	if (ESP_FAIL != create_tcp_server(true)) {
        		if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task)) {
        			ESP_LOGE(TAG, "tcp server Recv task create fail!");
        		} else {
                    ESP_LOGE(TAG, "tcp server Recv task create succeed!");
                }
        	}
        }
	}

tcp_fail:
	vTaskDelete(NULL);
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
