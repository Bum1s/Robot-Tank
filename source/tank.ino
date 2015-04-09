/*
Author: Rokas Urlakis
*/

bool SELF_DRIVING = true;

#include <SoftwareSerial.h>
#include <NewPing.h>

// Engine
bool engineStarted = false;
long bluetoothCommandMillis = 0;

// Sonar
#define ECHO_PIN 2 // Echo Pin
#define TRIGGER_PIN 4 // Trigger Pin
#define MAX_DISTANCE 200 // in centimeters
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
unsigned int pingSpeed = 40;
unsigned long pingTimer;
int distance;
bool isObstacle = false;

// Bluetooth
byte bluetoothTx = 10;
byte bluetoothRx = 7;
SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);

// Driving Motors
#define PWM_A   3
#define DIR_A   12
#define BRAKE_A 9
#define PWM_B   11
#define DIR_B   13
#define BRAKE_B 8

// Top Motors
#define TURRET_MOTOR_PIN2 A0
#define TURRET_MOTOR_PIN1 A1
#define BARREL_MOTOR_PIN1 A3
#define BARREL_MOTOR_PIN2 A4
#define SHOOT_PIN A5
float turretRot = 180.0;
long turretRotLong = 18000;
float needToRotate = 0.0;
long shootingEndTime = 0;
#define TURRET_ROT_SPEED 7300.0// ms/90*

// Commands
#define FRONT 'w'
#define BACK 's'
#define LEFT 'a'
#define RIGHT 'd'
#define BREAK ' '
#define STOP '0'
#define NONE 'n'
char currentMoveCommand = NONE;
char currentCommand = NONE;
long lastMoveCommandMillis = 0;
long lastCommandMillis = 0;
int selfMoveTime = 0;
int waitForNextCmdInMs = 50;

// Photoresistor
#define PHOTORESISTOR_PIN A2
word lightIntensityAvarange = 0;
word lightIntensitySum = 0;
word lightIntensityCount = 0;
#define LIGHT_INTENSITY_MAX_COUNT 10
#define TURN_LIGHTS_ON_INTENSITY 40
#define HEADLIGHTS_PIN 6
#define BACKLIGHTS_PIN 5
#define PARKING_LIGHTS_INTENSITY 50 // 0-255
#define BRAKE_LIGHTS_INTENSITY 255 // 0-255
byte headlightsStatus = LOW;
byte backlightsStatus = 0;

//Movment
bool canMoveFront = true;
void setup()
{
  // Setup usb serial connection to computer
  Serial.begin(9600);  

  // Setup Channel A Motor
  pinMode(DIR_A, OUTPUT); //Initiates Motor Channel A pin
  pinMode(BRAKE_A, OUTPUT); //Initiates Brake Channel A pin

  // Setup Channel B Motor
  pinMode(DIR_B, OUTPUT); //Initiates Motor Channel A pin
  pinMode(BRAKE_B, OUTPUT);  //Initiates Brake Channel A pin
  
  // Top motors
  pinMode(SHOOT_PIN, OUTPUT); 
  pinMode(TURRET_MOTOR_PIN1, OUTPUT); 
  pinMode(TURRET_MOTOR_PIN2, OUTPUT);
  pinMode(BARREL_MOTOR_PIN1, OUTPUT); 
  pinMode(BARREL_MOTOR_PIN2, OUTPUT);
  digitalWrite(BARREL_MOTOR_PIN1, LOW);
  digitalWrite(BARREL_MOTOR_PIN2, LOW);
  digitalWrite(TURRET_MOTOR_PIN1, LOW);
  digitalWrite(TURRET_MOTOR_PIN2, LOW);
  //turretRotLong = EEPROMReadlong(0);
  //turretRot = turretRotLong/100.0;

  // Photoresistor
  pinMode(PHOTORESISTOR_PIN, INPUT);

  // Ligts
  pinMode(HEADLIGHTS_PIN, OUTPUT);
  pinMode(BACKLIGHTS_PIN, OUTPUT);
  // Setup Bluetooth
  delay(100);
  bluetooth.begin(9600);

  // Start sonar
  pingTimer = millis();
}

bool moveRobot(char cmd, byte moveSpeed, int time = 0)
{
  lastMoveCommandMillis = millis();
  if (currentMoveCommand == cmd)
    return 1;
  switch (cmd)
  {
    case FRONT:
      {
        if (!canMoveFront)
          return 0;
        digitalWrite(BRAKE_A, LOW);  // disable motor brake
        digitalWrite(DIR_A, LOW); // forward

        digitalWrite(BRAKE_B, LOW);  // disable motor brake
        digitalWrite(DIR_B, HIGH); // forward

        analogWrite(PWM_A, moveSpeed);     // Set the speed of the motor
        analogWrite(PWM_B, moveSpeed);     // Set the speed of the motor
        break;
      }
    case BACK:
      {
        digitalWrite(BRAKE_A, LOW);  // disable motor brake
        digitalWrite(DIR_A, HIGH);    // backward

        digitalWrite(BRAKE_B, LOW);  // disable motor brake
        digitalWrite(DIR_B, LOW);    // backward

        analogWrite(PWM_A, moveSpeed);     // Set the speed of the motor
        analogWrite(PWM_B, moveSpeed);     // Set the speed of the motor
        break;
      }
    case LEFT:
      {
        digitalWrite(BRAKE_A, LOW);  // disable motor brake
        digitalWrite(DIR_A, LOW);    // forward

        digitalWrite(BRAKE_B, LOW);  // disable motor brake
        digitalWrite(DIR_B, LOW);    // backward

        analogWrite(PWM_A, moveSpeed);     // Set the speed of the motor
        analogWrite(PWM_B, moveSpeed);     // Set the speed of the motor
        break;
      }
    case RIGHT:
      {
        digitalWrite(BRAKE_A, LOW);  // disable motor brake
        digitalWrite(DIR_A, HIGH);    // backward

        digitalWrite(BRAKE_B, LOW);  // disable motor brake
        digitalWrite(DIR_B, HIGH);    // forward

        analogWrite(PWM_A, moveSpeed);     // Set the speed of the motor
        analogWrite(PWM_B, moveSpeed);     // Set the speed of the motor
        break;
      }
    case BREAK:
      {
        digitalWrite(BRAKE_A, HIGH);  // enable motor brake
        digitalWrite(BRAKE_B, HIGH);  // enable motor brake
        analogWrite(BACKLIGHTS_PIN, BRAKE_LIGHTS_INTENSITY);
        backlightsStatus = BRAKE_LIGHTS_INTENSITY;
        break;
      }
    default:
      return 0;
  }
  currentMoveCommand = cmd;
  if (time > 0)
    selfMoveTime = time;//delay(time);  
  return 1;
}
void turnTurrelLeft(float rot = 0)
{
  if(turretRot > 60)  
  {
    needToRotate = rot;
    digitalWrite(TURRET_MOTOR_PIN1, LOW);
    digitalWrite(TURRET_MOTOR_PIN2, HIGH); 
    digitalWrite(BARREL_MOTOR_PIN1, LOW);
    digitalWrite(BARREL_MOTOR_PIN2, LOW); 
  }
}
void turnTurrelRight(float rot = 0)
{
  if(turretRot < 300)  
  {
    needToRotate = rot;
    digitalWrite(TURRET_MOTOR_PIN1, HIGH);
    digitalWrite(TURRET_MOTOR_PIN2, LOW);
    digitalWrite(BARREL_MOTOR_PIN1, LOW);
    digitalWrite(BARREL_MOTOR_PIN2, LOW);
  }
}
void turnBarrelUp(float rot = 0)
{
  needToRotate = rot;
  digitalWrite(BARREL_MOTOR_PIN1, HIGH);
  digitalWrite(BARREL_MOTOR_PIN2, LOW); 
  digitalWrite(TURRET_MOTOR_PIN1, LOW);
  digitalWrite(TURRET_MOTOR_PIN2, LOW);
}
void turnBarrelDown(float rot = 0)
{
  needToRotate = rot;
  digitalWrite(BARREL_MOTOR_PIN1, LOW);
  digitalWrite(BARREL_MOTOR_PIN2, HIGH);
  digitalWrite(TURRET_MOTOR_PIN1, LOW);
  digitalWrite(TURRET_MOTOR_PIN2, LOW);
}

void turnTurrel(float rot = 0)
{
  if(turretRot < rot)
  {
    turnTurrelRight(rot - turretRot);
  }
  else if(turretRot > rot)
  {
    turnTurrelLeft(turretRot - rot);
  }
}

#define ROBOT_SPEED 28.0 // cm/s
#define ROBOT_TURN_SPEED 830 // ms/90*

long loopMillis = 0;
void loop()
{
  if(shootingEndTime > 0)
  {
    if(shootingEndTime < millis())
    {
      digitalWrite(SHOOT_PIN, LOW);
      currentCommand = NONE;
      shootingEndTime = 0;
    }
  }
  int diffMillis = millis() - loopMillis;
  if(selfMoveTime > 0)
  {
    selfMoveTime -= diffMillis;
    if(selfMoveTime <= 0)
    {
      // stoping movment
      analogWrite(PWM_A, 0);
      analogWrite(PWM_B, 0);
      currentMoveCommand = NONE;
      selfMoveTime = 0;
    }
  }
  loopMillis = millis();
  if (engineStarted)
  {
    // Photoresistor
    lightIntensitySum += analogRead(PHOTORESISTOR_PIN);
    lightIntensityCount++;
    if (lightIntensityCount == LIGHT_INTENSITY_MAX_COUNT)
    {
      lightIntensityAvarange = (int)((lightIntensitySum / (float)LIGHT_INTENSITY_MAX_COUNT));
      //Serial.print("lightIntensity: ");
      //Serial.println(lightIntensityAvarange);
      lightIntensitySum = 0;
      lightIntensityCount = 0;
      if (lightIntensityAvarange < TURN_LIGHTS_ON_INTENSITY)
      {
        if (headlightsStatus == LOW)
        {
          digitalWrite(HEADLIGHTS_PIN, HIGH);
          analogWrite(BACKLIGHTS_PIN, PARKING_LIGHTS_INTENSITY);
          headlightsStatus = HIGH;
          backlightsStatus = PARKING_LIGHTS_INTENSITY;
          //Serial.println(F("Lights ON"));
        }
      }
      else if (headlightsStatus == HIGH && lightIntensityAvarange > (TURN_LIGHTS_ON_INTENSITY + 10))
      {
        digitalWrite(HEADLIGHTS_PIN, LOW);
        analogWrite(BACKLIGHTS_PIN, 0);
        headlightsStatus = LOW;
        backlightsStatus = 0;
        //Serial.println(F("Lights OFF"));
      }
    }
  checkDistance:
    if (millis() >= pingTimer)
    {
      pingTimer += pingSpeed;
      distance = sonar.ping() / US_ROUNDTRIP_CM;
      if (distance < 50 && distance > 0)
      {
        if (isObstacle)
        {
          if (canMoveFront)
          {
            canMoveFront = false;
            //Serial.print(F("Obstacle ahead! "));
            //Serial.println(distance);
          }
          if(SELF_DRIVING)
          {
            if(currentMoveCommand == RIGHT)
              moveRobot(RIGHT, 255, ROBOT_TURN_SPEED/2.0);
            else  
              moveRobot(LEFT, 255, ROBOT_TURN_SPEED/2.0);
          }
          isObstacle = false;
          goto checkDistance;
        }
        else
        {
          isObstacle = true;
        }
      }
      else
      {
        if (!canMoveFront)
        {
          canMoveFront = true;
          //Serial.println(F("Safe to move ahead"));
        }
      }
      if(selfMoveTime == 0 && canMoveFront)
      {
        if(SELF_DRIVING)
        {
          moveRobot(FRONT, 255, 200);
        }
      }
      //Serial.print(F("Distance to obstacle: "));
      //Serial.println(distance);
    }

    // read from bluetooth and write to usb serial
  }
  if (bluetooth.available() || Serial.available())
  {        
    bluetoothCommandMillis = millis();
    char command = '#';
    if(bluetooth.available())
    {
      while(bluetooth.available())
      {
        command = (char)bluetooth.read();
        if(command == '=')
        {
          command = (char)bluetooth.read();
          bluetooth.read();
        } 
      }
    }
    else if(Serial.available())
    { 
      command = (char)Serial.read();    
    }
    if(command != '#')
    {
      if (currentMoveCommand != command)
      {
        analogWrite(PWM_A, 0);
        analogWrite(PWM_B, 0);
        if (backlightsStatus == BRAKE_LIGHTS_INTENSITY)
        {
          if (lightIntensityAvarange < TURN_LIGHTS_ON_INTENSITY + 10)
          {
            analogWrite(BACKLIGHTS_PIN, PARKING_LIGHTS_INTENSITY);
            backlightsStatus = PARKING_LIGHTS_INTENSITY;
          }
          else
          {
            analogWrite(BACKLIGHTS_PIN, 0);
            backlightsStatus = 0;
          }
        }
      }   
      if(command == STOP)
      {  
        if(engineStarted)
        {
          engineStarted = false;
          //Serial.println(F("Engine OFF"));
      
          // lights off
          digitalWrite(HEADLIGHTS_PIN, LOW);
          headlightsStatus = LOW;
          analogWrite(BACKLIGHTS_PIN, 0);
          backlightsStatus = 0;
      
          // motors off
          analogWrite(PWM_A, 0);
          analogWrite(PWM_B, 0);
        }  
        else
        {
          engineStarted = true;  
        }  
      }
      
      else if (command > '0' && command <= '9')
      { 
        if(SELF_DRIVING)
        {
          SELF_DRIVING = false;
        }
        lastCommandMillis = millis();
        if(currentCommand != command)
        {
          currentCommand = command;
          if(command == '5')
          {
            if(shootingEndTime == 0)
            {
              digitalWrite(TURRET_MOTOR_PIN1, LOW);
              digitalWrite(TURRET_MOTOR_PIN2, LOW);
              digitalWrite(BARREL_MOTOR_PIN1, LOW);
              digitalWrite(BARREL_MOTOR_PIN2, LOW);
              digitalWrite(SHOOT_PIN, HIGH); 
              shootingEndTime = millis()+3000;  
            } 
          }
          else
          {
            if(command == '4')
            {
              waitForNextCmdInMs = 20;
              turnTurrelLeft();
            }
            else if(command == '7')
            {
              waitForNextCmdInMs = 150;
              turnTurrelLeft();          
            }
            else if(command == '6')
            {
              waitForNextCmdInMs = 20;
              turnTurrelRight();
            }
            else if(command == '9')
            {
              waitForNextCmdInMs = 150;
              turnTurrelRight();
            }
            else if(command == '8')
            {
              waitForNextCmdInMs = 100;
              turnBarrelUp();
            }
            else if(command == '2')
            {
              waitForNextCmdInMs = 100;
              turnBarrelDown();
            }
          }
        }
      }
      else if (command == 'c')
      {
        if(!SELF_DRIVING)
        {
          SELF_DRIVING = true;
        }
      }
      else if (command == 'o')
      {
        Serial.println(distance);
      }
      else if (command == 'l')
      {
        Serial.println(lightIntensityAvarange);
      }
      else if (moveRobot(command, 255))
      { 
      }    
    } 
  }
  else
  {
    if (currentMoveCommand != NONE && ((millis() - lastMoveCommandMillis) > 100) && selfMoveTime == 0)
    { 
      // stoping movment
      analogWrite(PWM_A, 0);
      analogWrite(PWM_B, 0);  
      currentMoveCommand = NONE;
    }
    if(currentCommand != NONE && needToRotate == 0.0 && (millis() - lastCommandMillis) > waitForNextCmdInMs)
    {
      digitalWrite(TURRET_MOTOR_PIN1, LOW);
      digitalWrite(TURRET_MOTOR_PIN2, LOW);
      digitalWrite(BARREL_MOTOR_PIN1, LOW);
      digitalWrite(BARREL_MOTOR_PIN2, LOW);
      currentCommand = NONE;
    }
  }
  delay(10);
}
