#include <zephyr/kernel.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>

#include "uart.h"
#include "servo.h"
#include "stepper.h"
#include "rtc.h"
#include "usbd_configurator.h"
#include "power_management.h"

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
    
    // Testing setting motors pos
    ret = set_servo_deg_pos(10);
    ret = stepper_move_by_deg(10);

    // Checking wakeup cause
    log_wakeup_cause();
    
    // Testing setting an alarm for now + 5min
    struct rtc_time cur_time;
    get_rtc_date_time(&cur_time);
    struct rtc_time alarm_time = {
        .tm_min = (cur_time.tm_min + 5) % 60, // wrap minute to avoid going over 59min
    };

    alarm_ctx_t alarm_ctx = {
        .alarm_id = 1,
        .alarm_mask = RTC_ALARM_TIME_MASK_MINUTE,
        .alarm_time = alarm_time,
        .callback = NULL,
        .user_data = NULL,
    };

    enable_wakeup_src();
    set_rtc_alarm(alarm_ctx);
    go_to_sleep();

    return 0;
}