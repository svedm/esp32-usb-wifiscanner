#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_private/usb_phy.h"
#include "tusb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define USB_SYS "USB"

static usb_phy_handle_t phy_hdl;

int tinyusb_hw_init(void)
{
    usb_phy_config_t phy_conf = {
        .controller = USB_PHY_CTRL_OTG,
        .otg_mode = USB_OTG_MODE_DEVICE,
        .target = USB_PHY_TARGET_INT, /* only internal PHY supported */
    };

    if (usb_new_phy(&phy_conf, &phy_hdl) != ESP_OK)
    {
        ESP_LOGE(USB_SYS, "Install USB_SYS PHY failed");
        return -1;
    }

    return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
    ESP_LOGI(USB_SYS, "tud_mount_cb");
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    ESP_LOGI(USB_SYS, "tud_umount_cb");
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    ESP_LOGI(USB_SYS, "tud_suspend_cb");
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    ESP_LOGI(USB_SYS, "tud_resume_cb");
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
void cdc_task(void)
{
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    // if ( tud_cdc_connected() )
    {
        // connected and there are data available
        if (tud_cdc_available())
        {
            ESP_LOGI(USB_SYS, "CDC available");
            // read data
            char buf[64];
            uint32_t count = tud_cdc_read(buf, sizeof(buf));
            (void)count;

            ESP_LOGI(USB_SYS, "CDC data %s", buf);

            // Echo back
            // Note: Skip echo by commenting out write() and write_flush()
            // for throughput test e.g
            //    $ dd if=/dev/zero of=/dev/ttyACM0 count=10000

            tud_cdc_write(buf, count);
            tud_cdc_write("\r\n", 2);
            tud_cdc_write_flush();
        }
    }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    ESP_LOGI(USB_SYS, "tud_cdc_line_state_cb");
    (void)itf;
    (void)rts;

    // TODO set some indicator
    if (dtr)
    {
        ESP_LOGI(USB_SYS, "terminal connected");

        char hello_str[] = "Hello!\r\n\0";
        tud_cdc_write(hello_str, sizeof(hello_str));
        tud_cdc_write_flush();
    }
    else
    {
        ESP_LOGI(USB_SYS, "terminal disconnected");
    }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
    (void)itf;
    ESP_LOGI(USB_SYS, "tud_cdc_rx_cb");
}

//--------------------------------------------------------------------+
// USB VENDOR
//--------------------------------------------------------------------+

// Invoked when received new data
void tud_vendor_rx_cb(uint8_t itf)
{
    ESP_LOGI(USB_SYS, "tud_vendor_rx_cb");
}

// Invoked when last rx transfer finished
void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes)
{
    ESP_LOGI(USB_SYS, "tud_vendor_tx_cb");
}

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    ESP_LOGI(USB_SYS, "CONTROL XFER; Stage: %d Request type: %d", stage, request->bmRequestType_bit.type);
    // nothing to with DATA & ACK stage
    if (stage != CONTROL_STAGE_SETUP)
    {
        return true;
    }

    return true;
}

void vendor_task()
{
    if (tud_vendor_available())
    {
        uint8_t buf[64];
        uint32_t count = tud_vendor_read(buf, sizeof(buf));

        ESP_LOGI(USB_SYS, "VENDOR received: %s", buf);

        tud_vendor_write(buf, count);
        tud_vendor_write_flush();
    }
}

static void usb_task(void *pvParam)
{
    (void)pvParam;

    do
    {
        // TinyUSB device task
        tud_task();
        cdc_task();
        vendor_task();
    } while (true);
}

void app_main(void)
{
    tinyusb_hw_init();

    bool result = tud_init(BOARD_TUD_RHPORT);
    ESP_LOGI("MAIN", "tud inited %d", result);

    BaseType_t ret_val = xTaskCreatePinnedToCore(usb_task, "usb_task", 8 * 1024, NULL, 1, NULL, 0);
    ESP_LOGI("MAIN", "%d", (pdPASS == ret_val) ? 1 : 0);
}