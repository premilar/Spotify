#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <TinyGPS++.h>
#include <Wifi_S08_v2.h>
#include <TimeLib.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define GPSSerial Serial3
#define SCREEN U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI
#define SPI_CLK 14

/////////////////////// CONSTANT VARIABLES //////////////////////

const String SPOTIFY_USER = "";
const String KERBEROS = "";

const String PATH = "/6S08dev/" + KERBEROS + "/final/sb1.py";
const String IESC = "iesc-s2.mit.edu";
const int PUSH_BUTTON_PIN = 9;
const int LED_PIN = 13;

String MAC = "";

////////////////////// TIMERS ////////////////////////////////
elapsedMillis internet_update_timer = 0;
elapsedMillis GPS_update_timer = 0;

String response = "";

//////////////////// COMPONENT SETUP ///////////////////////

ESP8266 wifi = ESP8266(0,true);
SCREEN oled(U8G2_R2,10,15,16);
TinyGPSPlus gps;

//////////////////// CLASSES ////////////////////
// BUTTON CLASS
class Button {
  int state;
  int flag;
  elapsedMillis t_since_change; //timer since switch changed value
  elapsedMillis t_since_state_2; //timer since entered state 2 (reset to 0 when entering state 0)
  unsigned long debounce_time;
  unsigned long long_press_time;
  int pin;
  bool button_pressed;
public:
  Button(int p) {
    state = 0;
    pin = p;
    t_since_change = 0;
    t_since_state_2 = 0;
    debounce_time = 10;
    long_press_time = 1000;
    button_pressed = 0;
  }
  void read() {
    bool button_state = digitalRead(pin);  // true if HIGH, false if LOW
    button_pressed = !button_state; // Active-low logic is hard, inverting makes our lives easier.
    if (!button_state) {
      digitalWrite(LED_PIN, HIGH);
    }
    digitalWrite(LED_PIN, LOW);
  }
  int update() {
    flag = 0;
    if (state==0) {
        if (button_pressed) {
          state = 1;
          t_since_change = 0;
        }
      } 
    else if (state==1) {
        if(!button_pressed){
            state = 0;
            t_since_change=0;
        }
        else{
            if(button_pressed){
                if(t_since_change>=debounce_time){
                    //flag = 1;
                    t_since_state_2 = 0;
                    state = 2;
                }
            }
        }
    }
    else if (state == 2) {
      //Serial.println(state);
      if (!button_pressed) {
        t_since_change = 0;
        state = 4;
      }
      else if (button_pressed) {
        if (t_since_state_2 <= long_press_time) {
          state = 2;
        }
        else if (t_since_state_2>long_press_time) {
          //flag = 2;
          state = 3;
        }
      }
    }
    else if (state == 3) {
      //Serial.println(state);
      if (button_pressed) {
        state = 3;
      }
      else if (!button_pressed) {
        t_since_change = 0;
        state = 4;
      }
    }
    else if (state == 4) {
      //Serial.println(state);
      if (!button_pressed) {
        if (t_since_change <= debounce_time) {
          state = 4;
        }
        else if (t_since_change>debounce_time) {
          //t_since_change = 0;
          if (t_since_state_2<long_press_time) {
            flag = 1;
            //Serial.println("flag 1");
          }
          else if (t_since_state_2 >= long_press_time) {
            flag = 2;
          }
          //t_since_state_2 =0;
          state = 0;
        }
      }
      else if (button_pressed) {
        if (t_since_state_2<long_press_time) {
          state = 2;
          t_since_change = 0;
        }
        else if (t_since_state_2 >= long_press_time) {
          state = 3;
          t_since_change = 0;
        }
      }
    }
    return flag;
  }
};

// SYSTEM CLASS w/ STATES

//declare button objects
Button pushbutton(PUSH_BUTTON_PIN);

void setup(){
	//begin serial, oled, wifi, gps, connect the wifi
	Serial.begin(115200);
	SPI.setSCK(14);
	oled.begin();
	GPSSerial.begin(9600);
	pinMode(LED_PIN, OUTPUT);
	pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP);
	
	wifi.begin();
	wifi.connectWifi("6s08","iesc6s08");
	
	while(!wifi.isConnected());
	MAC = wifi.getMAC();

    update_display(0);
}

void loop(){
	pushbutton.read();
	int advance = pushbutton.update();
	
	//GPS code to occasionally send gps coords every so often using gps timer. include power stuff. this runs in specific states
	
	//sending GPS coords using wifi
	
	//update system, use class functions within system state machine class. update display should be within class?
	
}

///////////////////////// FUNCTIONS ////////////////////////

//display functions
void pretty_print(int startx, int starty, String input, int fwidth, int fheight, int spacing, SCREEN &display){
    if(input==""){
      return;
    }
    int lengtht = input.length();
    int y_current = starty;
    int current_letter = 0;
    int diff = SCREEN_WIDTH - startx;
    int numbergo = diff/fwidth;
    while(y_current<=SCREEN_HEIGHT){
        oled.setCursor(startx,y_current);
        String substrin = input.substring(current_letter,current_letter+numbergo);
        if(substrin.indexOf("\n")>=0){
            oled.print(input.substring(current_letter,current_letter+substrin.indexOf("\n")));
            current_letter+=substrin.indexOf("\n")+1;
        }
        else{
            oled.print(input.substring(current_letter,current_letter+numbergo));
            current_letter+=numbergo;
        }
        y_current +=(fheight+spacing);
        
        if(current_letter>lengtht){
            return;
        }
    }
}

void update_display(int update_type){
    oled.clearBuffer();
    oled.setFont(u8g2_font_5x7_mr);

    if(update_type==0){
        Serial.println("start");
        pretty_print(10,10,"start",5,7,0,oled);
    }



    oled.sendBuffer();
}
