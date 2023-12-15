#include "wifi.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "WiFi";

void setup_nvs() 
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
}

int scan_wifi(wifi_network result[], uint16_t max_networks)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint16_t size = max_networks;
    wifi_ap_record_t ap_info[size];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(*ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_scan_start(NULL, true);
    ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", size);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&size, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&size));
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, size);
    for (int i = 0; i < size; i++) {
        result[i].rssi = ap_info[i].rssi;
        memcpy(result[i].ssid, ap_info[i].ssid, sizeof ap_info[i].ssid);
    }

    return size;
}
