# SolaX Cloud Data Display

Arduino project for NodeMCU (ESP8266) to display real-time SolaX inverter data on a 16x4 I2C LCD screen. Data is fetched from the SolaX Cloud API and displayed on alternating screens.

## Features

- 📊 Real-time solar inverter data from SolaX Cloud API
- 🔄 Alternating display screens every 5 seconds
- 📡 Automatic WiFi reconnection
- 🔒 HTTPS secure connection
- 🗺️ Human-readable status code mapping
- ⚡ Displays AC power, daily/total yield, and PV input power

## Display Screens

### Screen 1 - Power Summary
```
AC:12345 W      
Today:12.3 kWh  
Total:2420 kWh  
Sts:Normal      
```

### Screen 2 - PV Details
```
PV1:6500 W     
PV2:5800 W     
                
Sts:Normal      
```

## Hardware Requirements

### Components
- **NodeMCU ESP8266** (v1.0 or similar)
- **16x4 I2C LCD Display** with I2C backpack module
- **Jumper wires** (4 wires minimum)
- **USB cable** for programming and power

### Wiring Diagram

```
NodeMCU ESP8266          16x4 I2C LCD
================         ============
3V3 (3.3V)      -------> VCC
GND             -------> GND
D2 (GPIO4)      -------> SDA
D1 (GPIO5)      -------> SCL
```

**Pin Mapping:**
| NodeMCU Pin | GPIO | I2C Function | LCD Pin |
|-------------|------|--------------|---------|
| D1          | GPIO5| SCL          | SCL     |
| D2          | GPIO4| SDA          | SDA     |
| 3V3         | -    | Power        | VCC     |
| GND         | -    | Ground       | GND     |

**Note:** Some NodeMCU boards can power the LCD from 3.3V, but if your LCD requires 5V, connect VCC to the VIN pin (which provides 5V from USB). The I2C communication will still work at 3.3V logic levels.

## Software Requirements

### Arduino IDE Setup

1. **Install Arduino IDE** (version 1.8.x or 2.x)
   - Download from: https://www.arduino.cc/en/software

2. **Add ESP8266 Board Support**
   - Open Arduino IDE
   - Go to: File → Preferences
   - Add to "Additional Boards Manager URLs":
     ```
     http://arduino.esp8266.com/stable/package_esp8266com_index.json
     ```
   - Go to: Tools → Board → Boards Manager
   - Search for "esp8266"
   - Install "ESP8266 by ESP8266 Community"

3. **Select Board**
   - Go to: Tools → Board → ESP8266 Boards
   - Select "NodeMCU 1.0 (ESP-12E Module)"

### Required Libraries

Install these libraries via Arduino IDE Library Manager (Sketch → Include Library → Manage Libraries):

1. **ESP8266WiFi** - Built-in with ESP8266 board package
2. **ESP8266HTTPClient** - Built-in with ESP8266 board package
3. **WiFiClientSecure** - Built-in with ESP8266 board package
4. **ArduinoJson** by Benoit Blanchon (version 6.x or higher)
   - Search: "ArduinoJson"
   - Install version 6.21.0 or newer
5. **LiquidCrystal_I2C** by Frank de Brabander
   - Search: "LiquidCrystal I2C"
   - Install "LiquidCrystal I2C" by Frank de Brabander

## Installation & Configuration

### Step 1: Clone or Download Project

Download all project files:
- `Arduino-SolaxRead.ino`
- `config.h`
- `statusMapper.h`

### Step 2: Configure WiFi and API Credentials

Open `config.h` and update the following:

```cpp
// WiFi Configuration
#define WIFI_SSID "YOUR_WIFI_SSID"        // Your WiFi network name
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD" // Your WiFi password

// SolaX Cloud API Configuration
#define SOLAX_TOKEN "202406120951433408735310"  // Already configured
#define SOLAX_WIFI_SN "SR8BVQYEHR"              // Already configured
```

**To get your SolaX API credentials:**
1. Log in to https://www.solaxcloud.com
2. Navigate to: Service → API
3. Copy your Token ID and WiFi Serial Number

### Step 3: Adjust LCD I2C Address (if needed)

Most I2C LCD modules use address `0x27`, but some use `0x3F`. If your display doesn't work:

1. Run an I2C scanner sketch to find your LCD address
2. Update `config.h`:
   ```cpp
   #define LCD_ADDRESS 0x3F  // Change to your LCD's address
   ```

### Step 4: Upload to NodeMCU

1. Connect NodeMCU to computer via USB
2. Select correct port: Tools → Port → (select your NodeMCU port)
3. Click Upload button (→) in Arduino IDE
4. Wait for "Done uploading" message

### Step 5: Monitor Serial Output (Optional)

- Open Serial Monitor: Tools → Serial Monitor
- Set baud rate to: 115200
- View connection status and debug information

## Troubleshooting

### LCD Shows Nothing
- Check I2C connections (SDA/SCL)
- Verify LCD I2C address (try 0x27 or 0x3F)
- Adjust LCD contrast potentiometer on I2C backpack
- Ensure LCD backlight is on (try adjusting the jumper on I2C module)

### WiFi Connection Failed
- Verify SSID and password in `config.h`
- Check if WiFi network is 2.4GHz (ESP8266 doesn't support 5GHz)
- Move NodeMCU closer to WiFi router

### API Error / No Data
- Verify Token ID and WiFi Serial Number in `config.h`
- Check Serial Monitor for detailed error messages
- Ensure your SolaX inverter is registered and online at solaxcloud.com
- Check internet connectivity

### Display Shows Garbage Characters
- Check baud rate in Serial Monitor (should be 115200)
- Verify I2C connections are secure
- Try different I2C address in config.h

### SSL/HTTPS Errors
- The code uses `client.setInsecure()` to bypass SSL certificate verification
- This is acceptable for API calls but not recommended for sensitive data
- If you experience SSL errors, ensure ESP8266 board package is up to date

## Status Code Mapping

The display shows human-readable status text. Full mapping:

| Code | Status Text   | Description                    |
|------|---------------|--------------------------------|
| 100  | Waiting       | Waiting for operation          |
| 101  | Self-test     | Self-test mode                 |
| 102  | Normal        | Normal operation               |
| 103  | Fault         | Recoverable fault              |
| 104  | Perm Fault    | Permanent fault                |
| 105  | Upgrade       | Firmware upgrade               |
| 106  | EPS Detect    | EPS detection                  |
| 107  | Off-grid      | Off-grid mode                  |
| 108  | Self-test IT  | Self-test (Italian regs)       |
| 109  | Sleep         | Sleep mode                     |
| 110  | Standby       | Standby mode                   |
| 111  | PV Wake Bat   | PV wake-up battery mode        |
| 112  | Gen Detect    | Generator detection            |
| 113  | Generator     | Generator mode                 |
| 114  | Fast Shtdwn   | Fast shutdown standby          |
| 130  | VPP           | VPP mode                       |
| 131  | TOU-Self      | TOU Self-use mode              |
| 132  | TOU-Charge    | TOU Charging mode              |
| 133  | TOU-Dschrg    | TOU Discharging mode           |

## Configuration Options

In `config.h`, you can customize:

```cpp
#define UPDATE_INTERVAL 60000          // API update (milliseconds) - default 60 seconds
#define SCREEN_SWITCH_INTERVAL 5000    // Screen switch (milliseconds) - default 5 seconds
```

## API Reference

This project uses the SolaX Cloud User API v2.0. 

**Endpoint:** `POST https://global.solaxcloud.com/api/v2/dataAccess/realtimeInfo/get`

**Headers:**
- `Content-Type: application/json`
- `tokenid: YOUR_TOKEN_ID`

**Request Body:**
```json
{
  "wifiSn": "YOUR_WIFI_SERIAL_NUMBER"
}
```

For full API documentation, refer to the SolaX Cloud User API documentation.

## License

This project is provided as-is for educational and personal use.

## Credits

- SolaX Power for the Cloud API
- ESP8266 Community for Arduino board support
- Arduino community for libraries

## Support

For issues or questions:
1. Check the Troubleshooting section above
2. Verify all wiring and configuration
3. Check Serial Monitor output for detailed error messages
4. Ensure your SolaX system is online at solaxcloud.com

---

**Last Updated:** December 2025

