#include <RGB_LED.h>
#include <OledDisplay.h>
#include <Sensor.h>
#include "AZ3166WiFi.h"
#include "EEPROMInterface.h"
#include "SerialLog.h"
#include "DevKitMQTTClient.h"
#include "AzureIotHub.h"


RGB_LED led; 
DevI2C *i2c;
HTS221Sensor *sensor;
float humidity = 0;
float temperature = 0;
int delayMs = 5000;
char* addr;
char* sensorID = ""; //change this per-device
//int delayMs = 900000; //15 mins
static bool hasWifi = false;

static void InitWifi()
{
  // taken from Microsoft's "Getting Started" MXChip Project.
  // requires network to have already been set up via serial interface.
  if (WiFi.begin() == WL_CONNECTED)
  {
    IPAddress ip = WiFi.localIP();
    addr = ip.get_address();
    Screen.print(3, addr);
    hasWifi = true;
  }
  else
  {
    hasWifi = false;
    Screen.print(3, "No Wi-Fi\r\n ");
  }
}

void dispWifiState()
{
  if (hasWifi)
  {
    Screen.print(3, addr);
  }
  else{
    Screen.print(3, "No Wi-Fi\r\n");
  }
}

void sendData(const char *data)
{
  time_t t = time(NULL);
  char buf[sizeof "2011-10-08T07:07:09Z"];
  strftime(buf, sizeof buf, "%FT%TZ", gmtime(&t));

  EVENT_INSTANCE* message = DevKitMQTTClient_Event_Generate(data, MESSAGE);
  DevKitMQTTClient_Event_AddProp(message, "$$SensorID", sensorID);
  DevKitMQTTClient_Event_AddProp(message, "$$CreationTimeUtc", buf);
  DevKitMQTTClient_Event_AddProp(message, "$$MessageSchema", "temperature;v1");
  DevKitMQTTClient_Event_AddProp(message, "$$ContentType", "JSON");
 
  DevKitMQTTClient_SendEventInstance(message);
}

void setup() {
  // put your setup code here, to run once:
  // init sensors
  i2c = new DevI2C(D14, D15);
  sensor = new HTS221Sensor(*i2c);
  sensor -> init(NULL);

  // init screen
  Screen.init();
  Screen.print("LEMI ^^", false);
  // init wifi
  Screen.print(1, "Connecting...");
  hasWifi = false;
  InitWifi();
  if (!hasWifi)
  {
    return;
  }
  DevKitMQTTClient_Init();
}

void loop() {
  // put your main code here, to run repeatedly:
  sensor -> enable();
  sensor -> getTemperature(&temperature);
  sensor -> getHumidity(&humidity);
  // TODO: tidy this up a bit - nb, switch doesn't help here.
  if ( temperature < 17 )
  {
    led.setColor(0, 0, 255);
  }
  if ( temperature >= 18 )
  {
    led.setColor(0, 255, 0);
  }
  if ( temperature >=23 )
  {
    led.setColor(255, 69, 0);
  }
  if (temperature > 25 ) 
  {
    led.setColor(255, 0, 0);
  }
  char buff[20];
  sprintf(buff, "T: %sC", f2s(temperature, 1));
  Screen.print(1, buff);
  sprintf(buff, "H: %s", f2s(humidity, 1));
  Screen.print(2, buff);
  char sensorData[200];
  sprintf_s(sensorData, sizeof(sensorData), "{\"temperature\":%s,\"humidity\":%s}", f2s(temperature, 1), f2s(humidity, 1));
  sendData(sensorData);
  delay(delayMs);
}