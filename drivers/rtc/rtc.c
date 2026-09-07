#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>

#include "rtc.h"

LOG_MODULE_REGISTER(rtc_drv, LOG_LEVEL_WRN);

static const struct device *rtc_dev = DEVICE_DT_GET(DT_NODELABEL(ds3231_rtc));

#define TM_YEAR_OFFSET 1900

static const char *const month_names[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char *const weekday_names[7] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

int init_rtc() {
    int ret = 0;

	if (!device_is_ready(rtc_dev)) {
		LOG_ERR("Device %s is not ready", rtc_dev->name);
		return -ENODEV;
	}

    LOG_DBG("RTC is %p, name is %s", rtc_dev, rtc_dev->name);

    return ret;
}

int set_rtc_date_time(struct rtc_time *tm) {
    int ret = 0;

    ret = rtc_set_time(rtc_dev, tm);
    if(ret < 0) {
        LOG_ERR("Error setting RTC time (errno=%d)", ret);
        return ret;
    }

    return ret;
}

int get_rtc_date_time(struct rtc_time *tm) {
    int ret = 0;

    ret = rtc_get_time(rtc_dev, tm);
    if(ret < 0) {
        LOG_ERR("Error getting RTC time (errno=%d)", ret);
        return ret;
    }

    return ret;
}

/* Fri Sep 04 2026 14:50:0 GMT+0000 */
void print_rtc_date_time(void) {
	int ret = 0;
	struct rtc_time tm;

	ret = rtc_get_time(rtc_dev, &tm);
	if(ret < 0) {
		return;
	}

	LOG_DBG("%s %s %2d %d %2d:%2d:%2d", weekday_names[tm.tm_wday], month_names[tm.tm_mon], tm.tm_mday, tm.tm_year + TM_YEAR_OFFSET, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

int set_rtc_alarm(alarm_ctx_t alarm_ctx) {
	int ret = 0;

	ret = rtc_alarm_set_time(rtc_dev, 0, alarm_ctx.alarm_mask, &(alarm_ctx.alarm_time));
	if (ret < 0) {
		LOG_ERR("Error setting RTC alarm %d time (errno=%d)", alarm_ctx.alarm_id, ret);
		return ret;
	}
	
	ret = rtc_alarm_set_callback(rtc_dev, 0, alarm_ctx.callback, alarm_ctx.user_data);
	if (ret < 0) {
		LOG_ERR("Error setting RTC alarm %d callback (errno=%d)", alarm_ctx.alarm_id, ret);
		return ret;
	}

	return ret;
}

