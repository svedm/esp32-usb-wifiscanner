#include <stdint.h>

typedef struct {
    uint8_t ssid[33];
    int8_t rssi;   
} wifi_network;

void setup_nvs();
int scan_wifi(wifi_network result[], uint16_t max_networks);