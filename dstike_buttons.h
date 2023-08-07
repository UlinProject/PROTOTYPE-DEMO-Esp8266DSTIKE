
#include "FunctionalInterrupt.h"

#define BUTTON_SLEEP_READ_TIME 65
#define BUTTON_SEARCH_INVALID_TIME 220

#define BUTTON_LONGUP_TIME 2 // 3*65->...
#define BUTTON_SET_ALWAYS_EN_LIGHT true
#define BUTTON_SERIAL_DEBUG false
#define SEARCH_FIRMWARE_BUG true

struct dstikeButton {
  uint8_t pin;
  uint8_t interrupt_pin;
  char* debug_str;

  void (*up)(dstikeButton *button);
  void (*down)(dstikeButton *button, const bool is_long, uint8_t long_time);
};

struct dstikeButtonStatus {
  bool is_null;
  bool is_up;
  bool is_drop;
  dstikeButton *button;

  uint8_t up_time;
  uint32_t next_check_millis;
};

dstikeButtonStatus inline null_dstike_status() {
  dstikeButtonStatus result;
  result.is_null = true;
  //result.is_drop = true;
  return result;
}

static uint8_t all_buttons_count = 0;
static dstikeButtonStatus all_buttons[] = { // 6
  null_dstike_status(),
  null_dstike_status(),
  null_dstike_status(),
  null_dstike_status(),
  null_dstike_status(),
  null_dstike_status()
};
static const uint8_t all_buttons_stacklength = sizeof(all_buttons) / sizeof(dstikeButtonStatus);


bool en_dstike_status(dstikeButton *button);
bool delete_dstike_status(dstikeButton *button);
void NewdstikeTrigEventButton(dstikeButton *button);
void reset_dstike_button_interrupt(dstikeButton *button);

void dstikeTrigButton(dstikeButton *button, const bool is_on) { // trig fn, + -> -
  if(all_buttons_count == 0) {
    return;
  }
  
  for (uint8_t i = 0; i < all_buttons_stacklength; i = i + 1) {
    dstikeButtonStatus *a_button = &all_buttons[i];
    if (a_button->is_null) {
      continue;
    }
    
    if (a_button->button == button) {
      if(a_button->is_drop) {
        return;
      }
      
      
      if(is_on) {
        a_button->is_up = true;
        
        // RISING  + -> -
        // FALLING - -> +
        //Serial.println("72 + -> -");
        detachInterrupt(button->interrupt_pin);
        attachInterrupt(button->interrupt_pin, std::bind(dstikeTrigButton, a_button->button, false), RISING);
        /*
        */
        
      }else {
        a_button->is_up = false;
        
        //Serial.println("81 - -> +");
        detachInterrupt(button->interrupt_pin);
        attachInterrupt(button->interrupt_pin, std::bind(dstikeTrigButton, a_button->button, true), FALLING);
      }

      uint32_t a_millis = millis();
      if (!a_button->is_up) {
        if ((a_millis - a_button->next_check_millis >= BUTTON_SLEEP_READ_TIME)) {
          a_button->is_drop = true;
          a_button->next_check_millis = a_millis;
    
          if (BUTTON_SERIAL_DEBUG) {
            Serial.print("BUTTON_MARKDEL: ");
            Serial.println(button->debug_str);
          }
  
          /*if(a_button->button->down != 0) { // BUTTON FUNCTION
            (a_button->button->down)(a_button->button, false);
          }*/
        }
      }
      
      a_button->next_check_millis = a_millis;
      
      return;
    }
  }
}

void search_invalid_dstike_buttons() {
  if(all_buttons_count == 0) {
    return;
  }
  
  uint8_t real_length = all_buttons_count;
  for (uint8_t i = 0; i < all_buttons_stacklength; i = i + 1) {
    dstikeButtonStatus *a_button = &all_buttons[i];
    if (a_button->is_null) {
      continue;
    }

    real_length = real_length - 1;

    uint32_t a_millis = millis();
    if(a_button->is_drop) {
      //Serial.println(a_button->button->pin);
      if (a_millis - a_button->next_check_millis >= BUTTON_SLEEP_READ_TIME) {
         a_button->is_null = true;
         all_buttons_count = all_buttons_count - 1;
  
        if (BUTTON_SERIAL_DEBUG) {
          Serial.print("BUTTON_2DISABLE: ");
          Serial.println(a_button->button->debug_str);
        }
        
        if(a_button->button->down != NULL) { // BUTTON FUNCTION
          (a_button->button->down)(a_button->button, a_button->up_time >= BUTTON_LONGUP_TIME, a_button->up_time);
        }
        
        // RISING  + -> -
        // FALLING - -> +
        reset_dstike_button_interrupt(a_button->button);
        //detachInterrupt(digitalPinToInterrupt(a_button->button->pin));
        //attachInterrupt(digitalPinToInterrupt(a_button->button->pin), std::bind(en_dstike_status, a_button->button), FALLING);
      }
      
    }else {
      // ACTIVE_BUTTON
      if(a_button->is_up) {
        if ((a_millis - a_button->next_check_millis) >= BUTTON_SEARCH_INVALID_TIME) {
          //Serial.print(digitalRead(a_button->button->pin));
          //Serial.print(" ");
          //Serial.println(a_button->button->pin);

          pinMode(a_button->button->pin, INPUT);
          if(digitalRead(a_button->button->pin) == HIGH) {
            a_button->is_null = true;
            all_buttons_count = all_buttons_count - 1;
  
            if (BUTTON_SERIAL_DEBUG) {
              Serial.print("BUTTON_A3DISABLE: ");
              Serial.println(a_button->button->debug_str);
            }
          
            if(a_button->button->down != NULL) { // BUTTON FUNCTION
              (a_button->button->down)(a_button->button, a_button->up_time >= BUTTON_LONGUP_TIME, a_button->up_time);
            }
            reset_dstike_button_interrupt(a_button->button);
          }else {
            a_button->next_check_millis = a_millis;

            if(a_button->up_time != 255) {
              a_button->up_time = a_button->up_time + 1;
            }
            
            if (BUTTON_SERIAL_DEBUG) {
              Serial.print("BUTTON_31__IGNORE: ");
              Serial.println(a_button->button->debug_str);
            }
          }
          pinMode(a_button->button->pin, INPUT_PULLUP);
        }/*else {
          if(a_button->up_time != 255) {
            a_button->up_time = a_button->up_time + 1;
          }
            
          if (BUTTON_SERIAL_DEBUG) {
            Serial.print("BUTTON_3IGNORE: ");
            Serial.println(a_button->button->debug_str);
          }
        }*/
      }else {
        
      }
    }
    
    if (real_length == 0) {
      break;
    }
  }
}

bool forcedel_dstike_status(dstikeButton *button) {
  if(all_buttons_count == 0) {
    return false;
  }
  
  for (uint8_t i = 0; i < all_buttons_stacklength; i = i + 1) {
    dstikeButtonStatus *a_button = &all_buttons[i];
    if (a_button->is_null) {
      continue;
    }
    
    if (a_button->button == button) {
      a_button->is_null = true;
       all_buttons_count = all_buttons_count - 1;
      if (!a_button->is_drop) {
        a_button->is_drop = true;

        if(a_button->button->down != NULL) { // BUTTON FUNCTION
          (a_button->button->down)(a_button->button, a_button->up_time >= BUTTON_LONGUP_TIME, a_button->up_time);
        }
      }
      
      // RISING  + -> -
      // FALLING - -> +
      reset_dstike_button_interrupt(a_button->button);
      return true;
    }
  }
  return false;
}

void inline destruct_dstike_button(dstikeButton *button) {
  forcedel_dstike_status(button); // del status
  detachInterrupt(button->interrupt_pin); // del trig
}

// hidden
void push_dstike_button_iterrupt(dstikeButton *button) {
  //Serial.println(12);
  if(SEARCH_FIRMWARE_BUG) {
    for (uint8_t i = 0; i < all_buttons_stacklength; i = i + 1) {
      dstikeButtonStatus *a_button = &all_buttons[i];
      if (a_button->is_null) {
        continue;
      }
      
      if (a_button->button == button) {
        Serial.println("Firmware bug, 2buttons<->1pin");
        return;
      }
    }
  }
  
  for (uint8_t i = 0; i < all_buttons_stacklength; i = i + 1) {
    dstikeButtonStatus *a_button = &all_buttons[i];
    if (a_button->is_null) {
      
      all_buttons_count = all_buttons_count + 1;
      a_button->is_null = false;
      a_button->is_up = true;
      a_button->is_drop = false;
      a_button->button = button;
      a_button->up_time = 0;
      a_button->next_check_millis = millis();

      if (BUTTON_SERIAL_DEBUG) {
        Serial.print("BUTTON_EN: ");
        Serial.println(button->debug_str);
      }
      
      // RISING  + -> -
      // FALLING - -> +
      //Serial.println("266 + -> -");
      detachInterrupt(button->interrupt_pin);
      attachInterrupt(button->interrupt_pin, std::bind(dstikeTrigButton, a_button->button, false), RISING);

      if(button->up != NULL) { // BUTTON FUNCTION
        (button->up)(button);
      }
      
      return;
    }
  }
}

// hidden
void inline reset_dstike_button_interrupt(dstikeButton *button) {
  //Serial.println("Reset TRIG");
  
  // RISING  + -> -
  // FALLING - -> +
  detachInterrupt(button->interrupt_pin);
  attachInterrupt(button->interrupt_pin, std::bind(push_dstike_button_iterrupt, button), FALLING);
}

void init_dstike_button(dstikeButton *button, const uint8_t pin, char* debug_str) {
  button->pin = pin;    
  button->debug_str = debug_str;
  button->up = NULL;
  button->down = NULL;
  button->interrupt_pin = digitalPinToInterrupt(button->pin);
  
  // RISING  + -> -
  // FALLING - -> +
  reset_dstike_button_interrupt(button);
}
 
void inline set_up_dstike_button(dstikeButton *sself, void (*up)(dstikeButton *button)) {
  sself->up = up;
}

void inline set_down_dstike_button(dstikeButton *sself, void (*down)(dstikeButton *button, const bool is_long, uint8_t long_time)) {
  sself->down = down;
}
