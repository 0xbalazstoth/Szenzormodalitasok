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
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// Websockets
#include <WebSocketsServer.h>
#include <Hash.h>

// DHT-22M
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

bool testWifi(void);
void launchWeb(void);
void setupAP(void);
ESP8266WebServer server(80);

const char* ssid = "";
const char* password = "";
String st;
String content;
int i = 0;
int statusCode;

WebSocketsServer webSocket = WebSocketsServer(81);

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println('\n');

  dht.begin();
  
  // connectToWiFi(ssid, password);
  setupEEPROM();

  if(!display.begin(I2C_ADDRESS, true)) { // Check if the display is connected properly
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();

  // // Display initial text
  displayOledText(0, 15, 1, SH110X_WHITE, SH110X_BLACK, "Balazs Toth");
  displayOledText(0, 25, 1, SH110X_WHITE, SH110X_BLACK, "MWZX0D");

  display.display();
}

void setupEEPROM() {
  Serial.println();
  Serial.println("Disconnecting previously connected WiFi");
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println();
  Serial.println();
  Serial.println("Startup");

  //---------------------------------------- Read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");

  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");

  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);


  WiFi.begin(esid.c_str(), epass.c_str());
  if (testWifi())
  {
    Serial.println("Succesfully Connected!!!");
    // launchWeb();

    webSocket.begin(); // Start the WebSocket server
    webSocket.onEvent(webSocketEvent); // Assign the event handler
    Serial.println("WebSocket server started.");

    return;
  }
  else
  {
    Serial.println("Turning the HotSpot On");
    launchWeb(); // For first time usage only!
    setupAP(); // Setup HotSpot
  }

  Serial.println();
  Serial.println("Waiting.");
  
  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.print(".");
    delay(100);
    server.handleClient();
  }
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

bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}

void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);

    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("mwzx0d_ff", "");
  Serial.println("softap");
  launchWeb();
  Serial.println("over");
}

void createWebServer()
{
  Serial.println("Web server started.");

  server.on("/", []() {
    IPAddress ip = WiFi.softAPIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    content = "<!DOCTYPE HTML>\r\n<html>";
    content += "<head><title>ESP8266 WiFi Settings</title></head><body>";
    content += "<h1>Hello from ESP8266 at " + ipStr + "</h1>";

    // Form to initiate a WiFi scan
    content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"Scan for WiFi networks\"></form>";

    // Form to set new WiFi credentials
    content += "<h2>Set WiFi Credentials</h2>";
    content += "<form method='get' action='setting'>";
    content += "<label>SSID: </label><input name='ssid' length=32 required='required'><br>";
    content += "<label>Password: </label><input name='pass' type='password' length=64 required='required'><br>";
    content += "<input type='submit' value='Set Credentials'>";
    content += "</form>";

    // Button to clear WiFi settings
    content += "<h2>Clear WiFi Settings</h2>";
    content += "<form action='/clearWifi' method='post'><button type='submit'>Clear WiFi Settings</button></form>";

    content += "</body></html>";
    server.send(200, "text/html", content);
});

  server.on("/scan", []() {
    //setupAP();
    IPAddress ip = WiFi.softAPIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

    content = "<!DOCTYPE HTML>\r\n<html>go back";
    server.send(200, "text/html", content);
  });

  server.on("/temperature", []() {
      float humidity = dht.readHumidity();
      float temperature = dht.readTemperature();
      if (isnan(humidity) || isnan(temperature)) {
          server.send(500, "text/plain", "Failed to read from DHT sensor!");
      } else {
          String json = "{\"humidity\": " + String(humidity, 1) + ", \"temperature\": " + String(temperature, 1) + "}";
          server.send(200, "application/json", json);
      }
  });

  server.on("/setting", []() {
    String qsid = server.arg("ssid");
    String qpass = server.arg("pass");
    if (qsid.length() > 0 && qpass.length() > 0) {
      Serial.println("clearing eeprom");
      for (int i = 0; i < 96; ++i) {
        EEPROM.write(i, 0);
      }
      Serial.println(qsid);
      Serial.println("");
      Serial.println(qpass);
      Serial.println("");

      Serial.println("writing eeprom ssid:");
      for (int i = 0; i < qsid.length(); ++i)
      {
        EEPROM.write(i, qsid[i]);
        Serial.print("Wrote: ");
        Serial.println(qsid[i]);
      }
      Serial.println("writing eeprom pass:");
      for (int i = 0; i < qpass.length(); ++i)
      {
        EEPROM.write(32 + i, qpass[i]);
        Serial.print("Wrote: ");
        Serial.println(qpass[i]);
      }
      EEPROM.commit();

      content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
      statusCode = 200;
      ESP.reset();
    } else {
      content = "{\"Error\":\"404 not found\"}";
      statusCode = 404;
      Serial.println("Sending 404");
    }
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(statusCode, "application/json", content);

  });

  server.on("/clearWifi", HTTP_POST, []() {
        // Clear the EEPROM memory where the SSID and password are stored
        for (int i = 0; i < 96; ++i) {  // Assuming SSID and password are within first 96 bytes
            EEPROM.write(i, 0);
        }
        EEPROM.commit();  // Ensure the changes are written to EEPROM

        // Provide feedback and restart ESP8266
        server.send(200, "text/html", "WiFi settings cleared. Device will restart now.");
        delay(1000);
        ESP.restart();
    });
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
  server.handleClient();
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

    Serial.println(WiFi.localIP());
    displayOledText(0, 45, 1, SH110X_WHITE, SH110X_BLACK, "IP address:");
    displayOledText(0, 55, 1, SH110X_WHITE, SH110X_BLACK, IpAddress2String(WiFi.localIP()));

    // Update the status bar on the OLED
    updateStatusBar(humidity, temperature, isConnected);

    webSocket.loop();
    sendSensorData();
  }

  // Call display.display() after all screen updates are done if necessary
  display.display();
}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

void sendSensorData() {
    static unsigned long lastSendTime = 0;
    unsigned long currentTime = millis();
    
    // Update interval: 1000 milliseconds
    if (currentTime - lastSendTime > 1000) {
        lastSendTime = currentTime;
        
        float humidity = dht.readHumidity();
        float temperature = dht.readTemperature();
        
        if (!isnan(humidity) && !isnan(temperature)) {
            char msg[100];
            snprintf(msg, sizeof(msg), "Temperature: %.2f C, Humidity: %.2f%%", temperature, humidity);
            
            // Send to all connected clients
            for(int i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++) {
                if (webSocket.clientIsConnected(i)) {
                    webSocket.sendTXT(i, msg);
                }
            }
        }
    }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    // When a WebSocket message is received
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connection from %s\n", num, ip.toString().c_str());
            break;
        }
        case WStype_TEXT:
            Serial.printf("[%u] Received text: %s\n", num, payload);
            // Check if the message is a request for sensor data
            if (strcmp((char *)payload, "getSensorData") == 0) {
                // Read sensor data
                float humidity = dht.readHumidity();
                float temperature = dht.readTemperature();
                // Check if any reads failed
                if (isnan(humidity) || isnan(temperature)) {
                    Serial.println("Failed to read from DHT sensor!");
                    webSocket.sendTXT(num, "Failed to read from sensor");
                } else {
                    // Prepare and send the sensor data
                    char msg[100];
                    snprintf(msg, sizeof(msg), "Temperature: %.2f C, Humidity: %.2f%%", temperature, humidity);
                    webSocket.sendTXT(num, msg);
                }
            }
            break;
        case WStype_BIN:
            Serial.printf("[%u] Binary data\n", num);
            break;
    }
}