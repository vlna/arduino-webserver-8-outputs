// This is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <UIPEthernet.h>

#define SERDEBUG //serial port debugging
#define RELAYS 8 //amount of controlled outputs (1..8)
#define INVERTED //inverted outputs

////////////////////////////////////////////////////////////////////////

#if RELAYS<1 or RELAYS>8
  #error "RELAYS must be in range 1..8"
#endif

//first relay, next relays are on pin 3 up to 9
const byte relayPin = 2;

byte relayValues = 0; //all relays off

byte mac[] = {
  0xDE, 0xAD, 0xFF, 0xEE, 0xDD, 0x01
};
IPAddress ip(192, 168, 2, 177);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

void setup() {
  // Open serial communications and wait for port to open:

  #ifdef SERDEBUG
    Serial.begin(9600);
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
  #endif
  
  for (int i=0;i<RELAYS;i++) {
    pinMode(relayPin+i, OUTPUT);
    digitalWrite(relayPin+i,(relayValues & (1<<i))?LOW:HIGH); 
  }
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();

  #ifdef SERDEBUG
    Serial.print("server is at ");
    Serial.println(Ethernet.localIP());
  #endif
}

void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    
    #ifdef SERDEBUG
      Serial.println("new client");
    #endif
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String currentLine = "";
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        #ifdef SERDEBUG
          Serial.write(c);
        #endif
        
        // send a reply after empty line

        // one time only on GET line, % escaped characters not allowed, spaces not allowed
        if ( (currentLine.length()==(5+RELAYS)) && currentLine.startsWith(F("GET /")) && currentLine.lastIndexOf(' ')==3 && currentLine.indexOf('%',4)==-1 ) {SetRelays(currentLine.substring(5));}
        
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html"));
          client.println(F("Connection: close"));  // closed after completion of the response
          client.println();
          client.println(F("<!DOCTYPE HTML>"));
          client.println(F("<html><style>a {font-size:10ex;text-decoration:none;} a.on {color:green;} a.off {color:red;}</style>"));

          for (int i=0;i<RELAYS;i++) {
            String relayChange = "XXXXXXXX";
            byte relayValue=relayValues & (1<<i);
            if (relayValue){
               #ifdef SERDEBUG
                 Serial.print(i+1);
                 Serial.println("=on");
              #endif
              client.print(F("<a class=\"on\" href=\"/"));
              relayChange.setCharAt(i,'0');
              client.print(relayChange.substring(0,RELAYS));
              client.println(F("\">&#9646</a>"));
            } else {
               #ifdef SERDEBUG
                 Serial.print(i+1);
                 Serial.println("=off");
              #endif
              client.print(F("<a class=\"off\" href=\"/"));
              relayChange.setCharAt(i,'1');
              client.print(relayChange.substring(0,RELAYS));
              client.println(F("\">&#9647</a>"));
            }
          }
          client.println(F("</html>"));
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          currentLine = "";
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
          if (currentLine.length()<=15) {
             currentLine = currentLine+c;
          }
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();

    #ifdef SERDEBUG
      Serial.println("client disconnected");
    #endif
  }
}

void SetRelays (String relays) {
  for (int i=0;i<RELAYS;i++) {
//    if (relays[i]==' ') return; //space is end of valid data
    if (relays[i]=='1') {
      #ifdef SERDEBUG
        Serial.println();
        Serial.print(i);
        Serial.println("->on");
      #endif
      #ifdef INVERTED
        digitalWrite(i+relayPin, LOW);
      #else
        digitalWrite(i+relayPin, HIGH);
      #endif
      relayValues=relayValues | (1<<i);
    }
    if (relays[i]=='0') {
      #ifdef SERDEBUG
        Serial.println();
        Serial.print(i);
        Serial.println("->off");
      #endif
      #ifdef INVERTED
        digitalWrite(i+relayPin, HIGH);
      #else
        digitalWrite(i+relayPin, LOW);
      #endif
      relayValues=relayValues & ~(1<<i);
    }
  }
}
