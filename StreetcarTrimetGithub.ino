/*

 Class used to parse a website to display
 Trimet information on an LED matrix
 
 Grabs information from a webiste that's
 populated with trimet_update.py
 
 
 Author: Charles Dunbar
 
 */

#include <Ethernet.h>
#include <EthernetUdp.h>
#include <HttpClient.h>
#include <SPI.h>
#include <Time.h>
#include <stdlib.h>
#include <ht1632c.h>

// Need to use port names of ATMEGA2560, not Arduino 2560
// See http://arduino.cc/en/Hacking/PinMapping2560
// PORTC maps to these arduino pins below
// ht1632c led = ht1632c(&PORTC, 30, 31, 32, 33, GEOM_32x16, 3);


///////////////////////////////////////////////////////////
// Global Variables

// Support 10 routes for now, 20 chars per route line
#define MAX_ROUTES 10
#define MAX_CHARS 20

// Set up LED matrix
ht1632c led = ht1632c(&PORTC, 7, 6, 5, 4, GEOM_32x16, 3);

// URL to parse
char* url = "site.com";
char* path = "/trimet";
// Configure ethernet with static IP
EthernetClient client;
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE }; 
IPAddress ip(192,168,0,240);
// Configure NTP data
EthernetUDP Udp;
IPAddress timeServer(216, 228, 192, 69); // nist-time-server.eoni.com
const int timeZone = -8;  // Pacific Standard Time (USA)
unsigned int localPort = 8888;  // local port to listen for UDP packets
// Variables used for printing the time
String timeString;
char stringBuf[50];
time_t prevDisplay = 0; // when the digital clock was displayed

char routes[MAX_ROUTES][MAX_CHARS];
///////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  Serial.println("Streetcar Trimet Program");
  
  // Clear LED Matrix, set pwm to 1
  led.clear();
  led.pwm(1);
  timeString = String ();
  // Configure ethernet
  Ethernet.begin(mac,ip);
  Serial.print("IP is ");
  Serial.println(Ethernet.localIP());
  Udp.begin(localPort);
  Serial.println("Starting loop");
}

// Loop() starts by setting time via NTP every hour
// followed by updating the time and variables used
// to store arrivals every minute on the top half.
// Every 5 seconds the bottom half is updated with an
// arrival time
void loop() {

  //NTP Update code here
  setSyncProvider(getNtpTime);

  // Run every minute for an hour, then restart loop()
  for (int i = 0; i < 59; i++){
    //Update clock time display
    if (timeStatus() != timeNotSet) {
      if (now() != prevDisplay) { //update the display only if time has changed
        prevDisplay = now();
        digitalClockDisplay();
        led.centertext(0, stringBuf, RED);
      }
    }

    // Update arrays of streetcar time
    int retCode = update_streetcar_time();
    Serial.print("Return code was ");
    Serial.println(retCode);
    if (retCode != 200) { // Wait one sec before trying website again
      delay(1000);
      i--;
      continue;  // Restart loops, don't want bad data in array
    }

    int n = 0;  // Counter used in next loop
    for (int x = 0; x < 12; x++){ // 5s * 12 = 60s,
      // Cycle through streetcar time and print
      // new value every 5s
      if (routes[n][0] < 48){ // 48 == 0 in ASCII
        n = 0; // End of filled arrays, restart
      }
      Serial.println("Displaying new data");
      for(int j = 0; j < MAX_CHARS; j++){
        Serial.print(routes[n][j]);
      }
      // Reset led, set time and bottom text
      led.clear();
      led.centertext(0, stringBuf, RED);
      led.centertext(8, routes[n], RED);
      n++;
      delay(5000);
    }
  }
}

int update_streetcar_time() {
  // Edited form the example in
  // https://github.com/amcewen/HttpClient
  int err = 0;
  int retCode = 0;

  // Reset arrays
  for (int i = 0; i < MAX_ROUTES; i++) {
    for (int j = 0; j < MAX_CHARS; j++){
      routes[i][j] = '\0';
    }
  }

  EthernetClient c;
  HttpClient http(c);

  err = http.get(url, path);
  if (err == 0)
  {
    Serial.println("startedRequest ok");

    err = http.responseStatusCode();
    if (err >= 0)
    {
      Serial.print("Got status code: ");
      Serial.println(err);
      retCode = err;

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get

      err = http.skipResponseHeaders();
      if (err >= 0)
      {
        int bodyLen = http.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println();
        Serial.println("Body returned follows:");

        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;
        int i = 0;
        int j = 0;
        // Whilst we haven't timed out & haven't reached the end of the body
        while ( http.connected() || http.available() )
        {
          if (http.available())
          {
            c = http.read();
            // Print out this character
            Serial.print(c);
            if (c == '\n') {
              i++;
              j = 0;
              continue;
              //Serial.println("Incremented string");
            }
            routes[i][j] = c;
            j++;
          }
          else{
            // Break out when connection done - likes to hang without this
            break;
          }
          //Serial.println("http no longer available");
        }
        //Serial.println("Http no longer connected");
      }
      else
      {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    }
    else
    {    
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();
  return retCode;
}

// Digital clock display of the time
// Build a string, then convert it back to a charArray
void digitalClockDisplay() {
  timeString = "";
  timeString += "The time is ";
  timeString += hour();
  printDigits(minute());
  timeString.toCharArray(stringBuf, 50);
}

// Utility for digital clock display: prints preceding colon and leading 0
void printDigits(int digits){
  timeString += ":";
  if(digits < 10)
    timeString += '0';
  timeString += digits;
}

/*-------- NTP code ----------*/
// Grabbed from TimeNTP Example code

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}









