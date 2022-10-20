// code for slave Arduino

#include "Arduino.h"
#include <Wire.h>

int yellowLedPin[] = {8,10,12};
int blueLedPin[] = {9,11,13};
int offButton = 2;
int LDR = 0;
bool muted = false;
bool alarm = false;
bool off_button_pressed = false;

void setup() 
{
  // assigns the pin modes for each component on the arduinos and initialise wire connection
  Serial.begin(9600);
  for(int i = 0; i < 3; i++)
  {
    pinMode(blueLedPin[i], OUTPUT);
    pinMode(yellowLedPin[i], OUTPUT);
  }
  pinMode(offButton, INPUT_PULLUP);
  Wire.begin(1);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

// the main code of he program where the logic of turning the alarm on or off takes place
void loop() 
{
  int lightOn = analogRead(LDR);
  bool isDark = lightOn > 1000;
  delay(100);
  // if the alarm is set to turn on then the if statement in the loop constantly checks for a button input from the user
  if (alarm)
  {
    int isButtonPressed = !digitalRead(offButton);
    if(muted)
    {
      if(isDark)
      {
        muted = false;
      }
    }
    else
    {
      if(isButtonPressed && !isDark)
      {
        muted = true;
      }
    }
  }
  if(alarm && !muted)
  {
    turnAlarmOn();
  }
  else
  {
    turnAlarmOff();
  }
}

// the function below flashes the blue and yellow LEDs intermittently 
void turnAlarmOn()
{
  digitalWrite(blueLedPin[0], HIGH);
  digitalWrite(blueLedPin[1], HIGH);
  digitalWrite(blueLedPin[2], HIGH);
  delay(100);
  digitalWrite(blueLedPin[0], LOW);
  digitalWrite(blueLedPin[1], LOW);
  digitalWrite(blueLedPin[2], LOW);
  digitalWrite(yellowLedPin[0], HIGH);
  digitalWrite(yellowLedPin[1], HIGH);
  digitalWrite(yellowLedPin[2], HIGH);
  delay(100);
  digitalWrite(yellowLedPin[0], LOW);
  digitalWrite(yellowLedPin[1], LOW);
  digitalWrite(yellowLedPin[2], LOW);    
}


// the function below simply turns all the blue and yellow LEDs off
void turnAlarmOff()
{
  for(int i = 0; i < 3; i++)
  {
    digitalWrite(blueLedPin[i], LOW);
    digitalWrite(yellowLedPin[i], LOW); 
  }  
}


// this checks the reading sent from the master writer as a key and value pair
// this pair is sent in the form "key=value;" with the key being "muted" and the value being "on" or "off"
// after this is received, the boolean values for "alarm" and "muted" are updated as required
void receiveEvent(int howMany)
{
  String key = "";
  String value = "";
  boolean keyDone = false;
  while(Wire.available())
  {
    char c = Wire.read();
    if(c == '=')
    {
      keyDone = true;
    }
    else if (c == ';')
    {
      break;
    }
    else if (keyDone)
    {
      value = value + c;
    }
    else
    {
      key = key + c;
    }
  }
  if(key && value)
  {
    if (key == "alarm")
    {
      if (value == "on")
      {
        alarm = true;
      }
      else if (value == "off")
      {
        if(alarm)
        {
          muted = false;
        }
        alarm = false;
      }
    }
  }
}


// the function below sends whether the alarm has been muted or not to the Master Arduino using the Wire library
void requestEvent()
{
  if(muted)
  {
    Wire.write("muted=true;");
  }
  else 
  {
    Wire.write("muted=false;");
  }
}
