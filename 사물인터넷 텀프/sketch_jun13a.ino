#include <DHT11.h>
#include "WiFiEsp.h"
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#endif

int pin_dht11=4;
int pin_DC_A = 10;
int pin_DC_B = 11;

DHT11 dht11(pin_dht11); 

char ssid[] = "Twim";            // your network SSID (name)
char pass[] = "12345678";        // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

WiFiEspServer server(80);

// use a ring buffer to increase speed and reduce memory allocation
RingBuffer buf(8);

void setup()
{
  Serial.begin(9600);
  Serial1.begin(9600);
  // DC A pin Output Setup
  pinMode(pin_DC_A, OUTPUT);
  pinMode(pin_DC_B, OUTPUT);
  WiFi.init(&Serial1);

   if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network");
  printWifiStatus();
  
  // start the web server on port 80
  server.begin();
}

void Motor_CW()
{  
  digitalWrite(pin_DC_A, LOW);
  digitalWrite(pin_DC_B, HIGH);
}

void Motor_CCW()
{  
  digitalWrite(pin_DC_B, LOW);
  digitalWrite(pin_DC_A, HIGH);
}

void Motor_Stop(void)
{  
  digitalWrite(pin_DC_A, LOW);
  digitalWrite(pin_DC_B, LOW);
}

float getHumidity()
{
  float humi;
  float temp;
  if(dht11.read(humi, temp)==0)
  {
    return humi;
  }
  else
  {
    return -1;
  }
}

float getTemperature()
{
  float humi;
  float temp;
  if(dht11.read(humi, temp)==0)
  {
    return temp;
  }
  else
  {
    return -1;
  }
}



void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  
  // print where to go in the browser
  Serial.println();
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
  Serial.println();
}

void sendHttpResponse(WiFiEspClient client)
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  
  // the content of the HTTP response follows the header:
  client.println("<br>");
  
  client.println("<a href=\"/A\">Get Humidity</a>");
  client.println("<a href=\"/B\">Get Temparature</a>");
  client.println("<a href=\"/C\">Rotate Motor CW</a>");
  client.println("<a href=\"/D\">Rotate Motor CCW</a>");
  client.println("<a href=\"/E\">Motor Stop</a>");
  
  // The HTTP response ends with another blank line:
  client.println();
}

void loop()
{
  WiFiEspClient client = server.available();  // listen for incoming clients

  if (client) {                               // if you get a client,
    Serial.println("New client");             // print a message out the serial port
    buf.init();                               // initialize the circular buffer
    while (client.connected()) {              // loop while the client's connected
      if (client.available()) {               // if there's bytes to read from the client,
        char c = client.read();               // read a byte, then
        buf.push(c);                          // push it to the ring buffer

        // printing the stream to the serial monitor will slow down
        // the receiving of data from the ESP filling the serial buffer
        //Serial.write(c);
        
        // you got two newline characters in a row
        // that's the end of the HTTP request, so send a response
        if (buf.endsWith("\r\n\r\n")) {
          sendHttpResponse(client);
          break;
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (buf.endsWith("GET /A")) {
          Serial.println(getHumidity());
        }
        else if (buf.endsWith("GET /B")) {
          Serial.println(getTemperature());
        }
        else if (buf.endsWith("GET /C")) {
          Motor_CW();
        }
        else if (buf.endsWith("GET /D")) {
          Motor_CCW();
        }
        else if (buf.endsWith("GET /E")) {
          Motor_Stop();
        }
        
      }
    }
    
    // close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
}


