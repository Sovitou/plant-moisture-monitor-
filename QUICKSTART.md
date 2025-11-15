# Quick Start Guide

## 🚀 3 Steps to Get Started

### Step 1: Edit Configuration

Open `main/plant-moisture-monitor.c` and change these lines (around line 30-40):

```c
// Your Wi-Fi name and password
#define WIFI_SSID       "Your_WiFi_Name"
#define WIFI_PASSWORD   "Your_WiFi_Password"

// Your Telegram bot details
#define BOT_TOKEN       "123456:ABC-DEF..."
#define CHAT_ID         "987654321"
```

### Step 2: Create Telegram Bot

1. Open Telegram and search for `@BotFather`
2. Send `/newbot` and follow instructions
3. Copy the bot token it gives you
4. Search for `@userinfobot` and send it `/start`
5. Copy your chat ID

### Step 3: Build and Flash

Open **ESP-IDF Terminal** and run:

```bash
idf.py build
idf.py flash
idf.py monitor
```

## 🌱 How to Use

1. Plug in your ESP32 with moisture sensor
2. Wait for "System ready!" message
3. Place sensor in your plant's soil
4. You'll receive alerts when plant needs water!

## ⚙️ Adjust Settings

In `plant-moisture-monitor.c`, you can change:

```c
#define DRY_THRESHOLD         2000  // Alert when value goes above this
#define CHECK_INTERVAL_MINUTES 1    // Check every X minutes
```

**Finding the right threshold:**

- Run `idf.py monitor` to see sensor readings
- Note the value when soil is dry
- Note the value when soil is wet
- Set threshold between these two values

## 📁 Project Structure (Super Simple!)

```
plant-moisture-monitor/
├── main/
│   ├── plant-moisture-monitor.c   ← ALL CODE IS HERE
│   └── CMakeLists.txt              ← Build config (don't touch)
├── README_SIMPLE.md                ← Full documentation
└── QUICKSTART.md                   ← This file
```

## ❓ Common Issues

**"Failed to connect to Wi-Fi"**
→ Check WIFI_SSID and WIFI_PASSWORD are correct

**"Failed to send alert"**
→ Check BOT_TOKEN and CHAT_ID are correct

**"Sensor always reads 0 or 4095"**
→ Check sensor is connected to GPIO 34

## 💡 Tips

- Test the sensor first by monitoring the readings
- Keep sensor clean and dry when not in use
- Adjust threshold based on your specific sensor
- Battery lasts longer with higher CHECK_INTERVAL_MINUTES
