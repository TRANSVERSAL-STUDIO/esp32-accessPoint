#include <WiFiConfig.h>
#include <FS.h>
#include <WiFi.h>
#include <Preferences.h>

void setup() {
  // Start Serial communication for debugging
  Serial.begin(115200);

  // Begin Preferences
  preferences.begin("network", false); // Start Preferences with the namespace "network"

  // Check if it's already connected to a network
  if (WiFi.status() != WL_CONNECTED) {
    // If not connected, try to connect to the last saved Wi-Fi network
    shouldRetry = connectToSavedNetwork();

    // If we're here, it means we're not connected to any network, so create an Access Point
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // Set subnet mask to 255.255.255.0
    WiFi.softAP(ssid, password);

    // Print Access Point information
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("Access Point SSID: ");
    Serial.println(ssid);
    Serial.print("Access Point IP address: ");
    Serial.println(apIP);
  } else {
    // If already connected, serve the connected page
    serveConnectedPage();
    return;
  }

  // Serve an HTML page with fields for network name (SSID) and password
  serveWiFiConfigPage();
}

void loop() {
  // Your main code here
}
