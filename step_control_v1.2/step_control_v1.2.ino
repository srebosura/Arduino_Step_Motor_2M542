// include the library code:
#include <LiquidCrystal.h>
/*-----( Declare Constants, Pin Numbers )-----*/

// set pin numbers:
LiquidCrystal lcd(42, 44, 52, 50, 48, 46);
const int start_buttonPin = 2;     // the number of the start pushbutton pin
const int stop_buttonPin = 3;     // the number of the stop pushbutton pin
const int motor1_home_sensor = 18;     // the number of the motor1 home sensor pin
const int motor2_home_sensor = 19;     // the number of the motor2 home sensor pin
const int run_ledPin =  13;      // the number of the run/monitor LED pin

/*
microstep driver 2M542
Pul+ goes to +5V
Pul- goes to Arduino Pin 8 & 29
Dir+ goes to +5V
Dir- goes to to Arduino Pin 9 & 27
Enable+ to nothing
Enable- to nothing
*/

//motor1 pins (feeder)
int m1_step = 8;          
int m1_dir = 9;

//motor2 pins (magazine/rack)
int m2_step = 29;          
int m2_dir = 27;

/*-----( Declare Variables )-----*/

// variables will change:
int read_start;    //start button flag
int read_stop;    // stop button flag
int start_buttonState;         // variable for reading the start pushbutton status
int run_ledState = LOW;         // the current state of the output pin
int last_start_ButtonState = HIGH;   // the previous reading from the input pin
int run_status;         //run status flag
int m1_home_status;     //motor1 home status as triggered by interrupt service 
int m2_home_status;     //motor2 home status as triggered by interrupt service 
//int motor_speed;        //variable for setting motor speed, 1=1ms=1000hz
int step_per_rev = 800; //step_per_rev is the driver's micro step settings

void(* resetFunc) (void) = 0; //software reset vector

void setup()   /*----( SETUP: RUNS ONCE )----*/
{
  // initialize the LED pin as an output:
  pinMode(run_ledPin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(start_buttonPin, INPUT);
  pinMode(stop_buttonPin, INPUT);
  pinMode(motor1_home_sensor, INPUT);
  pinMode(motor2_home_sensor, INPUT);
  //set interrupt to activate anytime the stop button
  attachInterrupt(digitalPinToInterrupt(stop_buttonPin), read_sensors, CHANGE);
  attachInterrupt(digitalPinToInterrupt(motor1_home_sensor), read_m1_home_sensor , RISING);
  attachInterrupt(digitalPinToInterrupt(motor2_home_sensor), read_m2_home_sensor , RISING);
  // set initial LED state
  digitalWrite(run_ledPin, run_ledState);
  // set stepper motor_1 output pins
  pinMode(m1_step, OUTPUT);
  pinMode(m1_dir, OUTPUT);
  //initial state
  digitalWrite(m1_step, LOW);
  digitalWrite(m1_dir, LOW);
  // set stepper motor_2 output pins
  pinMode(m2_step, OUTPUT);
  pinMode(m2_dir, OUTPUT);
  digitalWrite(m2_step, LOW);
  digitalWrite(m2_dir, LOW);
  //serial port for debugging and monitor 
  Serial.begin(9600);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.setCursor(1,0);
  lcd.print("Motor_Ctrl_v1.1");
  
  
}/*--(end setup )---*/

void loop()   /*----( LOOP: RUNS CONSTANTLY )----*/
{
    lcd.setCursor(0,1);
    lcd.print("  Standby Mode");
    run_status = read_switches();
    while (run_status == 1)  {
    digitalWrite(run_ledPin, HIGH);  
    Serial.print("run_status = "); Serial.println(run_status);
    lcd.clear();
    display_status_lcd(0,0,"Run Status = ",run_status);
    //motor_speed = 2;
    set_m2_home();
    m2_home_status = LOW;//reset m1 home status flag for debug, this will be activated by sensor
    set_m1_home();
    m1_home_status = LOW;//reset m1 home status flag for debug, this will be activated by sensor
    delay(1000);
    Serial.println("Seaquence start!");
    auto_mode(3); //set the number of feeds
    run_status = 0;
    display_status_lcd(0,0,"Run Status = ",run_status);
    Serial.println("Seaquence end!");
    }
  if (run_status == 0)  {
   // turn start LED off and stepper motor1:
   
    digitalWrite(run_ledPin, LOW);
    Serial.print("run_status = ");
    Serial.println("Stop");
    //resetFunc();
  }
  
}/* --(end main loop )-- */
int read_switches() {
 // read the state of the switch into a local variable:
  int read_start = digitalRead(start_buttonPin);
  delay (5);
  if (read_start == HIGH) {
  Serial.println("Start Button Activated");  
    return 1;
    }
  int read_stop = digitalRead(stop_buttonPin);
  delay (1);
  if (read_stop == HIGH) {
    return 0;
    }
  else { return 11;
  }  
 }

void read_sensors() {
 //Interrupt Service Routine, read the state of the stop switch into a local variable: 
  int read_stop = digitalRead(stop_buttonPin);
  delay (50);
  if (read_stop == HIGH) {
    Serial.println("Reset");
    run_status = 0;
    lcd.clear();  
    resetFunc();
    }  
 }
void read_m1_home_sensor() {
  Serial.println("ISR for m1_home_sensor activated!");
  m1_home_status = HIGH;
  delay(100);  
}
void read_m2_home_sensor() {
  Serial.println("ISR for m2_home_sensor activated!");
  m2_home_status = HIGH;
  delay(100);  
}
void run_step_m1(int dir1, int rev1) {    
  //dir1 set direction of m1
  //rev1 set the number of revolutions for m1
  //step_per_rev is the driver's micro step settings 
  //start stepper motor1
  Serial.print("Revs_m1_cw = ");Serial.println(rev1);
  digitalWrite(m1_dir, dir1);  
  for (int x = 0; x <= rev1 * step_per_rev; x++) {
  digitalWrite(m1_step, HIGH);
  //time_delay(motor_speed);
  set_speed();
  digitalWrite(m1_step, LOW);
  //time_delay(motor_speed);
  set_speed();
    }
}

void run_step_m2(int dir2, int rev2) {    
  //dir2 set direction of m2
  //rev2 set the number of revolutions for m2
  //step_per_rev is the driver's micro step settings 
  //start stepper motor2 cw:
  Serial.print("Revs_m2_cw = ");Serial.println(rev2);
  digitalWrite(m2_dir, dir2);  
  for (int z = 0; z <= rev2 * step_per_rev; z++) {
  digitalWrite(m2_step, HIGH);
  //time_delay(motor_speed);
  set_speed();
  digitalWrite(m2_step, LOW);
  //time_delay(motor_speed);
  set_speed();
    }
}

void set_speed() {
  int sensorReading = analogRead(A0);
  // map it to a range from 0 to 100:
  int motor_speed = map(sensorReading, 0, 1023, 0, 100);
  //delayMicroseconds(motor_speed*4);
  delay(motor_speed);
  
}

void set_m1_home()  {
  delay (10);
  Serial.print("m1_home_status = "); 
  Serial.println(m1_home_status);
  while (m1_home_status == LOW) {
  Serial.println("Setting m1 to home position.....");
  display_lcd(0,1,"Set M1 to home");
  //motor_speed = 1;
  run_step_m1(0,1);
  Serial.print("m1_home_sensor = "); 
  Serial.println(m1_home_status); 
    }
  Serial.println("m1 in home position.....");   
}  
void set_m2_home()  {
  delay (10);
  Serial.print("m2_home_status = "); 
  Serial.println(m2_home_status);
  while (m2_home_status == LOW) {
  Serial.println("Setting m2 to home position.....");
  display_lcd(0,1,"Set M2 to home ");
  //motor_speed = 2;
  run_step_m2(0,1);
  Serial.print("m2_home_sensor = "); 
  Serial.println(m2_home_status); 
    }
  Serial.println("m2 in home position.....");   
}  
void auto_mode (int cycle) {
  Serial.print("Total Cycle = ");Serial.println(cycle);     
  for (int y = 0; y <cycle; y++) {
    Serial.print("Cycle no  = ");Serial.println(y+1);
    display_status_lcd(0,1,"Cycle No   =  ",y+1);
    //motor_speed = 2;
    run_step_m1(1,1);
    delay (1000);
    run_step_m1(0,1);
    delay (1000);
    run_step_m2(1,2);
    delay (1000);
  }
}      
void display_lcd(int col, int row, String msg) {
  
  lcd.setCursor(col,row);
  lcd.print(msg);
}
void display_status_lcd(int col, int row, String msg, int stat) {
  
  lcd.setCursor(col,row);
  lcd.print(msg);
  lcd.setCursor(col+13,row);
  lcd.print(stat);
}
/* ( THE END ) */
