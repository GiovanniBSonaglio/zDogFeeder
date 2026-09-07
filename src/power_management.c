#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/logging/log.h>

#include "power_management.h"

LOG_MODULE_REGISTER(power_management, LOG_LEVEL_INF);

#define WAKE_GPIOS DT_ALIAS(wakeup_pins)
#define BUILD_GPIO_SPEC(node_id, prop, idx) GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx),

#if !DT_NODE_HAS_STATUS_OKAY(WAKE_GPIOS)
#error "Unsupported: wakeup-pins node/alias not defined"
#endif

/**
 * ds3231 only has one wakeup pin, but there are RTCs that have multiple
 */
static const struct gpio_dt_spec wake_gpios[] = {
    DT_FOREACH_PROP_ELEM(WAKE_GPIOS, gpios, BUILD_GPIO_SPEC)
};

static int enable_wakeup_gpio(int gpio) {
    int ret = 0;

    if(!esp_sleep_is_valid_wakeup_gpio(gpio)) {
        LOG_ERR("Configured wakeup GPIO (pin %d) is not valid wakeup source.", gpio);
        return -EINVAL;
    }

    ret = esp_sleep_enable_ext0_wakeup(gpio, 0);
    if (ret != ESP_OK) {
        LOG_ERR("Error enabling wakeup GPIO (pin %d) as a wakeup source.", gpio);
        return ret;
    }

    LOG_INF("GPIO %d successfully set as wakeup source.", gpio);

    return ret;
}

int enable_wakeup_src() {
    return enable_wakeup_gpio(wake_gpios[0].pin);
}

void go_to_sleep() {
    LOG_INF("Entering sleep mode...");
	sys_poweroff();
}

esp_sleep_wakeup_cause_t get_wakeup_cause(void) {
	uint32_t causes = esp_sleep_get_wakeup_causes();

	if (causes == 0) {
		return ESP_SLEEP_WAKEUP_UNDEFINED;
	}
	return (esp_sleep_wakeup_cause_t)__builtin_ctz(causes);
}

void log_wakeup_cause() {
    esp_sleep_wakeup_cause_t sleep_wakeup_cause = get_wakeup_cause();

    switch (sleep_wakeup_cause) {
        case ESP_SLEEP_WAKEUP_EXT0: {
                LOG_INF("Wake up from GPIO");
            }
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            LOG_INF("Wake up from timer.");
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            LOG_INF("Not a deep sleep reset");
	}
}