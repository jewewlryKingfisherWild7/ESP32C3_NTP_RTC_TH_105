//////////////////////////////////////////////////
//    ESP32C3_NTP_RTC_TH_105.ino
//       last edit: 2023.02.08
//       by:        jewelry.kingfisher
//       note:      ESP32C3 NTP clock from WiFi and AM2322 T,H sensor display to AQM1602Y LCD 
//                  external WiFi configure with WiFiManager and Preferences
//////////////////////////////////////////////////

//#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <RTClib.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <HTTPClient.h>
#include <ST7032_asukiaaa.h>
#include <Wire.h>
#include "AM232X.h"
#include <WiFiManager.h> 
#include <Preferences.h>

Preferences prefs;
WiFiManager wm;
struct tm timeInfo;
ST7032_asukiaaa lcd;
AM232X AM2322;

int count = 0;
time_t timeNow = 0;       // time now 
time_t prevDisplay = 0;   // when the digital clock was displayed

float tt;
float hh;
int   AM2322status;


//////////////////////////////////////////////////////////////////////////////////////
/////  READ AM2322 DATA
//////////////////////////////////////////////////////////////////////////////////////
void readAM2322( ) {
  AM2322status = AM2322.read();
  switch (AM2322status) {
  case AM232X_OK:
    tt = AM2322.getTemperature();
    hh = AM2322.getHumidity();
    break;
  /*************
  default:
    lcd.setCursor(0, 1);  //LCD display
    lcd.print( "Sensor not found" );
    break;
  *************/
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// setup
//////////////////////////////////////////////////////////////////////////////////////
void setup() {
  char titlechr[] =  "NTPclock,Tmp,Hum";
  char startChar[] = "[ver.1.05]setup,";
  bool res;
  const char* ssid = "??YOUR_SSID??";        // SSID clear word
  const char* pass = "???YOUR_PASS???";      // PASS clear word
  int presetDoneKey = 999999;

  Serial.begin(115200);
  Serial.println();

  Wire.begin();        // init. I2C
  AM2322.begin();
  AM2322.wakeUp();
  delay(2000);
  Serial.begin(115200);

  lcd.begin(16, 2);     // config LCD AQM1602Y (col num=16, line num=2)
  lcd.setContrast(45);  // set LCD contrast (0..63)

  lcd.setCursor(0, 0);  // LCD display
  lcd.print( titlechr );
  lcd.setCursor(0, 1);  // LCD display
  lcd.print( startChar );


/********************
  // detecting the AM2322 sensor at start up
  int rrCount = 5;
  while ((AM2322.getTemperature()==0) && (AM2322.getHumidity()==0) && (rrCount > 0)) {
    rrCount--;
    delay (2000);
    lcd.setCursor(0, 1);  // LCD display
    lcd.print( startChar );
  }
********************/

  if ( AM2322.read() != AM232X_OK ) {
    Serial.println("Sensor not found");
    lcd.setCursor(0, 0);  //LCD display
    lcd.print( "Sensor not found" );
    //while (1);
  }


  // check WiFi connection at start up
  int countWiFiwait = 20;
  int resetWiFiconfigFlag =0;    // clear flag
  res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
  while ((WiFi.status() != WL_CONNECTED) && (countWiFiwait > 0)) {
    Serial.print(".");
    countWiFiwait--;
    if (countWiFiwait == 0 ) { resetWiFiconfigFlag = 1; }   // set the reset WiFi config flag
    delay (1000);
  }

  // WiFiManager process start
  // Check if WiFi initial setting is completed, if not set, advance WiFi setting from outside by "AutoConnectAP"
  prefs.begin("my-nvmem", false);

  if (resetWiFiconfigFlag == 1) {
    prefs.putInt("value", (presetDoneKey-presetDoneKey) );  // write value=0 for reset WiFi setting
  }

  int val = prefs.getInt("value", 0);       // get (for 2nd param be return value of no data)
  Serial.print("my-nvram val= ");
  Serial.println( val );

  if ( val != presetDoneKey ) {              // if not peset done ?
    if ((WiFi.status() != WL_CONNECTED)) {   // if WiFi not connect ?
      WiFi.begin(ssid, pass);                //    then, force write the clear word to ssid, pass of WiFi EE-ROM 
      Serial.println("##### force cleared, ssid, pass of ESP32 WiFi EE-ROM #####");
      delay(2000);
    
    prefs.putInt("value", presetDoneKey);    // write (for 2nd param be write value)
    prefs.end();                             // close Preferences
    }
  }

  res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
  if(!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  } 
  else {
    //if you get here you have connected to the WiFi    
    Serial.println("WiFi connected... Congratulation!");
  }

  // wait for WiFi connection
  Serial.print("Waiting for WiFi to connect...");
  while ((WiFi.status() != WL_CONNECTED)) {
    Serial.print(".");
    delay (1000);
  }
  Serial.println("connected");
  Serial.println(WiFi.localIP());

  lcd.setCursor(0, 0);  //LCD display
  lcd.print( "WiFi.local IP=  " );
  lcd.setCursor(0, 1);  //LCD display
  lcd.print( "                " );
  lcd.setCursor(3, 1);  //LCD display
  lcd.print( WiFi.localIP() );
  delay(2000);

  ///// config NTP to JST local time /////
  configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp"); 
  lcd.setCursor(0, 1);  //LCD display
  lcd.print( "JST time w' NTP " );
  delay(1000);
}


//////////////////////////////////////////////////////////////////////////////////////
// LCDdisplay
//////////////////////////////////////////////////////////////////////////////////////
void LCDdisplay() {
  char timeChar[30];
  char dispA[16];
  int i;
  char buff0;

  if ( getLocalTime(&timeInfo, 1000) == true ) {
    sprintf(timeChar, "%04d/%02d/%02d %02d:%02d:%02d",
        timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
        timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
    //Serial.println("just get accurate NTP server time...");
    Serial.println( timeChar );
  }

  // #1 line display NTP yyyy/mm/dd, and tt:mm:ss
  for ( i= 0; i < 16 ; i++) {
    dispA[i] = timeChar[i+3];    // "yyyy" display process below
  }
  // display "yyyy" turn around each 1 char, so y-->2-->0-->2-->--3-->y... 
  if ( count == 0 ) {
    buff0 = 'y'; } else {
    buff0 = timeChar[count-1];
  }
  dispA[0] = buff0;
  count++;
  if ( count > 4 ) { count = 0; }
  lcd.setCursor(0, 0);   //LCD display)
  lcd.print( dispA );

  // display Temperature, Humidity
  readAM2322();          // read sensor
  if (( tt == 0 ) && ( hh == 0 )) { return ; }  // if sensor miss read, then no LCD display
  lcd.setCursor(0, 1);
  lcd.print("T=     ,  ");
  lcd.setCursor(3, 1);
  lcd.print(String(tt,1));
  lcd.setCursor(9, 1);
  lcd.print("H=    %");
  lcd.setCursor(11, 1);
  lcd.print(String(hh,1));

/////for test ///////////////////////////////////////6666///
  // Serial.println();
  // Serial.print("readAM2322,  T= ");
  // Serial.print( String(tt,1) );
  // Serial.print("   ,H= ");
  // Serial.println(String(hh,1));
///////////////////////////////////////////////////////

}


//////////////////////////////////////////////////////////////////////////////////////
// loop
//////////////////////////////////////////////////////////////////////////////////////
void loop() {
  bool ntpStatus = getLocalTime(&timeInfo, 1000);
  if ( !ntpStatus ) {
    lcd.setCursor(0, 1);
    lcd.print("ERROR, can't NTP");
    Serial.println("ERROR, Failed to connect NTP server");
    //delay ( 3000 );
  }

  while ((WiFi.status() != WL_CONNECTED)) {   // if WiFi not connect ?
    lcd.setCursor(0, 1);
    lcd.print("ERROR,can't WiFi");
    Serial.println("ERROR! can not connect to WiFi");
    delay(1000);
  }
  
  timeNow = mktime(&timeInfo);
  if ( timeNow != prevDisplay) { //update the display only if time has changed
    prevDisplay = timeNow;
    LCDdisplay();
  }
}  
