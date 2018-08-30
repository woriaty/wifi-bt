#ifndef _CMD_H_
#define _CMD_H_

#include "wifi_bt_storage.h"
#include "wifi_udp.h"
#include "esp_system.h"
#include "list.h"

#define TAG "$>"
#define ARRAY_SIZE(array)	sizeof(array)/sizeof(array[0])
#define TO_STR(a)			#a

struct cmd_ops {
	int (*cmd_send)(int port, const char *buff, int len);
	char data[1024];
	int len;		/* data length */
	int port;
	char *pdata;	/* private data */
};

struct cmd_node {
	int current_state;
	char *data;
};

typedef struct {
	char *cmd;
	char *data;
	int data_type;
	char *id;
	void (*pfn)(struct list_head *head, const char *ops_data);
	struct list_head list;
} CMD_TYPE;

struct cmd_table {
	char *cmd;
	int data_type;
	char *id;
	void (*pfn)(struct list_head *head, const char *ops_data);
};

enum cmd_state { CMD_NORMAL, CMD_CMD };
enum cmd_data_type {DATA_CHAR, DATA_INT, DATA_NONE};

extern char *prompt;
extern struct get_user_data user_data;
extern struct cmd_node cmd_data;

void cmd_entry(struct cmd_ops *ops, const char *str);
int cmd_cli(struct cmd_ops *ops, const char *buff, int len);
int cmd_init(void);


#endif
