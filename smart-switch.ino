#include <Servo.h>
#include <algorithm> // std::min

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define LED D0

enum servo_flip_to_state {ON, OFF};

const char* ssid = "000-Blue-Puppy";
const char* password = "280uVrNbBL";
const String device_name = "Bathroom-Boiler-Switch";

const unsigned long full_day = 24UL * 3600; // Time in seconds
const unsigned long start_night = 0 * 3600UL + 5 * 60; // Time in seconds since midnight. 00:05
const unsigned long end_night = 7 * 3600UL + 45 * 60; // Time in seconds since midnight. 07:45
const unsigned long loop_delay_time = 1UL * 60 * 1000; // 1 minute in milliseconds

const unsigned long day_duration = (full_day + start_night - end_night) % full_day;
const unsigned long night_duration = (full_day + end_night - start_night) % full_day;

const int servo_pin = 5;
const int led_pin = 1;
const int flip_time = 200; // Time in milliseconds
const int wifi_attempt_time = 1000;
const int utcOffsetInSeconds = 7200; // Central European Time

const int angle_off = 170;
const int angle_on = 10;
const int angle_mid = 85;

Servo servo;

// variables to hold current time
bool is_daytime = true;
unsigned long custom_duration = full_day;

void flip(const servo_flip_to_state new_state) {  // One movement of servo and back to mid
  if (new_state == ON) {
    servo.write(angle_on);
  } else {
    servo.write(angle_off);
  }
  delay(flip_time);
  servo.write(angle_mid);
}

bool isDay (unsigned long current_time) {
  if (start_night > end_night) {  // E.g. start_night 23.00, end_night 8.00
    return ((current_time > end_night) && (current_time < start_night));
  } else {  // E.g. start_night 01.00, end night 8.00
    return ((current_time > end_night) || (current_time < start_night));
  }
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

unsigned long getCurrentTime() {
  // Return the number of seconds since midnight today
  Serial.println("Getting current time...");
  WiFiUDP Udp;
  NTPClient timeClient(Udp, "pool.ntp.org", utcOffsetInSeconds);
  timeClient.begin();
  timeClient.update();
  unsigned long hh = timeClient.getHours();
  unsigned long mm = timeClient.getMinutes();
  unsigned long ss = timeClient.getSeconds();
  Serial.println((String)"Current time is " + hh + " hours " + mm + " minutes " + ss + " seconds");
  unsigned long result = 3600UL * hh + 60UL * mm + ss;
  return result;
}

void splitDelay(unsigned long time_to_delay) {
  unsigned long remaining_wait_time = time_to_delay;
  unsigned long current_delay;
  Serial.println((String)"Starting delay of " + time_to_delay/1000 + " seconds...");
  while (remaining_wait_time > 0) {
    current_delay = std::min(remaining_wait_time, loop_delay_time);
    delay(current_delay);
    remaining_wait_time -= current_delay;
    Serial.println((String)"Wait time: still " + remaining_wait_time/1000 + " seconds to go");
  }
  Serial.println((String)"Delay of " + time_to_delay/1000 + " seconds complete!");
}

void oneOperation(bool is_daytime, unsigned long custom_duration) {
  unsigned long duration;
  if (is_daytime) {
    Serial.println("Good morning! Turning the boiler OFF");
    flip(OFF);
    digitalWrite(LED, HIGH);
    duration = std::min(day_duration, custom_duration);
  } else {
    Serial.println("Good night! Turning the boiler ON");
    flip(ON);
    digitalWrite(LED, LOW); // turn on the led at night
    duration = std::min(night_duration, custom_duration);
  }
  Serial.println((String)"The next operation will happen in " + duration + " seconds");
  splitDelay(1000UL * duration);
}

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW); // Red LED while connecting to wifi

  servo.attach(servo_pin);
  connectToWifi();

  digitalWrite(LED, HIGH);

  // get current time
  unsigned long current_time = getCurrentTime();

  // run the first operation with custom duration 
  // (since the start is sometime during the day/nigh);
  if (isDay(current_time)) {
    is_daytime = true;
    custom_duration = (full_day + start_night - current_time) % full_day; // if start_night is before midnight
  } else {
    is_daytime = false;
    custom_duration = (full_day + end_night - current_time) % full_day;
  }
  
  oneOperation(is_daytime, custom_duration);
  is_daytime = !is_daytime;
}

void loop() {
  oneOperation(is_daytime, full_day);
  is_daytime = !is_daytime;
}
