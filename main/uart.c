#include "uart.h"

EventGroupHandle_t uart_event_group;

int uart_cmd_send(int port, const char *buff, int len)
{
	uart_write_bytes(port, (const char*) buff, len);
	return 0;
}

struct cmd_ops uart_cmd_ops = {
		.cmd_send = uart_cmd_send,
		.port = UART_NUM_0,
		.pdata = "uart",
		.len = 0,
};

void uart_task(void *pvParameters)
{
	struct sys_data *wifi_bt_data = pvParameters;
	int uart_band = 0;
	esp_err_t err;
	ESP_LOGI(UART_TAG, "Create uart send task...\n\r");

	const int uart_num = UART_NUM_0;
	//xEventGroupWaitBits(uart_event_group, UART_CONNECTED_BIT, false, true, portMAX_DELAY);
	ESP_LOGI(UART_TAG, "Start uart send task...\n\r");

	uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		//.rx_flow_ctrl_thresh = 122,
	};

	err = nvs_read_i32(UART_STORAGE_KEY, &uart_band);
	if(err != ESP_OK)
		ESP_LOGI(UART_TAG, "uart buand rate read error\n\r");
	if(uart_band)
		uart_config.baud_rate = uart_band;

	uart_param_config(uart_num, &uart_config);
	uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
	//Install UART driver (we don't need an event queue here)
	//In this example we don't even use a buffer for sending data.
	uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);

	uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
	while(1) {
		//Read data from UART
		int len = uart_read_bytes(uart_num, data, BUF_SIZE, 20 / portTICK_RATE_MS);

		if (cmd_data.current_state == CMD_NORMAL) {
			cmd_entry(&uart_cmd_ops, (const char *)data);
			/*
			//Write data to UDP
			if(wifi_bt_data->wifi_udp_enabled)
			send_Buff_with_UDP((const char*) data, len);
			*/
			if(wifi_bt_data->wifi_tcp_enabled)
				send_buff_with_tcp((const char*) data, len);
			if(len && connector_data)
				esp_spp_write(connector_data->srv_open.handle, len, data);
			//uart_write_bytes(uart_num, (const char*) data, len);
		} else if (cmd_data.current_state == CMD_CMD) {
			cmd_cli(&uart_cmd_ops, (const char *)data, len);
		}

		//vTaskDelay(300 / portTICK_RATE_MS);
	}
}
