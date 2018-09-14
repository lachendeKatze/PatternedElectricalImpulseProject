/**
 * pulseTrain Sketch for Arduino101
 * 10/23/2017
 * 
 * Written By:
 * Gregory O. Voronin, DVM
 * Director, In Vivo Research Services
 * Office of Research Advancement
 * Rutgers, The State University of New jersey
 * 
 * For:
 * Dr. Antonio Merolli
 * 
 * The pulseTrain sketch is desgined to generate a pulse train on an Arduino101
 * GPIO pin. The pulse train parameters of peak duration, peak number, cycle 
 * period, cycle number and pause period between cycle numbers are all variable and 
 * can all be set over BLE using an android app.
 * 
 * Parameter Definitions:
 *  Cycle:
 *  Peak Duration:
 *  Peak Number:
 *  Cycle Period:
 *  Cycle Number:
 *  Pause Period:
 * 
 * 10/23/2017: comments not complete
 * 10/25/2017: Local ID "Red"
 * 11/9/2017: Default Pattern 1 - pulse train starts in setup()
 * 11/9/2017: correct error in defauly PAUSEPERIOD Parameter
 **/

#include <CurieBLE.h>
#include <CurieTimerOne.h>

// Default Pulse Train Parameters - Pattern 1
#define PEAKDURATION 2000       // 2 milliseconds, 2000 microseconds
#define PEAKNUMBER 2            // total number of peaks, on + off
#define CYCLEPERIOD 100         // 100 millisecond, 100000 microseconds
#define CYCLENUMBER 10          // number of cycles before pause
#define PAUSEPERIOD 1000000    // 10000 milliseconds, 10000000 microseconds

// Hardware parameters, default to gpio pin# 10
#define PULSEPIN1 10

// pulse train parameters
unsigned int peakDuration, peakNumber, cyclePeriod, cycleNumber, pausePeriod;

// pulse train state variables
long currentTime, previousTime;
unsigned int currentCycleCount;
boolean pulseOn, toggle, pulseTrainPaused;


/**
* Define BLE service and characteristics here
* Due to an issue with properly sending numbers with MIT App Inventor Android App
* All characteristic values are sent from the app to the Arduino101 as strings.
* These string characteristic values are then "translated" into their
* respective integer values.
* 
**/

BLEPeripheral blePeripheral;                                    // BLE Peripheral Device (the board you're programming)
BLEService peipService("917649A0-D98E-11E5-9EEC-0002A5D5C51B"); // Custom UUID
BLEUnsignedCharCharacteristic switchCharacteristic("917649A1-D98E-11E5-9EEC-0002A5D5C51B", BLEWrite);

// pulse train characterisitics
BLECharacteristic peakDurationCharacteristic("917649A2-D98E-11E5-9EEC-0002A5D5C51B", BLEWrite, 3);
BLECharacteristic peakNumberCharacteristic("917649A3-D98E-11E5-9EEC-0002A5D5C51B", BLEWrite, 2);
BLECharacteristic cyclePeriodCharacteristic("917649A4-D98E-11E5-9EEC-0002A5D5C51B", BLEWrite, 8);
BLECharacteristic cycleNumberCharacteristic("917649A5-D98E-11E5-9EEC-0002A5D5C51B", BLEWrite, 3);
BLECharacteristic pauseCharacteristic("917649A6-D98E-11E5-9EEC-0002A5D5C51B", BLEWrite, 8);

void setup() 
{
  // set pulse train to defaults to defauly pattern 1
  peakDuration  = PEAKDURATION;
  peakNumber    = PEAKNUMBER;
  cyclePeriod   = CYCLEPERIOD;
  cycleNumber   = CYCLENUMBER;
  pausePeriod   = PAUSEPERIOD;

  //hardare setup
  pinMode(PULSEPIN1,OUTPUT);
  
  //pulse state & timers - this is the "default" start state
  currentTime = 0;
  previousTime = 0;
  currentCycleCount = 0;
  pulseOn = true; // start pulse train from setup()/reset
  toggle = false;
  pulseTrainPaused = false;
  
  
  // initilize the ble service, characterisitcs and advertising
  blePeripheral.setLocalName("Yellow"); // local name will vary with the board this is downloaded into
  blePeripheral.setAdvertisedServiceUuid(peipService.uuid());
  blePeripheral.addAttribute(peipService);
  blePeripheral.addAttribute(switchCharacteristic);
  blePeripheral.addAttribute(peakDurationCharacteristic);
  blePeripheral.addAttribute(peakNumberCharacteristic);
  blePeripheral.addAttribute(cyclePeriodCharacteristic);
  blePeripheral.addAttribute(cycleNumberCharacteristic);
  blePeripheral.addAttribute(pauseCharacteristic);
  
  // add event handlers here
  blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
  switchCharacteristic.setEventHandler(BLEWritten, switchCharacteristicWritten);
  peakDurationCharacteristic.setEventHandler(BLEWritten, peakDurationCharacteristicWritten);
  peakNumberCharacteristic.setEventHandler(BLEWritten, peakNumberCharacteristicWritten);
  cyclePeriodCharacteristic.setEventHandler(BLEWritten, cyclePeriodCharacteristicWritten);
  cycleNumberCharacteristic.setEventHandler(BLEWritten, cycleNumberCharacteristicWritten);
  pauseCharacteristic.setEventHandler(BLEWritten, pauseCharacteristicWritten);
  
  // set an initial values for the characteristics
  const unsigned char initializePeakDuration[3] = {0,0,0};
  const unsigned char initializePeakNumber[2]   = {0,0};
  const unsigned char initializeCyclePeriod[8]  = {0,0,0,0,0,0,0,0};
  const unsigned char initializeCycleNumber[3]  = {0,0,0};
  const unsigned char initializePause[8]        = {0,0,0,0,0,0,0,0};
  
  switchCharacteristic.setValue(-1);
  peakDurationCharacteristic.setValue(initializePeakDuration,3);
  peakNumberCharacteristic.setValue(initializePeakNumber,2);
  cyclePeriodCharacteristic.setValue(initializeCyclePeriod,8);
  cycleNumberCharacteristic.setValue(initializeCycleNumber,3);
  pauseCharacteristic.setValue(initializePause,8);
  
  // advertise the service
  blePeripheral.begin();
}

void loop() 
{
  blePeripheral.poll();
  pulseTrain();
}

void blePeripheralConnectHandler(BLECentral& central)
{
  // central connected event handler
}

void blePeripheralDisconnectHandler(BLECentral& central)
{
  // central disconnected event handler
}

/**
 * Callback handler for changes in the switch characteristic.
 * Due to an issue with properly sending numbers with MIT App Inventor Android 
 * App, all characteristic values are sent from the app to the Arduino101 as 
 * strings. These string characteristic values are then "translated" into their
 * respective integer values.
 * int 48 = "0" Pulse Train On
 * int 49 = "1" Pulse Train Off
 * int 50 = "2" Stop Pulse Train, Reset to default parameters, Start Pulse Train
 **/

void switchCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic)
{

  int switchValue = switchCharacteristic.value();
  switch(switchValue)
  {
    case 48:
    // Turn pulse train on
      CurieTimerOne.kill();
      currentCycleCount = 0;
      pulseTrainPaused = false;
      toggle = false;
      pulseOn = true;
      break;
    case 49:
      // Turn pulse train off
      pulseOn = false;    // order could be important, don't allow any loping in case of delay in register set versus loop() & call to kill() function
      CurieTimerOne.kill();
      currentCycleCount = 0;
      toggle = false;
      pulseTrainPaused = false;
      break;
    case 50:
      pulseOn = false;
      CurieTimerOne.kill();
      peakDuration  = PEAKDURATION;
      peakNumber    = PEAKNUMBER;
      cyclePeriod   = CYCLEPERIOD;
      cycleNumber   = CYCLENUMBER;
      pausePeriod   = PAUSEPERIOD;
      toggle        = false;
      currentCycleCount = 0;
      pulseTrainPaused = false;
      pulseOn = true;
      break;
    case 51:
      // for debugging/expanded functionality later
      break;
  }
}

/**
 * Callback handler for the peak duration characteristic.
 * The peak duration is a numbe between 0 and 999, sent as
 * a 3 byte array and translated here into a int 0-999.
 * Conversion to microseconds in performed as the last step
 * as CurieTimerOne requires microseconds.
 * 
 **/

void peakDurationCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic)
{
  const unsigned char* dataBytes = peakDurationCharacteristic.value();
  String peakDurationString = "";
  
  // change 3 here to a #DEFINE of other later
  for (int i = 0; i<3; i++)
  { 
    if (isDigit(char(dataBytes[i])))
    {
      peakDurationString += char(dataBytes[i]);
    }
    else
    {
      break;
    }
  }
  peakDuration = peakDurationString.toInt()*1000; // millis to microseconds
}

/**
 * Callback handler for peark number characteristic.
 **/ 

void peakNumberCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic)
{
  const unsigned char* dataBytes = peakNumberCharacteristic.value();
  String peakNumberString = "";
  
  // change 3 here to a #DEFINE of other later
  for (int i = 0; i<2; i++)
  { 
    if (isDigit(char(dataBytes[i])))
    {
      peakNumberString += char(dataBytes[i]);
    }
    else
    {
      break;
    }
  }
  peakNumber = peakNumberString.toInt();
}

/**
 *  Callback handler for cycle period characteristic
 * 
 **/
void cyclePeriodCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic)
{
  const unsigned char* dataBytes = cyclePeriodCharacteristic.value();
  String cyclePeriodString = "";
  
  for (int i=0; i<8; i++)
  {
    if (isDigit(char(dataBytes[i])))
    {
      cyclePeriodString += char(dataBytes[i]);
    }
    else
    {
      break;
    }
  }
  //Serial.print("Cycle Period: ");Serial.println(cyclePeriodString);
  cyclePeriod = cyclePeriodString.toInt();
}

/**
 * Callback handler for cycle number characteristic change.
 **/
void cycleNumberCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic)
{
  const unsigned char* dataBytes = cycleNumberCharacteristic.value();
  String cycleNumberString = "";

  // change 3 here to a #DEFINE of other later
  for (int i = 0; i<3; i++)
  { 
    if (isDigit(char(dataBytes[i])))
    {
      cycleNumberString += char(dataBytes[i]);
    }
    else
    {
      break;
    }
  }  
  //Serial.print("cylce number string: ");Serial.println(cycleNumberString);
  cycleNumber = cycleNumberString.toInt(); 
}

/**
 * Callback handler for pause characteristic change.
 **/
void pauseCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic)
{
  const unsigned char* dataBytes = pauseCharacteristic.value();
  String pauseString = "";
  
  //change 8 here to a #DEFINE of other later
  for (int i = 0; i<8; i++)
  { 
     if (isDigit(char(dataBytes[i])))
     {
       pauseString += char(dataBytes[i]);
     }
     else 
     {
       break;
     }
  }
  //Serial.print("pause string: ");Serial.println(pauseString);
  pausePeriod = pauseString.toInt();
}

/**
 * 
 **/
void pulseInterrupt()
{
  toggle = !toggle;
  digitalWrite(PULSEPIN1,toggle);
}

/**
 * 
 **/
void pausePulseTrain()
{
  pulseOn = true;
  pulseTrainPaused = true;  
  currentCycleCount = 0;
}

/**
 * The is function generates the pulse train. It is only called every time the microcontroller
 * 'loops' through the loop() function but only executes code in the pulseOn state var is 
 * set to true.
 * 
 */
void pulseTrain()
{
 if (pulseOn)
 {
    currentTime = millis();
    if ( (currentTime-previousTime) >= cyclePeriod )
    {
      previousTime = currentTime;
      if (pulseTrainPaused == true) 
      {
        CurieTimerOne.kill();
        pulseTrainPaused = false; 
      }
      
      CurieTimerOne.start(peakDuration,&pulseInterrupt); 
      currentCycleCount++;
    }
    if (CurieTimerOne.readTickCount() >= peakNumber)
    {
      CurieTimerOne.kill();  
      //Serial.println("killed by peakNumber");
    }
    if (currentCycleCount > cycleNumber)
    {
      CurieTimerOne.kill();
      //Serial.print("Cycle Count: ");Serial.println(currentCycleCount);
      pulseOn = false;  
      digitalWrite(PULSEPIN1,false);
      CurieTimerOne.start(pausePeriod,&pausePulseTrain);
    }
 } 
}