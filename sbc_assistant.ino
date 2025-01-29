#include <Arduino.h>
#include <ESP8266WiFi.h>

//                  ########## Wifi Manager ##########
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266mDNS.h>
#define HOSTNAME "SBC_Assistant"


//                  ###### SINRIC.PRO #######
#include "SinricPro.h"
#include "SinricProSwitch.h"

#define APP_KEY           "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define APP_SECRET        "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

#define SWITCH_ID_1       "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define RELAYPIN_1        13


//                  ########## OTA UPDATE ##########
#define OTA_LOGIN         "admin"
#define OTA_PASS          "12345"
#include <ESPAsyncTCP.h>
#include <ElegantOTA.h>
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);


//                  ########### PINOUT ############

int led_wifi = 12;         // GPIO12
int N2_gpio = 5;           // GPIO5
int pin_output_power = 4;  // GPIO4
int input_5v_pin = 14;      // GPIO14
int input_5v_analog = 0;

const int input_5v_trigger = 615;      // When N2 powered is of, 5v drops to 2.1V, so we compare to around 3V with the 615 analog value (0 to 1023 range)
int hold = 0;
int hold_old = 1;
int on = 0;
int on_old = 1;
bool power_output = false;

#define BAUD_RATE         115200

bool onPowerState1(const String &deviceId, bool &state) {
 Serial.printf("Device 1 turned %s", state?"on":"off");
 Serial.printf("\r\n\r\n");
 digitalWrite(RELAYPIN_1, state ? HIGH:LOW);
 return true;
}

void setupWifi() {

  WiFiManager wm;

  wm.setClass("invert"); // dark theme

  // wm.resetSettings(); // wipe settings

  bool res;
  res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

  if(!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  } 
  else {
    // connected to the WiFi    
    Serial.println("Connected!");
    digitalWrite(led_wifi, LOW);
  }

  // Display the status
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  // Serial.println(".local");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // start the multicast DNS publishing
  if (MDNS.begin(HOSTNAME)) {
    Serial.println("MDNS responder started");
  }
  Serial.print("Host name: ");
  Serial.print(HOSTNAME);
  
  Serial.println("");
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
  //    server.send(200, "text/plain", "Welcome to N2 Assistant web server for firmware updates.\r\n");   
  server.send(200, "text/html", "<h2>Welcome to SBC Assistant web server for firmware updates.</h2><br>"
                             "<p>IP is: " + WiFi.localIP().toString() + "</p><br>"
                             "You can access the update interface <a href='http://" + 
                             HOSTNAME + ".local/update'>here</a>.");
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
  pinMode(led_wifi, OUTPUT);

  digitalWrite(pin_output_power, LOW);
  digitalWrite(N2_gpio, LOW);
  digitalWrite(led_wifi, HIGH);

  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n");
  while(!Serial) {
    ;
  }
  
  setupWifi();
  
  delay(5000);
  setupOTA();
  setupSinricPro();
}

void loop() {  
  // update the DNS information
  MDNS.update();
  //  WPS_check();
  server.handleClient();
  ElegantOTA.loop();
  SinricPro.handle();
  delay(100);
  read_input();

  if ((on == 1) & (on_old == 0)) {
    power_output = true;
    digitalWrite(N2_gpio, HIGH);      // Set GPIO pin HIGH
  }
  
  if ((on == 0) & (on_old == 1)) {
    digitalWrite(N2_gpio, LOW);       // Set GPIO pin LOW (tell OS to shutdown)
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
}

void poweron() {
  digitalWrite(pin_output_power, HIGH);       // ENABLE POWER
}
