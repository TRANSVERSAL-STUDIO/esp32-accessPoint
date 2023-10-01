#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>


Preferences preferences;

const char* ssid = "trvs-baton";
const char* password = ""; // No password for the access point
IPAddress apIP(10, 1, 1, 1);
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

bool saveNetworkSettings(const char* hostIP, const char* hostPort) {
  preferences.begin("network", false); // Start Preferences with the namespace "network"
  // Save host IP and port to Preferences
  preferences.putString("tcp_host", hostIP);
  preferences.putInt("tcp_port", atoi(hostPort));
  return true; // Successful save
}

void saveHostInfo(const char* host, int port) {
  preferences.begin("network", false); // Start Preferences with the namespace "network"
  // Save host and port to Preferences
  preferences.putString("tcp_host", host);
  preferences.putInt("tcp_port", port);
  preferences.end(); // Save changes
}

void handleSave(AsyncWebServerRequest *request) {
  preferences.begin("network", false); // Start Preferences with the namespace "network"
  String tcp_host;
  String tcp_port;

  // Get TCP host and port from the request
  if (request->hasParam("tcp_host", true)) {
    tcp_host = request->getParam("tcp_host", true)->value();
  }
  if (request->hasParam("tcp_port", true)) {
    tcp_port = request->getParam("tcp_port", true)->value();
  }

  if (tcp_host.length() > 0 && tcp_port.length() > 0) {
    // Convert tcp_port string to integer
    int port = tcp_port.toInt();

    // Save TCP settings and check for success
    saveNetworkSettings(tcp_host.c_str(), tcp_port.c_str());
    Serial.println("TCP settings saved");

    // Add OSC settings
    String osc1_host;
    String osc2_host;
    String osc_port;

    // Get OSC settings from the request
    if (request->hasParam("osc1_host", true)) {
      osc1_host = request->getParam("osc1_host", true)->value();
    }
    if (request->hasParam("osc2_host", true)) {
      osc2_host = request->getParam("osc2_host", true)->value();
    }
    if (request->hasParam("osc_port", true)) {
      osc_port = request->getParam("osc_port", true)->value();
    }

    // Save OSC settings
    preferences.putString("osc1_host", osc1_host);
    preferences.putString("osc2_host", osc2_host);
    preferences.putInt("osc1_port", atoi(osc_port.c_str()));

    Serial.println("OSC settings saved");
    preferences.end(); // Save changes
    request->send(200, "text/html", "Settings saved.");
  } else {
    Serial.println("Incomplete or invalid settings. Please try again.");
    preferences.end(); // Save changes
    request->send(400, "text/html", "Incomplete or invalid settings. Please try again.");
  }
}

void serveConnectedPage() {
  // Begin Preferences
  preferences.begin("network", false); // Start Preferences with the namespace "network"

  String htmlPage = "<html><body>";
  htmlPage += "<h2>Connected to Wi-Fi</h2>";
  htmlPage += "<p>IP Address: ";
  htmlPage += WiFi.localIP().toString();
  htmlPage += "</p>";

  // Display TCP host and port
  htmlPage += "<h3>TCP Settings</h3>";
  htmlPage += "<p>TCP Host: " + preferences.getString("tcp_host", "Not Set") + "</p>";
  htmlPage += "<p>TCP Port: " + String(preferences.getInt("tcp_port", 0)) + "</p>";

  // Display OSC host and port
  htmlPage += "<h3>OSC Settings</h3>";
  htmlPage += "<p>OSC Host 1: " + preferences.getString("osc1_host", "Not Set") + "</p>";
  htmlPage += "<p>OSC Host 2: " + preferences.getString("osc2_host", "Not Set") + "</p>";
  htmlPage += "<p>OSC Port: " + String(preferences.getInt("osc1_port", 0)) + "</p>";

  // Add form for saving host settings
  htmlPage += "<h3>Host Settings</h3>";
  htmlPage += "<form method='post' action='/save'>";
  htmlPage += "TCP Host: <input type='text' name='tcp_host' value='" + preferences.getString("tcp_host", "") + "'><br>";
  htmlPage += "TCP Port: <input type='text' name='tcp_port' value='" + String(preferences.getInt("tcp_port", 0)) + "'><br>";
  htmlPage += "OSC Host 1: <input type='text' name='osc1_host' value='" + preferences.getString("osc1_host", "") + "'><br>";
  htmlPage += "OSC Host 2: <input type='text' name='osc2_host' value='" + preferences.getString("osc2_host", "") + "'><br>";
  htmlPage += "OSC Port: <input type='text' name='osc_port' value='" + String(preferences.getInt("osc1_port", 0)) + "'><br>";
  htmlPage += "<input type='submit' value='Save'></form>";

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

  // Serve the HTML Save page
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    handleSave(request);
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

  // Clear TCP and OSC settings
  preferences.remove("tcp_host");
  preferences.remove("tcp_port");
  preferences.remove("osc1_host");
  preferences.remove("osc2_host");
  preferences.remove("osc1_port");

  preferences.end(); // Save changes
  // Disconnect from the current network
  WiFi.disconnect(true);
  delay(100);

  // Restart the ESP32
  ESP.restart();
}

#endif // WIFI_CONFIG_H
