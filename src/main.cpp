#include <Arduino.h>
#include <Adafruit_MAX31865.h>

#define DEBUG 1

#include "main.h"
#include "debug.h"
#include "output.h"

// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  100.0

#define Q_RATE 7.5
#define FLOWSENSOR 2
#define PRESSURE_OFFSET 0.693
#define PRESSURE_MAX 232.06
#define PRESSURE_MAX_VOLTAGE 5.0

unsigned long currentTime;
unsigned long cloopTime;
volatile uint16_t flow_frequency; // Measures flow sensor pulsesunsigned 

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 thermo = Adafruit_MAX31865(10, 11, 12, 13);

float read_pressure() {

  float V, P;
  int resist;

  resist = analogRead(A0);
  
  V = (resist * PRESSURE_MAX_VOLTAGE) / 1024;     //Sensor output voltage
  
  //P = (V - OffSet) * 400;       //Calculate water pressure

  P = ((PRESSURE_MAX)*(V - PRESSURE_OFFSET))/(PRESSURE_MAX_VOLTAGE - PRESSURE_OFFSET);

  debug("Voltage: %d", V);
  debug("Resistance: %dΩ", resist);
  debug("Pressure: %d psi", P);

  return P;
}


float read_temperature() {
  uint16_t rtd = thermo.readRTD();

  debug("RTD value: %d", rtd);

  float ratio = (float)rtd;
  ratio /= 32768;

  float temperature = thermo.temperature(RNOMINAL, RREF);

  debug("Ratio = %d", ratio);
  debug("Resistance = %dΩ", (RREF*ratio));
  debug("Temperature = %dC", temperature);

  // Check and print any faults
  uint8_t fault = thermo.readFault();
  if (fault) {
    debug("Fault 0x%X", fault);
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
      debug("RTD High Threshold"); 
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
      debug("RTD Low Threshold"); 
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
      debug("REFIN- > 0.85 x Bias"); 
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
      debug("REFIN- < 0.85 x Bias - FORCE- open"); 
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
      debug("RTDIN- < 0.85 x Bias - FORCE- open"); 
    }
    if (fault & MAX31865_FAULT_OVUV) {
      debug("Under/Over voltage"); 
    }
    thermo.clearFault();
  }

  return temperature;
}

void flow () // Interrupt function
{
   flow_frequency++;
}

void setup()
 {
   pinMode(FLOWSENSOR, INPUT);
   digitalWrite(FLOWSENSOR, HIGH); // Optional Internal Pull-Up

   Serial.begin(115200);

   attachInterrupt(digitalPinToInterrupt(FLOWSENSOR), flow, RISING); // Setup Interrupt
   sei(); // Enable interrupts
   currentTime = millis();
   cloopTime = currentTime;
   thermo.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
}

void loop ()
{
   currentTime = millis();
   // Every second, calculate and print litres/minute
   if(currentTime >= (cloopTime + 1000))
   {
      float temperature = read_temperature();
      float pressure = read_pressure();
      
      cloopTime = currentTime; // Updates cloopTime
      // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.
      float l_min = ((float)flow_frequency / Q_RATE); // (Pulse frequency) / 7.5Q = flowrate in L/minute
      flow_frequency = 0; // Reset Counter

      debug("%d L/min", l_min);

      output("+++%d,%d,%d", temperature, l_min, pressure);

   }
}
