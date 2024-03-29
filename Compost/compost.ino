
#include <Adafruit_Si7021.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Servo.h>
#include <Arduino_JSON.h>

/********************************************************************/
//Variable Definitions

//Button Definitions
int buttonState;
const int BUTTON_PIN = 4;

//Light Sensor Definitions
int lightValue;
const int containerOpenThreshold = 80;
const int LIGHT_SENSOR_PIN = 0;

//Gas Sensor Definitions
double methanePPM;
const int GAS_SENSOR_PIN = 1;

//Moisture Sensor Definitions;
int moistureValue;
const int MOISTURE_SENSOR_PIN = 3;

//Sleep Definitions
const long button_read_sleep = 250;
const long sensor_read_sleep = 60000;
long sleepTracker = 0;

//Vent Definitions
const int VENT_SERVO_PIN = 9;
Servo ventServo;
bool ventState;
const int vent_closed_pos = 134;
const int vent_open_pos = 0;
const int servo_motion_sleep = 1000;

//Temperature Definitions

//Compost Temperature Definitions
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int compostTempValue;

//Ambient Temp/Humidity Definitions
Adafruit_Si7021 sensorAmbTemp = Adafruit_Si7021();
int envDataReturnArr[2];
int* ambientEnvData;
int ambientTempValue;
int ambientHumidityValue;

/********************************************************************/

int getInternalTemperature(void)
{
  sensors.requestTemperatures();
  float sensorValue =  sensors.getTempFByIndex(0);
  if (sensorValue < 0) {
    sensorValue = 0.0;
  }
  return sensorValue;
}

int * getAmbientEnvData(int *arr) {
  int temperatureC = sensorAmbTemp.readTemperature();
  int temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
  int humidity = sensorAmbTemp.readHumidity();
  arr[0] = temperatureF;
  arr[1] = humidity;
  return arr;
}

double getMethanePPM() {
  float sensorValue = analogRead(GAS_SENSOR_PIN);
  double ppm = 10.938 * exp(1.7742 * (sensorValue * 5.0 / 4095));
  if (ppm < 0) {
    ppm = 0;
  }
  return ppm;
}

int getMoistureValue()
{
  float sensorValue = analogRead(MOISTURE_SENSOR_PIN);
  return (sensorValue / 1023) * 100;
}

void setup() {
  Serial.begin(115200);
  sensors.begin();
  sensorAmbTemp.begin();
  pinMode(BUTTON_PIN, INPUT);
  ventServo.attach(VENT_SERVO_PIN);
  ventState = false; //False means closed, True means open
  moveVentServo(vent_closed_pos);
  buttonState = 0;
  sleepTracker = 0;
}

void consumeSensorData() {
  methanePPM = getMethanePPM();
  compostTempValue = getInternalTemperature();
  ambientEnvData = getAmbientEnvData(envDataReturnArr);
  ambientTempValue = ambientEnvData[0];
  ambientHumidityValue = ambientEnvData[1];
  lightValue = analogRead(LIGHT_SENSOR_PIN);
  moistureValue = getMoistureValue();
  //Serial.println(lightValue);

  JSONVar jsonObject;
  jsonObject["ambient_temperature"] = ambientTempValue;
  jsonObject["ambient_humidity"] = ambientHumidityValue;
  jsonObject["compost_temperature"] = compostTempValue;
  jsonObject["methane_ppm"] = methanePPM;
  jsonObject["compost_moisture"] = moistureValue;
  if (lightValue > containerOpenThreshold) {
    jsonObject["container_state"] = "OPEN";
  }
  else {
    jsonObject["container_state"] = "CLOSED";
  }
  if (ventState) {
    jsonObject["vent_state"] = "OPEN";
  }
  else {
    jsonObject["vent_state"] = "CLOSED";
  }


  String jsonString = JSON.stringify(jsonObject);
  Serial.println(jsonString);
}

void moveVentServo(int value) {
  ventServo.write(value);
  delay(servo_motion_sleep);
}

void toggleVentState() {
  if (ventState == true) {
    //vent is open, close it
    ventState = false;
    moveVentServo(vent_closed_pos);
  }
  else {
    //vent is closed, open it
    ventState = true;
    moveVentServo(vent_open_pos);
  }
}

void loop() {
  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == HIGH) {
    toggleVentState();
  }
  if (sleepTracker >= sensor_read_sleep) {
    consumeSensorData();
    sleepTracker = 0;
  }
  else {
    sleepTracker += button_read_sleep;
  }
  delay(button_read_sleep);
}
