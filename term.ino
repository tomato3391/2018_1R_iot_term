#include "WiFiEsp.h"

#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7);
#endif

char ssid[] = "";
char pass[] = "";
int status = WL_IDLE_STATUS;
int pin_rain = A0;
int motor_dir1 = 12;
int motor_dir2 = 13;
int motor_pwm = 11;
int win_status = HIGH;
int rain_pos = 0;
WiFiEspServer server(80);
RingBuffer buf(8);

void setup()
{
  pinMode(motor_dir1, OUTPUT);
  pinMode(motor_dir2, OUTPUT);
  pinMode(motor_pwm, OUTPUT);
  pinMode(pin_rain, INPUT);

  digitalWrite(motor_dir1, LOW);
  digitalWrite(motor_dir2, LOW);
  digitalWrite(motor_pwm, HIGH);
  Serial.begin(115200);
  Serial1.begin(9600);
  WiFi.init(&Serial1);

  if (WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println("WiFi Shield not present");
    while(true);
  }
  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  }
  Serial.println("You're connected to the network");
  printWiFiStatus();
  server.begin();
}

void Motor_On()
{
  unsigned long pretime = millis();
  unsigned long curtime = millis();
  digitalWrite(motor_dir1, LOW);
  digitalWrite(motor_dir2, HIGH);
  while (curtime - pretime < 10000)
  {
    analogWrite(motor_pwm, 255);
    curtime = millis();
  }
}

void Motor_Off()
{
  digitalWrite(motor_dir1, LOW);
  digitalWrite(motor_dir2, LOW);
  digitalWrite(motor_pwm, HIGH);
}

void printWifiStatus()
{
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println();
  Serial.print("To see this page in action, open a browser with ");
  Serial.println(ip);
  Serial.println();
}

void main_page(WiFiEspClient client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  client.println("<html>");
  client.println("<body>");
  client.print("Current window status : ");
  client.println(win_status? "Open" : "Close");
  client.print("Possibility of raindrop(0~1023) : ");
  client.println(rain_pos);
  client.println("<br/>");
  client.println("<a href=\"/close\">CLOSE WINDOW RIGHT NOW!</a>");
  client.println("</body>");
  client.println("</html>");
  client.println();
}

void close_page(WiFiEspClient client)
{
  client.println("HTTP/1.1 200 OK);
  client.println("Content-type:text/html");
  client.println();
  client.println("Current window status is \"CLOSE\".");
  client.println("Be cautious: You must restart your program with your window opened to simulate once again.");
  client.println();
}

void loop()
{
  rain_pos = 1023 - analogRead(pin_rain);
  WiFiEspClient client = server.available();
  if (client)
  {
    Serial.println("New Client");
    buf.init();
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        buf.push(c);
        if (buf.endsWith("\r\n\r\n"))
          break;
        else if (buf.endsWith("GET / "))
          response_type = 1;
        else if (buf.endsWith("GET /close "))
          response_type = 2;
      }
    }
    switch(response_type)
    {
      case 1: main_page(client); break;
      case 2: Motor_On(); Motor_Off(); win_status = LOW; close_page(client); break;
    }
  }
}
