#include <Arduino.h>
#include <ESP8266WiFi.h>

//                  ########## WPS ##########
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#define HOSTNAME "ESP-03_WPS"
char cfgssid[256];
char cfgpassword[256];
//                  ########## WPS ##########

#include "SinricPro.h"
#include "SinricProSwitch.h"

//#define WIFI_SSID         "**********"
//#define WIFI_PASS         "**********"
#define APP_KEY           "**********"
#define APP_SECRET        "**********"

#define SWITCH_ID_1       "**********"
#define RELAYPIN_1        13

#define BAUD_RATE         115200

//                  ########## OTA UPDATE ##########
#define OTA_LOGIN         "**********"
#define OTA_PASS          "**********"
#include <ESPAsyncTCP.h>
#include <ElegantOTA.h>
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
//                  ########## OTA UPDATE ##########

int N2_gpio = 16;
int pin_output_power = 12;
int input_5v_pin = 14;
int input_5v_analog = 0;

const int input_5v_trigger = 615;      // When N2 powered is of, 5v drops to 2.1V, so we compare to around 3V with the 615 analog value (0 to 1023 range)
int hold = 0;
int hold_old = 1;
int on = 0;
int on_old = 1;
bool power_output = false;

bool onPowerState1(const String &deviceId, bool &state) {
 Serial.printf("Device 1 turned %s", state?"on":"off");
 Serial.printf("\r\n\r\n");
 digitalWrite(RELAYPIN_1, state ? HIGH:LOW);
 return true;
}

void setupWPS() {
  // commit 512 bytes of ESP8266 flash 
  // this step actually loads the content (512 bytes) of flash into 
  // a 512-byte-array cache in RAM
  EEPROM.begin(512);
  int i = 0;
  // Read the settings
  for(i = 0;i<256;++i) {
    cfgssid[i]=EEPROM.read(i);
    if(!cfgssid[i])
      break;
  }
  cfgssid[i]=0;
  i=0;
  for(i = 0;i<256;++i) {
    cfgpassword[i]=(char)EEPROM.read(i+256);
    if(!cfgpassword[i])
      break;
  }
  cfgpassword[i]=0;
  // Initialize the WiFi and connect
  WiFi.mode(WIFI_STA);  
  bool done = false;
  while (!done) {
    // Connect to Wi-Fi
    WiFi.begin(cfgssid, cfgpassword);
    Serial.print("Connecting to WiFi");
    // try this for 10 seconds, then check for WPS
    for (int i = 0; i < 20 && WL_CONNECTED != WiFi.status(); ++i) {
      Serial.print(".");
      delay(500);
    }
    Serial.println("");
    // If we're not connected, wait for a WPS signal
    if (WL_CONNECTED != WiFi.status()) {
      Serial.print("Connection to ");
      Serial.print(cfgssid);
      Serial.println(" failed. Entering auto-config mode, press the WPS button on your router");
      bool ret = WiFi.beginWPSConfig();
      if (ret) {
        String newSSID = WiFi.SSID();
        if (0 < newSSID.length()) {
          Serial.println("Auto-configuration successful. Saving.");
          strcpy(cfgssid, newSSID.c_str());
          strcpy(cfgpassword, WiFi.psk().c_str());
          int c = strlen(cfgssid);
          for(int i = 0;i<c;++i)
            EEPROM.write(i,cfgssid[i]);
          EEPROM.write(c,0);
          c = strlen(cfgpassword);
          for(int i = 0;i<c;++i)
            EEPROM.write(i+256,cfgpassword[i]);
          EEPROM.write(c+256,0);
          EEPROM.end();
          Serial.println("Restarting...");
          ESP.restart();
        } else {
          ret = false;
        }
      }
    } else
      done = true;
    // if we didn't get connected, loop
  }
  // Display the status
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("Host name: ");
  Serial.print(HOSTNAME);
  Serial.println(".local");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // start the multicast DNS publishing
  if (MDNS.begin(HOSTNAME)) {
    Serial.println("MDNS responder started");
  }
}

void WPS_check() {
  // reconnect to the WiFi if we got disconnected
  if (WL_CONNECTED != WiFi.status()) {
    // Connect to Wi-Fi
    WiFi.begin(cfgssid, cfgpassword);
    Serial.print("Connecting to WiFi");
    for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; ++i) {
      Serial.print(".");
      delay(500);
    }
    if (WL_CONNECTED != WiFi.status()) {
      Serial.println("Could not reconnect. Restarting.");
      ESP.restart();
    }
  }
  // update the DNS information
  MDNS.update();
}

void setupSinricPro() {
  // add devices and callbacks to SinricPro
  pinMode(RELAYPIN_1, OUTPUT);
    
  SinricProSwitch& mySwitch1 = SinricPro[SWITCH_ID_1];
  mySwitch1.onPowerState(onPowerState1);
  
  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.restoreDeviceStates(true); // Uncomment to restore the last known state from the server.
   
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void setupOTA() {
  server.on("/", []() {
    server.send(200, "text/plain", "Welcome to N2 Assistant web server for firmware updates.\r\n");
  });
  ElegantOTA.begin(&server);    // Start ElegantOTA
  ElegantOTA.setAuth(OTA_LOGIN, OTA_PASS);
  ElegantOTA.setAutoReboot(true);
  server.begin();
  Serial.println("HTTP server started");
}

void setup() {
  pinMode(pin_output_power, OUTPUT);
  pinMode(N2_gpio, OUTPUT);
  pinMode(input_5v_pin, INPUT);

  digitalWrite(pin_output_power, LOW);
  digitalWrite(N2_gpio, LOW);

  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n");
  while(!Serial) {
    ;
  }
  setupWPS();
  delay(5000);
  setupOTA();
  setupSinricPro();
}

void loop() {  
  WPS_check();
  server.handleClient();
  ElegantOTA.loop();
  SinricPro.handle();
  delay(100);
  read_input();

  if ((on == 1) & (on_old == 0)) {
    power_output = true;
  }
  
  if ((on == 0) & (on_old == 1)) {
    digitalWrite(N2_gpio, LOW);
  }

  if ((hold == 0) & (hold_old == 1)) {
    power_output = false;
  }

  /*
  Serial.printf("power_output: %d\r\n",power_output);
  Serial.printf("on: %d\r\n",on);
  Serial.printf("on_old: %d\r\n",on_old);
  Serial.printf("hold: %d\r\n",hold);
  Serial.printf("hold_old: %d\r\n",hold_old);
  */
  delay(500);
  on_old = on;
  hold_old = hold;

  if (power_output == true) {
    poweron();
  } else {
    poweroff();
  }
}

void read_input() {
  on = digitalRead(RELAYPIN_1);
  input_5v_analog = analogRead(input_5v_pin);
  if (input_5v_analog > input_5v_trigger) {
    hold = true;
  } else {
    hold = false;
  }
}

void poweroff() {
  digitalWrite(pin_output_power, LOW);        // DISABLE POWER
  digitalWrite(N2_gpio, LOW);
}

void poweron() {
  digitalWrite(pin_output_power, HIGH);       // ENABLE POWER
  digitalWrite(N2_gpio, HIGH);
}
