#include <IRremote.h>
#include  <TimerOne.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_LIGHT_RELAY        2
#define PIN_FAN_RELAY          3

#define PIN_LIGHT_TRIAC        A0
#define PIN_FAN_TRIAC_1        A1
#define PIN_FAN_TRIAC_2        A2
#define PIN_FAN_TRIAC_3        A3
#define PIN_FAN_TRIAC_4        A4

#define PIN_IR_RECEIVER        8

IRrecv irrecv(PIN_IR_RECEIVER);
decode_results results;

volatile int i=0;               // Variable to use as a counter
volatile boolean zero_cross=0;  // Boolean to store a "switch" to tell us if we have crossed zero
int fan_speed = 0;              // Fan speed (0-9)  0 = off, 9 = full speed

const int digitPins[4] = {4,5,6,7};  //4 common anode pins of the display
const int clockPin = 11;             //74HC595 Pin 11 
const int latchPin = 12;             //74HC595 Pin 12
const int dataPin = 13;              //74HC595 Pin 14

const byte digit[10] =               //seven segment digits in bits
{
  B00111111, //0
  B00000110, //1
  B01011011, //2
  B01001111, //3
  B01100110, //4
  B01101101, //5
  B01111101, //6
  B00000111, //7
  B01111111, //8
  B01101111  //9
};

int digitBuffer[4] = {0};
int digitScan = 0;
float tempC;
int count = 0;
boolean bAutoMode = true;

// ************** ONE WIRE Ds1820
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 9
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

void clearDisp()
{
    for(byte j=0; j<4; j++)  
        digitalWrite(digitPins[j], HIGH);

    digitalWrite(latchPin, LOW);  
    shiftOut(dataPin, clockPin, MSBFIRST, B00000000);
    shiftOut(dataPin, clockPin, MSBFIRST, B00000000);
    digitalWrite(latchPin, HIGH);
} 

// Write one digit of temperature on the 4 digit display and the 
// fan speed in the 1 digit display
void updateDisp(){
    clearDisp();

    digitalWrite(digitPins[digitScan], LOW); 

    digitalWrite(latchPin, LOW);  

    // Push fan speed to single digit 7 segment display connected to the second 74HC595 register
    shiftOut(dataPin, clockPin, MSBFIRST, digit[fan_speed]);

    // Push tempC digit to 4 digit 7 segment display connected to the first 74HC595 register
    if(digitScan==2)
        shiftOut(dataPin, clockPin, MSBFIRST, (digit[digitBuffer[digitScan]] | B10000000)); //print the decimal point on the 3rd digit
    else
        shiftOut(dataPin, clockPin, MSBFIRST, digit[digitBuffer[digitScan]]);

    digitalWrite(latchPin, HIGH);

    digitScan++;
    if(digitScan>3)
        digitScan=0; 
}


void getTemperature()
{
    Serial.println("*********** DS1820 *********");
    sensors.requestTemperatures(); // Send the command to get temperatures
    Serial.print("Temp C: ");
    Serial.println(sensors.getTempCByIndex(0)); 

    tempC = (float)sensors.getTempCByIndex(0) * 100;
    Serial.print("tempC: ");
    Serial.println(tempC);

    digitBuffer[3] = int(tempC)/1000;
    digitBuffer[2] = (int(tempC)%1000)/100;
    digitBuffer[1] = (int(tempC)%100)/10;
    digitBuffer[0] = (int(tempC)%100)%10;
}

void change_fan_speed()
{
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
            digitalWrite(PIN_FAN_TRIAC_2, HIGH);
            digitalWrite(PIN_FAN_TRIAC_3, LOW);
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
    Serial.begin(9600);
    irrecv.enableIRIn(); // Starts the receiver

    pinMode(PIN_LIGHT_RELAY, OUTPUT);
    pinMode(PIN_FAN_RELAY,   OUTPUT);

    pinMode(PIN_LIGHT_TRIAC, OUTPUT);
    pinMode(PIN_FAN_TRIAC_1, OUTPUT);
    pinMode(PIN_FAN_TRIAC_2, OUTPUT);
    pinMode(PIN_FAN_TRIAC_3, OUTPUT);
    pinMode(PIN_FAN_TRIAC_4, OUTPUT);

    for(int k=0;k<4;k++)
    {
        pinMode(digitPins[k],OUTPUT);
    }

    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);

    digitalWrite(PIN_LIGHT_RELAY, HIGH);
    digitalWrite(PIN_FAN_RELAY,   HIGH);

    getTemperature();
}

void loop(){ 
    if(++count > 1000)
    {
        clearDisp();
        getTemperature();
        count = 0;
        
        if (bAutoMode)
        {
            Serial.println("Checking if temperature is below 22.0 C ....");
            if (tempC < 2200)
            {
                digitalWrite(PIN_FAN_TRIAC_1, LOW);
                digitalWrite(PIN_FAN_TRIAC_2, LOW);
                digitalWrite(PIN_FAN_TRIAC_3, LOW);
                digitalWrite(PIN_FAN_TRIAC_4, LOW);                
            }
        }
    }
   
    //decodes the infrared input
    if (irrecv.decode(&results))
    {
        long int cmd = results.value;
        Serial.println(cmd);

        switch (cmd)
        {
            case 16738455: /* 0 */
                fan_speed = 0;
                change_fan_speed();
                break;
            case 16724175: /* 1 */
                fan_speed = 1;
                change_fan_speed();
                break;
            case 16718055: /* 2 */
                fan_speed = 2;
                change_fan_speed();
                break;
            case 16743045: /* 3 */
                fan_speed = 3;
                change_fan_speed();
                break;
            case 16716015: /* 4 */
                fan_speed = 4;
                change_fan_speed();
                break;
            case 16726215: /* 5 */
                fan_speed = 5;
                change_fan_speed();
                break;
            case 16734885: /* 6 */
                fan_speed = 6;
                change_fan_speed();
                break;
            case 16748655: /* + */
                if(fan_speed<6)
                    fan_speed++;
                change_fan_speed();
                break;
            case 16754775: /* - */
                if(fan_speed>0)
                    fan_speed--;
                change_fan_speed();
                break;
            case 16736925: /* Mode */
                digitalWrite(PIN_LIGHT_TRIAC, LOW);
                break;
            case 16769565: /* mute */
                digitalWrite(PIN_LIGHT_TRIAC, HIGH);
                break;
            case 16712445: /* rewind */
                bAutoMode = true;
                break;
            case 16761405: /* forward */
                bAutoMode = false;
                break;
        }
        irrecv.resume(); // Receives the next value from the button you press
    }

    updateDisp();
    delay(2);
}
