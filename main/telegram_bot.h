#pragma once

// Send a simple message via the configured Telegram bot
void telegram_send_message(const char *message);

// Start the Telegram polling task (non-blocking)
void telegram_start(void);

// Stop the Telegram polling task
void telegram_stop(void);
