#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <TinyGPS++.h>
#include <Wifi_S08_v2.h>
#include <TimeLib.h>
#include <MPU9250.h>
#include <math.h>
#include <Snooze_6s08.h>
#include <SIXS08_util.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define GPSSerial Serial3
#define SCREEN U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI
#define SPI_CLK 14

const int BUTTON_PIN = 9;
const int BUTTON_PIN_2=6;
const int LED_PIN = 13;
const int SECONDS_PER_HOUR = 3600;         // for time zone adjustment
const int IOT_HTTP_TIMEOUT = 5000;            // how long to wait for get/post responses
const int DISPLAY_UPDATE_INTERVAL = 2000;  // how often to print GPS data to serial/oled
const int IOT_UPDATE_INTERVAL = 10000;     // how often to send/pull from cloud
const String USERNAME = "premila";
String Spotify_name="Claire Hsu"; 
const String SPOTIFY_USERNAME= "Simran (s1mmyp)";// replace with your username
const String SPOTIFY_USER = "s1mmyp";
//const String SPOTIFY_USERNAME = "Simran";
//const String SPOTIFY_USER = "s1mtest";

// IoT constants
const String PATH_1 = "/6S08dev/premila/final/sb1.py";
const String PATH_2 = "/6S08dev/premila/final/sb2.py";
const String PATH_4 = "/6S08dev/clhsu/final/sb4.py";
const String UPDATE_PATH = "/6S08dev/clhsu/final/sb3.py";
const String IESC = "iesc-s2.mit.edu";
String array_users[25];
String array_users_ID[25];
String array_playlist[20];
String array_playlist_ID[50];
String array_songs[50];
String array_song_ID[50];
String likes[50];
String diversity_data_points="";
String num_songs=0;
String num_playlists=0;
String songname = "";
String artist = "";
String album = "";
int user_count=0;  
String SPLITTER_1=":";
String SPLITTER_2=",";
String SPLITTER_3="##";
String SPLITTER_4="&&";


// Global variables
String MAC = "";
String area = "";
String students_nearby = "";
String student_usernames = "";
String playlists = "";
String songs = "";
bool spoof = true;


elapsedMillis iot_update_timer = 0;             // time since cloud  update
elapsedMillis update_interval_timer = 0;// time since last serial/oled update
elapsedMillis iot_t=0; //internet

String response = "";   // response from wifi
String response_2 = "";
String response_3 = "";

bool updatehome = true;

// Instantiate objects
MPU9250 imu;
ESP8266 wifi = ESP8266(0,true);
SCREEN oled(U8G2_R2, 10, 15,16);  
TinyGPSPlus gps;
//PowerMonitor pm(oled, imu, wifi, 0x0F);




class Button{
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
      t_since_state_2= 0;
      debounce_time = 10;
      long_press_time = 1000;
      button_pressed = 0;
    }
    void read() {
    bool button_state = digitalRead(pin);  // true if HIGH, false if LOW
    button_pressed = !button_state; // Active-low logic is hard, inverting makes our lives easier.
    if (!button_state) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("on");
    }
    digitalWrite(LED_PIN, LOW);
  }
int update() {
  read();
  flag = 0;
  if (state==0) { // Unpressed, rest state
    if (button_pressed) {
      state = 1;
      t_since_change = 0;
    }
  } else if (state==1) { //Tentative pressed
    if (!button_pressed) {
      state = 0;
      t_since_change = 0;
    } else if (t_since_change >= debounce_time) {
      state = 2;
      t_since_state_2 = 0;
    }
  } else if (state==2) { // Short press
    if (!button_pressed) {
      state = 4;
      t_since_change = 0;
    } else if (t_since_state_2 >= long_press_time) {
      state = 3;
    }
  } else if (state==3) { //Long press
    if (!button_pressed) {
      state = 4;
      t_since_change = 0;
    }
  } else if (state==4) { //Tentative unpressed
    if (button_pressed && t_since_state_2 < long_press_time) {
      state = 2; // Unpress was temporary, return to short press
      t_since_change = 0;
    } else if (button_pressed && t_since_state_2 >= long_press_time) {
      state = 3; // Unpress was temporary, return to long press
      t_since_change = 0;
    } else if (t_since_change >= debounce_time) { // A full button push is complete
      state = 0;
      if (t_since_state_2 < long_press_time) { // It is a short press
        flag = 1;
      } else {  // It is a long press
        flag = 2;
      }
    }
  }
  return flag;
}
};


class Users_nearby{
  //array array_users=[Claire Hsu, santharowles, Simran];
  String message;
  String Spotify_name = "";
  String username="";
  String playlist="";
  String playlist_ID="";
  String song="";
  String song_ID="";
  String user_location="";
  String users_posting ="";
  int name_index;
  int state;
  int p;
  int s;
  String response;
  String new_response_3;
  bool dispplot;
  elapsedMillis scrolling_timer;
  
  int scrolling_threshold = 150;
  float angle_threshold = 0.3;
  public:
    Users_nearby(){
      state = 0;
      message ="";
      name_index = 0;
      p=0;
      s=0;
    }

    int get_state(){
      return state;
    }

    bool get_update(){
      return dispplot;
    }

    void parse_songinfo(String songinfo){ 
        int htmltag = songinfo.indexOf("<html>")+6;
        int endhtmltag = songinfo.indexOf("</html>");
        int position1 = songinfo.indexOf("##");
        int position2= songinfo.indexOf("##",position1+1);
        songname = songinfo.substring(htmltag, position1);
        artist=songinfo.substring(position1+2,position2);
        album= songinfo.substring(position2+2,endhtmltag);   
    }
    
  String update(float angle, int button, int button2){

    if (state==0){ //DISPLAYS DIVERSITY, HOME PAGE
      if(updatehome==false){
            dispplot=false;
        }
      if(updatehome){
        //update_display(diversity_data_points);
        iot_update_timer = 0;
          if (!wifi.isBusy()) {
                iot_t=0;
                String data = "selfuserID=" + SPOTIFY_USER; 
                wifi.sendRequest(GET,"iesc-s2.mit.edu", 80, PATH_4, data , true);;
              }
           while (!wifi.hasResponse() && iot_t < IOT_HTTP_TIMEOUT); //wait for response
              if (wifi.hasResponse()) {
                response_3 = wifi.getResponse();
                new_response_3=response_3.substring(response_3.indexOf("<html>")+6,response_3.indexOf("##"));
                new_response_3+=": "+ response_3.substring(response_3.indexOf("##")+2,response_3.indexOf("NUM"));
                diversity_data_points=response_3.substring(response_3.indexOf("NUM:")+4, response_3.indexOf("</html>"));
                Serial.println(diversity_data_points);
                 Serial.print("Got response at t=");
                 Serial.println(millis());
                 Serial.println(response_3);
                 updatehome = false;
                 dispplot=true;
                 
              }    
              else {
                  Serial.println("No timely response");
              }
        }
        
        if(button==1){
            state=1;
            Spotify_name="";
            response_3="";
            //return message;
          }

         if(button==2){
            state=20;
            response_3="";
         }
            
          
          return new_response_3 ; //DIVERSITY
      }
    else if(state==20){ //GROUP - GET DB SONGS LIST
      iot_update_timer = 0;
          if (!wifi.isBusy()) {
                iot_t=0;
                //String data = "selfuserID=" + SPOTIFY_USER +"&lat=" + String(gps.location.lat(),6) + "&lon=" + String(gps.location.lng(),6); 
                String data = "selfuserID=" +SPOTIFY_USER+"&lat=42.361084&lon=-71.092354";
                wifi.sendRequest(GET,"iesc-s2.mit.edu", 80, PATH_2, data , true);
                state=21;
                return "Query sent";
          }
          else{
              state=20;
              return "Wifi is busy";
          }
      }
    else if(state==21){ //GROUP - GET RESPONSE FOR DB SONG LIST
          iot_update_timer=0;
          iot_t = 0;
          while (!wifi.hasResponse() && iot_t < IOT_HTTP_TIMEOUT); //wait for response
            if (wifi.hasResponse()) {
              response_3 = wifi.getResponse();
               Serial.print("Got response at t=");
               Serial.println(millis());
               Serial.println(response_3);
               
                songs=response_3.substring(response_3.indexOf("<html>")+6,response_3.indexOf("Users Posting")); //change parsing to be for songs from database

                user_location = response_3.substring(response_3.indexOf("Area: ")+6, response_3.indexOf("</html>"));
                Serial.println(songs);
                users_posting = response_3.substring(response_3.indexOf("Posting: ")+9,response_3.indexOf("##Area"));
                num_songs=response_3.substring(response_3.indexOf("&&")+2,response_3.indexOf("##Users"));
                s=num_songs.toInt();
                for ( int i = 0; i < s; i++){
                  int position1 = songs.indexOf(SPLITTER_1);
                  int position2= songs.indexOf(SPLITTER_2);
                  int position3 = songs.indexOf(SPLITTER_3);
                  array_songs[i] = songs.substring(0, position1);
                  Serial.println(array_songs[i]);
                  array_song_ID[i]=songs.substring(position1+1,position3);
                  likes[i] = "Likes: "+songs.substring(position3+2,position2);
                  Serial.println(array_song_ID[i]);
                  songs.remove(0, position3 + SPLITTER_3.length()+2);
                 }
                 state=22;
                 return "Response received!";
            }
            else {
              state=21;
              Serial.println("No timely response");
              return "Waiting for response";
            }
      }

    else if(state==22){ //GROUP - SCROLL THROUGH DB SONGS LIST
        if(fabs(angle)>=angle_threshold && scrolling_timer>scrolling_threshold){
            if(angle>angle_threshold){
                if(name_index >= s-1){
                    name_index=0;
                    scrolling_timer=0;
                    }
                else{
                    name_index++;
                    scrolling_timer=0;
                    }
                }
            if(angle<angle_threshold){
                if(name_index==0){
                    name_index=s-1;
                    scrolling_timer=0;
                }
                else{
                    name_index--;
                    scrolling_timer=0;
                    }
                }
          }

          
       if(button==2){
            song=array_songs[name_index];
            song_ID=array_song_ID[name_index];
            name_index=0;
            state=23;
            return "";
              }
       if(button2==2){
            state = 0;
            dispplot=true;
            updatehome=true;
            return "";
       }
          String returnstring = "Location: " + user_location + "\n" +"Users Posting: "+users_posting+"\n"+array_songs[name_index] + "\n" + likes[name_index];
          return returnstring;      
      }
    else if(state==23){ //GROUP - REQUEST SONG INFO
        if (!wifi.isBusy()&&iot_update_timer>IOT_UPDATE_INTERVAL) {
            String data = "songID=" + song_ID; 
            wifi.sendRequest(GET,"iesc-s2.mit.edu", 80, PATH_4, data , true);
            state=24;
            Serial.print("Sending request at t=");
            Serial.println(millis());
            iot_update_timer = 0;
            return "Query sent";
          }
        else{
            state=23;
            return "Wifi is busy";
          }
      }
    else if(state==24){ //GROUP - WAIT FOR SONG INFO
//        if (wifi.isConnected() && !wifi.isBusy()) {
          iot_t=0;         
          while (!wifi.hasResponse() && iot_t < IOT_HTTP_TIMEOUT); //wait for response
            if (wifi.hasResponse()) {
               response_3 = wifi.getResponse();
               Serial.print("Got response at t=");
               Serial.println(millis());
               Serial.println(response_3);
               state = 25;
               Serial.println("but why");
               return "Response received";
            }    
            else {
                Serial.println("No timely response");
                return "Waiting for response";
            } 
    } 
    else if(state==25){ //GROUP - PARSE SONG INFO, LOOK AT BUTTONS
      parse_songinfo(response_3);
        //add to old playlist     
              if(button==1){
                if (!wifi.isBusy()) {
                  String data = "selfuserID=" + SPOTIFY_USER + "&nw=False";
                  wifi.sendRequest(GET,"iesc-s2.mit.edu", 80, PATH_2, data , true);
                  state=26;
                  return "Loading playlists....";
              }
              }
              //add to new playlist
              if(button==2){
                if (!wifi.isBusy()) {
                  String data = "selfuserID=" + SPOTIFY_USER +"&songID=" + song_ID +"&group_mode=False" + "&nw=True" + "&like=True" +"&lat=" + String(gps.location.lat(),6) + "&lon=" + String(gps.location.lng(),6); 
                  wifi.sendRequest(POST,"iesc-s2.mit.edu", 80, PATH_2, data , true);
                  state=22;
                  return "Adding to new playlist....";
                 }
              }
         if(button2==2){
            if (!wifi.isBusy()) {
                  String data = "songID=" + song_ID + "&like=False"+"&lat=" + String(gps.location.lat(),6) + "&lon=" + String(gps.location.lng(),6); 
                  wifi.sendRequest(POST,"iesc-s2.mit.edu", 80, PATH_2, data , true);
                  state=22;
                  return "Song downvoted....";
                 }
         }
         else if(button2==1){
          state = 22;
          return "back to song list";
         }
         
        //Serial.println(album);
        String output = songname+"\n"+artist+"\n"+album;
      return output;
  }
    else if(state==26){ //GROUP - so you're adding a song to your own playlist and extract your playlists
//        if (wifi.isConnected() && !wifi.isBusy()) {
//          Serial.print("Sending request at t=");
//          Serial.println(millis());
//          wifi.sendRequest(GET, IESC, 80, PATH_2, "");
          iot_t=0;
          while (!wifi.hasResponse() && iot_t < IOT_HTTP_TIMEOUT); //wait for response
            if (wifi.hasResponse()) {
               response_2 = wifi.getResponse();
               Serial.print("Got response at t=");
               Serial.println(millis());
               Serial.println(response);
                
            
          playlists=response_2.substring(response_2.indexOf("<html>")+6,response_2.indexOf("&&"));
          num_playlists=response_2.substring(response_2.indexOf("&&")+2,response_2.indexOf("</html>"));
          int p=num_playlists.toInt();
          for ( int i = 0; i < p; i++){
                int position1 = playlists.indexOf(SPLITTER_1);
                int position2= playlists.indexOf(SPLITTER_2);
                array_playlist[i] = playlists.substring(0, position1);
                array_playlist_ID[i]=playlists.substring(position1+1,position2);
                playlists.remove(0, position2 + SPLITTER_2.length());           
          }
          state=27;
          return "Response received!";
        }  
       else {
          return "Waiting for response";
       }

}
    else if(state==27){ //GROUP - choose a playlist you want to add to
              if(fabs(angle)>=angle_threshold && scrolling_timer>scrolling_threshold){
                  if(angle>angle_threshold){
                      if(name_index >= p-1){
                          name_index=0;
                          scrolling_timer=0;
                    }
                      else{
                          name_index++;
                          scrolling_timer=0;
                    }
                }
                  if(angle<angle_threshold){
                      if(name_index==0){
                        name_index=p-1;
                        scrolling_timer=0;
                }
                      else{
                        name_index--;
                        scrolling_timer=0;
                    }
                }
          }
   
             if(button==2){
                playlist=array_playlist[name_index];
                playlist_ID=array_playlist_ID[name_index];
                name_index=0;
                String data = "selfuserID=" + SPOTIFY_USER +"&songID=" + song_ID+ "&nw=False" +"&group_mode=False" + "&playlistID="+ playlist_ID+ "&like=True" +"&lat=" + String(gps.location.lat(),6) + "&lon=" + String(gps.location.lng(),6);
                if(!wifi.isBusy()){ 
                wifi.sendRequest(POST,"iesc-s2.mit.edu", 80, PATH_2, data , true);
                }
                state=22;
                return "";
              }
               return array_playlist[name_index];
            
      }
    else if(state==1){ //scroll users
        //pm.setPowerMode("imu",1);
        if(fabs(angle)>=angle_threshold && scrolling_timer>scrolling_threshold){
          //Serial.println("state 1");
            if(angle>angle_threshold){
                if(name_index >= user_count-1){
                    name_index=0;
                    scrolling_timer=0;
                    }
                else{
                    name_index++;
                    scrolling_timer=0;
                    }
                }
            if(angle<angle_threshold){
                if(name_index==0){
                    name_index=user_count-1;
                    scrolling_timer=0;
                }
                else{
                    name_index--;
                    scrolling_timer=0;
                    }
                }
          }
          
        if(button==2){
            Spotify_name=array_users[name_index];
            username=array_users_ID[name_index];
            name_index=0;
            state=2;
            return "";
              }
           return array_users[name_index];
      }
    else if(state==2){ //send request for user playlists
        if (!wifi.isBusy()&&iot_update_timer>IOT_UPDATE_INTERVAL) {
            String data = "selfuserID=" + username + "&nw=" + "False"; 
            wifi.sendRequest(GET,"iesc-s2.mit.edu", 80, PATH_4, data , true);;
            state=3;
            return "Query sent";
          }
        else{
            state=2;
            return "Wifi is busy";
          }
      }
    else if(state==3){ //receive response for playlist list
//        if (wifi.isConnected() && !wifi.isBusy()) {
//          Serial.print("Sending request at t=");
//          Serial.println(millis());
//          wifi.sendRequest(GET, IESC, 80, PATH_2, "");
          //pm.setPowerMode("gps",0);
          iot_update_timer=0;
          delay(5000);
          iot_t=0;
          while (!wifi.hasResponse() && iot_t < IOT_HTTP_TIMEOUT){ //wait for response
          }
            if (wifi.hasResponse()) {
              response_2 = wifi.getResponse();
               Serial.print("Got response at t=");
               Serial.println(millis());
               Serial.println(response);
                
            
          playlists=response_2.substring(response_2.indexOf("token_info")+10,response_2.indexOf("&&"));

          num_playlists=response_2.substring(response_2.indexOf("&&")+2,response_2.indexOf("</html>"));
          p=num_playlists.toInt();

          for ( int i = 0; i < p; i++){
                int position1 = playlists.indexOf(SPLITTER_1);
                int position2= playlists.indexOf(SPLITTER_2);
                array_playlist[i] = playlists.substring(0, position1);
                array_playlist_ID[i]=playlists.substring(position1+1,position2);
                playlists.remove(0, position2 + SPLITTER_2.length());           
          }
          state=4;
          Serial.println(array_playlist[1]);
          return "Response received!";
        }  
       else {
          state = 3;
          return "Waiting for response";
       }

}
    else if(state==4){ //scroll through playlists
//        pm.setPowerMode("imu",1);
//        pm.setPowerMode("gps",1);
        if(fabs(angle)>=angle_threshold && scrolling_timer>scrolling_threshold){
            if(angle>angle_threshold){
                if(name_index >= p-1){
                    name_index=0;
                    scrolling_timer=0;
                    }
                else{
                    name_index++;
                    scrolling_timer=0;
                    }
                }
            if(angle<angle_threshold){
              Serial.println("idk");
                if(name_index==0){
                    name_index=p-1;
                    scrolling_timer=0;
                }
                else{
                    name_index--;
                    scrolling_timer=0;
                    }
                }
          }
   
       if(button==2){
            playlist=array_playlist[name_index];
            playlist_ID=array_playlist_ID[name_index];
            name_index=0;
            state=5;
            return "";
              }
           return array_playlist[name_index];
      
      }
    else if(state==5){ //send playlist request for song list
        if (!wifi.isBusy() && iot_update_timer>IOT_UPDATE_INTERVAL) {
            String data = "playlistID=" + playlist_ID + "&userID=" + username; 
            wifi.sendRequest(GET,"iesc-s2.mit.edu", 80, PATH_4, data , true);
            state=6;
            return "Query sent";
          }
        else{
            state=5;
            return "Wifi is busy";
          }
      }
    else if(state==6){ //collect response for song list
//        if (wifi.isConnected() && !wifi.isBusy()) {
//          
//          Serial.print("Sending request at t=");
//          Serial.println(millis());
//          wifi.sendRequest(GET, IESC, 80, PATH_2, "");
          iot_update_timer=0;
          iot_t = 0;
          while (!wifi.hasResponse() && iot_t < IOT_HTTP_TIMEOUT); //wait for response
            if (wifi.hasResponse()) {
              response_3 = wifi.getResponse();
               Serial.print("Got response at t=");
               Serial.println(millis());
               Serial.println(response_3);
               
               songs=response_3.substring(response_3.indexOf("token_info")+10,response_3.indexOf("&&"));
                num_songs=response_3.substring(response_3.indexOf("&&")+2,response_3.indexOf("</html>"));
                Serial.println(num_songs);
                s=num_songs.toInt();
                for ( int i = 0; i < s; i++){
                  int position1 = songs.indexOf(SPLITTER_1);
                  int position2= songs.indexOf(SPLITTER_2);
                  array_songs[i] = songs.substring(0, position1);
                  array_song_ID[i]=songs.substring(position1+1,position2);
                  songs.remove(0, position2 + SPLITTER_2.length());
                 }
                 state=7;
                 return "Response received!";
            }
            else {
              state=6;
              Serial.println("No timely response");
              return "Waiting for response";
            }
      }
    else if(state==7){ //song scroll
        //pm.setPowerMode("imu",1);
        if(fabs(angle)>=angle_threshold && scrolling_timer>scrolling_threshold){
            if(angle>angle_threshold){
                if(name_index >= s-1){
                    name_index=0;
                    scrolling_timer=0;
                    }
                else{
                    name_index++;
                    scrolling_timer=0;
                    }
                }
            if(angle<angle_threshold){
                if(name_index==0){
                    name_index=s-1;
                    scrolling_timer=0;
                }
                else{
                    name_index--;
                    scrolling_timer=0;
                    }
                }
          }
   
       if(button==2){
            song=array_songs[name_index];
            song_ID=array_song_ID[name_index];
            name_index=0;
            state=8;
            return "";
              }
      return array_songs[name_index];      
      }
    else if(state==8){ //send request for song info
        if (!wifi.isBusy()&&iot_update_timer>IOT_UPDATE_INTERVAL) {
            String data = "songID=" + song_ID ; 
            wifi.sendRequest(GET,"iesc-s2.mit.edu", 80, PATH_2, data , true);
            state=9;
            Serial.print("Sending request at t=");
            Serial.println(millis());
            iot_update_timer = 0;
            return "Query sent";
          }
        else{
            state=8;
            return "Wifi is busy";
          }
      }
    else if(state==9){ //receive response for song info
          //pm.setPowerMode("gps",0);
//        if (wifi.isConnected() && !wifi.isBusy()) {
          iot_t=0;         
          while (!wifi.hasResponse() && iot_t < IOT_HTTP_TIMEOUT); //wait for response
            if (wifi.hasResponse()) {
               response_3 = wifi.getResponse();
               Serial.print("Got response at t=");
               Serial.println(millis());
               Serial.println(response_3);
               state = 10;
               Serial.println("but why");
               return "Response received";
            }    
            else {
                Serial.println("No timely response");
                return "Waiting for response";
            } 
    }
    else if(state==10){ //state with the parsed song info, button to send request
      //pm.setPowerMode("gps",1);
      parse_songinfo(response_3);
//      if(button==2){
//          state=0;
//          updatehome=true;
//        }
        //go back to song list (for now)
        if(button==1){
          state = 7;
        }
        //add to old playlist
        if(button==2){
          if (!wifi.isBusy()) {
            String data = "selfuserID=" + SPOTIFY_USER + "&nw=False"; 
            wifi.sendRequest(GET,"iesc-s2.mit.edu", 80, PATH_4, data , true);
            state=11;
            return "Loading playlists....";
           }
        }
        //Serial.println(album);
        String output = songname+"\n"+artist+"\n"+album;
      return output;
  }
    else if(state==11){ //collect own user's playlists
//        if (wifi.isConnected() && !wifi.isBusy()) {
//          Serial.print("Sending request at t=");
//          Serial.println(millis());
//          wifi.sendRequest(GET, IESC, 80, PATH_2, "");
          iot_t=0;
          while (!wifi.hasResponse() && iot_t < IOT_HTTP_TIMEOUT); //wait for response
            if (wifi.hasResponse()) {
               response_2 = wifi.getResponse();
               Serial.print("Got response at t=");
               Serial.println(millis());
               Serial.println(response);
                
            
          playlists=response_2.substring(response_2.indexOf("<html>")+6,response_2.indexOf("&&"));
          num_playlists=response_2.substring(response_2.indexOf("&&")+2,response_2.indexOf("</html>"));
          int p=num_playlists.toInt();
          for ( int i = 0; i < p; i++){
                int position1 = playlists.indexOf(SPLITTER_1);
                int position2= playlists.indexOf(SPLITTER_2);
                array_playlist[i] = playlists.substring(0, position1);
                array_playlist_ID[i]=playlists.substring(position1+1,position2);
                playlists.remove(0, position2 + SPLITTER_2.length());           
          }
          state=12;
          return "Response received!";
        }  
       else {
          return "Waiting for response";
       }

}
    else if(state==12){ //scroll through own user's playlists to add song
      //pm.setPowerMode("imu",1);
        if(fabs(angle)>=angle_threshold && scrolling_timer>scrolling_threshold){
            if(angle>angle_threshold){
                if(name_index >= p-1){
                    name_index=0;
                    scrolling_timer=0;
                    }
                else{
                    name_index++;
                    scrolling_timer=0;
                    }
                }
            if(angle<angle_threshold){
                if(name_index==0){
                    name_index=p-1;
                    scrolling_timer=0;
                }
                else{
                    name_index--;
                    scrolling_timer=0;
                    }
                }
          }
   
       if(button==2){
           if(state==0){
              if(fabs(angle)>=angle_threshold && scrolling_timer>scrolling_threshold){
                  if(angle>angle_threshold){
                      if(name_index >= p-1){
                          name_index=0;
                          scrolling_timer=0;
                    }
                      else{
                          name_index++;
                          scrolling_timer=0;
                    }
                }
                  if(angle<angle_threshold){
                      if(name_index==0){
                        name_index=p-1;
                        scrolling_timer=0;
                }
                      else{
                        name_index--;
                        scrolling_timer=0;
                    }
                }
          }
   
             if(button==2){
                playlist=array_playlist[name_index];
                playlist_ID=array_playlist_ID[name_index];
                name_index=0;
                state=5;
                return "";
              }
               return array_playlist[name_index];
            
      }
            else if(state==1){
                if (!wifi.isBusy()&&iot_update_timer>IOT_UPDATE_INTERVAL) {
                    String data = "selfuserID" + SPOTIFY_USER + "&playlistID="+playlist_ID ; 
                    wifi.sendRequest(POST,"iesc-s2.mit.edu", 80, PATH_4, data , true);
                    state=9;
                    Serial.print("Sending request at t=");
                    Serial.println(millis());
                    iot_update_timer = 0;
                    return "Query sent";
                  }
                else{
                    state=1;
                    return "Wifi is busy";
                  }
              }
            playlist=array_playlist[name_index];
            playlist_ID=array_playlist_ID[name_index];
            name_index=0;
            //add to playlist
            String data = "userID=" + SPOTIFY_USER + "&playlistID="+playlist_ID+"&songID"+ song_ID;
                if(!wifi.isBusy()){ 
                wifi.sendRequest(POST,"iesc-s2.mit.edu", 80, PATH_2, data , true);
                }
            state=7;
            return "Adding.......";
            }
           return playlist[name_index];    
      }
}
};

Users_nearby un;

/* UTILITY FUNCTIONS */
// print current GPS stats to Serial
void print_data() {
  update_interval_timer = 0; // reset the update_interval_timer

  Serial.print("\nTime: ");
  Serial.print(hour(), DEC); Serial.print(':');
  Serial.print(minute(), DEC); Serial.print(':');
  Serial.print(second(), DEC); Serial.print('.');
  Serial.println(gps.time.centisecond());
  Serial.print("Date: ");
  Serial.print(day(), DEC); Serial.print('/');
  Serial.print(month(), DEC); Serial.print('/');
  Serial.println(year(), DEC);
  Serial.print("Fix: "); Serial.println(gps.location.isValid());
  if (gps.location.isValid()) {
    Serial.print("Location: ");
    Serial.print(gps.location.lat(), 6); 
    Serial.print(", ");
    Serial.println(gps.location.lng(), 6);

    Serial.print("Speed (mph): "); Serial.println(gps.speed.mph());
    Serial.print("Course: "); Serial.println(gps.course.deg());
    Serial.print("Altitude (m): "); Serial.println(gps.altitude.meters());
    Serial.print("Satellites: "); Serial.println(gps.satellites.value());
  }
}


// Print GPS stats to OLED
void update_display(String diversity_data_points)
{
  oled.clearBuffer();
  oled.setFont(u8g2_font_profont11_mr);
  oled.setCursor(0,23);
  oled.print("50");
  oled.setCursor(0,33);
  oled.print("40");
  oled.setCursor(0,43);
  oled.print("30");
  oled.setCursor(0,53);
  oled.print("20");
  oled.setCursor(0,63);
  oled.print("10");
  oled.setCursor(0,12);  

    int len_response = 60;
    int shape[len_response];
    size_t position1 = 0;
    for ( int i = 0; i < len_response; i++){
      position1 = diversity_data_points.indexOf(SPLITTER_2);
      float val=diversity_data_points.substring(0, position1).toFloat();
      //float val = x;
      //int val = diversity_data_points.substring(diversity_data_points.indexOf("NUM"), diversity_data_points.indexOf(SPLITTER_2 )).toInt();
      shape[i] = val;
      diversity_data_points.remove(0, position1 + 1);
    }
//    int shape[len_response];
//    shape[0]=30;
//    shape[1]=40;
//    shape[2]=40;
//    shape[3]=50;
   
    for ( int k = 0; k < len_response; k = k + 4){
      oled.drawLine(shape[k],shape[k+1],shape[k+2],shape[k+3]);
    }
    delay(1000);
    //oled.sendBuffer();
 
  //diversity_data_points=response_3.substring(response_3.indexOf("NUM")+3, response_3.indexOf("</html>");  
 
  //plot(diversity_data_points);
  oled.setCursor(30,30);
  //oled.drawLine(30,50,40,70);
    
}


// Sends POST request to report the results, and retries until successful
void send_GPS_data() {
  if (wifi.isConnected() && !wifi.isBusy()) {
    Serial.print("Sending request at t=");
    Serial.println(millis());
    String postdata = "username=" + SPOTIFY_USER + "&Spotify_name=" + SPOTIFY_USERNAME
      + "&lat=" + String(gps.location.lat(),6) + "&lon=" + String(gps.location.lng(),6);
    postdata = "username=" +SPOTIFY_USER+"&Spotify_name="+SPOTIFY_USERNAME+"&lat=42.361084&lon=-71.092354";
   if(un.get_state()==0){
    wifi.sendRequest(POST, IESC , 80, PATH_1, postdata, false);
   }
    elapsedMillis t=0;
    while (!wifi.hasResponse() && t < IOT_HTTP_TIMEOUT); //wait for response
    if (wifi.hasResponse()) {
      response = wifi.getResponse();
      Serial.print("Got response at t=");
      Serial.println(millis());
      Serial.println(response);
    } else {
      Serial.println("No timely GPS response");
    }
  }
}

// Send GET request for latest info from server.
String pull_from_server() {
  if (wifi.isConnected() && !wifi.isBusy()) {
    Serial.print("Sending request at t=");
    Serial.println(millis());
    String data = "username="+SPOTIFY_USER;
    wifi.sendRequest(GET, IESC, 80, PATH_1, data,false);
    elapsedMillis t = 0;;
    while (!wifi.hasResponse() && t < IOT_HTTP_TIMEOUT); //wait for response
    if (wifi.hasResponse()) {
      response = wifi.getResponse();
      Serial.print("Got response at t=");
      Serial.println(millis());
      Serial.println(response);
    } else {
      Serial.println("No timely pull GPS response");
    }
  }
  return response;
}

void pretty_print(int startx, int starty, String input, int fwidth, int fheight, int spacing, SCREEN &display){
  int x = startx;
  int y = starty;
  String temp = "";
  for (int i=0; i<input.length(); i++){
     if (fwidth*temp.length()<= (SCREEN_WIDTH-fwidth -x)){
        if (input.charAt(i)== '\n'){
          display.setCursor(x,y);
          display.print(temp);
          y += (fheight + spacing);
          temp = "";
          if (y>SCREEN_HEIGHT) break;
        }else{
          temp.concat(input.charAt(i));
        }
     }else{
      display.setCursor(x,y);
      display.print(temp);
      temp ="";
      y += (fheight + spacing);
      if (y>SCREEN_HEIGHT) break;
      if (input.charAt(i)!='\n'){
        temp.concat(input.charAt(i));
      }else{
          display.setCursor(x,y);
          y += (fheight + spacing);
          if (y>SCREEN_HEIGHT) break;
      } 
     }
     if(i==input.length()-1){
        display.setCursor(x,y);
        display.print(temp);
     }
  }
}


void setup_angle(){
  char c = imu.readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);
  Serial.print("MPU9250: "); Serial.print("I AM "); Serial.print(c, HEX);
  Serial.print(" I should be "); Serial.println(0x73, HEX);
  if (c == 0x73){
    imu.MPU9250SelfTest(imu.selfTest);
    imu.initMPU9250();
    imu.calibrateMPU9250(imu.gyroBias, imu.accelBias);
    imu.initMPU9250();
    imu.initAK8963(imu.factoryMagCalibration);
  } // if (c == 0x73)
  else
  {
    while(1) Serial.println("NOT FOUND"); // Loop forever if communication doesn't happen
  }
    imu.getAres();
    imu.getGres();
    imu.getMres();
}

void get_angle(float&x, float&y){
  imu.readAccelData(imu.accelCount);
  x = imu.accelCount[0]*imu.aRes;
  y = imu.accelCount[1]*imu.aRes;
}

Button buttonx(BUTTON_PIN);
Button buttony(BUTTON_PIN_2);

void setup()
{
  Serial.begin(115200);
  SPI.setSCK(14);
  oled.begin();
  oled.setFont(u8g2_font_5x7_mf); //small, stylish font
  GPSSerial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);//set up pin!
  pinMode(BUTTON_PIN_2, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  setup_angle();
  wifi.begin();
  wifi.connectWifi("MIT","");
  //wifi.connectWifi("6s08","iesc6s08");
  
  while (!wifi.isConnected()); //wait for connection
  MAC = wifi.getMAC();
  //pm.setPowerMode("imu",0);
}


void loop()
{
  // read data from the GPS in the 'main loop'

//  if(un.get_state()==0){
//  while (GPSSerial.available())  
//  {
//    if (gps.encode(GPSSerial.read())) {    // true if a complete NMEA sentence is received
//
//      if (update_interval_timer > DISPLAY_UPDATE_INTERVAL) {  // update oled/display
//        // Pull GPS time into Time library
//        setTime(gps.time.hour(),gps.time.minute(),gps.time.second(),
//          gps.date.day(),gps.date.month(),gps.date.year());
//        // Offset for time zone difference
//        adjustTime(-5*SECONDS_PER_HOUR);
//        print_data();
//        //update_display(diversity_data_points);
//        update_interval_timer = 0;
//      }
//
//      // only send to cloud if valid fix & at certain intervals
//      if (gps.location.isValid() && iot_update_timer > IOT_UPDATE_INTERVAL&& !wifi.isBusy()) {
//        send_GPS_data();
//       
//        response = pull_from_server();
//        area = response.substring(response.indexOf("area:")+5,response.indexOf("##"));
//        int x = response.indexOf("user:") + 5;
//        student_usernames=response.substring(x,response.indexOf("##", x));
//        students_nearby = response.substring(response.indexOf("##",x)+2,response.indexOf(" users nearby"));
//        int y=students_nearby.toInt();
//        user_count=y;
//        Serial.println(user_count);
//        iot_update_timer = 0;
//        parse_usernames(student_usernames);
//        }
//      
//    }
//  }
//}
        if(spoof){
          student_usernames="Claire Hsu:1241183313,Premila Rowles:idk,s1mtest:Simran Pabla";
          user_count =3;
          parse_usernames(student_usernames);
        }

//  while(!wifi.hasResponse());
//  response=pull_from_server_2();
//  songs=response_2.substring(response_2.indexOf("<html>")+6,response_2.indexOf(&&));
//  playlists=response_2.substring(response_2.indexOf("<html>")+6,response_2.indexOf(&&));
//  num_playlists=response_2.substring(indexOf("&&")+2,response.indexOf("</html>"));
//  int p=num_playlists.toInt();
//  num_playlists=p;
//  num_songs=response_2.substring(indexOf("&&")+2,response.indexOf("</html>"));
//  int s=num_songs.toInt();
//  num_songs=s;
  
  //Serial.println("loop");
  float x,y;
  get_angle(x,y); //get angle values
  int bv = buttonx.update(); //get button value
  int bw = buttony.update();
 if(bv!=0){
  Serial.println(bv);
 }
  String output = un.update(y,bv, bw); //input: angle and button, output String to display on this timestep
  if(output=="Response received"){
    Serial.println("Response received");
  }
  if(un.get_update()==true||un.get_state()!=0){
  oled.clearBuffer();
  //if(un.get_update()==true||un.get_state()!=0){
    
    if(un.get_update()==true){
      Serial.println("fml");
      update_display(diversity_data_points); 
    } 
    pretty_print(0,8,output,5,11,0,oled);
  //}
  oled.sendBuffer();
  }
}

void parse_usernames(String student_usernames){
  for ( int i = 0; i < user_count; i++){
    int position1 = student_usernames.indexOf(SPLITTER_1);
    int position2= student_usernames.indexOf(SPLITTER_2);
    array_users_ID[i] = student_usernames.substring(0, position1);
    array_users[i]=student_usernames.substring(position1+1,position2);
    student_usernames.remove(0, position2 + SPLITTER_2.length());
    
  }
}



