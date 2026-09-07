#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include <esp_sleep.h>

int enable_wakeup_src(void);
void go_to_sleep(void);
esp_sleep_wakeup_cause_t get_wakeup_cause(void);
void log_wakeup_cause(void);

#endif // POWER_MANAGEMENT_H