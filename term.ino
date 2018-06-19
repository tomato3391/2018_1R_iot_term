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
int response_type = 1;
int motor_init = 0;
WiFiEspServer server(80);
RingBuffer buf(100);

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

void Motor_Act()
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
  digitalWrite(motor_dir1, LOW);
  digitalWrite(motor_dir2, LOW);
  digitalWrite(motor_pwm, HIGH);
}

void printWiFiStatus()
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

void sendHttpResponse(WiFiEspClient client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  if (win_status)
  {
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
  }
  else if (!win_status)
  {
    client.println("<html>");
    client.println("<body>");
    client.println("Current window status : \"CLOSE\".");
    client.println("<br/>");
    client.println("Be cautious: You must restart your program with your window opened to simulate once again.");
    client.println("</body>");
    client.println("</html>");
    if (motor_init == 1)
      Motor_Act();
    motor_init = 0;
  }
  else
  {
    client.println("Invalid Access to Server.");
  }
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
        {
          sendHttpResponse(client);
          break;
        }
        if (buf.endsWith("GET /close"))
        {
          if (win_status)
          {
            win_status = LOW;
            motor_init = 1;
          }
        }
      }
    }
    client.stop();
    Serial.println("Client disconnected");
  }
}
