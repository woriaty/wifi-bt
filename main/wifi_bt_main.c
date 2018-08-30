#include <stdio.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"

#include "wifi_tcp.h"
#include "wifi_udp.h"
#include "bt_server.h"
#include "uart.h"
#include "cmd.h"

#include "nvs_flash.h"

void udp_conn(void *pvParameters)
{
	struct sys_data *wifi_bt_data = pvParameters;
	ESP_LOGI(TAG, "task udp_conn start... \n\r");

	xEventGroupWaitBits(udp_event_group, WIFI_CONNECTED_BIT, false, true,
			portMAX_DELAY);
	wifi_bt_data->wifi_udp_enabled = ENABLE;
	xEventGroupSetBits(uart_event_group, UART_CONNECTED_BIT);

	ESP_LOGI(TAG,"esp32 is ready !!!\n\r");

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



void app_main(void)
{
	esp_err_t ret;

	struct sys_data *wifi_bt_data = malloc(sizeof(struct sys_data));
	wifi_bt_data->wifi_tcp_enabled = DISABLE;
	wifi_bt_data->wifi_udp_enabled = DISABLE;
	wifi_bt_data->bt_enabled = DISABLE;

	ESP_LOGI(TAG, "%s: %d\n", __func__, __LINE__);

	ret = nvs_flash_init();

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
	        ESP_ERROR_CHECK(nvs_flash_erase());
	        ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	cmd_init();

	ESP_LOGI(TAG, "%s: %d\n", __func__, __LINE__);

	uart_event_group = xEventGroupCreate();
	wifi_init_softap();
	ESP_LOGI(TAG, "wifi bt Server Demo ...\n\r");

	xTaskCreate(&udp_conn, "udp_conn", 4096, wifi_bt_data, 5, NULL);
	xTaskCreate(&tcp_conn, "tcp_conn", 4096, wifi_bt_data, 5, NULL);

	bt_server_init();
	xTaskCreate(&uart_task, "uart_send_task", 4096, wifi_bt_data, 10, NULL);
}
