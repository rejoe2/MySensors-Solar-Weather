//Quelle: https://forum.mysensors.org/topic/841/solar-powered-mini-weather-station/94
//Interessant: https://github.com/chiemseesurfer/arduinoWeatherstation/blob/master/weatherstation/weatherstation.ino


    #include <SPI.h>
    #include <MySensor.h>  
    #include <dht.h>
    #include <BH1750.h>
    #include <Wire.h> 
    #include <Adafruit_BMP085.h>
    #include <MySigningAtsha204.h>
    
    #define CHILD_ID_HUM 0
    #define CHILD_ID_TEMP 1
    #define CHILD_ID_LIGHT 2
    #define CHILD_ID_BARO 3
    #define CHILD_ID_BTEMP 4
    #define CHILD_ID_WINDSPEED 5
    #define CHILD_ID_RAIN 6
    
    #define MESSAGEWAIT 500
    #define DIGITAL_INPUT_RAIN_SENSOR 3
    #define HUMIDITY_SENSOR_DIGITAL_PIN 5
    #define ENCODER_PIN 2
    #define INTERRUPT DIGITAL_INPUT_RAIN_SENSOR-2

    const int  windSpeedPin = 2; // contact on pin 2 digital
    int windSpeedPinCounter = 0; // impuls counter
    int windSpeedPinStatus = 0; // actual impuls
    int windSpeedPinStatusAlt = 0; // oude Impuls-Status
    unsigned long windmeterStart;
    unsigned long windmeterStartAlt = 0;
    int windSpeed; // Variable voor Wind Speed
    int beaufort = 0; // Variable Wind in Beaufort
    const int windmeterTime = 10000;
    //float knoten = 0.0;
    //float wind = 0.0;
    unsigned int knoten;
    unsigned int wind;
   
    boolean metric = false;
    int altitude = 16; // meters above sealevel
    float lastBmpTemp = -1;
    float lastPressure = -1;
    float lastHum = -1;
    float lastTemp = -1;
    double temp;
    double hum;
    int BATTERY_SENSE_PIN = A0;
    int lastRainValue = -1;
    int nRainVal;
    boolean bIsRaining = false;
    String strRaining = "NO";
    int lastBatteryPcnt = 0;
    int updateAll = 60;
    int updateCount = 0;
    uint16_t lastLux;
    // unsigned long SLEEP_TIME = 60000;
    unsigned long SLEEP_TIME = 600;
    int batteryBasement = 800;
    float batteryConstant = 100.0 / (1023 - batteryBasement);

    MyTransportNRF24 radio;  // NRFRF24L01 radio driver
    MyHwATMega328 hw; // Select AtMega328 hardware profile
    MySigningAtsha204 signer(true); // Select HW ATSHA signing backend
    
    Adafruit_BMP085 bmp = Adafruit_BMP085();
    BH1750 lightSensor;
    dht DHT;
    MySensor gw(radio, hw, signer);

    MyMessage msgHum(CHILD_ID_HUM, V_HUM);
    MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
    MyMessage msgLux(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
    MyMessage msgBtemp(CHILD_ID_BTEMP, V_TEMP);
    MyMessage msgPressure(CHILD_ID_BARO, V_PRESSURE);
    MyMessage msgWindSpeed(CHILD_ID_WINDSPEED, V_WIND);
    MyMessage msgWGust(CHILD_ID_WINDSPEED, V_GUST);
    MyMessage msgWDirection(CHILD_ID_WINDSPEED, V_DIRECTION);   
    MyMessage msgRain(CHILD_ID_RAIN, V_TRIPPED);

    void setup()  
    {
      analogReference(INTERNAL);
      gw.begin(incomingMessage, 3, true);  
      //dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 
      bmp.begin();
      gw.sendSketchInfo("Weather Sensor", "1.0");
      gw.present(CHILD_ID_HUM, S_HUM, "WS Humidity");
      gw.present(CHILD_ID_TEMP, S_TEMP, "WS Temperature");
      gw.present(CHILD_ID_LIGHT, S_LIGHT_LEVEL, "WS Lux");
      gw.present(CHILD_ID_BARO, S_BARO, "WS Pressure");
      gw.present(CHILD_ID_BTEMP, S_TEMP, "WS P Temperature");
      gw.present(CHILD_ID_WINDSPEED, S_WIND, "WS Windspeed");
      gw.present(CHILD_ID_RAIN, S_MOTION, "WS Rain");

      pinMode(DIGITAL_INPUT_RAIN_SENSOR, INPUT);
      lightSensor.begin();
      metric = gw.getConfig().isMetric;
    }
    
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
    
    void loop()      
    {
      updateCount += 1;
      if (updateCount == updateAll) {
        lastTemp = -1;
        lastHum = -1;
        lastLux = -1;
        lastBmpTemp = -1;
        lastPressure = -1;
        lastRainValue = -1;
        lastBatteryPcnt = -1;
        updateCount = 0;
      }
      delay(2000);
      int chk = DHT.read22(HUMIDITY_SENSOR_DIGITAL_PIN);
      temp = DHT.temperature;
             //Serial.print("Temperature DHT :");
             //Serial.println(temp);
      if (isnan(temp)) {
          lastTemp = -1;
      } else if (temp != lastTemp) {
        lastTemp = temp;
        if (!metric) {
          temp = temp * 1.8 + 32.0;
        }
        gw.send(msgTemp.set(temp, 1));
        }
      hum = DHT.humidity;
      if (isnan(hum)) {
          lastHum = -1;
      } else if (hum != lastHum) {
          lastHum = hum;
          gw.send(msgHum.set(hum, 1));
      }
      uint16_t lux = lightSensor.readLightLevel();
      if (lux != lastLux) {
          gw.send(msgLux.set(lux));
          lastLux = lux;
      }
      float pressure = bmp.readSealevelPressure(altitude) * 0.01;
      float bmptemp = bmp.readTemperature();
      if (!metric) {
        bmptemp = bmptemp * 1.8 + 32.0;
      }
      if (bmptemp != lastBmpTemp) {
        gw.send(msgBtemp.set(bmptemp,1));
        lastBmpTemp = bmptemp;
      }
      if (pressure != lastPressure) {
        gw.send(msgPressure.set(pressure, 0));
        lastPressure = pressure;
      }
      
      bIsRaining = !(digitalRead(DIGITAL_INPUT_RAIN_SENSOR));
      if(bIsRaining){
          strRaining = "YES";
      }
      else{
          strRaining = "NO";
      }
  
      //Serial.print("Raining?: ");
      //Serial.print(strRaining);  
      //Serial.print("\t Moisture Level: ");
      //Serial.println(nRainVal);
      //http://henrysbench.capnfatz.com/henrys-bench/arduino-sensors-and-input/arduino-rain-sensor-module-guide-and-tutorial/

      gw.send(msgRain.set(bIsRaining, 1));

      wind = windmeter();
      Serial.print("Wind : ");
      Serial.println(wind);
      int wdirection = 1;
      int wgust = 1;
      gw.send(msgWindSpeed.set(wind, 1));
      gw.send(msgWGust.set(wgust, 1));
      gw.send(msgWDirection.set(wdirection, 1));
      
      int sensorValue = analogRead(BATTERY_SENSE_PIN);
      int batteryPcnt = (sensorValue - batteryBasement) * batteryConstant;
      if (lastBatteryPcnt != batteryPcnt) {
        gw.sendBatteryLevel(batteryPcnt);
        lastBatteryPcnt = batteryPcnt;
      }
      gw.sleep(SLEEP_TIME);
   }

    void incomingMessage(const MyMessage & message) {
    // We only expect one type of message from controller. But we better check anyway.
     if (message.isAck()) {
       Serial.println("This is an ack from gateway");
     }
    }
