#include <IRremote.h>

/*####################################################################
 FILE: dht11_functions.pde - DHT11 Usage Demo.
 VERSION: 2S0A

 PURPOSE: Measure and return temperature & Humidity. Additionally provides conversions.

 LICENSE: GPL v3 (http://www.gnu.org/licenses/gpl.html)
 GET UPDATES: https://www.virtuabotix.com/

      --##--##--##--##--##--##--##--##--##--##--
      ##  ##  ##  ##  ##  ##  ##  ##  ##  ##  ##
      ##  ##  ##  ##  ##  ##  ##  ##  ##  ##  ##
      | ##  ##  ##  ##  ##  ##  ##  ##  ##  ## |
      ##  ##  ##  ##  ##  ##  ##  ##  ##  ##  ##
      ##  ##  ##  ##  ##  ##  ##  ##  ##  ##  ##
      | ##  ##  ##  ##  ##  ##  ##  ##  ##  ## |
      ##  ##  ##  ## DHT11 SENSOR ##  ##  ##  ##
      ##  ##  ##  ##  ##FRONT ##  ##  ##  ##  ##
      | ##  ##  ##  ##  ##  ##  ##  ##  ##  ## |
      ##  ##  ##  ##  ##  ##  ##  ##  ##  ##  ##
      ##  ##  ##  ##  ##  ##  ##  ##  ##  ##  ##
      | ##  ##  ##  ##  ##  ##  ##  ##  ##  ## |
      ##  ##  ##  ##  ##  ##  ##  ##  ##  ##  ##
      ##  ##  ##  ##  ##  ##  ##  ##  ##  ##  ##
      --##--##--##--##--##--##--##--##--##--##--
          ||       ||          || (Not    ||
          ||       ||          || Used)   ||
        VDD(5V)   Readout(I/O)          Ground

  HISTORY:
  Joseph Dattilo (Virtuabotix LLC) - Version 2S0A (27 May 12)
  -Rewritten to with more powerful Versalino functionality
  Joseph Dattilo (Virtuabotix LLC) - Version 0.4.5 (11/11/11)
  -Made Library Arduino 1.0 Compatible
  Joseph Dattilo (Virtuabotix LLC) - Version 0.4.0 (06/11/11)
  -Fixed bugs (squish)
  Mod by Rob Tillaart - Version 0.3 (28/03/2011)
  Mod by SimKard - Version 0.2 (24/11/2010)
 George Hadjikyriacou - Original version (??)
#######################################################################*/
#include <dht11.h>
#include  <TimerOne.h>          // Avaiable from  http://www.arduino.cc/playground/Code/Timer1


int PIN_IR_RECEIVER = 8; 
IRrecv irrecv(PIN_IR_RECEIVER);
decode_results results;

volatile int i=0;               // Variable to use as a counter
volatile boolean zero_cross=0;  // Boolean to store a "switch" to tell us if we have crossed zero
int AC_pin = 3;                 // Output to Opto Triac
int fan_speed = 0;              // Fan speed (0-9)  0 = off, 9 = full speed

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


void zero_cross_detect() {    
  zero_cross = true;               // set the boolean to true to tell our dimming function that a zero cross has occured
  i=0;
  digitalWrite(AC_pin, LOW);       // turn off TRIAC (and AC)
}                                 

// Turn on the TRIAC at the appropriate time
void dim_check() {                   
  if(zero_cross == true) {              
    if(i> (9-fan_speed)) {                     
      digitalWrite(AC_pin, HIGH); // turn on light       
      i=0;  // reset time step counter                         
      zero_cross = false; //reset zero cross detection
    } 
    else {
      i++; // increment time step counter                     
    }                                
  }                                  
}                                   
/*
 * created by Rui Santos, http://randomnerdtutorials.com
 * Temperature Sensor Displayed on 4 Digit 7 segment common anode 
 * 2013
 */
const int digitPins[4] = {
  4,5,6,7};                 //4 common anode pins of the display
const int clockPin = 11;    //74HC595 Pin 11 
const int latchPin = 12;    //74HC595 Pin 12
const int dataPin = 13;     //74HC595 Pin 14

const byte digit[10] =      //seven segment digits in bits
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
int digitScan = 0, flag=0,  soft_scaler = 0;
float tempK, tempC, tempF, temp;
 

 
//writes the temperature on display
void updateDisp(){
    for(byte j=0; j<4; j++)  
        digitalWrite(digitPins[j], HIGH);
 
    digitalWrite(latchPin, LOW);  
    shiftOut(dataPin, clockPin, MSBFIRST, B00000000);
    shiftOut(dataPin, clockPin, MSBFIRST, B00000000);
    digitalWrite(latchPin, HIGH);
 
    delayMicroseconds(100);
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
 
dht11 DHT11;

#define PIN_DHT11_TEMP_SENSOR    9

char str[6];
int count = 0;


void getTemperature()
{
    float humidity;
    int chk = DHT11.read();

    Serial.print("Read sensor: ");
    switch (chk)
    {
        case 0: Serial.println("OK"); break;
        case -1: Serial.println("Checksum error"); break;
        case -2: Serial.println("Time out error"); break;
        default: Serial.println("Unknown error"); break;
    }

    humidity = (float)DHT11.humidity;
    Serial.print("Humidity (%): ");
    Serial.println(humidity, DEC);

    Serial.print("Temperature (°C): ");
    Serial.println((float)DHT11.temperature, DEC);
    tempC = (float)DHT11.temperature * 100;
    Serial.print("tempC: ");
    Serial.println(tempC);

    digitBuffer[3] = int(tempC)/1000;
    digitBuffer[2] = (int(tempC)%1000)/100;
    digitBuffer[1] = (int(tempC)%100)/10;
    digitBuffer[0] = (int(tempC)%100)%10;
 
    Serial.print("Temperature (°F): ");
    Serial.println(DHT11.fahrenheit(), DEC);

    Serial.print("Temperature (°K): ");
    Serial.println(DHT11.kelvin(), DEC);

    Serial.print("Dew Point (°C): ");
    Serial.println(DHT11.dewPoint(), DEC);

    Serial.print("Dew PointFast (°C): ");
    Serial.println(DHT11.dewPointFast(), DEC);
}

void setup()
{
    DHT11.attach(PIN_DHT11_TEMP_SENSOR);
    Serial.begin(9600);
    Serial.println("DHT11 TEST PROGRAM ");
    Serial.print("LIBRARY VERSION: ");
    Serial.println(DHT11LIB_VERSION);

    pinMode(AC_pin, OUTPUT);                          // Set the Triac pin as output
    attachInterrupt(0, zero_cross_detect, RISING);   // Attach an Interupt to Pin 2 (interupt 0) for Zero Cross Detection
    // Use the TimerOne Library to attach an interrupt
    // to the function we use to check to see if it is 
    // the right time to fire the triac.  This function 
    // will now run every freqStep in microseconds.                                            
    Timer1.initialize(freqStep);                      // Initialize TimerOne library for the freq we need
    Timer1.attachInterrupt(dim_check, freqStep);      

    for(int i=0;i<4;i++)
    {
        pinMode(digitPins[i],OUTPUT);
    }

    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);

    getTemperature();
    irrecv.enableIRIn(); // Starts the receiver
}

void loop(){ 
    if(count++ > 10000)
    {
        getTemperature();
        count = 0;
    }

    //decodes the infrared input
    if (irrecv.decode(&results)){
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
        }
        irrecv.resume(); // Receives the next value from the button you press
    }

    updateDisp();

    delay(2);
}

