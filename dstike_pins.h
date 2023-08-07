/*
  Highlight LED on GPIO 16
  1x Neopixel (WS2812b) on GPIO 15 (D8)
  Integrated LiPo with charger
  ESP-07 with 1MByte flash
  Micro USB connector
  1.3" I2C OLED SH1106 on SDA = GPIO 5 (D1), SCL = GPIO 4 (D2)
  
  Button Up on GPIO 12
  Button Down on GPIO 13
  Button A on GPIO 14
  
  Side Buttons on GPIO 0 and GPIO 2
*/

// https://github.com/lspoplove/Deauther-Project/blob/master/Deauther-Wristband/Deauther_Wristband_v2_Cover_schematic.pdf
// https://github.com/kitesurfer1404/WS2812FX

// 
#define DSTIKE_BUTTON_UP_PIN         GPIO12
#define DSTIKE_BUTTON_DOWN_PIN       GPIO13
#define DSTIKE_BUTTON_SELECT_PIN     GPIO14

#define DSTIKE_BUTTON_1_PIN          GPIO0
#define DSTIKE_BUTTON_2_PIN          GPIO2

#define DSTIKE_LIGHT_1_PIN           GPIO16
#define DSTIKE_LIGHT_WS2812b_PIN     GPIO15

void inline dstike_pins_begin() {
  pinMode(DSTIKE_BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(DSTIKE_BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(DSTIKE_BUTTON_SELECT_PIN, INPUT_PULLUP);

  pinMode(DSTIKE_BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(DSTIKE_BUTTON_2_PIN, INPUT_PULLUP);

  pinMode(DSTIKE_LIGHT_1_PIN, OUTPUT);
  pinMode(DSTIKE_LIGHT_WS2812b_PIN, OUTPUT);
  
  //digitalWrite(LIGHT_1, LOW);
  digitalWrite(DSTIKE_LIGHT_1_PIN, HIGH);
  digitalWrite(DSTIKE_LIGHT_WS2812b_PIN, LOW);
}
