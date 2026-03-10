#include <Arduino.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include "config.h"
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
SoftwareSerial espSerial(ESP_RX_PIN, ESP_TX_PIN);
enum KeyType { KEY_NONE, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_SELECT };
struct RelayInfo { String name; bool state; };
RelayInfo relays[FRONTPANEL_RELAYS];
uint8_t selectedRelay = 0; unsigned long lastKeyAt = 0; unsigned long lastStatusRequestAt = 0; String rxLine;
KeyType readKey(){ int v = analogRead(KEYPAD_PIN); if(v<60)return KEY_RIGHT; if(v<200)return KEY_UP; if(v<400)return KEY_DOWN; if(v<600)return KEY_LEFT; if(v<850)return KEY_SELECT; return KEY_NONE; }
void sendLine(const String &line){ espSerial.println(line); }
void requestSnapshot(){ sendLine("GET:ALL"); }
void initRelayData(){ for(uint8_t i=0;i<FRONTPANEL_RELAYS;i++){ relays[i].name="Rele "+String(i+1); relays[i].state=false; } }
void drawScreen(){ lcd.clear(); String line1=">"+String(selectedRelay+1)+" "+relays[selectedRelay].name; if(line1.length()>16) line1=line1.substring(0,16); String line2=relays[selectedRelay].state?"Status: PAA ":"Status: AV  "; line2 += "Sel"; lcd.setCursor(0,0); lcd.print(line1); lcd.setCursor(0,1); lcd.print(line2.substring(0,16)); }
void handleCommand(const String &cmd){
  if(cmd.startsWith("NAME:")){ int first=cmd.indexOf(':',5); if(first>0){ int idx=cmd.substring(5,first).toInt()-1; if(idx>=0&&idx<FRONTPANEL_RELAYS) relays[idx].name=cmd.substring(first+1);} }
  else if(cmd.startsWith("STATE:")){ int first=cmd.indexOf(':',6); if(first>0){ int idx=cmd.substring(6,first).toInt()-1; if(idx>=0&&idx<FRONTPANEL_RELAYS){ String val=cmd.substring(first+1); val.trim(); relays[idx].state=(val=="1"||val=="ON"); } } }
}
void readEspSerial(){ while(espSerial.available()){ char c=(char)espSerial.read(); if(c=='\r') continue; if(c=='\n'){ if(rxLine.length()){ handleCommand(rxLine); rxLine=""; drawScreen(); } } else { rxLine+=c; if(rxLine.length()>120) rxLine.remove(0,40);} } }
void handleKeys(){ KeyType key=readKey(); if(key==KEY_NONE) return; if(millis()-lastKeyAt<KEY_DEBOUNCE_MS) return; lastKeyAt=millis();
  if(key==KEY_UP){ selectedRelay=(selectedRelay+1)%FRONTPANEL_RELAYS; drawScreen(); }
  else if(key==KEY_DOWN){ selectedRelay=(selectedRelay==0)?FRONTPANEL_RELAYS-1:selectedRelay-1; drawScreen(); }
  else if(key==KEY_SELECT){ sendLine("TOGGLE:"+String(selectedRelay+1)); }
  else if(key==KEY_RIGHT){ sendLine("SET:"+String(selectedRelay+1)+":1"); }
  else if(key==KEY_LEFT){ sendLine("SET:"+String(selectedRelay+1)+":0"); } }
void setup(){ initRelayData(); lcd.begin(16,2); lcd.print("UNO Frontpanel"); delay(700); drawScreen(); Serial.begin(SERIAL_BAUD); espSerial.begin(SERIAL_BAUD); delay(200); requestSnapshot(); }
void loop(){ handleKeys(); readEspSerial(); if(millis()-lastStatusRequestAt>STATUS_REQUEST_MS){ lastStatusRequestAt=millis(); requestSnapshot(); } }
