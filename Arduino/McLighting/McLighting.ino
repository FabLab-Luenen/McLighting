#include "definitions.h"
#include "version.h"

// ***************************************************************************
// Load libraries for: WebServer / WiFiManager / WebSockets
// ***************************************************************************
#include <ESP8266WiFi.h>           //https://github.com/esp8266/Arduino

// needed for library WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>           //https://github.com/tzapu/WiFiManager

#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <EEPROM.h>

#include <WebSockets.h>            //https://github.com/Links2004/arduinoWebSockets
#include <WebSocketsServer.h>
  
#ifdef ENABLE_BUTTON_GY33
  // needed for MCU
  #include <GY33_MCU.h>            //https://github.com/FabLab-Luenen/GY33_MCU/archive/master.zip ; //https://github.com/pasko-zh/brzo_i2c
  // ***************************************************************************
  // Initialize Color Sensor
  // ***************************************************************************
  GY33_MCU tcs;
#endif

// OTA
#ifdef ENABLE_OTA
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>
#endif

#include <ArduinoJson.h>         //https://github.com/bblanchon/ArduinoJson

// MQTT
#ifdef ENABLE_MQTT
  #include <PubSubClient.h>
  WiFiClient espClient;
  PubSubClient mqtt_client(espClient);
#endif

#ifdef ENABLE_AMQTT
  #include <AsyncMqttClient.h>     //https://github.com/marvinroger/async-mqtt-client
                                   //https://github.com/me-no-dev/ESPAsyncTCP
  AsyncMqttClient amqttClient;
  WiFiEventHandler wifiConnectHandler;
  WiFiEventHandler wifiDisconnectHandler;
#endif

#ifdef ARDUINOJSON_VERSION
  #if !(ARDUINOJSON_VERSION_MAJOR == 6 and ARDUINOJSON_VERSION_MINOR == 7)
    #error "Install ArduinoJson v6.7.0-beta"
  #endif
#endif

#ifdef ENABLE_E131
  #include <ESPAsyncUDP.h>         //https://github.com/me-no-dev/ESPAsyncUDP
  #include <ESPAsyncE131.h>        //https://github.com/forkineye/ESPAsyncE131
  ESPAsyncE131 e131(END_UNIVERSE - START_UNIVERSE + 1);
#endif

#ifdef ENABLE_REMOTE
  #include <IRremoteESP8266.h>       //https://github.com/markszabo/IRremoteESP8266
  #include <IRrecv.h>
  #include <IRutils.h>
#endif
// ***************************************************************************
// Instanciate HTTP(80) / WebSockets(81) Server
// ***************************************************************************
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

#ifdef HTTP_OTA
#include <ESP8266HTTPUpdateServer.h>
ESP8266HTTPUpdateServer httpUpdater;
#endif

// ***************************************************************************
// Load libraries / Instanciate WS2812FX library
// ***************************************************************************

#include <WS2812FX.h>              // https://github.com/kitesurfer1404/WS2812FX

  #ifdef RGBW
    WS2812FX strip = WS2812FX(NUMLEDS, PIN, NEO_GRBW + NEO_KHZ800);
  #else
    WS2812FX strip = WS2812FX(NUMLEDS, PIN, NEO_GRB + NEO_KHZ800);
  #endif

const uint8_t ws2812fx_options = SIZE_SMALL + FADE_MEDIUM; // WS2812FX setSegment OPTIONS, see: https://github.com/kitesurfer1404/WS2812FX/blob/master/extras/WS2812FX%20Users%20Guide.md

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

#ifdef USE_WS2812FX_DMA // Uses GPIO3/RXD0/RX, more info: https://github.com/Makuna/NeoPixelBus/wiki/ESP8266-NeoMethods
  #include <NeoPixelBus.h>
  #ifdef RGBW
    NeoEsp8266Dma800KbpsMethod dma = NeoEsp8266Dma800KbpsMethod(NUMLEDS, 4);  //800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
  #else
    NeoEsp8266Dma800KbpsMethod dma = NeoEsp8266Dma800KbpsMethod(NUMLEDS, 3);  //800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
  #endif
  //NeoEsp8266Dma400KbpsMethod dma = NeoEsp8266Dma400KbpsMethod(NUMLEDS, 3);  //400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
#endif
#ifdef USE_WS2812FX_UART1 // Uses UART1: GPIO1/TXD0/TX, more info: https://github.com/Makuna/NeoPixelBus/wiki/ESP8266-NeoMethods
  #include <NeoPixelBus.h>
  #ifdef RGBW
    NeoEsp8266Uart0800KbpsMethod dma = NeoEsp8266Uart0800KbpsMethod(NUMLEDS, 4);
  #else
    NeoEsp8266Uart0800KbpsMethod dma = NeoEsp8266Uart0800KbpsMethod(NUMLEDS, 3);
  #endif
#endif
#ifdef USE_WS2812FX_UART2 // Uses UART2: GPIO2/TXD1/D4, more info: https://github.com/Makuna/NeoPixelBus/wiki/ESP8266-NeoMethods
  #include <NeoPixelBus.h>
  #ifdef RGBW
    NeoEsp8266Uart1800KbpsMethod dma = NeoEsp8266Uart1800KbpsMethod(NUMLEDS, 4);
  #else
    NeoEsp8266Uart1800KbpsMethod dma = NeoEsp8266Uart1800KbpsMethod(NUMLEDS, 3);
  #endif
#endif
#if defined(USE_WS2812FX_DMA) or defined(USE_WS2812FX_UART1) or defined(USE_WS2812FX_UART2)
  void DMA_Show(void) {
    if(dma.IsReadyToUpdate()) {
      memcpy(dma.getPixels(), strip.getPixels(), dma.getPixelsSize());
      dma.Update();
    }
  }
#endif

// ***************************************************************************
// Load library "ticker" for blinking status led
// ***************************************************************************
#include <Ticker.h>
Ticker ticker;
#ifdef ENABLE_HOMEASSISTANT
  Ticker ha_send_data;
#endif
#ifdef ENABLE_AMQTT
  Ticker mqttReconnectTimer;
  Ticker wifiReconnectTimer;
#endif

Ticker settings_save_state;

void tick()
{
  //toggle state
  int state = digitalRead(LED_BUILTIN);  // get the current state of GPIO1 pin
  digitalWrite(LED_BUILTIN, !state);     // set pin to the opposite state
}

#ifdef ENABLE_STATE_SAVE_EEPROM
  // ***************************************************************************
  // EEPROM helper
  // ***************************************************************************
  String readEEPROM(int offset, int len) {
    String res = "";
    for (int i = 0; i < len; ++i)
    {
      res += char(EEPROM.read(i + offset));
      //DBG_OUTPUT_PORT.println(char(EEPROM.read(i + offset)));
    }
    DBG_OUTPUT_PORT.printf("readEEPROM(): %s\n", res.c_str());
    return res;
  }
  
  void writeEEPROM(int offset, int len, String value) {
    DBG_OUTPUT_PORT.printf("writeEEPROM(): %s\n", value.c_str());
    for (int i = 0; i < len; ++i)
    {
      if (i < value.length()) {
        EEPROM.write(i + offset, value[i]);
      } else {
        EEPROM.write(i + offset, 0);
      }
    }
  }
#endif

#ifdef ENABLE_REMOTE
  IRrecv irrecv(ENABLE_REMOTE);
  decode_results results;
  
#endif

// ***************************************************************************
// Saved state handling
// ***************************************************************************
// https://stackoverflow.com/questions/9072320/split-string-into-string-array
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// ***************************************************************************
// Callback for WiFiManager library when config mode is entered
// ***************************************************************************
//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  DBG_OUTPUT_PORT.println("Entered config mode");
  DBG_OUTPUT_PORT.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  DBG_OUTPUT_PORT.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);

  uint16_t i;
  for (i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0, 0, 0, 255);
  }
  strip.show();
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  DBG_OUTPUT_PORT.println("Should save config");
  shouldSaveConfig = true;
}

// ***************************************************************************
// Include: Webserver
// ***************************************************************************
#include "spiffs_webserver.h"

// ***************************************************************************
// Include: Request handlers
// ***************************************************************************
#include "request_handlers.h"

// ***************************************************************************
// Include: Color modes
// ***************************************************************************
#ifdef ENABLE_TV
  #include "colormodes.h"
#endif

// ***************************************************************************
// MAIN
// ***************************************************************************
void setup() {
//  system_update_cpu_freq(160);

  DBG_OUTPUT_PORT.begin(115200);
  EEPROM.begin(512);

  // set builtin led pin as output
  pinMode(LED_BUILTIN, OUTPUT);
  // button pin setup
#ifdef ENABLE_BUTTON
  pinMode(ENABLE_BUTTON,INPUT_PULLUP);
#endif

#ifdef ENABLE_BUTTON_GY33
  pinMode(ENABLE_BUTTON_GY33, INPUT_PULLUP);
  if (tcs.begin()) {
    DBG_OUTPUT_PORT.println("Found GY-33 sensor");
  } else {
    DBG_OUTPUT_PORT.println("No GY33 sensor found ... check your connections");
  }
#endif

DBG_OUTPUT_PORT.println("");
DBG_OUTPUT_PORT.println("Starting....");

  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.5, tick);

  // ***************************************************************************
  // Setup: SPIFFS
  // ***************************************************************************
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }

    FSInfo fs_info;
    SPIFFS.info(fs_info);
    DBG_OUTPUT_PORT.printf("FS Usage: %d/%d bytes\n\n", fs_info.usedBytes, fs_info.totalBytes);
  }

  wifi_station_set_hostname(const_cast<char*>(HOSTNAME));

  // ***************************************************************************
  // Setup: Neopixel
  // ***************************************************************************
  strip.init();
  #if defined(USE_WS2812FX_DMA) or defined(USE_WS2812FX_UART)
    dma.Initialize();
    strip.setCustomShow(DMA_Show);
  #endif
  strip.setBrightness(brightness);
// parameters: index, start, stop, mode, color, speed, options
  strip.setSegment(0,  0,  NUMLEDS-1, FX_MODE_COMET, hex_colors, convertSpeed(ws2812fx_speed), ws2812fx_options);
  strip.start();

  // ***************************************************************************
  // Setup: WiFiManager
  // ***************************************************************************
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  #if defined(ENABLE_MQTT) or defined(ENABLE_AMQTT)
    #if defined(ENABLE_STATE_SAVE_SPIFFS) and (defined(ENABLE_MQTT) or defined(ENABLE_AMQTT))
      (readConfigFS()) ? DBG_OUTPUT_PORT.println("WiFiManager config FS Read success!"): DBG_OUTPUT_PORT.println("WiFiManager config FS Read failure!");
    #else
      String settings_available = readEEPROM(134, 1);
      if (settings_available == "1") {
        readEEPROM(0, 64).toCharArray(mqtt_host, 64);   // 0-63
        readEEPROM(64, 6).toCharArray(mqtt_port, 6);    // 64-69
        readEEPROM(70, 32).toCharArray(mqtt_user, 32);  // 70-101
        readEEPROM(102, 32).toCharArray(mqtt_pass, 32); // 102-133
        DBG_OUTPUT_PORT.printf("MQTT host: %s\n", mqtt_host);
        DBG_OUTPUT_PORT.printf("MQTT port: %s\n", mqtt_port);
        DBG_OUTPUT_PORT.printf("MQTT user: %s\n", mqtt_user);
        DBG_OUTPUT_PORT.printf("MQTT pass: %s\n", mqtt_pass);
      }
    #endif
    WiFiManagerParameter custom_mqtt_host("host", "MQTT hostname", mqtt_host, 64);
    WiFiManagerParameter custom_mqtt_port("port", "MQTT port", mqtt_port, 6);
    WiFiManagerParameter custom_mqtt_user("user", "MQTT user", mqtt_user, 32);
    WiFiManagerParameter custom_mqtt_pass("pass", "MQTT pass", mqtt_pass, 32);
  #endif

  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  #if defined(ENABLE_MQTT) or defined(ENABLE_AMQTT)
    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    //add all your parameters here
    wifiManager.addParameter(&custom_mqtt_host);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_pass);
  #endif

  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  
  // Uncomment if you want to restart ESP8266 if it cannot connect to WiFi.
  // Value in brackets is in seconds that WiFiManger waits until restart
  #ifdef WIFIMGR_PORTAL_TIMEOUT
  wifiManager.setConfigPortalTimeout(WIFIMGR_PORTAL_TIMEOUT);
  #endif

  // Uncomment if you want to set static IP 
  // Order is: IP, Gateway and Subnet 
  #ifdef WIFIMGR_SET_MANUAL_IP
  wifiManager.setSTAStaticIPConfig(IPAddress(_ip[0], _ip[1], _ip[2], _ip[3]), IPAddress(_gw[0], _gw[1], _gw[2], _gw[3]), IPAddress(_sn[0], _sn[1], _sn[2], _sn[3]));
  #endif

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(HOSTNAME)) {
    DBG_OUTPUT_PORT.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();  //Will be removed when upgrading to standalone offline McLightingUI version
    delay(1000);  //Will be removed when upgrading to standalone offline McLightingUI version
  }

  #if defined(ENABLE_MQTT) or defined(ENABLE_AMQTT)
    //read updated parameters
    strcpy(mqtt_host, custom_mqtt_host.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_pass, custom_mqtt_pass.getValue());

    //save the custom parameters to FS
    #if defined(ENABLE_STATE_SAVE_SPIFFS) and (defined(ENABLE_MQTT) or defined(ENABLE_AMQTT))
      (writeConfigFS(shouldSaveConfig)) ? DBG_OUTPUT_PORT.println("WiFiManager config FS Save success!"): DBG_OUTPUT_PORT.println("WiFiManager config FS Save failure!");
    #endif
    #if defined(ENABLE_STATE_SAVE_EEPROM)
      if (shouldSaveConfig) {
        DBG_OUTPUT_PORT.println("Saving WiFiManager config");

        writeEEPROM(0, 64, mqtt_host);   // 0-63
        writeEEPROM(64, 6, mqtt_port);   // 64-69
        writeEEPROM(70, 32, mqtt_user);  // 70-101
        writeEEPROM(102, 32, mqtt_pass); // 102-133
        writeEEPROM(134, 1, "1");        // 134 --> always "1"
        EEPROM.commit();
      }
    #endif
  #endif
  
  #ifdef ENABLE_AMQTT
    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  #endif

  //if you get here you have connected to the WiFi
  DBG_OUTPUT_PORT.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(LED_BUILTIN, LOW);
  //switch LED off
  //digitalWrite(LED_BUILTIN, HIGH);

  // ***************************************************************************
  // Configure OTA
  // ***************************************************************************
  #ifdef ENABLE_OTA
    DBG_OUTPUT_PORT.println("Arduino OTA activated.");

    // Port defaults to 8266
    ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname(HOSTNAME);

    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA.onStart([]() {
      DBG_OUTPUT_PORT.println("Arduino OTA: Start updating");
    });
    ArduinoOTA.onEnd([]() {
      DBG_OUTPUT_PORT.println("Arduino OTA: End");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      DBG_OUTPUT_PORT.printf("Arduino OTA Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      DBG_OUTPUT_PORT.printf("Arduino OTA Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) DBG_OUTPUT_PORT.println("Arduino OTA: Auth Failed");
      else if (error == OTA_BEGIN_ERROR) DBG_OUTPUT_PORT.println("Arduino OTA: Begin Failed");
      else if (error == OTA_CONNECT_ERROR) DBG_OUTPUT_PORT.println("Arduino OTA: Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) DBG_OUTPUT_PORT.println("Arduino OTA: Receive Failed");
      else if (error == OTA_END_ERROR) DBG_OUTPUT_PORT.println("Arduino OTA: End Failed");
    });

    ArduinoOTA.begin();
    DBG_OUTPUT_PORT.println("");
  #endif


  // ***************************************************************************
  // Configure MQTT
  // ***************************************************************************
  #ifdef ENABLE_MQTT_HOSTNAME_CHIPID
    snprintf(mqtt_clientid, 64, "%s-%08X", HOSTNAME, ESP.getChipId());
  #endif

  #ifdef ENABLE_MQTT
    if (mqtt_host != "" && atoi(mqtt_port) > 0) {
      snprintf(mqtt_intopic, sizeof mqtt_intopic, "%s/in", HOSTNAME);
      snprintf(mqtt_outtopic, sizeof mqtt_outtopic, "%s/out", HOSTNAME);

      DBG_OUTPUT_PORT.printf("MQTT active: %s:%d\n", mqtt_host, String(mqtt_port).toInt());

      mqtt_client.setServer(mqtt_host, atoi(mqtt_port));
      mqtt_client.setCallback(mqtt_callback);
    }
  #endif

  #ifdef ENABLE_AMQTT
    if (mqtt_host != "" && atoi(mqtt_port) > 0) {
      amqttClient.onConnect(onMqttConnect);
      amqttClient.onDisconnect(onMqttDisconnect);
      amqttClient.onMessage(onMqttMessage);
      amqttClient.setServer(mqtt_host, atoi(mqtt_port));
      if (mqtt_user != "" or mqtt_pass != "") amqttClient.setCredentials(mqtt_user, mqtt_pass);
      amqttClient.setClientId(mqtt_clientid);

      connectToMqtt();
    }
  #endif

  // #ifdef ENABLE_HOMEASSISTANT
  //   ha_send_data.attach(5, tickerSendState); // Send HA data back only every 5 sec
  // #endif

  // ***************************************************************************
  // Setup: MDNS responder
  // ***************************************************************************
  bool mdns_result = MDNS.begin(HOSTNAME);

  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(WiFi.localIP());
  DBG_OUTPUT_PORT.println("/ to open McLighting.");

  DBG_OUTPUT_PORT.print("Use http://");
  DBG_OUTPUT_PORT.print(HOSTNAME);
  DBG_OUTPUT_PORT.println(".local/ when you have Bonjour installed.");

  DBG_OUTPUT_PORT.print("New users: Open http://");
  DBG_OUTPUT_PORT.print(WiFi.localIP());
  DBG_OUTPUT_PORT.println("/upload to upload the webpages first.");

  DBG_OUTPUT_PORT.println("");


  // ***************************************************************************
  // Setup: WebSocket server
  // ***************************************************************************
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // ***************************************************************************
  // Setup: SPIFFS Webserver handler
  // ***************************************************************************
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/esp_status", HTTP_GET, []() {
    DynamicJsonDocument jsonBuffer;
    JsonObject json = jsonBuffer.to<JsonObject>();
    json["HOSTNAME"] = HOSTNAME;
    json["version"] = SKETCH_VERSION;
    json["heap"] = ESP.getFreeHeap();
    json["sketch_size"] = ESP.getSketchSize();
    json["free_sketch_space"] = ESP.getFreeSketchSpace();
    json["flash_chip_size"] = ESP.getFlashChipSize();
    json["flash_chip_real_size"] = ESP.getFlashChipRealSize();
    json["flash_chip_speed"] = ESP.getFlashChipSpeed();
    json["sdk_version"] = ESP.getSdkVersion();
    json["core_version"] = ESP.getCoreVersion();
    json["cpu_freq"] = ESP.getCpuFreqMHz();
    json["chip_id"] = ESP.getFlashChipId();
    #if defined(USE_WS2812FX_DMA)
      json["animation_lib"] = "WS2812FX_DMA";
      json["ws2812_pin"] = 3;
    #elif defined(USE_WS2812FX_UART1)
      json["animation_lib"] = "WS2812FX_UART1";
      json["ws2812_pin"] = 2;
    #elif defined(USE_WS2812FX_UART2)
      json["animation_lib"] = "WS2812FX_UART2";
      json["ws2812_pin"] = 1;
    #else
      json["animation_lib"] = "WS2812FX";
      json["ws2812_pin"] = PIN;
    #endif
    json["number_leds"] = NUMLEDS;
    #ifdef RGBW
      json["rgbw_mode"] = "ON";
    #else
      json["rgbw_mode"] = "OFF";
    #endif
    #ifdef ENABLE_BUTTON
      json["button_mode"] = "ON";
      json["button_pin"] = ENABLE_BUTTON;
    #else
      json["button_mode"] = "OFF";
    #endif
    #ifdef ENABLE_BUTTON_GY33
      json["button_gy33"] = "ON";
      json["gy33_pin"] = ENABLE_BUTTON_GY33;
    #else
      json["button_gy33"] = "OFF";
    #endif
    #ifdef ENABLE_REMOTE
      json["ir_remote"] = "ON";
      json["tsop_ir_pin"] = ENABLE_REMOTE;
    #else
      json["ir_remote"] = "OFF";
    #endif
    #ifdef ENABLE_AMQTT
      json["amqtt"] = "ON";
    #endif
    #ifdef ENABLE_MQTT
      json["mqtt"] = "ON";
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      json["home_assistant"] = "ON";
    #else
      json["home_assistant"] = "OFF";
    #endif
    #ifdef ENABLE_LEGACY_ANIMATIONS
      json["legacy_animations"] = "ON";
    #else
      json["legacy_animations"] = "OFF";
    #endif
    #ifdef ENABLE_TV
      json["tv_animation"] = "ON";
    #else
      json["tv_animation"] = "OFF";
    #endif
    #ifdef ENABLE_E131
      json["e131_animations"] = "ON";
    #else
      json["e131_animations"] = "OFF";
    #endif
    #ifdef HTTP_OTA
      json["esp8266_http_updateserver"] = "ON";
    #endif
    #ifdef ENABLE_OTA
      json["arduino_ota"] = "ON";
    #endif
    #ifdef ENABLE_STATE_SAVE_SPIFFS
      json["state_save"] = "SPIFFS";
    #endif
    #ifdef ENABLE_STATE_SAVE_EEPROM
      json["state_save"] = "EEPROM";
    #endif
    
    String json_str;
    serializeJson(json, json_str);
    jsonBuffer.clear();
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json_str);
  });


  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      handleNotFound();
  });

  server.on("/upload", handleMinimalUpload);

  server.on("/restart", []() {
    DBG_OUTPUT_PORT.printf("/restart\n");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "restarting..." );
    ESP.restart();
  });

  server.on("/reset_wlan", []() {
    DBG_OUTPUT_PORT.printf("/reset_wlan\n");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Resetting WLAN and restarting..." );
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  });

  server.on("/start_config_ap", []() {
    DBG_OUTPUT_PORT.printf("/start_config_ap\n");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Starting config AP ..." );
    WiFiManager wifiManager;
    wifiManager.startConfigPortal(HOSTNAME);
  });

  server.on("/format_spiffs", []() {
    DBG_OUTPUT_PORT.printf("/format_spiffs\n");
    server.send(200, "text/plain", "Formatting SPIFFS ..." );
    SPIFFS.format();
    server.send(200, "text/plain", "Formatting SPIFFS complete" );
  });

  // ***************************************************************************
  // Setup: SPIFFS Webserver handler
  // ***************************************************************************
  server.on("/set_brightness", []() {
    getArgs();
    mode = SET_BRIGHTNESS;
    #ifdef ENABLE_MQTT
    mqtt_client.publish(mqtt_outtopic, String(String("OK %") + String(brightness)).c_str());
    #endif
    #ifdef ENABLE_AMQTT
    amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String(String("OK %") + String(brightness)).c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      stateOn = true;
      if(!ha_send_data.active())  ha_send_data.once(5, tickerSendState);
    #endif
    getStatusJSON();
  });

  server.on("/get_brightness", []() {
    String str_brightness = String((int) (brightness / 2.55));
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", str_brightness );
    DBG_OUTPUT_PORT.print("/get_brightness: ");
    DBG_OUTPUT_PORT.println(str_brightness);
  });

  server.on("/set_speed", []() {
    getArgs();
    mode = SET_SPEED;
    #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String(String("OK ?") + String(ws2812fx_speed)).c_str());
    #endif
    #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String(String("OK ?") + String(ws2812fx_speed)).c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      if(!ha_send_data.active())  ha_send_data.once(5, tickerSendState);
    #endif
    getStatusJSON();
  });

  server.on("/get_speed", []() {
    String str_speed = String(ws2812fx_speed);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", str_speed );
    DBG_OUTPUT_PORT.print("/get_speed: ");
    DBG_OUTPUT_PORT.println(str_speed);
  });

  server.on("/get_switch", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", (mode == OFF) ? "0" : "1" );
    DBG_OUTPUT_PORT.printf("/get_switch: %s\n", (mode == OFF) ? "0" : "1");
  });

  server.on("/get_color", []() {
    char rgbcolor[9];
    snprintf(rgbcolor, sizeof(rgbcolor), "%02X%02X%02X%02X", main_color.white, main_color.red, main_color.green, main_color.blue);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", rgbcolor );
    DBG_OUTPUT_PORT.print("/get_color: ");
    DBG_OUTPUT_PORT.println(rgbcolor);
  });
  
  server.on("/get_color2", []() {
    char rgbcolor[9];
    snprintf(rgbcolor, sizeof(rgbcolor), "%02X%02X%02X%02X", back_color.white, back_color.red, back_color.green, back_color.blue);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", rgbcolor );
    DBG_OUTPUT_PORT.print("/get_color2: ");
    DBG_OUTPUT_PORT.println(rgbcolor);
  });

  server.on("/get_color3", []() {
    char rgbcolor[9];
    snprintf(rgbcolor, sizeof(rgbcolor), "%02X%02X%02X%02X", xtra_color.white, xtra_color.red, xtra_color.green, xtra_color.blue);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", rgbcolor );
    DBG_OUTPUT_PORT.print("/get_color3: ");
    DBG_OUTPUT_PORT.println(rgbcolor);
  });


  server.on("/status", []() {
    getStatusJSON();
  });

  server.on("/off", []() {
    mode = OFF;
    //getArgs();
    getStatusJSON();
    #ifdef ENABLE_MQTT
    mqtt_client.publish(mqtt_outtopic, String("OK =off").c_str());
    #endif
    #ifdef ENABLE_AMQTT
    amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String("OK =off").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      stateOn = false;
    #endif
  });

    server.on("/auto", []() {
    handleAutoStart();
    getStatusJSON();
    #ifdef ENABLE_MQTT
    mqtt_client.publish(mqtt_outtopic, String("OK =auto").c_str());
    #endif
    #ifdef ENABLE_AMQTT
    amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String("OK =auto").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      stateOn = false;
    #endif
  });

  server.on("/all", []() {
    getArgs();
    ws2812fx_mode = FX_MODE_STATIC;
    mode = SET_ALL;
    getStatusJSON();
    #ifdef ENABLE_MQTT
    mqtt_client.publish(mqtt_outtopic, String("OK =all").c_str());
    #endif
    #ifdef ENABLE_AMQTT
    amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String("OK =all").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      stateOn = true;
    #endif
  });

  #ifdef ENABLE_LEGACY_ANIMATIONS
    server.on("/wipe", []() {
      getArgs();
      ws2812fx_mode = FX_MODE_COLOR_WIPE;
      mode = SET_ALL;
      getStatusJSON();
    #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =wipe").c_str());
    #endif
    #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String("OK =wipe").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
    #endif
    });
  
    server.on("/rainbow", []() {
      getArgs();
      ws2812fx_mode = FX_MODE_RAINBOW;
      mode = SET_ALL;
      getStatusJSON();
    #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =rainbow").c_str());
    #endif
    #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String("OK =rainbow").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
    #endif
    });
  
    server.on("/rainbowCycle", []() {
      getArgs();
      ws2812fx_mode = FX_MODE_RAINBOW_CYCLE;
      mode = SET_ALL;
      getStatusJSON();
    #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =rainbowCycle").c_str());
    #endif
    #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String("OK =rainbowCycle").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      stateOn = true;
    #endif
    });
  
    server.on("/theaterchase", []() {
      getArgs();
      ws2812fx_mode = FX_MODE_THEATER_CHASE;
      mode = SET_ALL;
      getStatusJSON();
    #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =theaterchase").c_str());
    #endif
    #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String("OK =theaterchase").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      stateOn = true;
    #endif
    });
  
    server.on("/twinkleRandom", []() {
      getArgs();
      ws2812fx_mode = FX_MODE_TWINKLE_RANDOM;
      mode = SET_ALL;
      getStatusJSON();
    #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =twinkleRandom").c_str());
    #endif
    #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String("OK =twinkleRandom").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      stateOn = true;
    #endif
    });
    
    server.on("/theaterchaseRainbow", []() {
      getArgs();
      ws2812fx_mode = FX_MODE_THEATER_CHASE_RAINBOW;
      mode = SET_ALL;
      getStatusJSON();
    #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =theaterchaseRainbow").c_str());
    #endif
    #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String("OK =theaterchaseRainbow").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
    #endif
    });
  #endif
  
  #ifdef ENABLE_E131
    server.on("/e131", []() {
      mode = E131;
      getStatusJSON();
    #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =e131").c_str());
    #endif
    #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String("OK =e131").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
    #endif
    });
  #endif
  
  #ifdef ENABLE_TV  
    server.on("/tv", []() {
    mode = TV;
    getStatusJSON();
    #ifdef ENABLE_MQTT
      mqtt_client.publish(mqtt_outtopic, String("OK =tv").c_str());
    #endif
    #ifdef ENABLE_AMQTT
      amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String("OK =tv").c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
        stateOn = true;
    #endif
    });
  #endif

  server.on("/get_modes", []() {
    getModesJSON();
  });

  server.on("/set_mode", []() {
    getArgs();
    mode = SET_MODE;
    getStatusJSON();

    #ifdef ENABLE_MQTT
    mqtt_client.publish(mqtt_outtopic, String(String("OK /") + String(ws2812fx_mode)).c_str());
    #endif
    #ifdef ENABLE_AMQTT
    amqttClient.publish(mqtt_outtopic.c_str(), qospub, false, String(String("OK /") + String(ws2812fx_mode)).c_str());
    #endif
    #ifdef ENABLE_HOMEASSISTANT
      stateOn = true;
      if(!ha_send_data.active())  ha_send_data.once(5, tickerSendState);
    #endif
  });

  #ifdef HTTP_OTA
    httpUpdater.setup(&server, "/update");
  #endif

  server.begin();

  // Start MDNS service
  if (mdns_result) {
    MDNS.addService("http", "tcp", 80);
  }

  #ifdef ENABLE_E131
  // Choose one to begin listening for E1.31 data
  // if (e131.begin(E131_UNICAST))                             // Listen via Unicast
  if (e131.begin(E131_MULTICAST, START_UNIVERSE, END_UNIVERSE)) // Listen via Multicast
      DBG_OUTPUT_PORT.println(F("Listening for data..."));
  else
      DBG_OUTPUT_PORT.println(F("*** e131.begin failed ***"));
  #endif
  
  #ifdef ENABLE_STATE_SAVE_SPIFFS
    (readStateFS()) ? DBG_OUTPUT_PORT.println(" Success!") : DBG_OUTPUT_PORT.println(" Failure!");
  #endif
  
  #ifdef ENABLE_STATE_SAVE_EEPROM
    // Load state string from EEPROM
    String saved_state_string = readEEPROM(256, 66);
    String chk = getValue(saved_state_string, '|', 0);
    if (chk == "STA") {
      DBG_OUTPUT_PORT.printf("Found saved state: %s\n", saved_state_string.c_str());
      setModeByStateString(saved_state_string);
    }
  #endif
  #ifdef ENABLE_BUTTON_GY33
    tcs.setConfig(MCU_LED_06, MCU_WHITE_ON);
//    delay(2000);
//    tcs.setConfig(MCU_LED_OFF, MCU_WHITE_OFF);
  #endif

  #ifdef ENABLE_REMOTE
    irrecv.enableIRIn();  // Start the receiver
    sprintf(last_state, "STA|%2d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d", prevmode, strip.getMode(), ws2812fx_speed, brightness, main_color.red, main_color.green, main_color.blue, main_color.white, back_color.red, back_color.green, back_color.blue, back_color.white, xtra_color.red, xtra_color.green, xtra_color.blue,xtra_color.white);
  #endif
}

void loop() {
  #ifdef ENABLE_BUTTON
    button();
  #endif

  #ifdef ENABLE_BUTTON_GY33
    button_gy33();
  #endif 

  server.handleClient();
  webSocket.loop();

  #ifdef ENABLE_OTA
    ArduinoOTA.handle();
  #endif

  #ifdef ENABLE_MQTT
    if (WiFi.status() != WL_CONNECTED) {
      #ifdef ENABLE_HOMEASSISTANT
         ha_send_data.detach();
      #endif
      DBG_OUTPUT_PORT.println("WiFi disconnected, reconnecting!");
      WiFi.disconnect();
      WiFi.setSleepMode(WIFI_NONE_SLEEP);
      WiFi.mode(WIFI_STA);
      WiFi.begin();
    } else {
      if (mqtt_host != "" && String(mqtt_port).toInt() > 0 && mqtt_reconnect_retries < MQTT_MAX_RECONNECT_TRIES) {
        if (!mqtt_client.connected()) {
          #ifdef ENABLE_HOMEASSISTANT
           ha_send_data.detach();
          #endif
          DBG_OUTPUT_PORT.println("MQTT disconnected, reconnecting!");
          mqtt_reconnect();
        } else {
          mqtt_client.loop();
        }
      }
    }
  #endif
  #ifdef ENABLE_HOMEASSISTANT
//   if(!ha_send_data.active())  ha_send_data.once(5, tickerSendState);
   if (new_ha_mqtt_msg) sendState();
  #endif
          
  // Simple statemachine that handles the different modes
  if (mode != OFF) {
    if(!strip.isRunning()) strip.start();
  } 
   
  // Only for modes with WS2812FX functionality
  if (( mode == AUTO)|| (mode == HOLD)) {
    strip.service();
  }
  
  if ((prevmode == AUTO) && (mode != AUTO)) { handleAutoStop(); }
  
  if (mode == OFF) {
    if(strip.isRunning()) {
      strip.strip_off();         // Workaround: to be shure,
      delay(10);                 // that strip is really off. Sometimes strip.stop isn't enought
      strip.stop();              // should clear memory
    }
  }
  
  #ifdef ENABLE_TV
    if (mode == TV) {
      tv();
    }
  #endif
  
  #ifdef ENABLE_E131
    if (mode == E131) {
      handleE131();
    }
  #endif
  
  if (mode == SET_ALL) {
    strip.setMode(ws2812fx_mode);
    convertColors();
    strip.setColors(0, hex_colors);
    strip.setSpeed(convertSpeed(ws2812fx_speed));
    strip.setBrightness(brightness);
    prevmode = SET_ALL;
    mode = HOLD;
    strip.trigger();
  }  
  
  if (mode == SET_MODE) {
    strip.setMode(ws2812fx_mode);
    prevmode = SET_MODE;
    mode = HOLD;
    strip.trigger();
  }
    
  if (mode == SET_COLOR) {
    convertColors();
    strip.setColors(0, hex_colors);
    mode = prevmode;
    prevmode = SET_COLOR;
    if (mode == HOLD) strip.trigger();
  }
  if (mode == SET_SPEED) {
    strip.setSpeed(convertSpeed(ws2812fx_speed));
    mode = prevmode;
    prevmode = SET_SPEED;
    if (mode == HOLD) strip.trigger();
  }
  if (mode == SET_BRIGHTNESS) {
    strip.setBrightness(brightness);
    mode = prevmode;
    prevmode = SET_BRIGHTNESS;
    if (mode == HOLD) strip.trigger();
  }
 
  if (prevmode != mode) {
    if (mode != AUTO) {  // do not save if AUTO Mode is set
      if(!settings_save_state.active()) settings_save_state.once(5, tickerSaveState);
      sprintf(last_state, "STA|%2d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d|%3d", prevmode, strip.getMode(), ws2812fx_speed, brightness, main_color.red, main_color.green, main_color.blue, main_color.white, back_color.red, back_color.green, back_color.blue, back_color.white, xtra_color.red, xtra_color.green, xtra_color.blue,xtra_color.white);
    }
  }
  if (updateState){
  #ifdef ENABLE_STATE_SAVE_SPIFFS
      (writeStateFS()) ? DBG_OUTPUT_PORT.println(" Success!") : DBG_OUTPUT_PORT.println(" Failure!");
  #endif
  #ifdef ENABLE_STATE_SAVE_EEPROM
      writeEEPROM(256, 66, last_state); // 256 --> last_state (reserved 66 bytes)
      EEPROM.commit();
      updateState = false;
      settings_save_state.detach();
  #endif
  }
  prevmode = mode;
  #ifdef ENABLE_REMOTE
    handleRemote();
  #endif
}
