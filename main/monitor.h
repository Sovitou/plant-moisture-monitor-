// monitor.h
#pragma once

#include <stdbool.h>

// Start the periodic moisture detection task (non-blocking)
void monitor_start(void);

// Stop the periodic moisture detection task
void monitor_stop(void);

// Return whether monitor is currently running
bool monitor_is_running(void);

// Get last measured sensor value (raw ADC)
int monitor_get_last_value(void);

// Set check interval in minutes
void monitor_set_interval_minutes(int minutes);
