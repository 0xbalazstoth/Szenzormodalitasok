// Icons
static const unsigned char PROGMEM wifi_bitmap[] = {
  0b00000100, 0b00000000,
  0b00001010, 0b00000000,
  0b00010001, 0b00000000,
  0b00100000, 0b10000000,
  0b01000000, 0b01000000,
  0b10000000, 0b00100000,
  0b00000000, 0b00000000,
  0b00111100, 0b00000000,
  0b01000010, 0b00000000,
  0b10000001, 0b00000000,
  0b00000000, 0b00000000,
  0b00011000, 0b00000000,
  0b00100100, 0b00000000,
  0b01000010, 0b00000000,
  0b00000000, 0b00000000,
  0b00001100, 0b00000000,
  0b00010010, 0b00000000,
  0b00100001, 0b00000000
};

// Wi-Fi
#include <ArduinoWiFiServer.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiGratuitous.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiServer.h>
#include <WiFiServerSecure.h>
#include <WiFiServerSecureBearSSL.h>
#include <WiFiUdp.h>
#include <DHT.h>

// Display
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin is not used so we set this to -1
#define I2C_ADDRESS 0x3C
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// DHT22-M sensor
#define DHTPIN D4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "VodafoneMobileWiFi-D9A8_2.4GHz";
const char* password = ".Master454Q";

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println('\n');

  dht.begin();
  
  connectToWiFi(ssid, password);

  if(!display.begin(I2C_ADDRESS, true)) { // Check if the display is connected properly
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();

  // // Display initial text
  displayOledText(0, 15, 1, SH110X_WHITE, SH110X_BLACK, "Balazs Toth");
  displayOledText(0, 25, 1, SH110X_WHITE, SH110X_BLACK, "MWZX0D");

  // delay(5000);

  // scanAndDisplayNetworks();

  display.display();
}

void connectToWiFi(const char* ssid, const char* password) {
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Function to display text on OLED
void displayOledText(int x, int y, uint8_t textSize, uint16_t textColor, uint16_t backgroundColor, String text) {
  display.setTextSize(textSize); // Set the text size
  display.setTextColor(textColor, backgroundColor); // Set the text color and the background color
  display.setCursor(x, y); // Set the cursor to the desired position
  display.println(text); // Print the text
}

void displayOledTextWrapped(int x, int &y, uint8_t textSize, uint16_t textColor, uint16_t backgroundColor, String text) {
    int maxCharsPerLine = SCREEN_WIDTH / (6 * textSize); // Approximate max number of characters per line
    while (text.length() > maxCharsPerLine) {
        // Extract part of the string that fits into the display width
        String part = text.substring(0, maxCharsPerLine);
        // Display this part
        displayOledText(x, y, textSize, textColor, backgroundColor, part.c_str());
        // Move Y coordinate for the next line
        y += 10 * textSize; // Adjust Y position based on text size
        // Remove the displayed part from the original string
        text = text.substring(maxCharsPerLine);
    }
    // Display the remaining part (or the whole text if it was shorter than maxCharsPerLine)
    displayOledText(x, y, textSize, textColor, backgroundColor, text.c_str());
    y += 8 * textSize; // Prepare Y for the next potential call
}

void scanAndDisplayNetworks() {
  display.clearDisplay(); // Clear the display to start fresh
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100); // Wait a bit for the disconnect to complete
  int n = WiFi.scanNetworks();
  if (n == 0) {
    displayOledText(0, 0, 1, SH110X_WHITE, SH110X_BLACK, "No Wi-Fi networks found.");
  } else {
    int yPos = 0; // Start from top of the display
    for (int i = 0; i < n && yPos < SCREEN_HEIGHT - 10; ++i) {
      String numberedSSID = "[" + String(i + 1) + ".] " + WiFi.SSID(i);
      displayOledTextWrapped(0, yPos, 1, SH110X_WHITE, SH110X_BLACK, numberedSSID);
    }
  }
  display.display();
}

void readDHTSensor() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Read temperature as Celsius

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Print the results to the Serial Monitor
  Serial.print(F("Humidity: "));
  Serial.print(humidity);
  Serial.print(F("%  Temperature: "));
  Serial.print(temperature);
  Serial.println(F("°C"));
}

void updateStatusBar(float humidity, float temperature, bool isConnected) {
  // Prepare the status bar string
  String statusBar = "H: " + String(humidity, 1) + "%  T: " + String(temperature, 1) + "C";
  if (isConnected) {
    statusBar += "  C";
  } else {
    statusBar += "  X";
  }

  // Clear the status bar area to prevent flickering
  display.fillRect(0, 0, SCREEN_WIDTH, 9, SH110X_BLACK); // Adjust the height to include the line

  // Set the status bar's text properties
  display.setTextSize(1); // Smaller text for the status bar
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0); // Start from the top left corner
  display.print(statusBar); // Print the status bar

  // Draw the line under the status bar
  int lineY = 8; // Y position for the line, adjust as needed based on your font size
  display.drawLine(0, lineY, SCREEN_WIDTH, lineY, SH110X_WHITE);
}

void loop() {
  // Wait a few seconds between measurements.
  delay(2000);
  
  // Reading temperature and humidity
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Read temperature as Celsius
  bool isConnected = WiFi.status() == WL_CONNECTED; // Check if connected to WiFi

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  } else {
    // Print the results to the Serial Monitor
    Serial.print(F("Humidity: "));
    Serial.print(humidity);
    Serial.print(F("%  Temperature: "));
    Serial.print(temperature);
    Serial.println(F("°C"));

    // Update the status bar on the OLED
    updateStatusBar(humidity, temperature, isConnected);
  }

  // Call display.display() after all screen updates are done if necessary
  display.display();
}

void displayWifiIcon(int x, int y) {
  display.drawBitmap(x, y, wifi_bitmap, 16, 9, SH110X_WHITE);
}