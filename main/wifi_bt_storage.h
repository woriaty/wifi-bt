#ifndef _WIFI_BT_STORAGE_H_
#define _WIFI_BT_STORAGE_H_

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"

#define STORAGE_NAMESPACE "storage"

#define WIFI_STORAGE_KEY	"wifi_storage"
#define UART_STORAGE_KEY	"uart_storage"
#define BT_STORAGE_KEY		"bt_storage"
#define WIFI_SSID_ST_KEY	"wifi_ssid_st"
#define WIFI_PWD_ST_KEY		"wifi_pwd_st"
#define WIFI_JOIN_ST_KEY	"wifi_join_st"
#define WIFI_CHAN_ST_KEY	"wifi_ssid_st"
#define WIFI_DHCP_ST_KEY	"wifi_dhcp_st"
#define WIFI_ADDR_ST_KEY	"wifi_addr_st"
#define WIFI_GATE_ST_KEY	"wifi_gate_st"
#define WIFI_MASK_ST_KEY	"wifi_mask_st"


char *nvs_read_str(const char *key);
esp_err_t nvs_read_i32(const char *key, int *val);

esp_err_t nvs_write_i32(const char *key, const int val);
esp_err_t nvs_write_str(const char *key, const char *str);

esp_err_t nvs_erase_data(const char *key);

#endif
