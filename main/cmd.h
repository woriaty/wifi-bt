#ifndef _CMD_H_
#define _CMD_H_

#include "wifi_bt_storage.h"
#include "wifi_udp.h"
#include "esp_system.h"

#define TAG "$>"

extern char *prompt;

#define ARRAY_SIZE(array)	sizeof(array)/sizeof(array[0])
#define TO_STR(a)			#a
enum cmd_state { CMD_NORMAL, CMD_CMD };

struct get_user_data {
	int uart_buand;
	int wifi_join;
	int wifi_chan;
	int wifi_dhcp;
	char *wifi_ssid;
	char *wifi_pwd;
	char *wifi_address;
	char *wifi_gateway;
	char *wifi_netmask;
};

struct cmd_ops {
	int (*cmd_send)(int port, const char *buff, int len);
	char data[1024];
	int len;		/* data length */
	int port;
	char *pdata;	/* private data */
};

#define container_of(ptr, type, member)		(type *)(ptr-(unsigned long)&((type *)0)->member)

enum data_type {UART, WIFI_JOIN, WIFI_CH, WIFI_DHCP, WIFI_SSID,\
				WIFI_ADDR, WIFI_GW, WIFI_NM, SAVE, EXIT, REBOOT};

extern int cmd_state;
extern struct get_user_data user_data;

int enter_cmd_state(const char *str, int len);
//int cmd_set_user_data(const char *str, struct get_user_data *user_data);
int cmd_cli(struct cmd_ops *ops, const char *buff, int len);
int cmd_init(void);


#endif
