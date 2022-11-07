#include <Wire.h>
#include "DHT.h"
#include <Adafruit_BMP280.h>
#include <Adafruit_CCS811.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "MHZ19.h"

#define DHTPIN 14     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

Adafruit_BMP280 bmp; // use I2C interface
Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();

Adafruit_CCS811 ccs;

#define RX_PIN 13                                          // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 12                                          // Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600                                      // Device to MH-Z19 Serial baudrate (should not be changed)
MHZ19 myMHZ19;                                             // Constructor for library
unsigned long getDataTimer = 0;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT1 32 // OLED display height, in pixels
#define SCREEN_HEIGHT0 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT1, &Wire1, OLED_RESET);
Adafruit_SSD1306 display0(SCREEN_WIDTH, SCREEN_HEIGHT0, &Wire, OLED_RESET);

#define arraySizeLong 1200
#define arraySize 128
float DHTtempArray[arraySizeLong];
float DHThumArray[arraySize];
float CCSeCO2Array[arraySize];
float CCSTVOCArray[arraySize];
float BMPtempArray[arraySize];
float BMPpressArray[arraySize];
float MHZCO2Array[arraySizeLong];
int scrollIndex = 0;


void setup() {
  Wire1.setSCL(27);
  Wire1.setSDA(26);
  Wire.setSCL(5);
  Wire.setSDA(4);

  display1.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Default OLED address, usually
  display0.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Second OLED address, via onboard jumper

  display0.clearDisplay();
  display0.setTextSize(1);
  display0.setTextColor(WHITE);
  display0.cp437(true);         // Use full 256 char 'Code Page 437' font

  display1.clearDisplay();
  display1.setTextSize(1);
  display1.setTextColor(WHITE);
  display1.setCursor(0, 0);
  display1.cp437(true);         // Use full 256 char 'Code Page 437' font

  display1.clearDisplay();
  display0.clearDisplay();

  display1.drawRoundRect(0, 0, 128, 32, 1, SSD1306_WHITE);
  display0.drawRoundRect(0, 0, 128, 64, 1, SSD1306_WHITE);

  display1.setTextSize(3);
  display1.setCursor(16, 6);
  display1.print("Hello!");

  display0.setCursor(12, 16);
  display0.print("Indoor Air Quality");
  display0.setCursor(42, 24);
  display0.print("Monitor");
//  display0.setCursor(10, 52);
//  display0.print("www.niallsmith.com");

  display1.display();
  display0.display();

  delay(5000);

  display1.clearDisplay();
  display0.clearDisplay();

  display1.drawRoundRect(0, 0, 128, 32, 1, SSD1306_WHITE);
  display0.drawRoundRect(0, 0, 128, 64, 1, SSD1306_WHITE);

  display1.setTextSize(3);
  display1.setCursor(16, 6);
  display1.print("Hello!");

  display0.setCursor(16, 16);
  display0.print("Designed & built");
  display0.setCursor(58, 24);
  display0.print("by");
  display0.setCursor(16, 36);
  display0.print("Niall Smith 2022");

  display1.display();
  display0.display();

  //Serial.begin(9600);
  //while ( !Serial ) delay(100);   // wait for native usb

  //Serial.println(F("DHTxx test!"));
  dht.begin();

  //Serial.println(F("BMP280 Sensor event test"));
  unsigned status;
  status = bmp.begin(0x76);

  if (!status) {
    //    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
    //                     "try a different address!"));
    //    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(), 16);
    //    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    //    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    //    Serial.print("        ID of 0x60 represents a BME 280.\n");
    //    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  bmp_temp->printSensorDetails();

  if (!ccs.begin()) {
    //Serial.println("Failed to start sensor! Please check your wiring.");
    while (1);
  }

  while (!ccs.available()); {
  }

  Serial1.setRX(RX_PIN);
  Serial1.setTX(TX_PIN);
  Serial1.begin(BAUDRATE);
  myMHZ19.begin(Serial1);                                // *Serial(Stream) refence must be passed to library begin().
  myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))

  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(20, OUTPUT);

  digitalWrite(18, HIGH);
  digitalWrite(19, HIGH);
  digitalWrite(20, HIGH);
  delay(3000);
  digitalWrite(18, LOW);
  digitalWrite(19, LOW);
  digitalWrite(20, LOW);

  pinMode(6, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  pinMode(9, INPUT);

}




void loop() {

  /* Loop structure:
     1. Read data from all sensors
     2. Update data array
     3. Switch case statement from encoder output to choose data visual output
     4. Display LEDs
  */


  //
  // DHT22
  //

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float DHThum = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float DHTtemp = dht.readTemperature();


  //   Check if any reads failed and exit early (to try again).
  if (isnan(DHThum) || isnan(DHTtemp)) {
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float DHThi = dht.computeHeatIndex(DHTtemp, DHThum, false);

  //  Serial.print(F("DHT22: Humidity: "));
  //   Serial.print(DHThum);
  //  Serial.print(F("%  Temperature: "));
  //  Serial.print(DHTtemp);
  //  Serial.print(F("°C "));
  //  Serial.print(F("Heat index: "));
  //  Serial.print(DHThi);
  //  Serial.println(F("°C "));

  //
  // BMP280
  //

  sensors_event_t temp_event, pressure_event;
  bmp_temp->getEvent(&temp_event);
  bmp_pressure->getEvent(&pressure_event);

  //  Serial.print(F("BMP280: Temperature = "));
  //  Serial.print(temp_event.temperature);
  //  Serial.print(" °C  ");
  float BMPtemp = temp_event.temperature;
  float BMPpress = pressure_event.pressure;

  //  Serial.print(F("Pressure = "));
  //  Serial.print(pressure_event.pressure);
  //  Serial.println(" hPa");

  //
  // CCS811
  //

  int CCSeCO2;
  int CCSTVOC;

  if (ccs.available()) {

    if (!ccs.readData()) {

      CCSeCO2 = ccs.geteCO2();
      CCSTVOC = ccs.getTVOC();
    }
    //    else {
    //      Serial.println("CCS811 ERROR!");
    //    }
  }

  //  Serial.print(F("CCS811: eCO2 = "));
  //  Serial.print(CCSeCO2);
  //  Serial.print(" ppm  ");
  //
  //  Serial.print(F("TVOC = "));
  //  Serial.print(CCSTVOC);
  //  Serial.println(" ppb");

  //
  // MHZ-19
  //
  int MHZCO2;

  /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even
    if below background CO2 levels or above range (useful to validate sensor). You can use the
    usual documented command with getCO2(false) */

  MHZCO2 = myMHZ19.getCO2();                             // Request CO2 (as ppm)

  //  Serial.print("CO2 (ppm): ");
  //  Serial.println(MHZCO2);

  int8_t Temp;
  Temp = myMHZ19.getTemperature();                     // Request Temperature (as Celsius)
  //  Serial.print("Temperature (C): ");
  //  Serial.println(Temp);

  //
  // Update data arrays
  //

  for (int i = arraySize - 2; i > 0; i--) {
    DHThumArray[i] = DHThumArray[i - 1];
    CCSeCO2Array[i] = CCSeCO2Array[i - 1];
    CCSTVOCArray[i] = CCSTVOCArray[i - 1];
    BMPtempArray[i] = BMPtempArray[i - 1];
    BMPpressArray[i] = BMPpressArray[i - 1];
  }

  for (int i = arraySizeLong - 2; i > 0; i--) {
    DHTtempArray[i] = DHTtempArray[i - 1];
    MHZCO2Array[i] = MHZCO2Array[i - 1];
  }

  DHThumArray[0] = DHThum;
  CCSeCO2Array[0] = CCSeCO2;
  CCSTVOCArray[0] = CCSTVOC;
  BMPtempArray[0] = BMPtemp;
  BMPpressArray[0] = BMPpress;
  DHTtempArray[0] = DHTtemp;
  MHZCO2Array[0] = MHZCO2;

  //
  // ENCODER
  //

  int code8 = digitalRead(6);
  int code4 = digitalRead(7);
  int code2 = digitalRead(8);
  int code1 = digitalRead(9);

  byte code8421 = 0b00000000;
  code8421 = bitWrite(code8421, 3, code8);
  code8421 = bitWrite(code8421, 2, code4);
  code8421 = bitWrite(code8421, 1, code2);
  code8421 = bitWrite(code8421, 0, code1);

  int encoder_int = code8421;
  //  Serial.print("Encoder: ");
  //  Serial.println(encoder_int, HEX);

  display0.clearDisplay();
  display0.setTextSize(1);
  display0.setCursor(0, 0);

  display1.clearDisplay();
  display1.setTextSize(1);
  display1.setCursor(0, 0);



  switch (encoder_int) {
    case 0: //0{
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display0.display();
        display1.display();

        digitalWrite(18, LOW);
        digitalWrite(19, LOW);
        digitalWrite(20, LOW);

      }
      break;

    case 1: //1
      {
        int scrollSelect = floor(scrollIndex / 2);
        switch (scrollSelect) {
          case 0: //idx = 0 or 1
            {
              display0.clearDisplay();
              display1.clearDisplay();

              display1.setTextSize(1);
              display1.print("Current Temperature:");

              display0.setCursor(32, 16);
              display0.setTextSize(3);
              display0.print(DHTtemp, 1);
              display0.setCursor(96, 48);
              display0.setTextSize(2);
              display0.print((char)248);
              display0.print("C");

              display1.setTextSize(1);
              display1.setCursor(90, 25); display1.print((char)4);
              display1.setCursor(98, 25); display1.print((char)9);
              display1.setCursor(106, 25); display1.print((char)9);
              display1.setCursor(114, 25); display1.print((char)9);
              display1.setCursor(122, 25); display1.print((char)9);

              display0.display();
              display1.display();

              digitalWrite(18, LOW);
              digitalWrite(19, LOW);
              digitalWrite(20, LOW);

            }
            break;

          case 1: //idx = 2 or 3
            {
              display0.clearDisplay();
              display1.clearDisplay();

              display1.setTextSize(1);
              display1.print("Current Humidity:");

              display0.setCursor(32, 16);
              display0.setTextSize(3);
              display0.print(DHThum, 1);
              display0.setCursor(100, 48);
              display0.setTextSize(2);
              display0.print("%");

              display1.setTextSize(1);
              display1.setCursor(90, 25); display1.print((char)9);
              display1.setCursor(98, 25); display1.print((char)4);
              display1.setCursor(106, 25); display1.print((char)9);
              display1.setCursor(114, 25); display1.print((char)9);
              display1.setCursor(122, 25); display1.print((char)9);

              display0.display();
              display1.display();

              digitalWrite(18, LOW);
              digitalWrite(19, LOW);
              digitalWrite(20, LOW);

            }
            break;

          case 2: //idx = 4 or 5
            {
              display0.clearDisplay();
              display1.clearDisplay();

              display1.setTextSize(1);
              display1.print("Current CO2:");

              display0.setCursor(38, 16);
              display0.setTextSize(3);
              display0.print(MHZCO2);
              display0.setCursor(90, 48);
              display0.setTextSize(2);
              display0.print("ppm");

              display1.setTextSize(1);
              display1.setCursor(90, 25); display1.print((char)9);
              display1.setCursor(98, 25); display1.print((char)9);
              display1.setCursor(106, 25); display1.print((char)4);
              display1.setCursor(114, 25); display1.print((char)9);
              display1.setCursor(122, 25); display1.print((char)9);

              display0.display();
              display1.display();

              digitalWrite(18, LOW);
              digitalWrite(19, LOW);
              digitalWrite(20, LOW);

            }
            break;

          case 3: //idx = 6 or 7
            {
              display0.clearDisplay();
              display1.clearDisplay();

              display1.setTextSize(1);
              display1.print("Current air pressure:");

              display0.setCursor(12, 16);
              display0.setTextSize(3);
              display0.print(BMPpress, 1);
              display0.setCursor(90, 48);
              display0.setTextSize(2);
              display0.print("hPa");

              display1.setTextSize(1);
              display1.setCursor(90, 25); display1.print((char)9);
              display1.setCursor(98, 25); display1.print((char)9);
              display1.setCursor(106, 25); display1.print((char)9);
              display1.setCursor(114, 25); display1.print((char)4);
              display1.setCursor(122, 25); display1.print((char)9);

              display0.display();
              display1.display();

              digitalWrite(18, LOW);
              digitalWrite(19, LOW);
              digitalWrite(20, LOW);

            }
            break;
          case 4: //idx = 8 or 9
            {
              display0.clearDisplay();
              display1.clearDisplay();

              display1.setTextSize(1);
              display1.print("Current heat index:");

              display0.setCursor(32, 16);
              display0.setTextSize(3);
              display0.print(DHThi, 1);
              display0.setCursor(96, 48);
              display0.setTextSize(2);
              display0.print((char)248);
              display0.print("C");

              display1.setTextSize(1);
              display1.setCursor(90, 25); display1.print((char)9);
              display1.setCursor(98, 25); display1.print((char)9);
              display1.setCursor(106, 25); display1.print((char)9);
              display1.setCursor(114, 25); display1.print((char)9);
              display1.setCursor(122, 25); display1.print((char)4);

              display0.display();
              display1.display();

              digitalWrite(18, LOW);
              digitalWrite(19, LOW);
              digitalWrite(20, LOW);

            }
            break;
        }

        scrollIndex = scrollIndex + 1;
        if (scrollIndex >= 10) scrollIndex = 0;

      }
      break;

    case 2: //2
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display1.print("Temp.: ");
        display1.setTextSize(2);
        display1.print(DHTtemp, 1);
        display1.setTextSize(1);
        display1.print(" ");
        display1.print((char)248);
        display1.print("C");

        display0.print("Humid.: ");
        display0.setTextSize(2);
        display0.print(DHThum, 1);
        display0.setTextSize(1);
        display0.print(" %");

        display0.setCursor(0, 32);
        display0.print("Heat In.: ");
        display0.setTextSize(2);
        display0.print(DHThi);
        display0.setTextSize(1);
        display0.print(" ");
        display0.print((char)248);
        display0.print("C");

        display0.display();
        display1.display();

        ledDisplay(CCSeCO2, 1000, 600);
      }
      break;

    case 3: //3
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display1.print("CO2: ");
        display1.setTextSize(2);
        display1.print(MHZCO2);
        display1.setTextSize(1);
        display1.print(" ppm");

        display0.print("eCO2: ");
        display0.setTextSize(2);
        display0.print(CCSeCO2);
        display0.setTextSize(1);
        display0.print(" ppm");

        display0.setCursor(0, 32);
        display0.print("TVOC: ");
        display0.setTextSize(2);
        display0.print(CCSTVOC);
        display0.setTextSize(1);
        display0.print(" ppb");

        display0.display();
        display1.display();

        ledDisplay(CCSeCO2, 1000, 600);
      }
      break;

    case 4: //4
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display1.print("Temp.: ");
        display1.setTextSize(2);
        display1.print(DHTtemp, 1);
        display1.setTextSize(1);
        display1.print(" ");
        display1.print((char)248);
        display1.print("C");

        display0.print("Humid.: ");
        display0.setTextSize(2);
        display0.print(DHThum, 1);
        display0.setTextSize(1);
        display0.println(" %");

        display0.setCursor(0, 32);
        display0.print("CO2: ");
        display0.setTextSize(2);
        display0.print(MHZCO2);
        display0.setTextSize(1);
        display0.print(" ppm");

        display0.display();
        display1.display();

        ledDisplay(CCSeCO2, 1000, 600);
      }
      break;

    case 5: //5
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display1.print("Temp.: ");
        display1.setTextSize(2);
        display1.print(DHTtemp, 1);
        display1.setTextSize(1);
        display1.print(" ");
        display1.print((char)248);
        display1.print("C");

        display0.print("Humid.: ");
        display0.setTextSize(2);
        display0.print(DHThum, 1);
        display0.setTextSize(1);
        display0.print(" %");

        display0.setCursor(0, 32);
        display0.print("Press.: ");
        display0.setTextSize(2);
        display0.print(BMPpress, 0);
        display0.setTextSize(1);
        display0.print(" hPa");

        display0.display();
        display1.display();

        ledDisplay(CCSeCO2, 1000, 600);
      }
      break;

    case 6: //6
      {
        display0.clearDisplay();
        display1.clearDisplay();

        float maxdat = 0;
        float mindat = 10000;
        float MHZCO2total = 0;

        for (int i = 0; i < 1200; i++) {
          if (MHZCO2Array[i] >= maxdat) {
            maxdat = MHZCO2Array[i];
          }
          if (MHZCO2Array[i] <= mindat) {
            mindat = MHZCO2Array[i];
          }
          MHZCO2total = MHZCO2total + MHZCO2Array[i];
        }

        float MHZCO2avg = MHZCO2total / 1200;

        display1.print("CO2: ");
        display1.setTextSize(2);
        display1.print(MHZCO2avg, 0);
        display1.setTextSize(1);
        display1.print(" ppm");

        display0.print("Min.: ");
        display0.setTextSize(2);
        display0.print(mindat, 0);
        display0.setTextSize(1);
        display0.print(" ppm");

        display0.setCursor(0, 32);
        display0.print("Max.: ");
        display0.setTextSize(2);
        display0.print(maxdat, 0);
        display0.setTextSize(1);
        display0.print(" ppm");

        display0.display();
        display1.display();

        ledDisplay(MHZCO2, 1000, 600);
      }
      break;

    case 7: //7
      {
        display0.clearDisplay();
        display1.clearDisplay();

        float maxdat = 0;
        float mindat = 10000;
        float DHTtemptotal = 0;

        for (int i = 0; i < 1200; i++) {
          if (DHTtempArray[i] >= maxdat) {
            maxdat = DHTtempArray[i];
          }
          if (DHTtempArray[i] <= mindat) {
            mindat = DHTtempArray[i];
          }
          DHTtemptotal = DHTtemptotal + DHTtempArray[i];
        }

        float DHTtempavg = DHTtemptotal / 1200;

        display1.print("Temp: ");
        display1.setTextSize(2);
        display1.print(DHTtempavg);
        display1.setTextSize(1);
        display1.print(" ");
        display1.print((char)248);
        display1.print("C");

        display0.print("Min.: ");
        display0.setTextSize(2);
        display0.print(mindat, 1);
        display0.setTextSize(1);
        display0.print(" ");
        display0.print((char)248);
        display0.print("C");

        display0.setCursor(0, 32);
        display0.print("Max.: ");
        display0.setTextSize(2);
        display0.print(maxdat, 1);
        display0.setTextSize(1);
        display0.print(" ");
        display0.print((char)248);
        display0.print("C");

        display0.display();
        display1.display();

        ledDisplay(MHZCO2, 1000, 700);
      }
      break;

    case 8: //8
      {
        display0.clearDisplay();
        display1.clearDisplay();

        int mins = floor(millis() / 60000);
        int hours = floor(mins / 60);
        int days = floor(hours / 24);

        display1.setTextSize(1);
        display1.println("This program has been");
        display1.print("running for:");

        display0.setCursor(0, 0);
        display0.setTextSize(2);
        display0.print(days);
        display0.print(" day(s)");

        display0.setCursor(0, 20);
        display0.setTextSize(2);
        display0.print(hours % 24);
        display0.print(" hour(s)");

        display0.setCursor(0, 40);
        display0.setTextSize(2);
        display0.print(mins % 60);
        display0.print(" min(s)");

        display0.display();
        display1.display();

        digitalWrite(18, LOW);
        digitalWrite(19, LOW);
        digitalWrite(20, LOW);
      }
      break;

    case 9: //9
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display1.setCursor(0, 0);
        display1.print("DHT Temperature: ");
        display1.setTextSize(2);
        display1.setCursor(48, 16);
        display1.print(DHTtemp, 1);
        display1.setTextSize(1);
        display1.print(" ");
        display1.print((char)248);
        display1.print("C");

        screenDisplay(DHTtempArray);
        display0.setCursor(96, 24);
        display0.print("temp.");
        display0.setCursor(96, 34);
        display0.print("(");
        display0.print((char)248);
        display0.print("C)");

        display0.display();
        display1.display();

        ledDisplay(CCSeCO2, 1000, 600);
      }
      break;

    case 10: //A
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display1.setCursor(0, 0);
        display1.print("Humidity: ");
        display1.setTextSize(2);
        display1.setCursor(48, 16);
        display1.print(DHThum, 1);
        display1.setTextSize(1);
        display1.print(" %");

        screenDisplay(DHThumArray);
        display0.setCursor(96, 24);
        display0.print("hum.");
        display0.setCursor(96, 34);
        display0.print("(%)");

        display0.display();
        display1.display();

        ledDisplay(CCSeCO2, 1000, 600);
      }
      break;

    case 11: //B
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display1.setCursor(0, 0);
        display1.print("eCO2: ");
        display1.setTextSize(2);
        display1.setCursor(48, 16);
        display1.print(CCSeCO2);
        display1.setTextSize(1);
        display1.print(" ppm");

        screenDisplay(CCSeCO2Array);
        display0.setCursor(96, 24);
        display0.print("eCO2");
        display0.setCursor(96, 34);
        display0.print("(ppm)");

        display0.display();
        display1.display();

        ledDisplay(CCSeCO2, 1000, 600);
      }
      break;

    case 12: //C
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display1.setCursor(0, 0);
        display1.print("Pressure: ");
        display1.setTextSize(2);
        display1.setCursor(32, 16);
        display1.print(BMPpress, 1);
        display1.setTextSize(1);
        display1.print(" hpa");

        screenDisplay(BMPpressArray);
        display0.setCursor(96, 24);
        display0.print("press.");
        display0.setCursor(96, 34);
        display0.print("(hpa)");

        display0.display();
        display1.display();

        ledDisplay(CCSeCO2, 1000, 600);
      }
      break;

    case 13: //D
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display1.setCursor(0, 0);
        display1.print("BMP Temperature: ");
        display1.setTextSize(2);
        display1.setCursor(48, 16);
        display1.print(BMPtemp);
        display1.setTextSize(1);
        display1.print(" ");
        display1.print((char)248);
        display1.print("C");

        screenDisplay(BMPtempArray);
        display0.setCursor(96, 24);
        display0.print("temp.");
        display0.setCursor(96, 34);
        display0.print("(");
        display0.print((char)248);
        display0.print("C)");

        display0.display();
        display1.display();

        ledDisplay(CCSeCO2, 1000, 600);
      }
      break;

    case 14: //E
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display1.setCursor(0, 0);
        display1.print("CO2: ");
        display1.setTextSize(2);
        display1.setCursor(48, 16);
        display1.print(MHZCO2);
        display1.setTextSize(1);
        display1.print(" ppm");

        screenDisplay(MHZCO2Array);
        display0.setCursor(96, 24);
        display0.print("co2");
        display0.setCursor(96, 34);
        display0.print("(ppm)");

        display0.display();
        display1.display();

        ledDisplay(MHZCO2, 1000, 600);
      }
      break;

    case 15: //F
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display1.setTextSize(1);
        display1.setCursor(0, 0);
        display1.print("DHT");
        display1.setCursor(0, 10);
        display1.print(DHTtemp, 1);
        display1.setCursor(0, 20);
        display1.print(DHThum, 1);

        display1.setTextSize(1);
        display1.setCursor(64, 0);
        display1.print("BMP");
        display1.setCursor(64, 10);
        display1.print(BMPtemp);
        display1.setCursor(64, 20);
        display1.print(BMPpress);

        display0.setTextSize(1);
        display0.setCursor(0, 0);
        display0.print("CCS");
        display0.setCursor(0, 10);
        display0.print(CCSeCO2);
        display0.setCursor(0, 20);
        display0.print(CCSTVOC);

        display0.setTextSize(1);
        display0.setCursor(64, 0);
        display0.print("MHZ");
        display0.setCursor(64, 10);
        display0.print(MHZCO2);

        int mins = floor(millis() / 60000);
        display0.setTextSize(1);
        display0.setCursor(0, 42);
        display0.print("TIME");
        display0.setCursor(0, 52);
        display0.print(mins);

        display0.setTextSize(1);
        display0.setCursor(64, 42);
        display0.print("N. SMITH");
        display0.setCursor(64, 52);
        display0.print("AUG 2022");

        display0.display();
        display1.display();

        digitalWrite(18, LOW);
        digitalWrite(19, LOW);
        digitalWrite(20, LOW);
      }
      break;

    default:
      {
        display0.clearDisplay();
        display1.clearDisplay();

        display0.print("error!");
        display1.print("error!");

        display0.display();
        display1.display();
      }
      break;
  }

  //Serial.println();
  delay(2500);
}

void screenDisplay(float dataArray[]) {

  float maxdat = 0;
  float mindat = 10000;

  for (int i = 0; i < 72; i++) {
    if (dataArray[i] >= maxdat) {
      maxdat = dataArray[i];
    }
    if (dataArray[i] <= mindat) {
      mindat = dataArray[i];
    }
  }

  float y[72];

  for (int i = 0; i < 72; i++) {
    //float a = min(dataArray[i], maxdat);
    //a = max(a, mindat);
    y[i] = round(63 * ((dataArray[i] - mindat) / (maxdat - mindat)));
  }

  for (int i = 0; i < 72; i++) {
    display0.drawPixel(88 - i, 63 - y[i], SSD1306_WHITE);
    //display0.drawFastVLine(88 - i, 63 - y[i - 1], y[i] - y[i - 1], SSD1306_WHITE);
  }

  int maxdatdisp = maxdat;
  int mindatdisp = mindat;

  display0.setCursor(96, 0);
  display0.print(maxdatdisp);
  display0.setCursor(96, 56);
  display0.print(mindatdisp);
}


void ledDisplay(int var, int redlim, int yllwlim) {
  if (var >= redlim) {
    digitalWrite(18, LOW);
    digitalWrite(19, LOW);
    digitalWrite(20, HIGH);
  }
  else if (var >= yllwlim) {
    digitalWrite(18, LOW);
    digitalWrite(19, HIGH);
    digitalWrite(20, LOW);
  }
  else {
    digitalWrite(18, HIGH);
    digitalWrite(19, LOW);
    digitalWrite(20, LOW);
  }
}
