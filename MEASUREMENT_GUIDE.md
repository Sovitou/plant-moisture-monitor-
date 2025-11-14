# Plant Moisture Measurement Guide

## Overview

This guide explains how to interpret soil moisture sensor readings and calibrate your plant monitor system.

## How the System Works

### Detection Cycle

- **Interval**: Checks moisture every **1 minute** (configurable via `CHECK_INTERVAL_MINUTES`)
- **Reading**: Raw ADC value from 0-4095 (12-bit resolution)
- **Alert Logic**: Sends Telegram notification when soil is dry, resets when watered

### Current Configuration

- **ADC Channel**: GPIO 34 (ADC1_CHANNEL_6)
- **Threshold**: 2000 (raw ADC value)
- **Alert Behavior**: Single notification per dry period (prevents spam)

---

## Understanding Sensor Types

### 1. Resistive Sensors (Two Metal Probes)

**How they work:**

- Measures electrical resistance between two exposed metal pins
- Wet soil = low resistance = low ADC reading
- Dry soil = high resistance = high ADC reading

**Pros:**

- Cheap (~$1-2)
- Simple to use

**Cons:**

- Corrode quickly (weeks to months)
- DC current causes electrolysis
- Less accurate over time

**Recommendations:**

- Use pulsed/low-current measurements only
- Replace probes periodically
- Avoid continuous power

### 2. Capacitive Sensors (Recommended)

**How they work:**

- Measures dielectric constant changes (capacitance)
- No exposed metal in soil
- Analog voltage output

**Pros:**

- More durable (months to years)
- Less corrosion
- More stable readings

**Cons:**

- Slightly more expensive (~$3-5)
- May need voltage divider for 3.3V ADC

**Recommendations:**

- Preferred for long-term deployment
- Still calibrate per sensor unit

---

## Calibration Process

### Why Calibrate?

Each sensor, soil type, and installation varies. Raw ADC values are meaningless without calibration.

### Two-Point Calibration Method

#### Step 1: Measure Dry Point

1. Remove sensor from soil and leave in open air for 5+ minutes
2. Check current reading:
   ```bash
   curl http://YOUR_DEVICE_IP/status
   ```
3. Record the `last_value` as **DryADC** (e.g., 3500)

#### Step 2: Measure Wet Point

1. Place sensor in a cup of water (submerge sensing area only, not electronics)
2. Wait 1-2 minutes for reading to stabilize
3. Check current reading again:
   ```bash
   curl http://YOUR_DEVICE_IP/status
   ```
4. Record the `last_value` as **WetADC** (e.g., 1200)

#### Step 3: Calculate Moisture Percentage

Use this formula to convert raw ADC to moisture percentage:

```
moisture_percent = ((raw - DryADC) / (WetADC - DryADC)) * 100
```

**Note:** Lower ADC values = more moisture (inverse relationship for most sensors)

If using capacitive sensors that output higher voltage when wet, the formula is:

```
moisture_percent = ((raw - WetADC) / (DryADC - WetADC)) * 100
```

Clamp result to 0-100 range.

#### Step 4: Set Threshold

Example calculations (assuming DryADC=3500, WetADC=1200):

| ADC Value | Moisture % | Plant Status    |
| --------- | ---------- | --------------- |
| 3500      | 0%         | Extremely dry   |
| 3000      | ~22%       | **Needs water** |
| 2500      | ~43%       | Adequate        |
| 2000      | ~65%       | Good            |
| 1500      | ~87%       | Very moist      |
| 1200      | 100%       | Saturated       |

**Recommended thresholds:**

- Alert when moisture < 25-30% (ADC > 2800-3000)
- Most plants need watering at 20-40% moisture depending on species

---

## Setting the Right Threshold for Your Plant

### General Guidelines by Plant Type

#### Drought-Tolerant Plants (Succulents, Cacti)

- Water when: 10-20% moisture
- Threshold ADC: ~3200-3400 (for example calibration)

#### Average House Plants (Pothos, Monstera, Ferns)

- Water when: 25-35% moisture
- Threshold ADC: ~2700-3000

#### Moisture-Loving Plants (Ferns, Peace Lily)

- Water when: 40-50% moisture
- Threshold ADC: ~2200-2500

### Calibration Per Plant

1. Water plant normally
2. Wait until plant shows slight wilting or you know it needs water
3. Check ADC reading at that moment
4. Set threshold slightly higher than that reading (earlier warning)

---

## Understanding Your Readings

### Typical ADC Behavior (Resistive Sensors)

```
Air (dry):        3500-4095
Dry soil:         2500-3500
Moist soil:       1500-2500
Wet soil:         800-1500
Water:            500-1200
```

### What Affects Readings?

1. **Soil Type**
   - Sandy soil: drains fast, higher readings when "moist"
   - Clay soil: retains water, lower readings persist longer
2. **Sensor Depth**

   - Surface: dries faster, more variation
   - Deep: slower changes, more stable

3. **Temperature**

   - ADC readings can shift 5-10% with temperature
   - Negligible for most home use

4. **Sensor Degradation**
   - Resistive probes: readings drift over weeks
   - Capacitive: stable for months
   - Re-calibrate every 1-3 months

---

## Current System Threshold

The code uses:

```c
#define DRY_THRESHOLD  2000
```

**What this means (assuming typical resistive sensor):**

- Alert when ADC > 2000
- For example calibration (DryADC=3500, WetADC=1200):
  - 2000 ≈ 65% moisture
  - This is quite conservative (plant likely still moist)

**Recommendation:**
Adjust `DRY_THRESHOLD` based on your calibration:

- For 25% moisture alert: set to ~3000
- For 30% moisture alert: set to ~2800

---

## API Usage Examples

### Check Current Status

```bash
curl http://192.168.1.100/status
# Returns: {"running":true,"last_value":2345}
```

### Start/Stop Monitoring

```bash
curl -X POST http://192.168.1.100/start
curl -X POST http://192.168.1.100/stop
```

### Telegram Commands

Send these messages to your bot:

- `/status` - Get current moisture reading
- `/start` - Start monitoring
- `/stop` - Stop monitoring

---

## Advanced: Implementing Calibration in Code

To make calibration easier, you could add these endpoints:

### Suggested Endpoints (Future Enhancement)

```
POST /calibrate/dry   - Store current reading as dry point
POST /calibrate/wet   - Store current reading as wet point
GET  /calibrate       - Return calibration values and current %
```

### Storing Calibration (NVS)

Use ESP-IDF's NVS (Non-Volatile Storage) to persist:

```c
nvs_handle_t nvs_handle;
nvs_set_i32(nvs_handle, "dry_adc", dry_value);
nvs_set_i32(nvs_handle, "wet_adc", wet_value);
nvs_commit(nvs_handle);
```

---

## Troubleshooting

### Issue: Sensor always reads dry

- **Check wiring**: ensure ADC pin connected correctly
- **Check power**: sensor needs 3.3V or 5V depending on model
- **Verify ADC**: try reading in air vs water to see if values change

### Issue: Readings are noisy/jumping

- **Solution 1**: Average multiple samples
  ```c
  int sum = 0;
  for(int i=0; i<10; i++) {
      int val;
      adc_oneshot_read(adc1_handle, channel, &val);
      sum += val;
      vTaskDelay(pdMS_TO_TICKS(10));
  }
  int avg = sum / 10;
  ```
- **Solution 2**: Add capacitor (0.1µF) between ADC pin and GND

### Issue: Alerts keep sending even after watering

- **Possible cause**: Threshold too high for your sensor
- **Solution**: Re-calibrate or lower threshold value

### Issue: Sensor corroding quickly

- **Solution**: Switch to capacitive sensor or pulse power
- **Temporary fix**: Coat probes with nail polish (not sensing area)

---

## Safety Notes

1. **Do not leave resistive sensors powered continuously**

   - Causes electrolysis and soil pH changes
   - Current implementation is continuous (room for improvement)

2. **Keep electronics dry**

   - Only sensing area should touch soil
   - Use heat shrink or conformal coating on connections

3. **Voltage levels**
   - ESP32 ADC max: 3.3V (with proper attenuation)
   - Some sensors output 5V - use voltage divider

---

## Summary: Quick Start

1. ✅ **Implemented**: HTTP API with /status, /start, /stop
2. ✅ **Implemented**: Telegram bot with /status, /start, /stop commands
3. ✅ **Implemented**: 1-minute detection interval
4. 📝 **Next**: Calibrate your sensor (measure dry + wet ADC values)
5. 📝 **Next**: Update `DRY_THRESHOLD` in code based on calibration
6. 📝 **Optional**: Add calibration endpoints and percentage conversion
7. 📝 **Optional**: Implement moving average filter for stable readings

For questions or issues, check the ESP-IDF documentation or moisture sensor datasheet.
