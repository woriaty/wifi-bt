#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "wifi_tcp.h"
#include "wifi_udp.h"
#include "bt_server.h"
#include "uart.h"
#include "cmd.h"

void udp_conn(void *pvParameters)
{
	struct sys_data *wifi_bt_data = pvParameters;
	ESP_LOGI(TAG, "task udp_conn start... \n\r");

	xEventGroupWaitBits(udp_event_group, WIFI_CONNECTED_BIT, false, true,
			portMAX_DELAY);
	wifi_bt_data->wifi_udp_enabled = ENABLE;
	xEventGroupSetBits(uart_event_group, UART_CONNECTED_BIT);

	ESP_LOGI(TAG,"esp32 is ready !!!\n\r");

//�����ͻ��˲��Ҽ���Ƿ񴴽��ɹ�
	ESP_LOGI(TAG, "Now Let us create udp server ... \n\r");
	if (create_udp_server() == ESP_FAIL) {
		ESP_LOGI(TAG, " server create socket error , stop !!! \n\r");
		vTaskDelete(NULL);
	} else {
		ESP_LOGI(TAG, "server create socket Succeed  !!! \n\r");
	}

	//����һ�����ͺͽ������ݵ�����
	TaskHandle_t tx_rx_task;
	xTaskCreate(&send_recv_data, "send_recv_data", 4096, NULL, 4, &tx_rx_task);
	//�ȴ� UDP���ӳɹ���־λ
	xEventGroupWaitBits(udp_event_group, UDP_CONNCETED_SUCCESS, false, true,
			portMAX_DELAY);

	int bps;

	while (1) {

		total_data = 0;

		vTaskDelay(3000 / portTICK_RATE_MS);
		//ʱ�����뷢��һ������
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

void app_main(void)
{
	esp_err_t ret;
	struct sys_data wifi_bt_data = {
			.wifi_tcp_enabled = DISABLE,
			.wifi_udp_enabled = DISABLE,
			.bt_enabled = DISABLE,
	};

	ESP_LOGI(TAG, "%s: %d\n", __func__, __LINE__);

	ret = nvs_flash_init();

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
	        ESP_ERROR_CHECK(nvs_flash_erase());
	        ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );
	uart_event_group = xEventGroupCreate();
	wifi_init_softap();
	ESP_LOGI(TAG, "wifi bt Server Demo ...\n\r");

	xTaskCreate(&udp_conn, "udp_conn", 4096, &wifi_bt_data, 5, NULL);
	xTaskCreate(&tcp_conn, "tcp_conn", 4096, &wifi_bt_data, 5, NULL);

	bt_server_init();
	xTaskCreate(&uart_task, "uart_send_task", 2048, &wifi_bt_data, 10, NULL);
}
