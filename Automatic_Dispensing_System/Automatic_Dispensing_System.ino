/*/////////////////////////////////////////////////////////////////////////////////////////////////////////
 * 
 * Project - Arduino Automatic Dispensing System.
 * Engineer - Iroshana Herath
 * Date - 10th May. 2018
 * Email - iroshana.elance@gmail.com
 * 
//////////////////////////////////////////////////////////////////////////////////////////////////////// */

#include <EEPROM.h>

#define sensorPin A0
#define pumpPin 2
#define shuntRes  100.00

//************** Default Settings *******************

int TC = 120;
long DT = 90;
byte R = 21;

//////////////////////////////////////////////////////

int CR, PR, count;

float OT, vol, FM, EM, GAL;

unsigned long  start = 0, wait = 0, pumpStart;

boolean state = false, intl = false;


String input, caption = "";

byte MP[] = {34,38,42,46,50,52,54,56,58,60,
            62,64,66,68,70,73,75,77,79,81,
            83,86,89,91,95,98,100,102,105,107,
            110,113,117,120,123,127,132,136,141,146,
            153,158,165,171,179,186,196,213,223,239
            };

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  Serial.println("Initializing..");
  pinMode(sensorPin, INPUT);
  pinMode(pumpPin, OUTPUT);


  delay(1000);

  Serial.print("Do You Need to Change Settings? (Y/N) : ");
  
  input = "";
  wait = millis();
  Serial.flush();
  while(!Serial.available() && (millis() - wait < 30000));
  if(millis() - wait < 30000) input = Serial.readString();
  
  if(input == "Y" || input == "y")
  {
    Serial.println("\nSettings Mode..");
  
    caption = "Set Tank Capacity TC = ";
    btRead();
    if(isNumeric(input))  TC = input.toInt();
    else  Serial.println("Invalid Input.. Default Value Loaded..");

    caption = "\nDispense Time in seconds DT = ";
    btRead();
    if(isNumeric(input)) DT = input.toInt();
    else  Serial.println("Invalid Input.. Default Value Loaded..");

    caption = "\nSet Ratio R = ";
    btRead();
    if(isNumeric(input)) R = input.toInt();
    else  Serial.println("Invalid Input.. Default Value Loaded..");

    Serial.println("\nSettings Successfully Updated..");
    Serial.println("TC = " + String(TC));
    Serial.println("DT = " + String(DT));
    Serial.println("R = " + String(R));

  }

  else
  {
    Serial.println("\nLoaded Default Settings..");
    Serial.println("TC = " + String(TC));
    Serial.println("DT = " + String(DT));
    Serial.println("R = " + String(R));
  }

  PR = (EEPROM.read(100)<<8 | EEPROM.read(101)); 
  if(PR < 0)  PR = 0;
  
  Serial.println("Last Saved Reading = " + String(PR));
  
  Serial.println("Program Started..\n");


}

void loop() {
  // put your main code here, to run repeatedly:

  if(millis() - start > 1800000 ||(!intl && (millis() - start > 300000)))
  {
    intl = true;
    input = "";
    int reading = analogRead(sensorPin);
    Serial.print("Analog Read = " + String(reading) + "\t"); 
    
    boolean in = false;
    start = millis();
    CR = shuntRes*(1023.00/reading - 1.00);
    Serial.println("Current Reading = " + String(CR) + "\t Last Read = " + String(PR));
    if(CR < 0.95*PR)
    {
      //Serial.print("Inside If");
      FM = getMP(CR);
      Serial.print("FM = " + String(FM));
      EM = getMP(PR);
      Serial.println("\tEM = " + String(EM));
      
      GAL = (FM*TC) - (EM*TC);
      Serial.print("Accept Calculation GAL = " + String(GAL) + " ? (Y/N) : ");
      
      wait = millis();
      while(millis() - wait < 30000)
      {
        Serial.flush();
        while(!Serial.available() && (millis() - wait < 30000));

        if(millis() - wait >= 30000)  break;
        input = Serial.readString();
        
        if(input == "Y" || input == "y")
        {
          Serial.println("Calculated GAL Accepted.");
          
          OT = GAL/(float)R*DT;
          
          Serial.println("OT = " + String(OT));
          onPump();
          in = true;
          break;
        }

        else if(input == "N" || input == "n")
        {
           input = "";
           Serial.print("\nEnter Number of Gallons to add : ");
           Serial.flush();
           while(!Serial.available());
           input = Serial.readString();
           if(isNumeric(input) && input.toFloat() <= TC)
           {
             Serial.println("Manual GAL Input Accepted.");
             
             OT = input.toFloat()/R*DT;
             
             Serial.println("OT = " + String(OT));
             onPump();
             in = true;
             break;
           }
           else
            {
              Serial.println("\nInvalid Input!");
              Serial.print("\nAccept Calculation GAL = " + String(GAL) + " ? (Y/N) : ");
            }
        }

        else
        {
          Serial.println("\nInvalid Input!");
          Serial.print("\nAccept Calculation GAL = " + String(GAL) + " ? (Y/N) : ");
        }
        
      }

      
      if(!in)
      {
        OT = (float)GAL/R*DT;
        Serial.println("No User Input. Calculated GAL Accepted.");
        Serial.println("OT = " + String(OT));
        onPump();
      }
      
    }
    PR = CR;
    EEPROM.write(100, ((int)PR>>8));
    EEPROM.write(101, ((int)PR & 255));
  }

  ctrlPump();
  
}

void btRead()
{
  Serial.print(caption);
  input = "";
  Serial.flush();
  while(!Serial.available());
  input = Serial.readString();
  
}

float getMP(int val)
{
  if(val > MP[49])  return 0.00;
  else if(val < MP[0])  return  1.00;
  else
  {
    for(int i = 0; i < 49; i++)
    {
      if(val > MP[i] && val <= MP[i+1]) return  (1.00 - 0.02*(i+1));
    }
  }
}


boolean isNumeric(String str) 
{
    unsigned int stringLength = str.length();
 
    if (stringLength == 0) {
        return false;
    }
 
    boolean seenDecimal = false;
 
    for(unsigned int i = 0; i < stringLength; ++i) 
    {
        if (isDigit(str.charAt(i))) {
            continue;
        }
 
        if (str.charAt(i) == '.') {
            if (seenDecimal) {
                return false;
            }
            seenDecimal = true;
            continue;
        }
        return false;
    }
    return true;
}

void onPump()
{
  digitalWrite(pumpPin,HIGH);
  state = true;
  pumpStart = millis();
  count = 0;
  EEPROM.write(3,10);
  EEPROM.write(10, ((int)OT>>8));
  EEPROM.write(11, ((int)OT & 255));
  EEPROM.write(0,0);
  EEPROM.write(1,0);
  //Serial.println("Timer Value : " + String(EEPROM.read(10)) + "\t" + String(EEPROM.read(11)));
}

void ctrlPump()
{
  
  if(state && (millis() - pumpStart) > OT*1000)
  {
    digitalWrite(pumpPin, LOW);
    state = false;
    count = 0;
    EEPROM.write(3,0);
    EEPROM.write(0,0);
    EEPROM.write(1,0);
  }

  if(state && (millis() - pumpStart)%1000 ==0)
  {
    //digitalWrite(pumpPin, LOW);
    //state = false;
    count+=100;
    EEPROM.write(0,count>>8);
    EEPROM.write(1,(count & 255));
    //Serial.println("Timer EEPROM : " + String(EEPROM.read(0)) + "\t" + String(EEPROM.read(1)));
  }

}


