# Plant Moisture Monitor - Simplified Version

A simple ESP32 project that monitors soil moisture and sends alerts via Telegram when your plant needs water.

## What This Does

- Connects to your Wi-Fi network
- Reads moisture sensor data every minute
- Sends you a Telegram message when the plant is thirsty
- Automatically resets the alert when you water the plant

## Hardware Required

- ESP32 development board
- Capacitive soil moisture sensor
- Connect sensor to GPIO 34

## Setup

1. **Configure Wi-Fi**: Open `main/plant-moisture-monitor.c` and edit:

   ```c
   #define WIFI_SSID       "Your_WiFi_Name"
   #define WIFI_PASSWORD   "Your_WiFi_Password"
   ```

2. **Configure Telegram Bot**:

   - Create a bot using [@BotFather](https://t.me/botfather) on Telegram
   - Get your chat ID from [@userinfobot](https://t.me/userinfobot)
   - Edit these lines:

   ```c
   #define BOT_TOKEN       "your_bot_token_here"
   #define CHAT_ID         "your_chat_id_here"
   ```

3. **Adjust Sensor Settings** (optional):
   ```c
   #define DRY_THRESHOLD         2000  // Higher = drier soil
   #define CHECK_INTERVAL_MINUTES 1    // How often to check
   ```

## How to Build and Flash

```bash
# Build the project
idf.py build

# Flash to ESP32
idf.py flash

# View logs
idf.py monitor
```

## Code Structure

Everything is in one file: `main/plant-moisture-monitor.c`

The code is organized into clear sections:

1. **Configuration** - All your settings in one place
2. **Wi-Fi Connection** - Connects to your network
3. **Telegram Alert** - Sends messages to you
4. **Moisture Monitoring** - Reads sensor and triggers alerts
5. **Main Function** - Starts everything up

## How It Works

1. ESP32 boots up and connects to Wi-Fi
2. Starts reading the moisture sensor every minute
3. When moisture level goes above threshold (dry soil):
   - Sends you a Telegram alert
   - Won't send another alert until you water the plant
4. When you water the plant and moisture drops below threshold:
   - Resets the alert flag
   - Ready to send a new alert if it dries out again

## Troubleshooting

- **No Wi-Fi connection**: Check SSID and password
- **No Telegram alerts**: Verify bot token and chat ID
- **Wrong sensor readings**: Adjust `DRY_THRESHOLD` value

## Notes

- Sensor values range from 0-4095 (ESP32 12-bit ADC)
- Higher values = drier soil
- Lower values = wetter soil
- Test your sensor to find the right threshold value
