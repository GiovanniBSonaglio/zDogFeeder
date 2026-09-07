#include <zephyr/kernel.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>

#include "uart.h"
#include "servo.h"
#include "stepper.h"
#include "rtc.h"
#include "usbd_configurator.h"

LOG_MODULE_REGISTER(zDogFeederApp, CONFIG_LOG_DEFAULT_LEVEL);

static const struct device *const uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

static struct usbd_context usbd_ctx;

int main(void)
{
    int ret;

    if(init_uart(uart_dev)) {
        LOG_ERR("Error initializing UART");
        return -1;
    }
    
    ret = init_cdc_acm(&usbd_ctx);
	if (ret != 0) {
        LOG_ERR("Error initializing USB CDC ACM device support");
		return ret;
	}

    ret = init_stepper();
	if (ret != 0) {
        LOG_ERR("Error initializing stepper motor");
		return ret;
	}

    ret = init_rtc();
    if (ret != 0) {
        LOG_ERR("Error initializing RTC");
		return ret;
	}
    
    LOG_INF("ESP32 Successfully booted");
    
    ret = set_servo_deg_pos(10);
    ret = stepper_move_by_deg(10);

    return 0;
}