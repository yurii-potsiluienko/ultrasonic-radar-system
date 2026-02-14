#include "SR04.h"
#include <dht_nonblocking.h>
#include <Servo.h>

// --- ULTRASONIC SENSOR ---
#define TRIG_PIN 5
#define ECHO_PIN 7
//SR04 sr04 = SR04(ECHO_PIN, TRIG_PIN);
float distance_m = 1.0;

// measures the distance in meters, adjusted for temperature and humidity
float measure_distance_m_calibrated(float * temperature, float * humidity, bool * dht_sensor_active) {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration_us = pulseIn(ECHO_PIN, HIGH);
  if (duration_us <= 0) return -1.0; 
  else {
   float speed_sound_m_s;
   if (*dht_sensor_active) {
      speed_sound_m_s = (331.3 + (0.606 * *temperature) + (0.0124 * *humidity));
   } else{
      speed_sound_m_s = 344.0;
   }
   return (duration_us * speed_sound_m_s / 1000000.0) / 2.0;
  }
}

// --- TEMPERATURE AND HUMIDITY SENSOR ---
#define DHT_SENSOR_PIN 2
#define DHT_SENSOR_TYPE DHT_TYPE_11
DHT_nonblocking dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
unsigned long dht_interval = 5000;
float temperature_C = 20.0;     // Default temperature is 20C
float humidity_percent = 50.0;  // Default humidity is 50%
bool dht_active = false;

// updates temperature and humidity in specific intervals
void update_environment(float * temperature, float * humidity, unsigned long * interval) {
  static unsigned long last_update = 0;
  if (millis() - last_update >= *interval) {
    if (dht_sensor.measure(temperature, humidity)) {
      last_update = millis();
      dht_active = true; 
    }
  }
  if (millis() - last_update > (*interval * 3)) {
    dht_active = false; 
  }
}

// --- SERVO ---
#define SERVO_PIN 9
Servo myservo;
int servo_position = 90;
int servo_direction = 1;
unsigned long last_servo_move = 0;
unsigned long servo_interval = 100; // Servo sweep speed in milliseconds per degree
int FOV = 90;                       // Field of view
int high_limit = servo_position + FOV / 2;
int low_limit = servo_position - FOV / 2;

// sweeps servo with specific speed and limits, controlled by servo_interval, low_limit, and high_limit
bool sweep(unsigned long * servo_interval, unsigned long * last_servo_move, int * servo_direction, int * servo_position, int * low_limit, int * high_limit) {
  if (millis() - *last_servo_move >= *servo_interval) {
    // Increment position
    *servo_position += *servo_direction;
    // Change sweep direction when angle reaches the low or high limit
    if (*servo_position >= *high_limit || *servo_position <= *low_limit) {
      *servo_direction *= -1; // Flip direction
    }
    myservo.write(*servo_position);
    *last_servo_move = millis();
    return true; // return true if the servo has moved
  }
  return false;
}

// --- SETUP  ---

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  myservo.attach(SERVO_PIN);
  myservo.write(servo_position);
  
  delay(2000); 

  unsigned long startAttempt = millis();
  while (!dht_active && (millis() - startAttempt < 3000)) {
    if (dht_sensor.measure(&temperature_C, &humidity_percent)) {
      dht_active = true;
    }
    delay(100); 
  }
  while(Serial.available() > 0) { Serial.read(); }
}

// --- LOOP  ---

void loop() {
  // 1. Measure the temperature and humidity in specific intervals
  update_environment(&temperature_C, &humidity_percent, &dht_interval);
  // 2. Sweep the servo with specific speed, controlled by servo_interval
  bool moved = sweep(&servo_interval, &last_servo_move, &servo_direction, &servo_position, &low_limit, &high_limit);
  if (moved){
    // 3. If the servo has moved, measure distance and print it in the serial monitor in CSV format: "angle(degrees),distance(cm)"
    distance_m = measure_distance_m_calibrated(&temperature_C, &humidity_percent, &dht_active);
    Serial.print(servo_position);
    Serial.print(",");
    Serial.println(distance_m * 100, 1);
  }
}