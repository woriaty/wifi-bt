#include "wifi_bt_storage.h"
#include "cmd.h"

esp_err_t nvs_read_i32(const char *key, int *val)
{
	nvs_handle my_handle;
	esp_err_t err;
	// Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    	return NULL;

    // Read
    err = nvs_get_i32(my_handle, key, val);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    	return NULL;

    // Close
    nvs_close(my_handle);
    return err;
}

char *nvs_read_str(const char *key)
{
	nvs_handle my_handle;
	esp_err_t err;
	size_t length = 0;
	char *get_data = NULL;
	// Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    	return NULL;

    // Read
    err = nvs_get_str(my_handle, key, NULL, &length);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    	return NULL;
    ESP_LOGI(TAG, "got %s length: %d\n", key, length);
    if(length) {
		get_data = malloc(length);
		err = nvs_get_str(my_handle, key, get_data, &length);
		if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
			return NULL;
		if(get_data)
			ESP_LOGI(TAG, "got %s data: %s\n", key, get_data);
    }
    // Close
    nvs_close(my_handle);
    return get_data;
}

esp_err_t nvs_write_i32(const char *key, const int val)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    	return err;

    // Write
    err = nvs_set_i32(my_handle, key, val);
    if (err != ESP_OK)
    	return err;

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    	return err;

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t nvs_write_str(const char *key, const char *str)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    	return err;

    // Write
    ESP_LOGI(TAG, "%s: write string: %s\n", key, str);
    err = nvs_set_str(my_handle, key, str);
    if (err != ESP_OK)
    	return err;

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    	return err;

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t nvs_erase_data(const char *key)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    	return err;

    err = nvs_erase_key(my_handle, key);
    if (err != ESP_OK)
    	return err;

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}
