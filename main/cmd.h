#ifndef _CMD_H_
#define _CMD_H_

extern char *sys_tips;

#define TAG "$>"

#define ARRAY_SIZE(array)	sizeof(array)/sizeof(array[0])
#define TO_STR(a)			#a
enum cmd_state { CMD_NORMAL, CMD_CMD };

struct get_user_data {
	int uart_buand;
	int wifi_join;
	int wifi_chan;
	int wifi_dhcp;
	char *wifi_ssid;
	char *wifi_address;
	char *wifi_gateway;
	char *wifi_netmask;
};

enum data_type {UART, WIFI_JOIN, WIFI_CH, WIFI_DHCP, WIFI_SSID,\
				WIFI_ADDR, WIFI_GW, WIFI_NM};

extern int cmd_state;

int enter_cmd_state(const char *str, int len);
int cmd_set_user_data(const char *str, struct get_user_data *user_data);
void cmd_process(void *pvParameters);


#endif
