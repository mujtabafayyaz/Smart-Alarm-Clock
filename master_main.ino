// Master arduino main file

#include <Wire.h>
#include <LiquidCrystal.h>

// initialize LCD
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

// pins
int potentiometer = 0;
int button = 8;
int piezo = 9;

int buttonDelay = 1000; // how long to wait after a button press to stop two from registering

int currenttime[2];      // time program started, hours and minutes
unsigned long timesetat; // millis time this started
int alarmtime[2];        // time alarm will go off at, hours and minutes
int alarmtimeSnooze[2];  // time alarm will go off at. updated once they hit snooze
int snoozeUsed = 0;      // how many times snooze has been pressed, to check maxSnoozes
int maxSnoozes = 1;
int snoozeDuration = 3;         // minutes
int alarmDuration[2] = {0, 15}; // hours and minutes

float temp = 0;
char *weather; // eg "Cloudy"

boolean alarm = false; // is alarm on
unsigned long alarmOnAt = 0;

unsigned long showWeatherAt = millis() + 5000UL;
bool showingWeather = true;
unsigned long fetchWeatherAt = millis() + (1000UL * 60UL * 10UL);

void setup()
{
  pinMode(button, INPUT_PULLUP);

  lcd.begin(16, 2); // columns and role
  lcd.clear();
  lcd.print(F("Setting up..."));

  Wire.begin();         // join i2c bus (address optional for master)
  Serial.begin(115200); // start serial for output

  internetSetup();

  // get current time from the internet
  char *datetime = getDatetime();
  char hourChar[] = {datetime[11], datetime[12]};
  int hours = String(hourChar).toInt();
  char minsChar[] = {datetime[14], datetime[15]};
  int mins = String(minsChar).toInt();
  currenttime[0] = hours;
  currenttime[1] = mins;
  timesetat = millis();

  // get weather and temperature
  weather = getWeather(&temp);

  // get the initial alarm time
  lcd.clear();
  lcd.print(F("Alarm time:"));
  gettime(alarmtime);
  alarmtimeSnooze[0] = alarmtime[0];
  alarmtimeSnooze[1] = alarmtime[1];
}

int hours() // get current hour
{
  unsigned long currentTime = millis();
  unsigned long timePassed = currentTime - timesetat;
  unsigned long hoursPassed = timePassed / 3600000;
  int intHoursPassed = (int)hoursPassed;
  int timeSinceBegin = currenttime[0] + intHoursPassed;
  return timeSinceBegin % 24;
}

int mins() // get current minute
{
  unsigned long currentTime = millis();
  unsigned long timePassed = currentTime - timesetat;
  unsigned long minsPassed = timePassed / 60000;
  int intMinsPassed = (int)minsPassed;
  int timeSinceBegin = currenttime[1] + intMinsPassed;
  return timeSinceBegin % 60;
}

// change settings
void menu()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Menu"));

  int mode;
  while (true)
  {
    lcd.setCursor(0, 1);
    mode = map(analogRead(potentiometer), 0, 1023, 0, 3);
    if (mode == 0)
    {
      lcd.print(F("Alarm Time:         "));
    }
    else if (mode == 1)
    {
      lcd.print(F("Alarm Duration:           "));
    }
    else if (mode == 2)
    {
      lcd.print(F("Maximum snoozes:       "));
    }
    else if (mode == 3)
    {
      lcd.print(F("Snooze duration:    "));
    }

    if (!digitalRead(button))
    {
      delay(buttonDelay);
      break;
    }
  }

  lcd.clear();
  if (mode == 0)
  {
    lcd.print(F("Alarm Time:"));
    gettime(alarmtime);
    alarmtimeSnooze[0] = alarmtime[0];
    alarmtimeSnooze[1] = alarmtime[1];
  }
  else if (mode == 1)
  {
    lcd.print(F("Alarm duration:"));
    gettime(alarmDuration);
  }
  else if (mode == 2)
  {
    lcd.print(F("Maximum Snoozes:"));
    while (true)
    {
      maxSnoozes = map(analogRead(potentiometer), 0, 1023, 0, 5);
      lcd.setCursor(0, 1);
      lcd.print(maxSnoozes);
      lcd.print(F("     "));
      if (!digitalRead(button))
      {
        delay(buttonDelay);
        break;
      }
    }
  }
  else if (mode == 3)
  {
    lcd.print(F("Snooze duration:"));
    while (true)
    {
      snoozeDuration = map(analogRead(potentiometer), 0, 1023, 0, 59);
      lcd.setCursor(0, 1);
      lcd.print(snoozeDuration);
      lcd.print(F(" mins   "));
      if (!digitalRead(button))
      {
        delay(buttonDelay);
        break;
      }
    }
  }

  lcd.clear();
}

// take an int 0-59, turn it into a two character string (putting 0 in front if needed)
String intToStringPadded(int num)
{
  String st = String(num);
  if (st.length() == 1)
  {
    st = "0" + st;
  }
  return st;
}

// take an array of hour, minute and put in two values the user selects
void gettime(int toUpdate[])
{
  int hours = -1;
  int mins = -1;

  boolean showNum = true; // flash changing number

  lcd.setCursor(2, 1);
  lcd.print(":00");
  while (hours == -1 || mins == -1)
  {
    if (hours == -1)
    {
      // updating hours
      int hour = map(analogRead(potentiometer), 0, 1023, 0, 23);
      lcd.setCursor(0, 1);

      if (showNum)
      {
        showNum = false;
        lcd.print(intToStringPadded(hour));
      }
      else
      {
        showNum = true;
        lcd.print(F("  "));
      }

      if (!digitalRead(button))
      {
        hours = hour;
        lcd.setCursor(0, 1);
        lcd.print(intToStringPadded(hour));

        delay(buttonDelay);
      }
    }
    else
    {
      // updating minutes
      int min = map(analogRead(potentiometer), 0, 1023, 0, 59);
      lcd.setCursor(3, 1);

      if (showNum)
      {
        showNum = false;
        lcd.print(intToStringPadded(min));
      }
      else
      {
        showNum = true;
        lcd.print(F("  "));
      }

      if (!digitalRead(button))
      {
        mins = min;
        delay(buttonDelay);
      }
    }

    delay(500);
  }

  toUpdate[0] = hours;
  toUpdate[1] = mins;
  lcd.clear();
}

void loop()
{
  delay(100);

  // rotate showing weather/time every 5 seconds
  if (millis() > showWeatherAt)
  {
    showWeatherAt = millis() + 5000UL;
    showingWeather = !showingWeather;
  }

  // fetch updated weather info every 10 minutes
  if (millis() > fetchWeatherAt)
  {
    fetchWeatherAt = millis() + (1000UL * 60UL * 10UL);
    weather = getWeather(&temp);
  }

  // tell slave arduino alarm status
  if (alarm)
  {
    sendAlarmOn();
  }
  else
  {
    sendAlarmOff();
  }


  // check muted status from slave arduino
  Wire.requestFrom(1, 24);

  String key = "";
  String value = "";
  boolean keyDone = false;

  while (Wire.available())
  {
    char c = Wire.read();
    if (c == '=')
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
  if (key && value)
  {
    if (key == "muted")
    {
      // turn on or off piezo based on if the alarm is muted or not
      if (value == "true")
      {
        piezoOff();
      }
      else if (alarm) // only turn piezo back on if alarm is still on
      {
        piezoOn();
      }
    }
  }

  // how long is alarm on for, converted into milliseconds
  unsigned long alarmLength = 1000UL * 60UL * 60UL * (unsigned long)alarmDuration[0] + 1000UL * 60UL * (unsigned long)alarmDuration[1];
  if (!alarm && hours() == alarmtimeSnooze[0] && mins() == alarmtimeSnooze[1]) // turn on alarm
  {
    alarm = true;
    alarmOnAt = millis();
    piezoOn();
  }
  else if (alarm && millis() - alarmOnAt > alarmLength) // turn off alarm
  {
    alarm = false;
    piezoOff();
    alarmOnAt = 0;
    // reset snooze time
    alarmtimeSnooze[0] = alarmtime[0];
    alarmtimeSnooze[1] = alarmtime[1];
  }

  lcd.setCursor(0, 0);
  if (showingWeather)
  {
    lcd.print(F("Weather:        "));
    lcd.setCursor(0, 1);
    lcd.print(temp);
    lcd.print(F(" C - "));
    lcd.print(weather);
  }
  else
  {
    lcd.print(F("Current time:   "));
    lcd.setCursor(0, 1);
    lcd.print(intToStringPadded(hours()) + ":" + intToStringPadded(mins()));
    lcd.print(F("           "));
  }

  if (!digitalRead(button))
  {
    if (alarm && snoozeUsed < maxSnoozes) // if alarm is on and button is pressed, snooze
    {
      alarmtimeSnooze[1] += snoozeDuration;
      if (alarmtimeSnooze[1] > 59)
      {
        alarmtimeSnooze[1] -= 60;
        alarmtimeSnooze[0] += 1;
        if (alarmtimeSnooze[0] > 23)
        {
          alarmtimeSnooze[0] -= 24;
        }
      }

      alarm = false;
      alarmOnAt = 0;
      piezoOff();
      snoozeUsed += 1;
      delay(buttonDelay);
    }
    else
    {
      lcd.clear();
      delay(buttonDelay);
      menu();
    }
  }
}

void piezoOff()
{
  noTone(piezo);
}
void piezoOn()
{
  tone(piezo, 4000, 100000);
}

void sendAlarmOn()
{
  Wire.beginTransmission(1);
  Wire.write("alarm=on;");
  Wire.endTransmission();
}
void sendAlarmOff()
{
  Wire.beginTransmission(1);
  Wire.write("alarm=off;");
  Wire.endTransmission();
}
