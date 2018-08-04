#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "wifi_udp.h"
#include "nvs.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"

#include "wifi_tcp.h"

#define ECHO_TEST_TXD  (1)
#define ECHO_TEST_RXD  (3)
#define ECHO_TEST_RTS  (22)
#define ECHO_TEST_CTS  (19)

#define BUF_SIZE (1024)

static void uart_task(void *pvParameters)
{
	ESP_LOGI(TAG, "Create uart send task...\n\r");

    const int uart_num = UART_NUM_0;
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        //.rx_flow_ctrl_thresh = 122,
    };
    //Configure UART1 parameters
    uart_param_config(uart_num, &uart_config);
    //Set UART1 pins(TX: IO4, RX: I05, RTS: IO18, CTS: IO19)
    uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    //Install UART driver (we don't need an event queue here)
    //In this example we don't even use a buffer for sending data.
    uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);

    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
    while(1) {
        //Read data from UART
        int len = uart_read_bytes(uart_num, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        //Write data to UDP
        //send_Buff_with_UDP((const char*) data, len);
        send_buff_with_tcp((const char*) data, len);
        //uart_write_bytes(uart_num, (const char*) data, len);
        //vTaskDelay(300 / portTICK_RATE_MS);
    }
}

void udp_conn(void *pvParameters)
{
	ESP_LOGI(TAG, "task udp_conn start... \n\r");

	xEventGroupWaitBits(udp_event_group, WIFI_CONNECTED_BIT, false, true,
			portMAX_DELAY);

	ESP_LOGI(TAG,"esp32 is ready !!!\n\r");
	//vTaskDelay(5000 / portTICK_RATE_MS);

//创建客户端并且检查是否创建成功
	ESP_LOGI(TAG, "Now Let us create udp server ... \n\r");
	if (create_udp_server() == ESP_FAIL) {
		ESP_LOGI(TAG, " server create socket error , stop !!! \n\r");
		vTaskDelete(NULL);
	} else {
		ESP_LOGI(TAG, "server create socket Succeed  !!! \n\r");
	}

	//创建一个发送和接收数据的任务
	TaskHandle_t tx_rx_task;
	xTaskCreate(&send_recv_data, "send_recv_data", 4096, NULL, 4, &tx_rx_task);
	//等待 UDP连接成功标志位
	xEventGroupWaitBits(udp_event_group, UDP_CONNCETED_SUCCESS, false, true,
			portMAX_DELAY);

	int bps;

	while (1) {

		total_data = 0;

		vTaskDelay(3000 / portTICK_RATE_MS);
		//时隔三秒发送一次数据
		bps = total_data / 3;

		if (total_data <= 0) {
			int err_ret = check_connected_socket();
			if (err_ret == -1) {
				ESP_LOGW(TAG,
						"udp send & recv stop !!! will close socket ... \n\r");
				close_socket();
				break;
			}
		}

		ESP_LOGI(TAG, "udp recv %d byte per sec! total pack: %d \n\r", bps,
				success_pack);

	}

	vTaskDelete(tx_rx_task);
	vTaskDelete(NULL);
}

//this task establish a TCP connection and receive data from TCP
void tcp_conn(void *pvParameters)
{
	g_rxtx_need_restart = false;
    ESP_LOGI(TAG, "task tcp_conn...");

    /*wating for connecting to AP*/
    xEventGroupWaitBits(tcp_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    TaskHandle_t tx_rx_task = NULL;

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

        	if (ESP_FAIL != create_tcp_server(false)) {
        		if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task)) {
        			ESP_LOGE(TAG, "tcp server Recv task create fail!");
        		}
                    else {
                    	ESP_LOGE(TAG, "tcp server Recv task create succeed!");
                    }
        	}
        }
	}

tcp_fail:
	vTaskDelete(NULL);
}

void app_main(void) {

	esp_err_t ret = nvs_flash_init();

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		// NVS partition was truncated and needs to be erased
		const esp_partition_t* nvs_partition = esp_partition_find_first(
		      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
		assert(nvs_partition && "partition table must have an NVS partition");
		ESP_ERROR_CHECK( esp_partition_erase_range(nvs_partition, 0, nvs_partition->size) );
		// Retry nvs_flash_init
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	wifi_init_softap();
	ESP_LOGI(TAG, "UDP AP Server Demo ...\n\r");

	xTaskCreate(&udp_conn, "udp_conn", 4096, NULL, 5, NULL);
	//xTaskCreate(&tcp_conn, "tcp_conn", 5120, NULL, 5, NULL);
	xTaskCreate(&uart_task, "uart_send_task", 2048, NULL, 10, NULL);
	//vTaskStartScheduler();
}
