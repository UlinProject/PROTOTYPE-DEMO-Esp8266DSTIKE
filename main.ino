
// #UlinProject20

#include <WS2812FX.h>
#include <ESP8266WiFi.h>
#include "all_pins.h"
#include "dstike_pins.h"
#include "dstike_buttons.h"
#include "dstike_sleep.h"
#include <PubSubClient.h>
#include <ESP8266WebServer.h>

bool is_button1_up;
bool is_en_atmega;
bool is_warning;
bool is_update_warning;
bool is_update_warning_ignore_mqtt;
bool is_started;
bool is_update_emulator_mode;
int oldt_emulator_mode;
int c_check_connect;

WS2812FX ws2812fx = WS2812FX(
  1, DSTIKE_LIGHT_WS2812b_PIN, NEO_GRB + NEO_KHZ800
);

dstikeButton button_up;
dstikeButton button_down;
dstikeButton button_select;

dstikeButton button_1;
dstikeButton button_2;

#define idX_WARNING 2
#define idX_BPM 3
#define idX_SP02 1
#define mqttServer "10.42.0.1"
#define mqttPort 1883
#define mqttUser ""
#define mqttPassword ""
#define mqttin "domoticz/in"

#define MAX_INVALID_CONNECTS 6 *2 // 12s
#define SSID "RPI4"
#define PASSWORD "999999999"
#define HOSTNAME "PULSEUSER_001"

IPAddress ip(10,42,0,2);
IPAddress gateway(10,42,0,1);
IPAddress subnet(255,255,255,0);

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);

#define CHECK_MAX_INVALID_CONNECTS_TIME 500 // 500ms
#define MSG_BUFFER_SIZE  (60)
char mqttmsg[MSG_BUFFER_SIZE];

bool inline auto_mqtt_reconnect() {
  if (client.connected()) {
    return true;
  }
  client.connect(HOSTNAME, mqttUser, mqttPassword);
  yield();
  return client.connected();
}

bool mqtt_update_sp02(int value) {
  if (auto_mqtt_reconnect()) {
    yield();
    
    snprintf(mqttmsg, MSG_BUFFER_SIZE, "{\"idx\":%d,\"svalue\":\"%d\"}", idX_SP02, value);
    Serial.println(mqttmsg);
    if (!client.publish(mqttin, mqttmsg)) {
      return false;
    }
    return true;
  }
  return false;
}

bool mqtt_update_bpm(int value) {
  if (auto_mqtt_reconnect()) {
    yield();
    
    snprintf(mqttmsg, MSG_BUFFER_SIZE, "{\"idx\":%d,\"svalue\":\"%d\"}", idX_BPM, value);
    Serial.println(mqttmsg);
    if (!client.publish(mqttin, mqttmsg)) {
      return false;
    }
    return true;
  }
  return false;
}

bool mqtt_update_warning(bool value) {
  if(is_update_warning_ignore_mqtt) {
    is_update_warning_ignore_mqtt = false;
    return true;
  }
  if (auto_mqtt_reconnect()) {
    yield();

    if(value) {
      snprintf(mqttmsg, MSG_BUFFER_SIZE, "{\"idx\":%d,\"nvalue\":1}", idX_WARNING);
    }else {
      snprintf(mqttmsg, MSG_BUFFER_SIZE, "{\"idx\":%d,\"nvalue\":0}", idX_WARNING);
    }
    
    
    Serial.println(mqttmsg);
    if (!client.publish(mqttin, mqttmsg)) {
      return false;
    }
    return true;
  }
  return false;
}

void setup() {
  gpio_init();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  is_button1_up = false;
  is_en_atmega = true;

  is_warning = false;
  is_update_warning = true;
  is_update_warning_ignore_mqtt = false;
  is_started = false;
  is_update_started_mode= false;
  
  dstike_pins_begin();
  Serial.begin(115200);
  //Serial.setTimeout(2000);
  while(!Serial) {
    yield();
  }

  WiFi.hostname(HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(SSID, PASSWORD);
  WiFi.config(ip, gateway, subnet);
  c_check_connect = 0;
  while(WiFi.status() != WL_CONNECTED) {
    yield();
    digitalWrite(LED_BUILTIN, HIGH); // OFF
    yield();
    Serial.print(".");
    delay(CHECK_MAX_INVALID_CONNECTS_TIME/2);
    yield();
    c_check_connect = c_check_connect + 1;

    digitalWrite(LED_BUILTIN, LOW);
    yield();
    if (c_check_connect >= MAX_INVALID_CONNECTS) {
      ESP.restart();
      yield();
      return;
    }
    delay(CHECK_MAX_INVALID_CONNECTS_TIME/2);
    yield();
  }
  c_check_connect = 0;
  Serial.println("OK");
  Serial.println(WiFi.localIP());

  client.setServer(mqttServer, mqttPort);
  if(!auto_mqtt_reconnect()) {
    Serial.println("MQTT[INVALID_CONNECT]");
  }else {
    Serial.println("MQTT[OK]");
  }

  // BUTTONS
  init_dstike_button(&button_up,     DSTIKE_BUTTON_UP_PIN,      "UP");
  init_dstike_button(&button_down,   DSTIKE_BUTTON_DOWN_PIN,    "DOWN");
  init_dstike_button(&button_select, DSTIKE_BUTTON_SELECT_PIN,  "SELECT");
  
  init_dstike_button(&button_2,      DSTIKE_BUTTON_2_PIN,       "BUTTON2");
  init_dstike_button(&button_1,      DSTIKE_BUTTON_1_PIN,       "BUTTON1"); // SLEEP BUTTON

  set_up_dstike_button(&button_2, [](dstikeButton *button) {
    Serial.println("BUTTON2");
    digitalWrite(DSTIKE_LIGHT_1_PIN, LOW);
  });
  set_down_dstike_button(&button_2, [](dstikeButton *button, const bool is_long, uint8_t long_time) {
    if (is_long) {
      Serial.print("BUTTON2 DOWN LONG ");
    }else {
      Serial.print("BUTTON2 DOWN NOLONG ");
    }
    Serial.println(long_time);
    digitalWrite(DSTIKE_LIGHT_1_PIN, HIGH);
  });

  set_up_dstike_button(&button_select, [](dstikeButton *button) {
    is_started = !is_started;
    is_update_started_mode= true;
  });
  set_up_dstike_button(&button_up, [](dstikeButton *button) {
    is_warning = !is_warning;
    is_update_warning = true;
  });
  
  // leds
  ws2812fx.init();
  ws2812fx.setMode(FX_MODE_STATIC);
  //ws2812fx.setColor(0x040000); // 0xFFFFFF, 0xFF5900
  ws2812fx.setColor(120, 255, 0); // r g b
  ws2812fx.setSpeed(1000);
  ws2812fx.setBrightness(0);
  ws2812fx.stop();

  Serial.println("INIT END.");
  Serial.println("WAIT_BUTTON1.");
  //is_button1_up = true; // auto_sleep_mode
  digitalWrite(LED_BUILTIN, HIGH);

  delay(100);
  yield();
  
  set_up_dstike_button(&button_1, [](dstikeButton *button) {
    Serial.println("BUTTON1");
    is_button1_up = true;
  });

  server.on("/", []() {
    server.send(200, "text/plain", "");
  });
  server.on("/warn_on", []() {
    is_update_warning_ignore_mqtt = true;
    if(!is_warning) {
      is_warning = true;
      is_update_warning = true;
    
      server.send(200, "text/plain", "1");
    }else {
      server.send(200, "text/plain", "0");
    }
  });
  server.on("/warn_off", []() {
    is_update_warning_ignore_mqtt = true;
    if(is_warning) {
      is_warning = false;
      is_update_warning = true;
      
      server.send(200, "text/plain", "1");
    }else {
      server.send(200, "text/plain", "0");
    }
  });
  server.begin();
  oldt_started_mode= millis();
}

void init_button1() {
  init_dstike_button(&button_1,      DSTIKE_BUTTON_1_PIN,       "BUTTON1"); // SLEEP BUTTON
  set_up_dstike_button(&button_1, [](dstikeButton *button) {
    Serial.println("BUTTON1");
    is_button1_up = true;
  });
}

int sp02;
int bmp;
void loop() {
  if (is_update_warning) {
    is_update_warning = false;
    if (is_warning) {
      ws2812fx.start();
      ws2812fx.setColor(255, 0, 0); // r g b
      ws2812fx.setBrightness(20);
      ws2812fx.service();
    }else {
      ws2812fx.setBrightness(0);
      ws2812fx.service();
      ws2812fx.stop();
    }
    mqtt_update_warning(is_warning);
  }
  if (is_update_emulator_mode) {
    is_update_started_mode= false;
    oldt_started_mode= millis();

    if(!is_warning) {
      if(is_started) {
        ws2812fx.start();
        ws2812fx.setColor(0, 255, 0); // r g b
        ws2812fx.setBrightness(20);
        ws2812fx.service();
      }else {
        ws2812fx.setBrightness(0);
        ws2812fx.service();
        ws2812fx.stop();
      }
    }
  }else {
    if(!is_warning && is_started) {
      ws2812fx.start();
      ws2812fx.setColor(0, 255, 0); // r g b
      ws2812fx.setBrightness(20);
      ws2812fx.service();
    }
  }

  
  
  if (is_en_atmega == false) {
    while(is_en_atmega == false) {
      while (Serial.available() == 0) {
        yield();
      }
      //if(Serial.available()>0) {
      byte a = Serial.read();
      if (a == '!') {
        is_en_atmega = true;
        
        ws2812fx.start();
        ws2812fx.setColor(105, 92, 92);
        ws2812fx.setBrightness(125);
        ws2812fx.service();
        yield();
        delay(1000);
        ws2812fx.setBrightness(0);
        ws2812fx.service();
        ws2812fx.stop();
        yield();

        break;
      }
      //}
    }
  }else {
    while (Serial.available() > 0) {
      byte a = Serial.read();
      if(a == '[') {
        digitalWrite(DSTIKE_LIGHT_1_PIN, LOW);
        
        String data = "";
        bool is_continue = true;
        while (is_continue == true) {
          while (Serial.available() == 0) {
            yield();
          }
          a = Serial.read();
          
          while(is_continue == true) {
            while (Serial.available() == 0) {
              yield();
            }
            a = Serial.read();

            if(a == ']') {
              is_continue = false;
              break;
            }else {
              data = data + a;
            }
          }
        }
        if (data != "") {
          int int_data = data.toInt();
          if (int_data > 255) {
            int_data = 255;
          }
          ws2812fx.start();
          ws2812fx.setColor(int_data, 92, 92);
          ws2812fx.setBrightness(125);
          ws2812fx.service();
          yield();
          delay(1000);
          ws2812fx.setBrightness(0);
          ws2812fx.service();
          ws2812fx.stop();
          yield();
        }
      }
    }
    if (is_button1_up) {
      destruct_dstike_button(&button_1);
  
      ws2812fx.start();
      ws2812fx.setColor(205, 92, 92);
      ws2812fx.setBrightness(25);
      ws2812fx.service();
      yield();
      
      Serial.println("SLEEP_MODE, ON");
      Serial.flush();
      dstike_sleep_and_autocontinuecode();
      Serial.println("SLEEP_MODE, OFF");
  
      ws2812fx.setBrightness(0);
      ws2812fx.service();
      ws2812fx.stop();
  
      init_button1();
      is_button1_up = false;
    }
  }
  
  search_invalid_dstike_buttons();
  yield();
  ws2812fx.service();
  yield();
  server.handleClient();
  yield();
  

  if (is_started) {
    // CODE
  }
  
  delay(300);
}
