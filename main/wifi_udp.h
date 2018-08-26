#ifndef __UDP_XUHONG_H__
#define __UDP_XUHONG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_wifi.h"
#include "uart.h"

extern int udp_socket;

//ȫ���������Ƿ�server����������station�ͻ��� ------> trueΪ��������falseΪ�ͻ���
#define Server_Station_Option true

#define EXAMPLE_ESP_WIFI_MODE_AP true //TRUE:AP FALSE:STA
#define EXAMPLE_ESP_UDP_MODE_SERVER true //TRUE:server FALSE:client
#define EXAMPLE_ESP_UDP_PERF_TX false //TRUE:send FALSE:receive
#define EXAMPLE_PACK_BYTE_IS 97 //'a'

/*
 * Ҫ���ӵ�·�������ֺ�����
 */

//·����������
#define GATEWAY_SSID "AliyunOnlyTest"
//���ӵ�·��������
#define GATEWAY_PASSWORD "aliyun#123456"

//���ݰ���С
#define EXAMPLE_DEFAULT_PKTSIZE 1024

/*
 * �Լ���ΪAP�ȵ�ʱ��������Ϣ����
 */
//ssid
#define AP_SSID "globalpilotwifi"
//����
#define AP_PAW ""
//���������
#define EXAMPLE_MAX_STA_CONN 4

/*
 * stationģʽʱ�򣬷�������ַ����
 */
//�������ĵ�ַ������� 255.255.255.255���ھ��������ͣ���ָ��ĳ���豸
#define SERVER_IP "255.255.255.255"
//�˿ں�
#define SERVICE_PORT 8265

/* FreeRTOS event group to signal when we are connected to WiFi and ready to start UDP test*/
extern EventGroupHandle_t udp_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define UDP_CONNCETED_SUCCESS BIT1

extern int total_data;
extern int success_pack;

//using esp as station
void wifi_init_sta();
//using esp as softap
void wifi_init_softap();

esp_err_t create_udp_server();
esp_err_t create_udp_client();

//send or recv data task
void send_recv_data(void *pvParameters);

//get socket error code. return: error code
int get_socket_error_code(int socket);

//check connected socket. return: error code
int check_connected_socket();

int send_Buff_with_UDP(char *databuff, int length);

//close all socket
void close_socket();

#ifdef __cplusplus
}
#endif

#endif

