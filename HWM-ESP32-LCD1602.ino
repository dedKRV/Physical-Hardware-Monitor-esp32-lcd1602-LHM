/*
HWM ESP32 LCD1602 - https://github.com/dedKRV/Physical-Hardware-Monitor-esp32-lcd1602-LHM
WEB SERVER - https://github.com/TonTon-Macout/web-server-for-Libre-Hardware-Monitor
LHM - https://github.com/LibreHardwareMonitor/LibreHardwareMonitor
   - Date and Time mode (first mode)
   - Temperature mode with progress bars
   - Load mode with progress bars
   - Fan speeds mode
   - Network speeds mode
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#else
#include <WiFi.h>
#include <HTTPClient.h>
#endif

// Settings
const char* ssid = "ssid";
const char* password = "password";
const char* serverUrl = "serverUrl";
#define UPDATE_INTERVAL 1000
#define HTTP_TIMEOUT 2000

// LCD
#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// Button (D12 → Button → 10k → GND)
#define BUTTON_PIN 12
#define DEBOUNCE_DELAY 50
int buttonState;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
int displayMode = 0; // 0-date/time, 1-temps, 2-loads, 3-fans, 4-network

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 10800); // UTC+3 (3 * 3600 seconds)

// Custom character for progress bar (filled block)
byte progressBlock[8] = {
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};

// Min/Max values for easy editing
// Temperature ranges
const int CPU_TEMP_MIN = 27;
const int CPU_TEMP_MAX = 50;
const int GPU_TEMP_MIN = 40;
const int GPU_TEMP_MAX = 50;

// Load ranges
const int CPU_LOAD_MIN = 0;
const int CPU_LOAD_MAX = 86;
const int GPU_LOAD_MIN = 0;
const int GPU_LOAD_MAX = 74;

// Fan speed ranges
const int CPU_FAN_MIN = 696;
const int CPU_FAN_MAX = 881;
const int GPU_FAN_MIN = 0;
const int GPU_FAN_MAX = 3500;

// Network speed ranges
const int UPLOAD_SPEED_MIN = 0;
const int UPLOAD_SPEED_MAX = 199;
const int DOWNLOAD_SPEED_MIN = 0;
const int DOWNLOAD_SPEED_MAX = 9000;

WiFiClient client;
HTTPClient http;

// Data variables
int pc_GPULoad = 0;
int pc_CPUTemp = 0;
int pc_CPULoad = 0;
int pc_GPUTemp = 0;
int pc_CPUFan = 0;
int pc_GPUFan = 0;
int pc_UploadSpeed = 0;
int pc_DownloadSpeed = 0;

void setup() {
  Serial.begin(115200);
  
  // LCD initialization
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Initializing...");
  
  // Create custom character for progress bar
  lcd.createChar(0, progressBlock);
  
  // Button setup
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // WiFi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  lcd.clear();
  lcd.print("WiFi Connected");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());
  
  // Initialize NTP client
  timeClient.begin();
  
  delay(2000);
}

void checkButton() {
  int reading = digitalRead(BUTTON_PIN);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;
      
      if (buttonState == LOW) {
        displayMode = (displayMode + 1) % 5; // Cycle through
        Serial.println("Mode changed to: " + String(displayMode == 0 ? "Date/Time" : displayMode == 1 ? "Temperatures" : displayMode == 2 ? "Loads" : displayMode == 3 ? "Fans" : "Network"));
        updateLCD();
      }
    }
  }
  lastButtonState = reading;
}

String getFormattedDate() {
  timeClient.update();
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime(&rawtime);
  
  char buffer[12];
  sprintf(buffer, "%02d.%02d", ti->tm_mday, ti->tm_mon + 1);
  return String(buffer);
}

int calculateProgress(int value, int minVal, int maxVal) {
  float percentage = (float)(value - minVal) / (maxVal - minVal) * 100;
  percentage = constrain(percentage, 0, 100);
  return round(percentage / 10);
}

void updateLCD() {
  lcd.clear();
  
  if(displayMode == 0) { // Date/Time mode (first mode)
    timeClient.update();
    
    // Top row: "   DATA:DD.MM   "
    lcd.setCursor(0, 0);
    lcd.print("   DATA:");
    lcd.print(getFormattedDate());
    lcd.print("   ");
    
    // Bottom row: " TIME: HH:MM:SS "
    lcd.setCursor(0, 1);
    lcd.print(" TIME: ");
    lcd.print(timeClient.getFormattedTime());
    lcd.print(" ");
  }
  else if(displayMode == 1) { // Temperature mode
    // CPU Temperature
    lcd.setCursor(0,0);
    lcd.print("CPU");
    lcd.print(pc_CPUTemp);
    lcd.print("\xDF");
    
    int cpuTempBlocks = calculateProgress(pc_CPUTemp, CPU_TEMP_MIN, CPU_TEMP_MAX);
    lcd.setCursor(6,0);
    for(int i=0; i<10; i++) {
      lcd.write(i < cpuTempBlocks ? 0 : ' ');
    }
    
    // GPU Temperature
    lcd.setCursor(0,1);
    lcd.print("GPU");
    lcd.print(pc_GPUTemp);
    lcd.print("\xDF");
    
    int gpuTempBlocks = calculateProgress(pc_GPUTemp, GPU_TEMP_MIN, GPU_TEMP_MAX);
    lcd.setCursor(6,1);
    for(int i=0; i<10; i++) {
      lcd.write(i < gpuTempBlocks ? 0 : ' ');
    }
  } 
  else if(displayMode == 2) { // Load mode
    // CPU Load
    lcd.setCursor(0,0);
    lcd.print("CPU");
    lcd.print(pc_CPULoad);
    lcd.print("%");
    
    int cpuLoadBlocks = calculateProgress(pc_CPULoad, CPU_LOAD_MIN, CPU_LOAD_MAX);
    lcd.setCursor(6,0); 
    for(int i=0; i<10; i++) {
      lcd.write(i < cpuLoadBlocks ? 0 : ' ');
    }
    
    // GPU Load
    lcd.setCursor(0,1);
    lcd.print("GPU");
    lcd.print(pc_GPULoad);
    lcd.print("%");
    
    int gpuLoadBlocks = calculateProgress(pc_GPULoad, GPU_LOAD_MIN, GPU_LOAD_MAX);
    lcd.setCursor(6,1); // GPUXX%
    for(int i=0; i<10; i++) {
      lcd.write(i < gpuLoadBlocks ? 0 : ' ');
    }
  }
  else if(displayMode == 3) { // Fan speeds mode
    // CPU Fan
    lcd.setCursor(0,0);
    lcd.print("CPU");
    lcd.print(pc_CPUFan);
    
    int cpuFanBlocks = calculateProgress(pc_CPUFan, CPU_FAN_MIN, CPU_FAN_MAX);
    lcd.setCursor(6,0);
    for(int i=0; i<10; i++) {
      lcd.write(i < cpuFanBlocks ? 0 : ' ');
    }
    
    // GPU Fan
    lcd.setCursor(0,1);
    lcd.print("GPU");
    lcd.print(pc_GPUFan);
    
    int gpuFanBlocks = calculateProgress(pc_GPUFan, GPU_FAN_MIN, GPU_FAN_MAX);
    lcd.setCursor(6,1);
    for(int i=0; i<10; i++) {
      lcd.write(i < gpuFanBlocks ? 0 : ' ');
    }
  }
  else if(displayMode == 4) { // Network speeds mode
    // Upload Speed
    lcd.setCursor(0,0);
    lcd.print("UPS");
    lcd.print(pc_UploadSpeed);
    
    int uploadBlocks = calculateProgress(pc_UploadSpeed, UPLOAD_SPEED_MIN, UPLOAD_SPEED_MAX);
    lcd.setCursor(6,0);
    for(int i=0; i<10; i++) {
      lcd.write(i < uploadBlocks ? 0 : ' ');
    }
    
    // Download Speed
    lcd.setCursor(0,1);
    lcd.print("DOS");
    lcd.print(pc_DownloadSpeed);
    
    int downloadBlocks = calculateProgress(pc_DownloadSpeed, DOWNLOAD_SPEED_MIN, DOWNLOAD_SPEED_MAX);
    lcd.setCursor(6,1);
    for(int i=0; i<10; i++) {
      lcd.write(i < downloadBlocks ? 0 : ' ');
    }
  }
}

void getPCData() {
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected!");
    return;
  }

  http.begin(client, serverUrl);
  http.setTimeout(HTTP_TIMEOUT);
  
  int httpCode = http.GET();
  
  if(httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if(!error) {
      // Parse values
      String cpuLoadStr = doc["CPULoad"].as<String>();
      cpuLoadStr.replace(" %", "");
      cpuLoadStr.replace(',', '.');
      pc_CPULoad = round(cpuLoadStr.toFloat());
      
      String cpuTempStr = doc["CPUTemp"].as<String>();
      cpuTempStr.replace(" °C", "");
      cpuTempStr.replace(',', '.');
      pc_CPUTemp = round(cpuTempStr.toFloat());
      
      String gpuLoadStr = doc["GPULoad"].as<String>();
      gpuLoadStr.replace(" %", "");
      gpuLoadStr.replace(',', '.');
      pc_GPULoad = round(gpuLoadStr.toFloat());
      
      String gpuTempStr = doc["GPUTemp"].as<String>();
      gpuTempStr.replace(" °C", "");
      gpuTempStr.replace(',', '.');
      pc_GPUTemp = round(gpuTempStr.toFloat());
      
      // Parse fan speeds (extract numbers from strings like "725 RPM")
      String cpuFanStr = doc["CPUFan"].as<String>();
      cpuFanStr.replace(" RPM", "");
      pc_CPUFan = cpuFanStr.toInt();
      
      String gpuFanStr = doc["GPUFan"].as<String>();
      gpuFanStr.replace(" RPM", "");
      pc_GPUFan = gpuFanStr.toInt();
      
      // Parse network speeds (extract numbers from strings like "0,5 KB/s")
      String uploadStr = doc["UploadSpeed"].as<String>();
      uploadStr.replace(" KB/s", "");
      uploadStr.replace(',', '.');
      pc_UploadSpeed = round(uploadStr.toFloat());
      
      String downloadStr = doc["DownloadSpeed"].as<String>();
      downloadStr.replace(" KB/s", "");
      downloadStr.replace(',', '.');
      pc_DownloadSpeed = round(downloadStr.toFloat());
      
      // Constrain values to defined ranges
      pc_CPUTemp = constrain(pc_CPUTemp, CPU_TEMP_MIN, CPU_TEMP_MAX);
      pc_GPUTemp = constrain(pc_GPUTemp, GPU_TEMP_MIN, GPU_TEMP_MAX);
      pc_CPULoad = constrain(pc_CPULoad, CPU_LOAD_MIN, CPU_LOAD_MAX);
      pc_GPULoad = constrain(pc_GPULoad, GPU_LOAD_MIN, GPU_LOAD_MAX);
      pc_CPUFan = constrain(pc_CPUFan, CPU_FAN_MIN, CPU_FAN_MAX);
      pc_GPUFan = constrain(pc_GPUFan, GPU_FAN_MIN, GPU_FAN_MAX);
      pc_UploadSpeed = constrain(pc_UploadSpeed, UPLOAD_SPEED_MIN, UPLOAD_SPEED_MAX);
      pc_DownloadSpeed = constrain(pc_DownloadSpeed, DOWNLOAD_SPEED_MIN, DOWNLOAD_SPEED_MAX);
      
      if(displayMode != 0) { // Only update if not in date/time mode
        updateLCD();
      }
    }
  }
  http.end();
}

void loop() {
  checkButton();
  
  static unsigned long lastUpdate = 0;
  if(millis() - lastUpdate > UPDATE_INTERVAL) {
    getPCData();
    
    // Always update time display if in date/time mode
    if(displayMode == 0) {
      updateLCD();
    }
    
    lastUpdate = millis();
  }
  
  delay(10);
}