#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>


Preferences preferences;

const char* ssid = "Your Network"; // Enter the name of your network
const char* password = "Your Password"; // Enter the password for the access point
IPAddress apIP(192, 168, 1, 1); // Enter the IP of your access point
bool shouldRetry = true;

// Function prototypes
void serveWiFiConfigPage();
bool isRegularCharacters(const char* str);
bool connectToSavedNetwork();
void saveNetworkCredentials(const char* ssid, const char* password);
void handleConnect(AsyncWebServerRequest *request);
bool saveNetworkSettings(const char* hostIP, const char* hostPort);
void handleSave(AsyncWebServerRequest *request);
void serveConnectedPage();
void handleForget(AsyncWebServerRequest *request);

AsyncWebServer server(80);

void serveWiFiConfigPage() {
  String htmlPage = "<html><body>";
  htmlPage += "<h2>Enter Wi-Fi credentials</h2>";
  htmlPage += "<form method='post' action='/connect'>";
  htmlPage += "Network Name (SSID): <input type='text' name='ssid'><br>";
  htmlPage += "Password: <input type='password' name='password'><br>";
  htmlPage += "<input type='submit' value='Connect'></form>";
  htmlPage += "</body></html>";

  // Serve the HTML page
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // Set subnet mask to 255.255.255.0
  WiFi.softAP(ssid, password);
  server.on("/", HTTP_GET, [htmlPage](AsyncWebServerRequest *request) {
    request->send(200, "text/html", htmlPage);
  });
  server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request){    // Handle the form submission
    handleConnect(request);
  });
  server.begin();
}

bool connectToSavedNetwork() {
  preferences.begin("network", false); // Start Preferences with the namespace "network"
  // Attempt to load SSID and password from Preferences
  String storedSSID = preferences.getString("ssid", "");
  String storedPassword = preferences.getString("password", "");

  // If the stored SSID is empty, go directly to AP mode
  if (storedSSID.isEmpty()) {
    Serial.println("Stored SSID is empty. Entering AP mode.");
    return true; // Need to enter AP mode
  }

  Serial.println("Trying to recall last network with the following credentials");
  Serial.print("Network:");
  Serial.println(storedSSID);
  Serial.print("Password:");
  Serial.println(storedPassword);

  // Check if the stored SSID and password contain only regular characters
  bool validCredentials = isRegularCharacters(storedSSID.c_str()) && isRegularCharacters(storedPassword.c_str());

  if (validCredentials) {
    Serial.println("Connecting to saved network...");
    WiFi.begin(storedSSID.c_str(), storedPassword.c_str());
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 10) {
      delay(1000);
      if (retries > 0) {
        Serial.print("Could not connect retry ");
        Serial.print(retries);
        Serial.println(" out of 10");
      }
      retries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to saved network.");
      shouldRetry = false; // Successfully connected, no need to enter AP mode
      serveConnectedPage(); // Serve the connected page
      return false; // Return false to indicate that we're connected
    } else {
      Serial.println("Failed to connect to saved network.");
    }
  } else {
    Serial.println("Stored credentials contain non-regular characters. Entering AP mode.");
  }
preferences.end(); // Save changes
  return true; // Need to enter AP mode
}

void saveNetworkCredentials(const char* ssid, const char* password) {
  preferences.begin("network", false); // Start Preferences with the namespace "network"
  // Save SSID and password to Preferences
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end(); // Save changes
}

void handleConnect(AsyncWebServerRequest *request) {
  String ssid;
  String password;

  preferences.begin("network", false); // Start Preferences with the namespace "network"

  // Get SSID and password from the request
  if (request->hasParam("ssid", true)) {
    ssid = request->getParam("ssid", true)->value();
  }
  if (request->hasParam("password", true)) {
    password = request->getParam("password", true)->value();
  }

  if (ssid.length() > 0 && password.length() > 0) {
    // Save credentials and attempt to connect
    Serial.println("Trying to connect");
    saveNetworkCredentials(ssid.c_str(), password.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for the connection
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 10) {
      delay(1000);
      retries++;
    }

    // Check if the connection was successful
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to Wi-Fi.");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP()); // Print the new IP address
      shouldRetry = false; // Successfully connected, no need to enter AP mode
      request->send(200, "text/plain", "Connected to Wi-Fi.");
      delay(100);
      preferences.end(); // Save changes
      serveConnectedPage(); // Serve the connected page
      // Restart the ESP32
      ESP.restart();
    } else {
      Serial.println("Failed to connect to Wi-Fi.");
      preferences.end(); // Save changes
      request->send(200, "text/plain", "Failed to connect to Wi-Fi.");
    }
  } else {
    Serial.println("Invalid SSID or password.");
    preferences.end(); // Save changes
    request->send(400, "text/plain", "Invalid SSID or password");
  }
}

bool isRegularCharacters(const char* str) {
  // Check if the string contains only printable characters (excluding control characters)
  for (size_t i = 0; i < strlen(str); i++) {
    char c = str[i];
    if (!isprint(c)) {
      return false;
    }
  }
  return true;
}

void serveConnectedPage() {
  // Begin Preferences
  preferences.begin("network", false); // Start Preferences with the namespace "network"

  String htmlPage = "<html><body>";
  htmlPage += "<h2>Connected to Wi-Fi</h2>";
  htmlPage += "<p>IP Address: ";
  htmlPage += WiFi.localIP().toString();
  htmlPage += "</p>";

  // Add button to forget network
  htmlPage += "<br><form method='post' action='/forget'>";
  htmlPage += "<input type='submit' value='Forget Network'></form>";

  htmlPage += "</body></html>";

  // Serve the HTML page
  server.on("/", HTTP_GET, [htmlPage](AsyncWebServerRequest *request) {
    request->send(200, "text/html", htmlPage);
  });

  // Serve the HTML Forget page
  server.on("/forget", HTTP_POST, [](AsyncWebServerRequest *request){
    handleForget(request);
  });

  // End Preferences
  preferences.end();
}

void handleForget(AsyncWebServerRequest *request) {
  preferences.begin("network", false); // Start Preferences with the namespace "network"
  // Send the page first so we can see it
  request->send(200, "text/html", "Forgetting network and returning to access point.");
  delay(100);

  // Clear saved network credentials from Preferences
  preferences.remove("ssid");
  preferences.remove("password");

  preferences.end(); // Save changes
  // Disconnect from the current network
  WiFi.disconnect(true);
  delay(100);

  // Restart the ESP32
  ESP.restart();
}

#endif // WIFI_CONFIG_H
