#include "uart.h"

EventGroupHandle_t uart_event_group;

void uart_task(void *pvParameters)
{
	struct sys_data *wifi_bt_data = pvParameters;
	ESP_LOGI(UART_TAG, "Create uart send task...\n\r");

    const int uart_num = UART_NUM_0;
    xEventGroupWaitBits(uart_event_group, UART_CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(UART_TAG, "Start uart send task...\n\r");
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
        if(wifi_bt_data->wifi_udp_enabled)
        	send_Buff_with_UDP((const char*) data, len);
        if(wifi_bt_data->wifi_tcp_enabled)
        	send_buff_with_tcp((const char*) data, len);
        if(len && connector_data)
        	esp_spp_write(connector_data->srv_open.handle, len, data);
        //uart_write_bytes(uart_num, (const char*) data, len);
        vTaskDelay(300 / portTICK_RATE_MS);
    }
}
