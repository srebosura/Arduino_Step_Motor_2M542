void(* resetFunc) (void) = 0; //software reset vector
// include the library code:
#include <LiquidCrystal.h>
#include <EEPROM.h> 
/*-----( Declare Constants, Pin Numbers )-----*/

// set pin numbers:
LiquidCrystal lcd(42, 44, 52, 50, 48, 46);
const int start_buttonPin = 2;     // the number of the start pushbutton pin
const int stop_buttonPin = 3;     // the number of the stop pushbutton pin
const int select_buttonPin = 4;
const int frame_sensor1 = 5; //frame detect sensor IR  -frame sensor disable
const int brusher_status = 6 ; //brusher status/error flag
const int vacuum_status = 7 ; //vacuum pressure status/error flag
const int cover_safety_status = 8 ; //cover safety status/error flag
const int motor1_home_sensor = 19;     // the number of the motor1 home sensor pin
const int motor2_home_sensor = 18;     // the number of the motor2 home sensor pin
const int motor1_extreme_sensor = 20;     // the number of the motor1 max position sensor pin
const int motor2_extreme_sensor = 21;     // the number of the motor2 max position sensor pin
const int run_ledPin =  13; // the number of the run/monitor LED pin
const int service_light =  11; //service mode and error light

int piezoPin = 12;
int lastButtonPushed = 0;
int lastStartButtonState = HIGH;  
int lastStopButtonState = HIGH;   
int lastSelectButtonState = HIGH;   

long lastStartDebounceTime = 0;  
long lastStopDebounceTime = 0;  
long lastSelectDebounceTime = 0;  

long debounceDelay = 100;    // the debounce time   
long holddebounceDelay = 2000;

int menu_display = 0;
int service_menu_display = 0;
int ret;
int safe;
String error_msg;

//user save values
float EEPROM_read_pusher = 0.0f; // variable for pusher motor
float EEPROM_read_Frame = 0.0f; // variable for Frame motor
float EEPROM_read_pushdelay = 0.0f; // variable for pusher delay
float EEPROM_pusher_FW_spd =0.00; // variable for pusher speed forward
float EEPROM_pusher_BKD_spd = 0.00; // variable for pusher speed backward
float EEPROM_frame_thick = 0.00; // variable for frame thick

float push_delay = 0.0f;
float pusher_steps = 0.0f;
float Frame_steps = 0.0f;
float pusher_FW_spd;
float pusher_BKD_spd;
float frame_thick;

int EE_addr1 = 0;
int EE_addr2 = 4;
int EE_addr3 = 8;
int EE_addr4 = 16;
int EE_addr5 = 32;
int EE_addr6 = 64;

/*
microstep driver 2M542
Pul+ goes to +5V
Pul- goes to Arduino Pin 33 & 29
Dir+ goes to +5V
Dir- goes to to Arduino Pin 31 & 27
Enable+ to nothing
Enable- to nothing
*/

//motor1 pins (pusher)
int m1_step = 33;          
int m1_dir = 31;

//motor2 pins (Frame/rack)
int m2_step = 29;          
int m2_dir = 27;

/*-----( Declare Variables )-----*/

// variables will change:
int read_start;    //start button flag
int read_stop;    // stop button flag
int run_status = 0;         //run status flag
int m1_home_status;     //motor1 home status as triggered by interrupt service 
int m2_home_status;   //motor2 home status as triggered by interrupt service 
int m1_extreme_pos = HIGH;
int m2_extreme_pos = HIGH;
float motor_speed_m1;   //variable for setting motor speed, 1=1ms=1000hz
int motor_speed_m2;
int step_per_rev_m1 = 400; //step_per_rev is the driver's micro step settings
int step_per_rev_m2 = 400;
float magz_count;
int load_level;  


//void(* resetFunc) (void) = 0; //software reset vector

void setup()   /*----( SETUP: RUNS ONCE )----*/
{
  // initialize the LED pin as an output:
  pinMode(run_ledPin, OUTPUT);
  pinMode(service_light, OUTPUT);
  pinMode(piezoPin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(start_buttonPin, INPUT_PULLUP);
  pinMode(stop_buttonPin, INPUT_PULLUP);
  pinMode(select_buttonPin, INPUT_PULLUP);
  pinMode(motor1_home_sensor, INPUT_PULLUP);
  pinMode(motor2_home_sensor, INPUT_PULLUP);
  pinMode(motor1_extreme_sensor, INPUT_PULLUP);
  pinMode(motor2_extreme_sensor, INPUT_PULLUP);
  pinMode(brusher_status, INPUT_PULLUP);
  pinMode(vacuum_status, INPUT_PULLUP);
  pinMode(cover_safety_status, INPUT_PULLUP);  
  pinMode(frame_sensor1, INPUT_PULLUP); //active high

//default settings
//hold enter during power cycle
if (digitalRead(start_buttonPin)==0)  { 
   delay(1000);
   push_delay = 0;  
   pusher_steps = 163;
   Frame_steps = 150;
   pusher_FW_spd = 2;
   pusher_BKD_spd = 2.5;
   frame_thick = 0.2;
   EEPROM.put(EE_addr1,push_delay);
   EEPROM.put(EE_addr2,pusher_steps);
   EEPROM.put(EE_addr3,Frame_steps);
   EEPROM.put(EE_addr4,pusher_FW_spd);
   EEPROM.put(EE_addr5,pusher_BKD_spd);
   EEPROM.put(EE_addr6,frame_thick);
   
   lcd.begin(16, 2);
   lcd.setCursor(0,0);
   lcd.clear();
   lcd.print("Default Loaded..");
   delay(2000);
  }
   //read stored settings
   EEPROM.get(EE_addr1,EEPROM_read_pushdelay);
   push_delay = EEPROM_read_pushdelay;
   EEPROM.get(EE_addr2,EEPROM_read_pusher);
   pusher_steps = EEPROM_read_pusher;
   EEPROM.get(EE_addr3,EEPROM_read_Frame);
   Frame_steps = EEPROM_read_Frame;
   EEPROM.get(EE_addr4,EEPROM_pusher_FW_spd);
   pusher_FW_spd = EEPROM_pusher_FW_spd;
   EEPROM.get(EE_addr5,EEPROM_pusher_BKD_spd);
   pusher_BKD_spd = EEPROM_pusher_BKD_spd;
   EEPROM.get(EE_addr6,EEPROM_frame_thick);
   frame_thick = EEPROM_frame_thick;
   
  //set interrupt to activate anytime the stop button
  attachInterrupt(digitalPinToInterrupt(stop_buttonPin),read_sensors, FALLING);
  attachInterrupt(digitalPinToInterrupt(motor1_home_sensor),read_m1_home_sensor , FALLING);
  attachInterrupt(digitalPinToInterrupt(motor2_home_sensor),read_m2_home_sensor , FALLING);
  attachInterrupt(digitalPinToInterrupt(motor1_extreme_sensor),read_m1_extreme_sensor , FALLING);
  attachInterrupt(digitalPinToInterrupt(motor2_extreme_sensor),read_m2_extreme_sensor , FALLING);
  // set initial LED state
  digitalWrite(run_ledPin, LOW);
  digitalWrite(service_light, LOW);

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
  scroll("Auto_Feed_v1.1",15,1);
  tone( piezoPin, 2400, 250);
  noTone(piezoPin);
  delay(200);
  tone( piezoPin, 2300, 250);  
  Serial.println("System Started.....");
  Serial.println("Standby Mode.....");
  
}/*--(end setup )---*/

void loop()   /*----( LOOP: RUNS CONSTANTLY )----*/
{   
      read_buttons();
      main_menu();
      if (run_status == 2)  {
        lastButtonPushed = 20;
      }
   
}/* --(end main loop )-- */



void read_buttons() {
 // read the state of the switch into a local variable:
  int reading;
  int start_buttonState=HIGH;             
  int stop_buttonState=HIGH;             
  int select_buttonState=HIGH;                  

  //start button              
  reading = digitalRead(start_buttonPin);
  if (reading != lastStartButtonState) {
   lastStartDebounceTime = millis();
     }                
      if ((millis() - lastStartDebounceTime) > debounceDelay) {
        start_buttonState = reading;
        lastStartDebounceTime=millis();;
        }                               
        lastStartButtonState = reading; 
 //select button              
  reading = digitalRead(select_buttonPin);
  if (reading != lastSelectButtonState) {
   lastSelectDebounceTime = millis();
     }                
      if ((millis() - lastSelectDebounceTime) > debounceDelay) {
        select_buttonState = reading;
        lastSelectDebounceTime=millis();;
        }                               
        lastSelectButtonState = reading; 

        
//main menu actions

 if(start_buttonState==LOW && menu_display==0){
  tone( piezoPin, 2000, 250);
  lastButtonPushed=1;
 }else if(select_buttonState==LOW && menu_display==0){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=3;
 }else if(select_buttonState==LOW && menu_display==3){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=5;
 }else if(select_buttonState==LOW && menu_display==5){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=8;
 }else if(select_buttonState==LOW && menu_display==8){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=10;
 }else if(select_buttonState==LOW && menu_display==10){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=0; 
 }else if(start_buttonState==LOW && menu_display==3){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=4;
 }else if(start_buttonState==LOW && menu_display==5){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=6;
 }else if(start_buttonState==LOW && menu_display==8){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=9;
 }else if(start_buttonState==LOW && menu_display==10){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=11; 
 }else if(start_buttonState==LOW && menu_display==12){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=1;
 }else if(select_buttonState==LOW && menu_display==11){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=0;    
   
//service menu actions
  
 }else if(start_buttonState==LOW && service_menu_display==0){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=1; 
 }else if(select_buttonState==LOW && service_menu_display==0){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=2; 
 }else if(select_buttonState==LOW && service_menu_display==2){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=4;
 }else if(start_buttonState==LOW && service_menu_display==2){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=3; 
 }else if(start_buttonState==LOW && service_menu_display==3){
  tone( piezoPin, 2200, 250);
  lastButtonPushed=2;
 }else if(start_buttonState==LOW && service_menu_display==4){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=5;
 }else if(start_buttonState==LOW && service_menu_display==5){
  tone( piezoPin, 2200, 250);
  lastButtonPushed=4;
 }else if(start_buttonState==LOW && service_menu_display==6){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=7;
 }else if(start_buttonState==LOW && service_menu_display==7){
  tone( piezoPin, 2300, 250);
  lastButtonPushed=6;        
 }else if(select_buttonState==LOW && service_menu_display==4){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=6;
 }else if(select_buttonState==LOW && service_menu_display==6){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=8;
 }else if(start_buttonState==LOW && service_menu_display==8){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=9;  
 }else if(select_buttonState==LOW && service_menu_display==8){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=10;
 }else if(start_buttonState==LOW && service_menu_display==10){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=11; 
 }else if(select_buttonState==LOW && service_menu_display==10){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=12;
 }else if(start_buttonState==LOW && service_menu_display==12){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=13;  
 }else if(select_buttonState==LOW && service_menu_display==12){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=14;
 }else if(start_buttonState==LOW && service_menu_display==14){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=15;            
 }else if(select_buttonState==LOW && service_menu_display==14){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=16; 
 }else if(select_buttonState==LOW && service_menu_display==16){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=18;
 }else if(start_buttonState==LOW && service_menu_display==16){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=17;
 }else if(start_buttonState==LOW && service_menu_display==17){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=16;                 
 }else if(select_buttonState==LOW && service_menu_display==18){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=20;
 }else if(start_buttonState==LOW && service_menu_display==18){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=19; 
 }else if(start_buttonState==LOW && service_menu_display==19){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=18;

}else if(select_buttonState==LOW && service_menu_display==20){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=22;
 }else if(start_buttonState==LOW && service_menu_display==20){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=21; 
 }else if(start_buttonState==LOW && service_menu_display==21){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=20;
   
 }else if(select_buttonState==LOW && service_menu_display==22){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=0;   
 }else if(start_buttonState==LOW && service_menu_display==22){
  tone( piezoPin, 2100, 250);
  lastButtonPushed=23;                                                                           
  }                
}

void main_menu() {
  
   switch (lastButtonPushed) {
    case 0:
      lcd.setCursor(0,0);
      lcd.print("Enter to Start :");
      startup_display();
      menu_display = 0;
      
      break;
   
    case 1:
     safe = check_home();
      if (safe == 1) {
          error_msg = "<Cover is Open >";
          run_status = 2;
          lastButtonPushed = 20;            
          break;  
          }              
      run_status = 1;
      digitalWrite(run_ledPin, HIGH);  
      Serial.print("run_status = "); Serial.println(run_status);
      lcd.clear();
      display_status_lcd(0,0,"Run Status = ",run_status);     
      delay(1000);
      Serial.println("Sequence start!");
      ret = auto_mode(Frame_steps); //set the number of feeds
        if (ret == 1) {
          //lcd.clear();
          display_status_lcd(0,0,"Run Status = ",run_status);
          display_lcd(0,1,"Halt !");
          Serial.println("Halt!");         
          break;
          }
        if (ret == 2) {
          //lcd.clear();
          error_msg = "M1 Ref/POS Error";
          run_status = 2;
          lastButtonPushed = 20;            
          break;
        }      
        if (ret == 3) {
          error_msg = "<Brusher Error> ";
          run_status = 2;
          lastButtonPushed = 20;            
          break;  
          } 
        if (ret == 4) {
          error_msg = "< Vacuum Error >";
          run_status = 2;
          lastButtonPushed = 20;            
          break;  
          }
        if (ret == 5) {
          error_msg = "<Cover is Open >";
          run_status = 2;
          lastButtonPushed = 20;            
          break;  
          }        
      safe = check_home();
      if (safe == 1) {
          error_msg = "<Cover is Open >";
          run_status = 2;
          lastButtonPushed = 20;            
          break;  
          }              
      run_status = 0;
      display_status_lcd(0,0,"Run Status = ",run_status);
      display_lcd(0,1,"<Empty Frame>");
      Serial.println("Frame is Empty");
      digitalWrite(run_ledPin, LOW);
      lastButtonPushed=12;      
      delay(2000);         
      break;

        case 3:
      
      lcd.setCursor(0,0);
      lcd.print("Select or Enter:");
      lcd.setCursor(0,1);
      lcd.print("    Load Frame   ");
      menu_display = 3; 

      
      break;

    case 4:
      load();
      menu_display = 3;
      lastButtonPushed=0;
      break;

    case 5:
      lcd.setCursor(0,0);
      lcd.print("Select or Enter:");
      lcd.setCursor(0,1);
      lcd.print(" Home  Position "); //go to home position
      menu_display = 5;
      break;

    case 6:
      unload();
      menu_display = 5;
      lastButtonPushed=0;
      break;

    case 7:
        // turn start LED off and stepper motor1:
   
      digitalWrite(run_ledPin, LOW);
      Serial.print("run_status = ");
      Serial.println("Stop");
      lcd.setCursor(0,0);
      lcd.print("<STOP to RESET> ");
      lcd.setCursor(0,1);
      lcd.print("Stopped.........");
      menu_display = 7;          
      break;
      
    case 8:
      
      lcd.setCursor(0,0);
      lcd.print("Select or Enter:");
      lcd.setCursor(0,1);
      lcd.print("< Service Menu >");
      menu_display = 8; 
      break;
        
    case 9:    
      menu_display = 9;
      lastButtonPushed=0;
      service_menu();
      lastButtonPushed=0;
      break;
      
    case 10:
      lcd.setCursor(0,0);
      lcd.print("Select or Enter:");
      lcd.setCursor(0,1);
      lcd.print("< Service Mode >");
      menu_display = 10;
      
      break ;

    case 11:
      digitalWrite(run_ledPin, LOW);
      digitalWrite(service_light, HIGH);
      Serial.println("Service Mode.....");
      scroll(" OUT OF SERVICE ",13,2);
      display_lcd(0,0,"STOP / RESET "); 
      display_lcd(0,1,"To Restart .....");
      digitalWrite(service_light, LOW);
      tone( piezoPin, 2100, 250);
      delay(200);
      noTone(piezoPin);
      delay(200);
      digitalWrite(service_light, HIGH);
      tone( piezoPin, 2100, 250);
      delay(200);
      noTone(piezoPin);
      delay(200);
      digitalWrite(service_light, LOW);
      tone( piezoPin, 2000, 250);
      delay(200);
      tone( piezoPin, 2100, 250);
      digitalWrite(service_light, HIGH);
      delay(200);
      noTone(piezoPin);
      delay(200);
      tone( piezoPin, 2100, 250);
      digitalWrite(service_light, LOW);
      delay(200);
      noTone(piezoPin);
      delay(200);
      tone( piezoPin, 2000, 250);
      menu_display = 11;
      
      break ;    

    case 12:
      digitalWrite(run_ledPin, LOW);
      scroll(" <<< RELOAD >>>  ",13,1);
      display_lcd(0,0,"Frames Empty!"); 
      display_lcd(0,1,"<<<  RELOAD  >>>");
      digitalWrite(run_ledPin, HIGH);
      tone( piezoPin, 2100, 250);
      delay(200);
      noTone(piezoPin);
      delay(200);
      digitalWrite(run_ledPin, LOW);
      tone( piezoPin, 2100, 250);
      delay(200);
      noTone(piezoPin);
      delay(200);
      digitalWrite(run_ledPin, HIGH);
      tone( piezoPin, 2000, 250);
      delay(200);
      tone( piezoPin, 2100, 250);
      digitalWrite(run_ledPin, LOW);
      delay(200);
      noTone(piezoPin);
      delay(200);
      tone( piezoPin, 2100, 250);
      digitalWrite(run_ledPin, HIGH );
      delay(200);
      noTone(piezoPin);
      delay(200);
      tone( piezoPin, 2000, 250);
      
      menu_display = 12;
      break ;
   
    case 20:
      digitalWrite(run_ledPin, LOW);
      digitalWrite(service_light, HIGH);
      Serial.println(error_msg);
      scroll(" <<< ERROR >>>  ",13,2);
      display_lcd(0,0,error_msg); 
      display_lcd(0,1," <<< ERROR >>>  ");      
      digitalWrite(service_light, LOW);    
      tone( piezoPin, 2100, 250);
      delay(200);
      noTone(piezoPin);
      delay(200);
      digitalWrite(service_light, HIGH);
      tone( piezoPin, 2100, 250);
      delay(200);
      noTone(piezoPin);
      delay(200);
      digitalWrite(service_light, LOW);
      tone( piezoPin, 2000, 250);
      delay(200);
      tone( piezoPin, 2100, 250);
      digitalWrite(service_light, HIGH);
      delay(200);
      noTone(piezoPin);
      delay(200);
      tone( piezoPin, 2100, 250);
      digitalWrite(service_light, LOW);
      delay(200);
      noTone(piezoPin);
      delay(200);
      tone( piezoPin, 2000, 250);
      menu_display = 13;
      break ;         
   }
}

void service_menu() {
  while (menu_display == 9)  {
   read_buttons();
    switch (lastButtonPushed) {
      case 0:
        delay(60);
        lcd.setCursor(0,0);
        lcd.print("Select or Enter:");
        lcd.setCursor(0,1);
        lcd.print(" <Manual  Feed> ");
        service_menu_display = 0;     
      break;

      case 1:
        manual_feed();
        lastButtonPushed=0;     
      break;

      case 2:
        lcd.setCursor(0,0);
        lcd.print("Select or Enter:");
        lcd.setCursor(0,1);
        lcd.print("<Set Feed Delay>");
        service_menu_display = 2;     
      break;

      case 3:
        service_menu_display = 3;
        set_feed_delay();
        EEPROM.get(EE_addr1,EEPROM_read_pushdelay);
        push_delay = EEPROM_read_pushdelay;
        lastButtonPushed=2;
        
      break;
      
      case 4:
        lcd.setCursor(0,0);
        lcd.print("Select or Enter:");
        lcd.setCursor(0,1);
        lcd.print("Set Pusher Steps");      
        service_menu_display = 4;     
      break;

      case 5:
        service_menu_display = 5;
        set_pusher_steps();
        EEPROM.get(EE_addr2,EEPROM_read_pusher);
        pusher_steps = EEPROM_read_pusher;
        lastButtonPushed=4;
        
      break;
      
      case 6:
        lcd.setCursor(0,0);
        lcd.print("Select or Enter:");
        lcd.setCursor(0,1);
        lcd.print("SetFrameCapacity");
        service_menu_display = 6;     
      break;

      case 7:
        service_menu_display = 7;
        set_Frame_cap();
        EEPROM.get(EE_addr3,EEPROM_read_Frame);
        Frame_steps = EEPROM_read_Frame;
        lastButtonPushed=6;
        
      break;

      case 8:
        lcd.setCursor(0,0);
        lcd.print("Select or Enter:");
        lcd.setCursor(0,1);
        lcd.print("Move Pusher FWD ");
        service_menu_display = 8;     
      break;

      case 9:
        pusher_forward();
        lastButtonPushed=8;     
      break;

      case 10:
        lcd.setCursor(0,0);
        lcd.print("Select or Enter:");
        lcd.setCursor(0,1);
        lcd.print("Move Pusher BKD ");
        service_menu_display = 10;     
      break;
      
      case 11:
        pusher_backward();
        lastButtonPushed=10; 
      break;
      
      case 12:
        lcd.setCursor(0,0);
        lcd.print("Select or Enter:");
        lcd.setCursor(0,1);
        lcd.print("Move Frame Up  ");
        service_menu_display = 12;     
      break;

      case 13:
        Frame_forward();
        lastButtonPushed=12; 
      break;
      
      case 14:
        lcd.setCursor(0,0);
        lcd.print("Select or Enter:");
        lcd.setCursor(0,1);
        lcd.print("Move Frame Down");
        service_menu_display = 14;     
      break;
      
      case 15:
        Frame_backward();
        lastButtonPushed=14; 
      break;

      case 16:
        lcd.setCursor(0,0);
        lcd.print("Select or Enter:");
        lcd.setCursor(0,1);
        lcd.print("Set FW Push Spd ");
        service_menu_display = 16;     
      break;

      case 17:
        service_menu_display = 17;
        set_pusher_FW_speed();
        EEPROM.get(EE_addr4,EEPROM_pusher_FW_spd);
        pusher_FW_spd = EEPROM_pusher_FW_spd;
        lastButtonPushed=16;
        
      break;

      case 18:
        lcd.setCursor(0,0);
        lcd.print("Select or Enter:");
        lcd.setCursor(0,1);
        lcd.print("Set BKD Push Spd");
        service_menu_display = 18;     
      break;

      case 19:
        service_menu_display = 19;
        set_pusher_BKD_speed();
        EEPROM.get(EE_addr5,EEPROM_pusher_BKD_spd);
        pusher_BKD_spd = EEPROM_pusher_BKD_spd;
        lastButtonPushed=18;
        
      break;

      case 20:
        lcd.setCursor(0,0);
        lcd.print("Select or Enter:");
        lcd.setCursor(0,1);
        lcd.print("Set Frame thick ");
        service_menu_display = 20;     
      break;

      case 21:
        service_menu_display = 21;
        set_frame_thick();
        EEPROM.get(EE_addr6,EEPROM_frame_thick);
        frame_thick = EEPROM_frame_thick;
        lastButtonPushed=20;   
      break;
    
      case 22:
        lcd.setCursor(0,0);
        lcd.print("Select or Enter:");
        lcd.setCursor(0,1);
        lcd.print("EXIT ServiceMenu");
        service_menu_display = 22;     
      break;

      case 23:
      menu_display = 0;
        
      break;
    }
  } 
}

void read_sensors() {
 //Interrupt Service Routine, read the state of the stop switch into a local variable: 
  int read_stop = digitalRead(stop_buttonPin);
  tone( piezoPin, 2300, 250);
  delay (150);
  if (read_stop == LOW && run_status == 0) {
          Serial.println("Reset");
          resetFunc();
      }

  if (read_stop == LOW && run_status == 2) {
          Serial.println("Reset");
          lcd.clear();
          resetFunc();
      }
    tone( piezoPin, 2000, 250);  
    run_status = 0;
    lcd.clear(); 
    display_lcd(0,0,"STOP Activated!!");
    display_lcd(0,1,"Ending Sequence");
    digitalWrite(run_ledPin, LOW);
    Serial.print("run_status = ");
    Serial.println("Stop");
    lastButtonPushed=7;   
    //run_status = 0;
    delay(50);  
 }
 
void read_m1_home_sensor() {
  Serial.println("ISR for m1_home_sensor activated!");
  m1_home_status = LOW;
  delay(100);  
}

void read_m2_home_sensor() {
  Serial.println("ISR for m2_home_sensor activated!");
  m2_home_status = LOW;
  delay(100);  
}

void read_m1_extreme_sensor() {
  Serial.println("ERROR:ISR for m1_extreme_sensor activated!");
  m1_extreme_pos = LOW;
  delay(100);
      
}

void read_m2_extreme_sensor() {
  Serial.println("ERROR:ISR for m2_extreme_sensor activated!");
  m2_extreme_pos = LOW;
  delay(100);
  
}

void run_step_m1(int dir1, long rev1) {    
  //dir1 set direction of m1
  //rev1 set the number of revolutions for m1
  //step_per_rev is the driver's micro step settings 
  //start stepper motor1
  
  Serial.print("Revs_m1  =  ");Serial.println(rev1);
  digitalWrite(m1_dir, dir1);  
  for (int x = 0; x <= rev1 * step_per_rev_m1; x++) {
  digitalWrite(m1_step, HIGH);
  time_delay1(motor_speed_m1);
  digitalWrite(m1_step, LOW);
  time_delay1(motor_speed_m1);
  if (m1_extreme_pos == LOW) {
    //detachInterrupt(3);
    error_msg = "M1 in ExtremePOS";
    run_status=2;
    lastButtonPushed = 20;
    break;
   }
  }
}

void run_step_m2(int dir2, float rev2) {    
  //dir2 set direction of m2
  //rev2 set the number of revolutions for m2
  //step_per_rev is the driver's micro step settings 
  //start stepper motor2 cw:
  Serial.print("Revs_m2  =  ");Serial.println(rev2);
  digitalWrite(m2_dir, dir2);  
  for (int z = 0; z <= rev2 * step_per_rev_m2; z++) {
  digitalWrite(m2_step, HIGH);
  time_delay2(motor_speed_m2);
  digitalWrite(m2_step, LOW);
  time_delay2(motor_speed_m2);
  if (m2_extreme_pos == LOW) {
    //detachInterrupt(2);
    error_msg = "M2 in ExtremePOS";
    run_status=2;
    lastButtonPushed = 20;
    main_menu();
   }
    }
}

void time_delay1(float del) {
  for (int count = 0; count <=del; count++) {
  delayMicroseconds(100);
  }
}

void time_delay2(int del) {
  for (int count = 0; count <=del; count++) {
  delayMicroseconds(500);
  }
}

void set_push_delay(int del) {
 for (int count = 0; count <=del; count++) {
  delay(1000);
  } 
}

void set_m1_home()  {
  Serial.print("m1_home_status = "); 
  Serial.println(m1_home_status);
  Serial.println("Setting m1 to home position.....");
  display_lcd(0,1," Set M1 to home ");
  motor_speed_m1 = 2.5;
  while (m1_home_status == HIGH) {
  run_step_m1(1,1);
  if (m1_extreme_pos == LOW) {
    error_msg = "M1 in ExtremePOS";
    run_status=2;
    lastButtonPushed = 20;
    break;
      }
    }
  Serial.print("m1_home_sensor = ");
  Serial.println(m1_home_status);
  Serial.println("m1 in home position.....");   
}  

void set_m2_home()  {
  Serial.print("m2_home_status = "); 
  Serial.println(m2_home_status);
  Serial.println("Setting m2 to home position.....");
  display_lcd(0,1," Set M2 to home ");
  motor_speed_m2 = 1.75;
  while (m2_home_status == HIGH) {
  run_step_m2(0,frame_thick);
  if (m2_extreme_pos == LOW) {
    error_msg = "M2 in ExtremePOS";
    run_status=2;
    lastButtonPushed = 20;
    break;
    }
  }
  Serial.print("m2_home_sensor = ");
  Serial.println(m2_home_status);
  Serial.println("m2 in home position.....");   
}  

int auto_mode (int cycle) {
  if (digitalRead(cover_safety_status) == LOW) {
      run_status = 2;
      return 5;
  }
  int read_frame;
  Serial.print("Total Cycle = ");Serial.println(cycle);     
  for (int y = 0; y <cycle; y++) {
    Serial.print("Cycle no  = ");Serial.println(y+1);
    display_status_lcd(0,1,"Cycle No   =  ",(y+1));    
    motor_speed_m2 = 1.75;
    run_step_m2(1,frame_thick);
    run_step_m2(1,frame_thick);
   read_frame = digitalRead(frame_sensor1); //read_frame = 0; << to disable use this
   delay (250); //debounce delay to make sure noise is not read
    if (digitalRead(vacuum_status) == LOW) {
      run_status = 2;
      return 4;
      }
    if (read_frame == 0) { //frame sensor active high
      set_push_delay(push_delay);
      delay(500);
      //turn on solenoid
      motor_speed_m2 = 1.75;
      run_step_m2(0,frame_thick);
      run_step_m2(0,frame_thick);
      run_step_m2(0,frame_thick);
      run_step_m2(0,frame_thick);
      run_step_m2(0,frame_thick);
      run_step_m2(0,frame_thick);
      run_step_m2(0,frame_thick);
      run_step_m2(0,frame_thick);   
      delay(500);
      motor_speed_m1 = pusher_FW_spd; //user save through service menu
      run_step_m1(0,pusher_steps);
      delay (1500);
      motor_speed_m1 = pusher_BKD_spd; //user save through service menu
      run_step_m1(1,pusher_steps);      
      delay (500);
     }
    if (read_frame == 1) { //no frame activated
      Serial.print("Frame not detected ....");
      display_lcd(0,1,"Frame missing...");
      delay (500);
     }
    
    //m1_home_status = digitalRead(motor1_home_sensor);
    //delay (50);
    if (digitalRead(motor1_home_sensor) == HIGH) {
      run_status = 2;
      return 2;
    }
    if (digitalRead(brusher_status) == LOW) {
      run_status = 2;
      return 3;
    }
    if (run_status == 0) {
      lastButtonPushed = 7;
      return 1;
      }
    tone( piezoPin, 2000, 250);   
  }
  return 0;
}

int check_home() {
    if (digitalRead(cover_safety_status) == LOW) {
      error_msg = "<Brusher Error> ";
      run_status = 2;
      lastButtonPushed = 20;
      return 1;
    }
    display_lcd(0,0,"<Check Home POS>");
    int read_home_status;
    read_home_status = digitalRead(motor2_home_sensor);
    delay(100);
    m2_home_status = read_home_status ;
    set_m2_home();
    //m2_home_status = LOW;//reset m1 home status flag for debug, this will be activated by sensor
    read_home_status = digitalRead(motor1_home_sensor);
    delay(100);
    m1_home_status = read_home_status ;
    set_m1_home();   
    //m1_home_status = LOW;//reset m1 home status flag for debug, this will be activated by sensor
    return 0;         
}
void check_home_magazine() {
  display_lcd(0,0, "<CHeck Home POS>");
  int read_home_status;
  read_home_status = digitalRead(motor2_home_sensor);
  delay(100);
  m2_home_status = read_home_status;
  set_m2_home();
}
void unload() {
  check_home();
  //Serial.println("Unloading the Frame.....");
  display_lcd(0,1,"Go Home Position");
  //motor_speed_m2 = 1;
  //run_step_m2(1,Frame_steps/2);
  //Serial.print("..m2_unload_position"); 
  delay(1000); 
}

void load() {    
  magz_count = 94;
  load_level = 3;    
  Serial.println("Loading the Frame.....");
  check_home();
  delay(1000);
  motor_speed_m1 = 1.75;
  run_step_m1(0,150);
  delay(500);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Loading Frame....");
  magz_count=magz_count-(94*0.5); //orig 0.25
  motor_speed_m2 = 1;
  load_level=load_level-1;
  run_step_m2(1,magz_count/2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Hit ENT to Load");
  lcd.setCursor(0, 1);
  lcd.print(load_level);
   while (load_level != 0) {
        //read_buttons();
        if (digitalRead(start_buttonPin)== 0) {
          delay(150);
          load_level = load_level-1;
          tone( piezoPin, 2000, 150);
          lcd.setCursor(0, 1);
          lcd.print("Loading Frames....");
          run_step_m2(0, (Frame_steps*0.135)/2);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Hit ENT to Load");
            lcd.setCursor(0, 1);
            lcd.print(" LOAD  LEVEL = ");
            lcd.print(load_level);
        }
   lcd.setCursor(0, 1);
   lcd.print(" LOAD  LEVEL = ");
   lcd.print(load_level);
     }
  check_home_magazine();
  tone( piezoPin, 2000, 150);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("LOADING POSITION");
  delay(2000); 
}

void manual_feed() {
    check_home(); 
    Serial.println("Manual Feeding..");
    display_lcd(0,1,"Manual Feeding..");  
    motor_speed_m2 = 1;
    run_step_m2(1,frame_thick);
    run_step_m2(1,frame_thick);
    delay (1000);    
    motor_speed_m1 = pusher_FW_spd;
    run_step_m1(0,pusher_steps);
    delay (500);
    motor_speed_m1 = pusher_BKD_spd;
    run_step_m1(1,pusher_steps);
    delay (500);    
}

void pusher_forward() {
   if (digitalRead(motor1_extreme_sensor) == LOW) {
      display_lcd(0,1,"M1 Max Position ");
      delay(2000);      
      service_menu_display = 8;
      return;
   }
    Serial.println("Pusher Forward..");
    display_lcd(0,1,"Pusher Forward..");    
    motor_speed_m1 = 2;
    run_step_m1(0,1);
    delay (100);
}
void pusher_backward() {
    if (digitalRead(motor1_home_sensor) == LOW) {
      display_lcd(0,1,"M1 Min Position ");
      delay(2000);      
      service_menu_display = 10;
      return;
      }
    Serial.println("Pusher Backward ");
    display_lcd(0,1,"Pusher Backward ");    
    motor_speed_m1 = 2;
    run_step_m1(1,1);
    delay (100);
    
}

void Frame_forward() {
  if (digitalRead(motor2_extreme_sensor) == LOW) {
      display_lcd(0,1,"M2 Max Position ");
      delay(2000);      
      service_menu_display = 12;
      return;
      }
    Serial.println("Frame Backward ");
    display_lcd(0,1,"Frame Upward...");    
    motor_speed_m2 = 1;
    run_step_m2(1,frame_thick);
    run_step_m2(1,frame_thick);//new add
    delay (250);
}

void Frame_backward() {
    if (digitalRead(motor2_home_sensor) == LOW) {
      display_lcd(0,1,"M2 Min Position ");
      delay(2000);      
      service_menu_display = 14;
      return;
      }
    Serial.println("Frame Backward ");
    display_lcd(0,1,"Frame Downward...");    
    motor_speed_m2 = 1;
    run_step_m2(0,frame_thick);
    run_step_m2(0,frame_thick);
    delay (250);
}

void set_feed_delay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HIT  ENT to SAVE");
  lcd.setCursor(0, 1);
  lcd.print("FEED Delay   =");
  lcd.print(EEPROM_read_pushdelay,0);
  push_delay = EEPROM_read_pushdelay;
  delay(2000);
  while (lastButtonPushed==3) {
        read_buttons();
        if (digitalRead(select_buttonPin)== 0) {
          delay(150);
          push_delay = push_delay+1.0;
          tone( piezoPin, 2000, 150);
          if (push_delay > 99) {
            push_delay = 0;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("HIT  ENT to SAVE");
            lcd.setCursor(0, 1);   
            lcd.print("FEED Delay   =");
            lcd.print(push_delay,0); 
          }
        }
    lcd.setCursor(0, 1);   
    lcd.print("FEED Delay   =");
    lcd.print(push_delay,0); 
      }   
 EEPROM.put(EE_addr1, push_delay);
 lcd.clear();
 display_lcd(0,0,"Settings Saved! ");
 delay(2000);      
}
void set_pusher_steps() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HIT  ENT to SAVE");
  lcd.setCursor(0, 1);
  lcd.print("PUSHER Steps=");
  lcd.print(EEPROM_read_pusher,0);
  pusher_steps = EEPROM_read_pusher;
  delay(2000);
  while (lastButtonPushed==5) {
        read_buttons();
        if (digitalRead(select_buttonPin)== 0) {
          delay(150);
          pusher_steps = pusher_steps+1.0;
          tone( piezoPin, 2000, 150);
          if (pusher_steps > 165) {
            pusher_steps = 150;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("HIT  ENT to SAVE");
            lcd.setCursor(0, 1);   
            lcd.print("PUSHER Steps=");
            lcd.print(pusher_steps,0); 
          }
        }
    lcd.setCursor(0, 1);   
    lcd.print("PUSHER Steps=");
    lcd.print(pusher_steps,0); 
      }   
 EEPROM.put(EE_addr2, pusher_steps);
 lcd.clear();
 display_lcd(0,0,"Settings Saved! ");
 delay(2000);       
}
void set_Frame_cap() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HIT  ENT to SAVE");
  lcd.setCursor(0, 1);
  lcd.print("Frame Cap =");
  lcd.print(EEPROM_read_Frame,0);
  Frame_steps = EEPROM_read_Frame;
  delay(2000);
  while (lastButtonPushed==7) {
        read_buttons();
        if (digitalRead(select_buttonPin)== 0) {
          delay(150);
          Frame_steps = Frame_steps+1.0;
          tone( piezoPin, 2000, 150);
          if (Frame_steps > 120) {
            Frame_steps = 80;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("HIT  ENT to SAVE");
            lcd.setCursor(0, 1);   
            lcd.print("Frame Cap =");
            lcd.print(Frame_steps,0); 
          }
        }
    lcd.setCursor(0, 1);   
    lcd.print("Frame Cap =");
    lcd.print(Frame_steps,0); 
      }   
 EEPROM.put(EE_addr3, Frame_steps);
 lcd.clear();
 display_lcd(0,0,"Settings Saved! ");
 delay(2000);  
     
}

void set_pusher_FW_speed() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HIT  ENT to SAVE");
  lcd.setCursor(0, 1);
  lcd.print("FW Push Spd=");
  lcd.print(EEPROM_pusher_FW_spd,2);
  pusher_FW_spd = EEPROM_pusher_FW_spd;
  delay(2000);
  while (lastButtonPushed==17) {
        read_buttons();
        if (digitalRead(select_buttonPin)== 0) {
          delay(150);
          pusher_FW_spd = pusher_FW_spd+0.1;
          tone( piezoPin, 2000, 150);
          if (pusher_FW_spd > 3.00) {
            pusher_FW_spd = 2;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("HIT  ENT to SAVE");
            lcd.setCursor(0, 1);   
            lcd.print("FW Push Spd=");
            lcd.print(pusher_FW_spd,2); 
          }
        }
    lcd.setCursor(0, 1);   
    lcd.print("FW Push Spd=");
    lcd.print(pusher_FW_spd,2); 
      }   
 EEPROM.put(EE_addr4, pusher_FW_spd);
 lcd.clear();
 display_lcd(0,0,"Settings Saved! ");
 delay(2000);  
     
}

void set_pusher_BKD_speed() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HIT  ENT to SAVE");
  lcd.setCursor(0, 1);
  lcd.print("BKD PushSpd=");
  lcd.print(EEPROM_pusher_BKD_spd,2);
  pusher_BKD_spd = EEPROM_pusher_BKD_spd;
  delay(2000);
  while (lastButtonPushed==19) {
        read_buttons();
        if (digitalRead(select_buttonPin)== 0) {
          delay(150);
          pusher_BKD_spd = pusher_BKD_spd+0.1;
          tone( piezoPin, 2000, 150);
          if (pusher_BKD_spd > 3.00) {
            pusher_BKD_spd = 2;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("HIT  ENT to SAVE");
            lcd.setCursor(0, 1);   
            lcd.print("BKD PushSpd=");
            lcd.print(pusher_BKD_spd,2); 
          }
        }
    lcd.setCursor(0, 1);   
    lcd.print("BKD PushSpd=");
    lcd.print(pusher_BKD_spd,2); 
      }   
 EEPROM.put(EE_addr5, pusher_BKD_spd);
 lcd.clear();
 display_lcd(0,0,"Settings Saved! ");
 delay(2000);     
}

void set_frame_thick() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HIT  ENT to SAVE");
  lcd.setCursor(0, 1);
  lcd.print("Frame thick=");
  lcd.print(EEPROM_frame_thick,2);
  frame_thick = EEPROM_frame_thick;
  delay(2000);
  while (lastButtonPushed==21) {
        read_buttons();
        if (digitalRead(select_buttonPin)== 0) {
          delay(150);
          frame_thick = frame_thick+0.01;
          tone( piezoPin, 2000, 150);
          if (frame_thick > 0.50) {
            frame_thick = 0.01;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("HIT  ENT to SAVE");
            lcd.setCursor(0, 1);   
            lcd.print("Frame thick=");
            lcd.print(frame_thick,2); 
          }
        }
    lcd.setCursor(0, 1);   
    lcd.print("Frame thick=");
    lcd.print(frame_thick,2); 
      }   
 EEPROM.put(EE_addr6, frame_thick);
 lcd.clear();
 display_lcd(0,0,"Settings Saved! ");
 delay(2000);  
     
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

void scroll (String msg, int pos, int repeat) {
  
 for (int rep = 0; rep < repeat; rep++)  { 
  lcd.clear();
  lcd.begin(16, 2);
  lcd.print(msg);
  delay(100);
  // scroll 13 positions (string length) to the left
  // to move it offscreen left:
  for (int positionCounter = 0; positionCounter < pos; positionCounter++) {
    // scroll one position left:
    lcd.scrollDisplayLeft();
    // wait a bit:
    read_buttons();
    delay(100);
  }

  // scroll 13 positions (string length + display length) to the right
  // to move it offscreen right:
  for (int positionCounter = 0; positionCounter < pos; positionCounter++) {
    // scroll one position right:
    lcd.scrollDisplayRight();
    // wait a bit:
    read_buttons();
    delay(100);
  }
 
  // delay at the end of the full loop:
  delay(100);
  read_buttons();
 }
}

void startup_display()  {
    lcd.setCursor(0,1);
    lcd.print("  Standby Mode  ");
    //Serial.println("Standby Mode.....");
}
/* ( THE END ) */
