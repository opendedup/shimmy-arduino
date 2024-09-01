#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Streaming.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#define LED_PIN 13

Adafruit_INA219 ina219;
String a;
#define LED_COUNT 64
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

boolean ready = false;

unsigned long pixelPrevious = 0;   // Previous Pixel Millis
unsigned long patternPrevious = 0; // Previous Pattern Millis
int patternCurrent = 0;            // Current Pattern Number
int patternInterval = 5000;        // Pattern Interval (ms)
bool patternComplete = false;

int pixelInterval = 50;           // Pixel Interval (ms)
int pixelQueue = 0;               // Pattern Pixel Queue
int pixelCycle = 0;               // Pattern Pixel Cycle
uint16_t pixelNumber = LED_COUNT; // Total Number of Pixels

void get_power_readings()
{
  JsonDocument doc;

  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  doc["busvoltage"] = busvoltage;
  doc["shuntvoltage"] = shuntvoltage;
  doc["current_mA"] = current_mA;
  doc["power_mW"] = power_mW;
  doc["loadvoltage"] = loadvoltage;
  serializeJson(doc, Serial);
  Serial.println("");
}

uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbow(uint8_t wait)
{
  if (pixelInterval != wait)
    pixelInterval = wait;
  for (uint16_t i = 0; i < pixelNumber; i++)
  {
    strip.setPixelColor(i, Wheel((i + pixelCycle) & 255)); //  Update delay time
  }
  strip.show(); //  Update strip to match
  pixelCycle++; //  Advance current cycle
  if (pixelCycle >= 256)
    pixelCycle = 0; //  Loop the cycle back to the begining
}

void setup(void)
{
  // You can change the console prompt before calling begin(). By default it is "ESP32>"

  // You can change the baud rate and pin numbers similar to Serial.begin() here.
  Serial.begin(115200);
  Serial.setTimeout(1000);
  while (!Serial)
  {
  }

  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  if (!ina219.begin())
  {
    Serial.println("Failed to find INA219 chip");
    while (1)
    {
      delay(10);
    }
  }
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  // ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  // ina219.setCalibration_16V_400mA();

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(60); // Set BRIGHTNESS to about 1/5 (max = 255)
  strip.fill((0, 0, 0), 0, 64);
  strip.show();
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void loop()
{
  char buffer[2048];
  int cnt = 0;
  if (ready)
  {
    JsonDocument json_doc;
    ready = false;
    DeserializationError error = deserializeJson(json_doc, buffer);
    if (error)
    {
      Serial.print(F("{\"error\":\"Failed to deserialize\",\"reason\": \""));
      Serial.print(error.c_str());
      Serial.println("\"}");
    }
    else
    {
      const char *command = json_doc["command"];
      if (strcmp(command, "power") == 0)
      {
        get_power_readings();
      }
      else if (strcmp(command, "led") == 0)
      {
        const char *subcommand = json_doc["subcommand"];
        if (strcmp(subcommand, "pattern") == 0)
        {
          const char *pattern = json_doc["pattern"];
          if (strcmp(pattern, "rainbow") == 0)
          {
            rainbow(10);
            strip.show();
          }
        }
        else if (strcmp(subcommand, "turnoff") == 0)
        {
          strip.fill((0, 0, 0), 0, 64);
          strip.show();
        }
        else if (strcmp(subcommand, "fill") == 0)
        {
          JsonArray data = json_doc["colors"];
          int start = json_doc["start"];
          int num = json_doc["num"];
          uint16_t r = data[0];
          uint16_t b = data[1];
          uint16_t g = data[2];
          uint32_t color = strip.Color(r, g, b);
          strip.fill(color, start, num);
          strip.show();
        }
        else if (strcmp(subcommand, "paint") == 0)
        {
          JsonArray pixels = json_doc["pixels"];
          int pixel = 0;
          for (JsonVariant item : pixels)
          {
            int r = item[0];
            int b = item[1];
            int g = item[2];
            strip.setPixelColor(pixel, r, g, b);
            pixel++;
          }
          strip.show();
        }
        else if (strcmp(subcommand, "brightness") == 0)
        {
          int brigthtness = json_doc["value"];
          strip.setBrightness(brigthtness);
          strip.show();
        }
        JsonDocument doc;
        doc["status"] = "done";
        serializeJson(doc, Serial);
        Serial.println("");
      }
      else
      {
        JsonDocument doc;
        doc["error"] = "command not found";
        serializeJson(doc, Serial);
        Serial.println("");
      }
    }
  }
  else
    while (Serial.available())
    {
      char c = Serial.read();
      buffer[cnt++] = c;
      if ((c == '\n') || (cnt == sizeof(buffer) - 1))
      {
        buffer[cnt] = '\0';
        cnt = 0;
        ready = true;
      }
    }
}
