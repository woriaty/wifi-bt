#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "wifi_udp.h"
#include "wifi_tcp.h"

/* FreeRTOS event group to signal when we are connected to WiFi and ready to start UDP test*/
EventGroupHandle_t udp_event_group;
EventGroupHandle_t tcp_event_group;

int udp_socket;

static struct sockaddr_in remote_addr;
static unsigned int socklen;

int total_data = 0;
int success_pack = 0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		esp_wifi_connect();
		xEventGroupClearBits(udp_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_CONNECTED:
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI(TAG, "event_handler:SYSTEM_EVENT_STA_GOT_IP!");
		ESP_LOGI(TAG, "got ip:%s\n",
				ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		xEventGroupSetBits(udp_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_AP_STACONNECTED:
		ESP_LOGI(TAG, "station:"MACSTR" join,AID=%d\n",
				MAC2STR(event->event_info.sta_connected.mac),
				event->event_info.sta_connected.aid);
		xEventGroupSetBits(udp_event_group, WIFI_CONNECTED_BIT);
		xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_AP_STADISCONNECTED:
		ESP_LOGI(TAG, "station:"MACSTR"leave,AID=%d\n",
				MAC2STR(event->event_info.sta_disconnected.mac),
				event->event_info.sta_disconnected.aid);
		xEventGroupSetBits(udp_event_group, UDP_CONNCETED_SUCCESS);
		xEventGroupClearBits(udp_event_group, WIFI_CONNECTED_BIT);
		g_rxtx_need_restart = true;
		xEventGroupClearBits(tcp_event_group, WIFI_CONNECTED_BIT);
		break;
	default:
		break;
	}
	return ESP_OK;
}

void wifi_init_softap(void)
{
	char *wifi_ssid, *wifi_pwd;
	udp_event_group = xEventGroupCreate();
	tcp_event_group = xEventGroupCreate();

	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	wifi_config_t wifi_config = {
		.ap = {
			.ssid = AP_SSID,
			.password = AP_PAW,
			.ssid_len = 0,
			.max_connection = EXAMPLE_MAX_STA_CONN,
			.authmode = WIFI_AUTH_WPA_WPA2_PSK
		},
	};
	if (strlen(AP_PAW) == 0) {
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	wifi_ssid = nvs_read_str(WIFI_SSID_ST_KEY);
	if(wifi_ssid) {
		ESP_LOGI(TAG, "got saved wifi ssid : %s\n", wifi_ssid);
		/* clear the old ssid, in case we go a shorter one */
		memset(wifi_config.ap.ssid, 0x00, strlen(&wifi_config.ap.ssid));
		memcpy(wifi_config.ap.ssid, wifi_ssid, strlen(wifi_ssid));
	}
	ESP_LOGI(TAG, "use wifi ssid : %s\n", wifi_config.ap.ssid);

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, " Wifi_init_softap finished the SSID: %s password:%s \n",
			AP_SSID, AP_PAW);
}

//create a udp server socket. return ESP_OK:success ESP_FAIL:error
esp_err_t create_udp_server()
{
	ESP_LOGI(TAG, "Create Udp Server succeed port : %d \n", SERVICE_PORT);

	udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if (udp_socket < 0) {
		show_socket_error_reason("create_server", udp_socket);
		return ESP_FAIL;
	}
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVICE_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(udp_socket, (struct sockaddr *) &server_addr, sizeof(server_addr))
			< 0) {
		show_socket_error_reason("bind_server", udp_socket);
		close(udp_socket);
		return ESP_FAIL;
	}
	return ESP_OK;
}

//create a udp client socket. return ESP_OK:success ESP_FAIL:error
esp_err_t create_udp_client()
{
	ESP_LOGI(TAG, "create_udp_client()");
	ESP_LOGI(TAG, "connecting to %s:%d",
	SERVER_IP, SERVICE_PORT);

	udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if (udp_socket < 0) {
		show_socket_error_reason("Create client", udp_socket);
		return ESP_FAIL;
	}
	/*for client remote_addr is also server_addr*/
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(SERVICE_PORT);
	remote_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

	return ESP_OK;
}

extern char *sys_tips;
void send_recv_data(void *pvParameters)
{
	ESP_LOGI(TAG, "task send_recv_data start! \n");

	int len;
	char databuff[1024];
	struct get_user_data *user_data = malloc(sizeof(struct get_user_data));

	socklen = sizeof(remote_addr);
	memset(databuff, EXAMPLE_PACK_BYTE_IS, sizeof(databuff));

	ESP_LOGI(TAG, "start Recieve!\n");

	while (1) {
		//每次接收都要清空接收数组
		memset(databuff, 0x00, sizeof(databuff));
		//开始接收
		len = recvfrom(udp_socket, databuff, sizeof(databuff), 0,
				(struct sockaddr *) &remote_addr, &socklen);
		//打印接收到的数组
		ESP_LOGI(TAG, "recvData: %s\n", databuff);
		if(!strcmp(databuff, "hi")) {
			send_Buff_with_UDP(sys_tips, strlen(sys_tips));
		}
		if(enter_cmd_state(databuff, len)) {
			//xTaskCreate(cmd_process, "cmd process", 4096, NULL, 5, NULL);
			break;
		}

		if (len > 0) {
			total_data += len;
			success_pack++;
		} else {
			if (LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG) {
				show_socket_error_reason("send data", udp_socket);
			}
		}
	}
	vTaskDelete(NULL);
}

int send_Buff_with_UDP(char *databuff, int length)
{
	int result;
	result = sendto(udp_socket, databuff, length, 0,
			(struct sockaddr *) &remote_addr, sizeof(remote_addr));

	return result;
}

int get_socket_error_code(int socket) {
	int result;
	u32_t optlen = sizeof(int);
	if (getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen) == -1) {
		ESP_LOGE(TAG, "getsockopt failed");
		return -1;
	}
	return result;
}

int check_connected_socket() {
	int ret;
	ESP_LOGD(TAG, "check connect_socket");
	ret = get_socket_error_code(udp_socket);
	if (ret != 0) {
		ESP_LOGW(TAG, "socket error %d %s", ret, strerror(ret));
	}
	return ret;
}

void close_socket() {
	close(udp_socket);
}
