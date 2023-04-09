#include <Servo.h>
#include <algorithm> // std::min

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define LED D0

enum servo_flip_to_state {ON, OFF};

const char* ssid = "TBD";
const char* password = "TBD";
const String device_name = "Bathroom-Boiler-Switch";

const int no_custom_duration = (int)(1e9);
const int full_day = 24 * 3600; // Time in seconds
const int start_night = 0 * 3600 + 5 * 60; // Time in seconds since midnight. 00:05
const int end_night = 7 * 3600 + 45 * 60; // Time in seconds since midnight. 07:45

const int day_duration = (start_night - end_night + full_day) % full_day;
const int night_duration = (end_night - start_night + full_day) % full_day;

const int servo_pin = 5;
const int led_pin = 1;
const int flip_time = 500; // Time in milliseconds
const int wifi_attempt_time = 1000;
const int utcOffsetInSeconds = 7200; // Central European Time

const int angle_on = 170;
const int angle_off = 10;
const int angle_mid = 85;

Servo servo;

// variables to hold current time
bool is_daytime = true;
int custom_duration = no_custom_duration;

void flip(const servo_flip_to_state new_state) {  // One movement of servo and back to mid
  if (new_state == ON) {
    servo.write(angle_on);
  } else {
    servo.write(angle_off);
  }
  delay(flip_time);
  servo.write(angle_mid);
}

// Upon turning on, get time
// Check if it's day or night. If day, run day, if night, run night.
// Day Operation: flip off and wait till night. Then run Night Operation
// Night: flip on and wait till day. Then run Day Operation

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(device_name.c_str());
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(wifi_attempt_time);
    Serial.print(".");
  }
  //print a new line, then print WiFi connected and the IP address
  Serial.println("");
  Serial.println("WiFi connected");
  // Print the IP address
  Serial.println(WiFi.localIP());
}

int getCurrentTime() {
  // Return the number of seconds since midnight today
  Serial.println("Getting current time...");
  WiFiUDP Udp;
  NTPClient timeClient(Udp, "pool.ntp.org", utcOffsetInSeconds);
  timeClient.begin();
  timeClient.update();
  int hh = timeClient.getHours();
  int mm = timeClient.getMinutes();
  int ss = timeClient.getSeconds();
  Serial.println((String)"Current time is " + hh + " hours " + mm + " minutes " + ss + " seconds");
  int result = hh * 3600 + mm * 60 + ss;
  return result;
}

void oneOperation(bool is_daytime, int custom_duration) {
  int duration;
  if (is_daytime) {
    Serial.println("Good morning! Turning the boiler OFF");
    flip(OFF);
    duration = std::min(day_duration, custom_duration);
  } else {
    Serial.println("Good night! Turning the boiler ON");
    flip(ON);
    duration = std::min(night_duration, custom_duration);
  }
  Serial.println((String)"The next operation will happen in " + duration + " seconds");
  delay(duration * 1000);
}

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW); // Red LED while connecting to wifi

  servo.attach(servo_pin);
  connectToWifi();

  digitalWrite(LED, HIGH);

  // get current time
  int current_time = getCurrentTime();

  // run the first operation with custom duration 
  // (since the start is sometime during the day/nigh);
  if (current_time < end_night) {
    is_daytime = false;
    custom_duration = end_night - current_time;
  } else {
    is_daytime = true;
    custom_duration = (full_day + start_night - current_time) % full_day; // if start_night is before midnight
  }
  
  oneOperation(is_daytime, custom_duration);
  is_daytime = !is_daytime;
}

void loop() {
  oneOperation(is_daytime, no_custom_duration);
  is_daytime = !is_daytime;
}
