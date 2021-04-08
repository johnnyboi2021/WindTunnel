#include <Wire.h>

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);

#include <Servo.h>
Servo ESC;     // create servo object to control the ESC
int potValue;  // value from the analog pin

#include <Q2HX711.h>
#define lift_DT  A2
#define lift_SCK A3
#define drag_DT  A4
#define drag_SCK A5
Q2HX711 hx711_lift(lift_DT, lift_SCK);
Q2HX711 hx711_drag(drag_DT, drag_SCK);

float y1 = 45.36; // calibrated mass to be added
long dragratio = 88490;
long liftratio = 84127;
float avg_size = 10.0; // amount of averages for each mass measurement
float drag_b = 0;
float lift_b = 0;
float drag_g = 0;
float lift_g = 0;
float tare_d = 0;
float tare_l = 0;

#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads1115(0x48); // connect ADDR to gnd for 0x48
double rho  = 1.19654; // use air density calculator (T = 72F, P = 30.09inHg, RH = 44%)
int button  = 2;
double vref = 5, vset=2.5, a = 0;
unsigned long milliscntr;

long adc0;
long adc1;
double voltage;
double pressure;
double velo_ms, velo_mph;

void setup() 
{
  Wire.begin();
  
  lcd.begin();
  lcd.backlight();
  
  ads1115.begin();
  pinMode(button, INPUT_PULLUP);

  // Attach the ESC on pin 3
  ESC.attach(3,1000,2000); // (pin, min pulse width, max pulse width in micro-s)
}

void loop() 
{
  lcd.setCursor(0,0);
  lcd.print(velo_mph);
  lcd.print("  mph     ");  
  pitot();

  lcd.setCursor(0,1);
  lcd.print("Lift: ");
  lcd.print(-lift_g);
  lcd.print(" g    ");
  lcd.setCursor(0,2);
  lcd.print("Drag: ");
  lcd.print(drag_g);
  lcd.print(" g   "); 
  force();

  potValue = analogRead(A0);   // reads pot power input (0-1023)
  potValue = map(potValue, 0, 1023, 0, 180);   // map to servo (0-180)
  ESC.write(potValue);    // Send the signal to the ESC
  lcd.setCursor(0,3);
  lcd.print("Power: ");
  lcd.print(double(potValue)*100/180);
  lcd.print("%   ");
}

void force()
{
  // averaging reading
  lift_b = 0;
  drag_b = 0;
  for (int jj=0;jj<int(avg_size);jj++)
  {
    lift_b += hx711_lift.read();
    drag_b += hx711_drag.read();
  }
  lift_b/=long(avg_size);
  drag_b/=long(avg_size);
  // calculating grams force based on calibration and linear fit
  lift_g = (lift_b-tare_l)*y1/liftratio;
  drag_g = (drag_b-tare_d)*y1/dragratio;
}

void pitot()
{
  adc0 = 0;
  adc1 = 0;
  for (int i = 0; i < 10; i++)
  {
    adc0 += ads1115.readADC_SingleEnded(0);
    adc1 += ads1115.readADC_SingleEnded(1);
  }
  adc0 *= 1/10.0;
  adc1 *= 1/10.0;

  if (!digitalRead(button))
  {
    vset = bits2volts(adc0);
    vref = bits2volts(adc1);
    a = vref/2-vset;
    
    tare_l = 0;
    tare_d = 0;
    for (int jj=0;jj<int(avg_size);jj++)
    {
      tare_l += hx711_lift.read();
      tare_d += hx711_drag.read();
    }
    tare_l/=long(avg_size);
    tare_d/=long(avg_size);    
  }


  voltage  = bits2volts(adc0); 
  pressure = 1000*(5*(voltage+a)/vref-5/2.0);
  if (pressure > 0)
    velo_ms  = 0; 
  else
    velo_ms  = sqrt(2*abs(pressure)/rho);
  velo_mph = velo_ms*2.23694;
    
//  Serial.print("AIN0: "); Serial.println(adc0);
//  Serial.print("AIN1: "); Serial.println(adc1);
//  Serial.print("Volt: "); Serial.println(voltage,5);
//  Serial.print("delP: "); Serial.print(pressure); Serial.println(" Pa");
//  Serial.print("Velo: "); Serial.print(velo_ms);  Serial.println(" m/s");
//  Serial.print("Velo: "); Serial.print(velo_mph); Serial.println(" mph");
//  Serial.println(" ");
}

double bits2volts(double adc) // coefficients from adc to multimeter linear regression
{
  return adc*(1.8713727348099*pow(10,-4))-0.0076726282127;
}
