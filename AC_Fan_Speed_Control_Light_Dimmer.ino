#include <IRremote.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC
#include <Time.h>         //http://www.arduino.cc/playground/Code/Time  
#include <Wire.h>         //http://arduino.cc/en/Reference/Wire (included with Arduino IDE)

// All wiring required, only 3 defines for hardware SPI on 328P
#define __DC 9
#define __CS 10
// MOSI --> (SDA) --> D11
#define __RST 12
// SCLK --> (SCK) --> D13

// Color definitions
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0  
#define WHITE   0xFFFF

TFT_ILI9163C tft = TFT_ILI9163C(__CS, __DC, __RST);

#define PIN_LIGHT_RELAY        2
#define PIN_FAN_RELAY          3

#define PIN_LIGHT_TRIAC        4
#define PIN_FAN_TRIAC_1        5
#define PIN_FAN_TRIAC_2        6
#define PIN_FAN_TRIAC_3        7
#define PIN_FAN_TRIAC_4        8

#define PIN_IR_RECEIVER        A1

IRrecv irrecv(PIN_IR_RECEIVER);
decode_results results;

int fan_speed = 0;              // Fan speed (0-9)  0 = off, 9 = full speed
boolean bLights = false;
boolean bFan = false;
float tempC;
String tempCString;
int count = 0;
boolean bAutoMode = true;
boolean bAutoModeTriggered = false;
float AutoModeTemp = 23.50;

#define DEFAULT_FAN_SPEED       3

#define ONE_WIRE_BUS A0
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);


void digitalClockDisplay(void)
{
    tft.setTextColor(MAGENTA);  
    tft.setTextSize(1);

    //    Serial.print(hour());
    tft.print(hour());

    printDigits(minute());
    printDigits(second());
    //    Serial.print(' ');
    tft.print(' ');
    //    Serial.print(day());
    //    tft.print(day());
    //    Serial.print(' ');
    //    tft.print(' ');
    //    Serial.print(month());
    //    tft.print(month());
    //    Serial.print(' ');
    //    tft.print(' ');
    //    Serial.print(year()); 
    //    tft.print(year()); 
    //    Serial.println(); 
    tft.println(); 
}

void printDigits(int digits)
{
    // utility function for digital clock display: prints preceding colon and leading 0
    //    Serial.print(':');
    tft.print(':');
    if(digits < 10)
    {
        //        Serial.print('0');
        tft.print('0');
     }
     
     //    Serial.print(digits);
     tft.print(digits);
}

void clearDisp()
{
    tft.clearScreen();
} 

void updateDisp(){
    clearDisp();
    tft.setCursor(0,0);

    digitalClockDisplay();

    tft.setTextSize(1);
    tft.println();

    tft.setTextColor(WHITE);  
    tft.setTextSize(2);
    tft.println("Temp");

    tft.setTextColor(YELLOW);  
    tft.setTextSize(2);
    tft.print(tempC);
    tft.println(" C");
  //    tft.println();

    tft.setTextColor(GREEN);
    tft.setTextSize(1);
    tft.println();
    tft.println("Auto Switch Off");
    
    if (bAutoMode)
    {
        tft.print("  YES");
        if(bAutoModeTriggered)
        {
            tft.println("  Triggered");
        }
        else
        {
            tft.setTextColor(RED);
            tft.println("  Not triggered");
        }
    }
    else
    {
        tft.setTextColor(RED);
        tft.println("  NO");
    }

    tft.print("  Temperature:");
    tft.setTextColor(GREEN);
    tft.println(AutoModeTemp);
    tft.println();

    tft.setTextColor(WHITE);  
    tft.setTextSize(2);
    tft.println("Fan Light");

    if(bFan)
        tft.setTextColor(RED);
    else
        tft.setTextColor(GREEN);
        
    tft.setTextSize(3);
    tft.print(fan_speed);
    
    if(bLights)
    {
        tft.setTextColor(RED);
        tft.println("  On");
    }
    else
    {
        tft.setTextColor(GREEN);
        tft.println("  Off");
    }
    
    count=0;
}


void getTemperature()
{
    char buffer[10];
  
    sensors.requestTemperatures(); // Send the command to get temperatures
    //    Serial.print("Temp C: ");
    //    Serial.println(sensors.getTempCByIndex(0)); 

    tempC = (float)sensors.getTempCByIndex(0);
    tempCString = dtostrf(tempC, 4, 1, buffer);
    //    Serial.print("tempC: ");
    //    Serial.println(tempC);
}

void change_fan_speed()
{
    if(!bFan)
       return;
       
    switch (fan_speed)
    {
        case 0:
            digitalWrite(PIN_FAN_TRIAC_1, LOW);
            digitalWrite(PIN_FAN_TRIAC_2, LOW);
            digitalWrite(PIN_FAN_TRIAC_3, LOW);
            digitalWrite(PIN_FAN_TRIAC_4, LOW);                
            break;
        case 1:
           digitalWrite(PIN_FAN_TRIAC_1, HIGH);
           digitalWrite(PIN_FAN_TRIAC_2, LOW);
           digitalWrite(PIN_FAN_TRIAC_3, LOW);
           digitalWrite(PIN_FAN_TRIAC_4, LOW);                
           break;
       case 2:
           digitalWrite(PIN_FAN_TRIAC_1, LOW);
           digitalWrite(PIN_FAN_TRIAC_2, LOW);
           digitalWrite(PIN_FAN_TRIAC_3, HIGH);
           digitalWrite(PIN_FAN_TRIAC_4, LOW);                
           break;
       case 3:
          digitalWrite(PIN_FAN_TRIAC_1, HIGH);
          digitalWrite(PIN_FAN_TRIAC_2, LOW);
          digitalWrite(PIN_FAN_TRIAC_3, HIGH);
          digitalWrite(PIN_FAN_TRIAC_4, LOW);                
          break;
      case 4:
          digitalWrite(PIN_FAN_TRIAC_1, LOW);
          digitalWrite(PIN_FAN_TRIAC_2, HIGH);
          digitalWrite(PIN_FAN_TRIAC_3, HIGH);
          digitalWrite(PIN_FAN_TRIAC_4, LOW);                
          break;
      case 5:
          digitalWrite(PIN_FAN_TRIAC_1, HIGH);
          digitalWrite(PIN_FAN_TRIAC_2, HIGH);
          digitalWrite(PIN_FAN_TRIAC_3, HIGH);
          digitalWrite(PIN_FAN_TRIAC_4, LOW);                
          break;
       case 6:
          digitalWrite(PIN_FAN_TRIAC_1, LOW);
          digitalWrite(PIN_FAN_TRIAC_2, LOW);
          digitalWrite(PIN_FAN_TRIAC_3, LOW);
          digitalWrite(PIN_FAN_TRIAC_4, HIGH);                
          break;
    }
}


void setup()
{
    sensors.begin();
//    Serial.begin(9600);
    irrecv.enableIRIn(); // Starts the receiver

    pinMode(PIN_LIGHT_RELAY, OUTPUT);
    pinMode(PIN_FAN_RELAY,   OUTPUT);

    pinMode(PIN_LIGHT_TRIAC, OUTPUT);
    pinMode(PIN_FAN_TRIAC_1, OUTPUT);
    pinMode(PIN_FAN_TRIAC_2, OUTPUT);
    pinMode(PIN_FAN_TRIAC_3, OUTPUT);
    pinMode(PIN_FAN_TRIAC_4, OUTPUT);

    tft.begin();
    delay(100);

    digitalWrite(PIN_LIGHT_RELAY, HIGH);
    digitalWrite(PIN_FAN_RELAY,   HIGH);

    setSyncProvider(RTC.get);   // the function to get the time from the RTC

    getTemperature();
    updateDisp();
}

void loop(){ 
    count++;
  
    if(count > 20000)
    {
        count = 0;
        getTemperature();
        updateDisp();

        if (bAutoMode)
        {
      //            Serial.println("Checking if temperature is below 23.5 C ....");
            if (tempC < AutoModeTemp)
            {
                digitalWrite(PIN_FAN_TRIAC_1, LOW);
                digitalWrite(PIN_FAN_TRIAC_2, LOW);
                digitalWrite(PIN_FAN_TRIAC_3, LOW);
                digitalWrite(PIN_FAN_TRIAC_4, LOW);                
                bAutoModeTriggered = true;
                bFan = false;
            }
        }
    }
    else if (count == 10000)
    {
        clearDisp();
    }

    if (irrecv.decode(&results))
    {
        long int cmd = results.value;
//        Serial.println(cmd);

        switch (cmd)
        {
            case 16738455: // 0
                fan_speed = 0;
                bFan = false;
                change_fan_speed();
                updateDisp();
                break;
            case 16724175: // 1 
                fan_speed = 1;
                bFan = true;
                change_fan_speed();
                updateDisp();
                break;
            case 16718055: // 2 
                fan_speed = 2;
                bFan = true;
                change_fan_speed();
                updateDisp();
                break;
            case 16743045: // 3
                fan_speed = 3;
                bFan = true;
                change_fan_speed();
                updateDisp();
                break;
            case 16716015: // 4
                fan_speed = 4;
                bFan = true;
                change_fan_speed();
                updateDisp();
                break;
            case 16726215: // 5
                fan_speed = 5;
                bFan = true;
                change_fan_speed();
                updateDisp();
                break;
            case 16734885: // 6
                fan_speed = 6;
                bFan = true;
                change_fan_speed();
                updateDisp();
                break;

            case 16748655: // + 
                if(fan_speed<6)
                    fan_speed++;
                bFan = true;
                change_fan_speed();
                updateDisp();
                break;
            case 16754775: // -
                if(fan_speed>0)
                    fan_speed--;
                if(fan_speed == 0)
                {
                    bFan = false;
                }
                change_fan_speed();
                updateDisp();
                break;

            case 16753245: // "Power"
                if (bLights)
                {
                    bLights = false;
                    digitalWrite(PIN_LIGHT_TRIAC, LOW);
                }
                else
                {
                    bLights = true;
                    digitalWrite(PIN_LIGHT_TRIAC, HIGH);
                }
                updateDisp();
                break;
            case 16769565: // "Mute"
                if (bFan)
                {
                    bFan = false;
                }
                else
                {
                    bFan = true;
                    if (fan_speed == 0)
                        fan_speed = DEFAULT_FAN_SPEED;
                }
                change_fan_speed();
                updateDisp();
                break;

            case 16736925: // "Mode"
                if(bAutoMode)
                    bAutoMode = false;
                else
                    bAutoMode = true;
                    
                updateDisp();
                break;
                
            case 16712445: //  |<< (rewind)
                AutoModeTemp = AutoModeTemp - 0.25;
                updateDisp();
                break;    
            case 16761405: //  >>| (forward)
                AutoModeTemp = AutoModeTemp + 0.25;
                bAutoMode = false;
                updateDisp();
                break;
        }

        irrecv.resume(); // Receives the next value from the button you press
    }

    delay(2);
}
