#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#ifndef D0
  #define D0 16
  #define D1 5
  #define D2 4
  #define D3 0
  #define D4 2
  #define D5 14
  #define D6 12
  #define D7 13
  #define D8 15
#endif

// --- I2C Motion Sensor (MPU-6050) ---
#define MPU_SDA         D2     
#define MPU_SCL         D1     

// --- Zipper Sensor (Magnetic Reed Switch) ---
#define REED_PIN        D5     
#define REED_ACTIVE     HIGH   

// --- Output Alerts (Buzzer & LEDs) ---
#define BUZZER_PIN      D7     // Buzzer on D7
#define RED_LED_PIN     D0     // Red LED resistor on D0
#define GREEN_LED_PIN   D8     // Unused dummy pin
#define BLUE_LED_PIN    D4     // Built-in status LED

// --- GPS & System Constants ---
#define GPS_RX_PIN      D3     
#define GPS_TX_PIN      D8     
#define GPS_BAUD_RATE   9600   

#define WIFI_AP_SSID        "SecurePack"
#define WIFI_AP_PASSWORD    "securepack123"
#define WIFI_AP_CHANNEL     1
#define WIFI_AP_HIDDEN      false
#define WIFI_AP_MAX_CONN    4
#define HTTP_SERVER_PORT    80

#define DEFAULT_SECURITY_PIN "1234"
#define PIN_LENGTH           4

#define MOTION_CONFIRM_MS    350    
#define REED_DEBOUNCE_MS     150    
#define WARNING_DURATION_MS  4000   
#define FIND_BAG_TIMEOUT_MS  20000  

#define ACCEL_DELTA_LOW      4.5f   
#define ACCEL_DELTA_MEDIUM   2.2f   
#define ACCEL_DELTA_HIGH     1.0f   

#define BATTERY_SIMULATION_MODE  false
#define BATTERY_MAX_VOLTAGE      4.20f  
#define BATTERY_MIN_VOLTAGE      3.30f  
#define BATTERY_ADC_FACTOR       0.00412f 
#define BATTERY_ADC_PIN          A0
#endif // CONFIG_H