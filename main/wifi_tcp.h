#ifndef __TCP_PERF_H__
#define __TCP_PERF_H__

#ifdef __cplusplus
extern "C" {
#endif

/*test options*/
#define EXAMPLE_ESP_TCP_MODE_SERVER CONFIG_TCP_PERF_SERVER 
#define EXAMPLE_ESP_TCP_PERF_TX CONFIG_TCP_PERF_TX
#define EXAMPLE_ESP_TCP_DELAY_INFO CONFIG_TCP_PERF_DELAY_DEBUG

#ifdef CONFIG_TCP_PERF_SERVER_IP
#define EXAMPLE_DEFAULT_SERVER_IP CONFIG_TCP_PERF_SERVER_IP
#else
#define EXAMPLE_DEFAULT_SERVER_IP "192.168.4.1"
#endif 
#define EXAMPLE_PACK_BYTE_IS 97 //'a'

#define TCP_SERVER_CLIENT_OPTION true  //true will start softap and create tcp server

#define TCP_PORT 2000

/* FreeRTOS event group to signal when we are connected to wifi*/
extern EventGroupHandle_t tcp_event_group;
#define WIFI_CONNECTED_BIT BIT0

extern int  g_total_data;
extern bool g_rxtx_need_restart;

#if EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO
extern int g_total_pack;
extern int g_send_success;
extern int g_send_fail;
extern int g_delay_classify[5];
#endif/*EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO*/

//create a tcp server socket. return ESP_OK:success ESP_FAIL:error
esp_err_t create_tcp_server(bool isCreatServer);
//create a tcp client socket. return ESP_OK:success ESP_FAIL:error
esp_err_t create_tcp_client();

//send data task
void send_data(void *pvParameters);
//receive data task
void recv_data(void *pvParameters);

//close all socket
void close_tcp_socket();

//show socket error code. return: error code
int show_socket_error_reason(const char* str, int socket);

//check working socket
int check_working_socket();

int send_buff_with_tcp(char *databuff, int length);

void tcp_conn(void *pvParameters);


#ifdef __cplusplus
}
#endif


#endif /*#ifndef __TCP_PERF_H__*/

