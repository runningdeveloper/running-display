#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>

#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#include "config.h"

const char* ssid = SSID;
const char* password = SSID_PASS;
const char* connectionString = CONNECTION_STRING;

// for the button to debounce
int lastButtonState = 0;
int buttonState;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; 

// init led array so they are all blank (black)
int leds[18][3] = {
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0},
  {0,0,0}
};  

time_t epochTime;
static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void initWifi()
{
    // Attempt to connect to Wifi network:
    Serial.printf("Attempting to connect to SSID: %s.\r\n", ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.printf("Wifi connection to %s failed! Waiting 10 seconds to retry.\r\n", ssid);
      WiFi.begin(ssid, password);
      delay(10000);
    }
    Serial.printf("Connected to wifi %s.\r\n", ssid);
}

void initTime()
{
    configTime(0, 0, "pool.ntp.org", "time.nist.gov"); // maybe I shoud change to sa time

    while (true)
    {
        epochTime = time(NULL);

        if (epochTime == 0)
        {
            Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
            delay(2000);
        }
        else
        {
            Serial.printf("Fetched NTP epoch time is: %lu.\r\n", epochTime);
            break;
        }
    }
}

void initSerial()
{
    Serial.begin(115200);
    Serial.println("Serial successfully inited.");
}

// result of sending the messge to iothub
static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
    if (IOTHUB_CLIENT_CONFIRMATION_OK == result)
    {
        Serial.println("Message sent to Azure IoT Hub");
    }
    else
    {
        Serial.println("Failed to send message to Azure IoT Hub");
    }
}

// message from iot hub (mainly copied from sample)
IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void *userContextCallback)
{
    IOTHUBMESSAGE_DISPOSITION_RESULT result;
    const unsigned char *buffer;
    size_t size;
    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        Serial.println("Unable to get IoTHubMessage GetByteArray.");
        result = IOTHUBMESSAGE_REJECTED;
    }
    else
    {
        // buffer is not zero terminated
        char *temp = (char *)malloc(size + 1);

        if (temp == NULL)
        {
            return IOTHUBMESSAGE_ABANDONED;
        }

        strncpy(temp, (const char *)buffer, size);
        temp[size] = '\0';

        const size_t bufferSize = 18*JSON_ARRAY_SIZE(3) + JSON_ARRAY_SIZE(18) + JSON_OBJECT_SIZE(1) + 170;
        StaticJsonBuffer<bufferSize> jsonBuffer;

        JsonObject& root = jsonBuffer.parseObject(temp);
        JsonArray& dleds = root["leds"];
        dleds.copyTo(leds);

        Serial.println("Got message");
        dleds.prettyPrintTo(Serial);
        free(temp);
    }
    return IOTHUBMESSAGE_ACCEPTED;
}

void sendButtonPush()
{
  epochTime = time(NULL);
  char messagePayload[MESSAGE_MAX_LEN];
  StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["epoch"] = epochTime;
  root["deviceId"] = DEVICE_ID;

  root.printTo(messagePayload, MESSAGE_MAX_LEN);
  IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char *)messagePayload, strlen(messagePayload));
  if (messageHandle == NULL)
  {
      Serial.println("Unable to create a new IoTHubMessage.");
  }
  else
  {
      MAP_HANDLE properties = IoTHubMessage_Properties(messageHandle);
      Map_Add(properties, "button", "getStravaData");
      Serial.printf("Sending message: %s.\r\n", messagePayload);
      if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, NULL) != IOTHUB_CLIENT_OK)
      {
          Serial.println("Failed to hand over the message to IoTHubClient.");
      }
      else
      {
          Serial.println("IoTHubClient accepted the message for delivery.");
      }

      IoTHubMessage_Destroy(messageHandle);
  }

}

void setup()
{

  initSerial();
  delay(2000);

  initWifi();

  initTime();
  pinMode(14, INPUT_PULLUP); //button D5
  pixels.begin(); 

  // setup iot hub connection
  iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
  if (iotHubClientHandle == NULL)
  {
      Serial.println("Failed on IoTHubClient_CreateFromConnectionString.");
      while (1);
  }

  IoTHubClient_LL_SetOption(iotHubClientHandle, "product_info", "runningDisplay1");
  IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback, NULL);
  // IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, deviceMethodCallback, NULL); from sample for future
  // IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, twinCallback, NULL);

  IoTHubClient_LL_DoWork(iotHubClientHandle);
}

static int messageCount = 1;
void loop()
{
  // epochTime = time(NULL);
  // Serial.printf("Fetched NTP epoch time is: %lu.\r\n", epochTime);
  // debounce code https://www.arduino.cc/en/Tutorial/Debounce should have looked for a more general libray?
  int reading = digitalRead(14);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == 1) {
        Serial.println("button pushed.");
        sendButtonPush();
      }
    }
  }
  lastButtonState = reading;

  IoTHubClient_LL_DoWork(iotHubClientHandle);

  for(int i=0;i<18;i++){
    pixels.setPixelColor(i, pixels.Color(leds[i][0],leds[i][1],leds[i][2]));
    // for testing
    // pixels.setPixelColor(i, pixels.Color(0,150,0));
  }

  pixels.show(); 

  delay(20);    
}