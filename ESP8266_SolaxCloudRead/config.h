// WiFi Configuration
#define WIFI_SSID "Dimuthu Home"        // Replace with your WiFi SSID
#define WIFI_PASSWORD "Zone@123" // Replace with your WiFi password

// SolaX Cloud API Configuration
#define SOLAX_TOKEN "202406120951433408735310"
#define SOLAX_WIFI_SN "SR8BVQYEHR"
#define SOLAX_API_URL "https://global.solaxcloud.com/api/v2/dataAccess/realtimeInfo/get"

// LCD Configuration
#define LCD_ADDRESS 0x27  // Common I2C address. If display doesn't work, try 0x3F
#define LCD_COLUMNS 16
#define LCD_ROWS 4

// Timing Configuration
#define UPDATE_INTERVAL 60000     // API update interval in milliseconds (60 seconds)
#define SCREEN_SWITCH_INTERVAL 5000  // Screen switch interval in milliseconds (5 seconds)

