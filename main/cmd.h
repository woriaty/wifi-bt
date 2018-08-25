#ifndef _CMD_H_
#define _CMD_H_

extern char *sys_tips;

extern char *prompt;

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

struct cmd_ops {
	int (*cmd_send)(int port, const char *buff, int len);
	char data[1024];
	int len;		/* data length */
	int port;
	char *pdata;	/* private data */
};

#define container_of(ptr, type, member)		(ptr-(unsigned long)&(type *)0->member)

enum data_type {UART, WIFI_JOIN, WIFI_CH, WIFI_DHCP, WIFI_SSID,\
				WIFI_ADDR, WIFI_GW, WIFI_NM};

extern int cmd_state;

int enter_cmd_state(const char *str, int len);
int cmd_set_user_data(const char *str, struct get_user_data *user_data);
void cmd_process(void *pvParameters);
int cmd_cli(struct cmd_ops *ops, const char *buff, int len);


#endif
