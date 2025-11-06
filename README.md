# ESP32 Plant Moisture Monitor with Telegram Alerts

 <!-- Optional: Create a banner image -->

A simple, low-cost IoT project that monitors your plant's soil moisture and sends you an alert via Telegram when it's time to water it. Built using the ESP32 and the ESP-IDF framework.

This project is designed to be a "fire-and-forget" solution. Set it up once, and never forget to water your plants again!

## Table of Contents

- [ESP32 Plant Moisture Monitor with Telegram Alerts](#esp32-plant-moisture-monitor-with-telegram-alerts)
  - [Table of Contents](#table-of-contents)
  - [Features](#features)
  - [Hardware Requirements](#hardware-requirements)
  - [Wiring Diagram](#wiring-diagram)
  - [Software \& Setup](#software--setup)
    - [1. Get Telegram Credentials](#1-get-telegram-credentials)

## Features

- **Real-time Monitoring:** Continuously checks the soil moisture level.
- **Telegram Alerts:** Sends a notification directly to your phone when the plant needs watering.
- **Spam Protection:** Sends only one alert per "dry cycle" to avoid flooding your chat.
- **Low Cost & Simple:** Uses minimal, widely available, and affordable components.
- **Robust Firmware:** Built with the official Espressif IoT Development Framework (ESP-IDF) for stability.
- **Easy to Configure:** Uses `menuconfig` for setting up Wi-Fi and other parameters without changing the code.

## Hardware Requirements

You only need a few common components to get started:

| Component                       | Quantity | Notes                                              |
| ------------------------------- | :------: | -------------------------------------------------- |
| ESP32 Development Board         |    1     | Any model like ESP32-DevKitC will work.            |
| Capacitive Soil Moisture Sensor |    1     | v1.2 or similar. Recommended over resistive types. |
| Jumper Wires (3x)               |    1     | For connecting the sensor to the ESP32.            |
| Micro-USB Cable                 |    1     | For power and programming.                         |

## Wiring Diagram

The wiring is extremely simple. Connect the three pins from the sensor to the ESP32 as follows:

| Sensor Pin  |  ESP32 Pin  | Purpose          |
| :---------: | :---------: | ---------------- |
|   **VCC**   |   **3V3**   | Power for Sensor |
|   **GND**   |   **GND**   | Ground           |
| **AOUT/AO** | **GPIO 34** | Analog Signal    |

 <!-- Optional: Create a simple wiring diagram image -->

_Note: GPIO 34 is an ADC1 pin and is a great choice for reading analog values._

## Software & Setup

This project is built using the **ESP-IDF v5.0** or newer. Please ensure you have it [installed and configured](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) for your system. The VSCode extension is highly recommended.

### 1. Get Telegram Credentials

Before you begin, you need to get a **Bot Token** and your **Chat ID** from Telegram.

1.  **Create a Bot:**

    - Open Telegram and search for a user named `BotFather`.
    - Start a chat and send the command `/newbot`.
    - Follow the instructions to give your bot a name and username.
    - **BotFather** will give you a **Bot Token**. Save this token!

2.  **Get Your Chat ID:**
    - Find your newly created bot in Telegram and send it a message (e.g., "hello").
    - Open your web browser and go to the following URL, replacing `<YOUR_BOT_TOKEN>` with your token:
      ```
      https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates
      ```
    - Look for the `chat` section in the response. The `id` value is your **Chat ID**. Save this number!
