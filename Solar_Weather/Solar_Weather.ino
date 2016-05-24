/**
   The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters.
   Each repeater and gateway builds a routing tables in EEPROM which keeps track of
   the network topology allowing messages to be routed to nodes.

   Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
   Copyright (C) 2013-2015 Sensnology AB
   Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors

   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

 *******************************

   REVISION HISTORY
   Version 1.0 - idefix

   DESCRIPTION
   Basic Sketch was Arduino BH1750FVI Light sensor
   communicate using I2C Protocol
   this library enable 2 slave device addresses
   Main address  0x23
   secondary address 0x5C
   connect the sensor as follows :

     VCC  >>> 5V
     Gnd  >>> Gnd
     ADDR >>> NC or GND
     SCL  >>> A5
     SDA  >>> A4
   http://www.mysensors.org/build/light
   SDL_80422 for wind and rain measurement
*/


// Enable debug prints to serial monitor
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensor.h>
#include <BH1750.h>
#include <Wire.h>
#include "SDL_Weather_80422.h"

#define CHILD_CONFIG 103

#define CHILD_ID_LIGHT 1
#define CHILD_ID_RAIN 2
#define CHILD_ID_WIND 3
#define CHILD_ID_BATT 4

const unsigned long SLEEP_TIME = 180000;

BH1750 lightSensor;

// V_LIGHT_LEVEL should only be used for uncalibrated light level 0-100%.
// If your controller supports the new V_LEVEL variable, use this instead for
// transmitting LUX light level. (not yet supported by FHEM)
MyMessage msgLux(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
//MyMessage msgLux(CHILD_ID_LIGHT, V_LEVEL);
uint16_t lastlux = 0;


/* Obsolete using own interrupt routines using the SDL_80422-lib?
#define DIGITAL_INPUT_SENSOR1 2   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
#define INTERRUPT1 DIGITAL_INPUT_SENSOR1-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
MyMessage msg_M1(CHILD_ID_M1, V_TRIPPED);
#define DIGITAL_INPUT_SENSOR2 3   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
#define INTERRUPT2 DIGITAL_INPUT_SENSOR2-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
MyMessage msg_M2(CHILD_ID_M2, V_TRIPPED);*/

#define pinAnem    3  // Anenometer connected to pin 18 - Int 5 - Mega   / Uno pin 2
#define pinRain    2  // Gauge connected to pin 2 - Int 0 - Mega   / Uno Pin 3 
#define intAnem    1  // int 0 (check for Uno)
#define intRain    0  // int 1
#define pinWinddir A1
#define pinBattery A0

MyMessage msgBatLux(CHILD_ID_BATT, V_LIGHT_LEVEL);
MyMessage msgWindSpeed(CHILD_ID_WIND, V_WIND);
MyMessage msgWGust(CHILD_ID_WIND, V_GUST);
//Probably needs to be mapped manually within FHEM
MyMessage msgWDirection(CHILD_ID_WIND, V_DIRECTION);   
MyMessage msgRain(CHILD_ID_RAIN, V_RAIN);
MyMessage msgRainrate(CHILD_ID_RAIN, V_RAINRATE);

// initialize SDL_Weather_80422 library
SDL_Weather_80422 weatherStation(pinAnem, pinRain, intAnem, intRain, pinWinddir, SDL_MODE_INTERNAL_AD);
float currentWindSpeed;
float currentWindGust;
float totalRain;

//really needed???? SDL
uint8_t i;

int batteryBasement = 800;
float batteryConstant = 100.0 / (1023 - batteryBasement);


void setup()
{
  analogReference(INTERNAL);
  lightSensor.begin();
  metric = getConfig().isMetric;
  /*obsolete using SDL-lib?
  pinMode(DIGITAL_INPUT_SENSOR1, INPUT);      // sets the motion sensor digital pin as input
  attachInterrupt(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR1), raincounter, FALLING);
  pinMode(DIGITAL_INPUT_SENSOR2, INPUT);      // sets the motion sensor digital pin as input
  attachInterrupt(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR2), windcounter, FALLING);*/
};
  weatherStation.setWindMode(SDL_MODE_SAMPLE, 5.0);
  //weatherStation.setWindMode(SDL_MODE_DELAY, 5.0);
      totalRain = 0.0;

void presentation()  {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Weather Station", "0.1");

  // Register all sensors to gateway (they will be created as child devices)
  present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
  present(CHILD_ID_WIND, S_WIND);
  present(CHILD_ID_RAIN, S_RAIN);
  present(CHILD_ID_BATT, S_LIGHT_LEVEL);
};

void loop()
{
  //functions for windspeed, gust and direction needed incl. sending
  currentWindSpeed = weatherStation.current_wind_speed()/1.6;
  currentWindGust = weatherStation.get_wind_gust()/1.6;
  totalRain = totalRain + weatherStation.get_current_rain_total()/25.4;
  Serial.print("rain_total=");
  Serial.print(totalRain);
  Serial.print(""" wind_speed=");
  Serial.print(currentWindSpeed);
    Serial.print("MPH wind_gust=");
  Serial.print(currentWindGust);
  Serial.print("MPH wind_direction=");
  Serial.println(weatherStation.current_wind_direction());
  
  //dto. for battery level
  int sensorValue = analogRead(pinBattery);
  int batteryPcnt = (sensorValue - batteryBasement) * batteryConstant;
  if (lastBatteryPcnt != batteryPcnt) {
     send(msgBattery(batteryPcnt));
  lastBatteryPcnt = batteryPcnt;
  }


  uint16_t lux = lightSensor.readLightLevel();// Get Lux value
#ifdef MY_DEBUG
  Serial.println(lux);
#endif
  if (lux != lastlux) {
    send(msgLux.set(lux));
    lastlux = lux;
    };
  };
  sleep(SLEEP_TIME);
};

void raincounter() {
  // Read digital motion value
  tripped1 = digitalRead(DIGITAL_INPUT_SENSOR1) == HIGH;
#ifdef MY_DEBUG
  Serial.print("M1: ");
  Serial.println(tripped1);
#endif
  send(msg_M1.set(tripped1 ? "1" : "0")); // Send tripped values to gw
  if (lastlux < MinLux && tripped1 ) {
    digitalWrite(RELAY_1, RELAY_ON);
    send(msgRelay.set(true));
    onOff = true;
    switchtime = millis();
  };
};

void windcounter() {
  // Read digital motion value
  tripped2 = digitalRead(DIGITAL_INPUT_SENSOR2) == HIGH;
#ifdef MY_DEBUG
  Serial.print("M2: ");
  Serial.println(tripped2);
#endif
  send(msg_M2.set(tripped2 ? "1" : "0"));
};

// Wind Meter https://github.com/chiemseesurfer/arduinoWeatherstation/blob/master/weatherstation/weatherstation.ino
    
    float windmeter()
    {
        windmeterStart = millis(); // Actual start time measuringMessung
        windmeterStartAlt = windmeterStart; // Save start time
    
        windSpeedPinCounter = 0; // Set pulse counter to 0
        windSpeedPinStatusAlt = HIGH; // Set puls status High
    
        while ((windmeterStart - windmeterStartAlt) <= windmeterTime) // until 10000 ms (10 Seconds) ..
        {
            windSpeedPinStatus = digitalRead(windSpeedPin); // Read input pin 2
            if (windSpeedPinStatus != windSpeedPinStatusAlt) // When the pin status changed
            {
                if (windSpeedPinStatus == HIGH) // When status - HIGH
          {
              windSpeedPinCounter++; // Counter + 1
          }
      }
      windSpeedPinStatusAlt = windSpeedPinStatus; // Save status for next loop
      windmeterStart = millis(); // Actual time
  }
 
  windSpeed =  ((windSpeedPinCounter * 24) / 10) + 0.5; //  WindSpeed - one Pulse ~ 2,4 km/h, 
  windSpeed = (windSpeed / (windmeterTime / 1000)); // Devided in measure time in seconds
  Serial.print("wind Speed : ");
  Serial.println(windSpeed);    
  knoten = windSpeed / 1.852; //knot's
 
  return windSpeed;
}

