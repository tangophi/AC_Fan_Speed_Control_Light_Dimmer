#include <IRremote.h>
#include <dht11.h>
#include  <TimerOne.h>
#include <OneWire.h>
#include <DallasTemperature.h>

int PIN_IR_RECEIVER = 8; 
IRrecv irrecv(PIN_IR_RECEIVER);
decode_results results;

volatile int i=0;               // Variable to use as a counter
volatile boolean zero_cross=0;  // Boolean to store a "switch" to tell us if we have crossed zero
int AC_pin = 3;                 // Output to Opto Triac
int fan_speed = 0;              // Fan speed (0-9)  0 = off, 9 = full speed
int fan_speed_trigger = 0;      // Fan speed (0-9)  0 = off, 9 = full speed

// This is the delay-per-brightness step in microseconds.
// It is calculated based on the frequency of your voltage supply (50Hz or 60Hz)
// and the number of brightness steps you want. 
// 
// The only tricky part is that the chopper circuit chops the AC wave twice per
// cycle, once on the positive half and once at the negative half. This meeans
// the chopping happens at 120Hz for a 60Hz supply or 100Hz for a 50Hz supply. 

// To calculate freqStep you divide the length of one full half-wave of the power
// cycle (in microseconds) by the number of brightness steps. 
//
// (1000000 uS / 100 Hz) / 9 brightness steps = 1111 uS / brightness step
//
// 1000000 us / 100 Hz = 10000 uS, length of one half-wave.
int freqStep = 1111;    

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

void zero_cross_detect() {    
    zero_cross = true;               // set the boolean to true to tell our dimming function that a zero cross has occured
    i=0;
   digitalWrite(AC_pin, LOW);       // turn off TRIAC (and AC)
}                                 

// Turn on the TRIAC at the appropriate time
void dim_check() {                   
    if(zero_cross == true)
    {              
        if(i > (9-fan_speed_trigger))
        {                      
            digitalWrite(AC_pin, HIGH); // turn on fan       
            i=0;                        // reset time step counter                         
            zero_cross = false;         // reset zero cross detection
        } 
        else
        {
            i++; // increment time step counter                     
        }                                
    }                                  
}

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

void setup()
{
    pinMode(AC_pin, OUTPUT);                          // Set the Triac pin as output
    attachInterrupt(0, zero_cross_detect, RISING);   // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection

    // Use the TimerOne Library to attach an interrupt
    // to the function we use to check to see if it is 
    // the right time to fire the triac.  This function 
    // will now run every freqStep in microseconds.                                            
    Timer1.initialize(freqStep);                      // Initialize TimerOne library for the freq we need
    Timer1.attachInterrupt(dim_check, freqStep);      

    sensors.begin();
    Serial.begin(9600);
    irrecv.enableIRIn(); // Starts the receiver

    for(int k=0;k<4;k++)
    {
        pinMode(digitPins[k],OUTPUT);
    }

    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);

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
                fan_speed_trigger = 0;
            }
            else
            {
                fan_speed_trigger = fan_speed;
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
                break;
            case 16724175: /* 1 */
                fan_speed = 1;
                break;
            case 16718055: /* 2 */
                fan_speed = 2;
                break;
            case 16743045: /* 3 */
                fan_speed = 3;
                break;
            case 16716015: /* 4 */
                fan_speed = 4;
                break;
            case 16726215: /* 5 */
                fan_speed = 5;
                break;
            case 16734885: /* 6 */
                fan_speed = 6;
                break;
            case 16728765: /* 7 */
                fan_speed = 7;
                break;
            case 16730805: /* 8 */
                fan_speed = 8;
                break;
            case 16732845: /* 9 */
                fan_speed = 9;
                break;
            case 16748655: /* + */
                if(fan_speed<9)
                    fan_speed++;
                break;
            case 16754775: /* - */
                if(fan_speed>0)
                    fan_speed--;
                break;
            case 16736925: /* + */
                bAutoMode = true;
                break;
            case 16769565: /* + */
                bAutoMode = false;
                fan_speed_trigger = fan_speed;                
                break;
        }
        irrecv.resume(); // Receives the next value from the button you press
    }

    updateDisp();
    delay(2);
}
