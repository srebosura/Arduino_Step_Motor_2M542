
void(* resetFunc) (void) = 0; //software reset vector
// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
//LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
const int start_buttonPin = 2;     // the number of the start pushbutton pin
const int stop_buttonPin = 3;     // the number of the stop pushbutton pin
const int run_ledPin =  13;      // the number of the run/monitor LED pin
int run_ledState = LOW;         // the current state of the output pin

int read_start;    //start button flag
int read_stop;    // stop button flag
int run_status;


void setup() {
  pinMode(run_ledPin, OUTPUT);
  pinMode(start_buttonPin, INPUT_PULLUP);
  pinMode(stop_buttonPin, INPUT_PULLUP);
  digitalWrite(run_ledPin, run_ledState);
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(stop_buttonPin), read_stop_ISR, CHANGE);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.setCursor(5,0);
  lcd.print("SEABED");
  lcd.setCursor(2,1);
  lcd.print("WWW.SBGS.COM");
  delay(1000);
}

void loop() {
 run_status = read_switches();
 while (run_status == 1)  {
   digitalWrite(run_ledPin, HIGH);  
   Serial.print("run_status = "); Serial.println(run_status);
   lcd.clear();
   display_lcd(5,0,"SEABED");
   delay(2000);
   display_lcd(2,1,"WWW.SBGS.COM");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(5,1,"Sam");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(5,1,"Fab");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(5,1,"Osborn");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(5,1,"Yuri");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(5,1,"Steve");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(5,1,"Matt");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(5,1,"Evgeny");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(5,1,"Stewart");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(2,1,"The A-Team");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(1,1,"Hugin QC Dept");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(3,1,"NAV Dept");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(5,1,"ROV Dept");
   delay(2000);
   scroll("Merry Christmas");
   display_lcd(2,1,"Fugro Marine");
   delay(2000);
   scroll("WE LOVE HUGIN");
   delay(2000);
 }
 if (run_status == 0) {
  lcd.clear();
  lcd.print("Restarting .....");
  delay(5000);
  resetFunc();
   
 }
 
 if (run_status == 2) {
  lcd.clear();
  lcd.print("Goodbye!");
  delay(5000);
  Serial.println("Stop Button Activated");  
 }
}

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
  if (read_stop == LOW) {
    return 0;
    }
  else { return 11;
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



void read_stop_ISR () {
 //Interrupt Service Routine, read the state of the stop switch into a local variable: 
  //int read_stop = digitalRead(stop_buttonPin);
  delay (100);
  //if (read_stop == LOW) {
    run_status=2; 
    
    //resetFunc();
      
 }

void scroll (String msg) {
  lcd.clear();
  lcd.begin(16, 2);
  lcd.print(msg);
  delay(500);
    // scroll 13 positions (string length) to the left
  // to move it offscreen left:
  for (int positionCounter = 0; positionCounter < 13; positionCounter++) {
    // scroll one position left:
    lcd.scrollDisplayLeft();
    // wait a bit:
    delay(250);
  }

  // scroll 29 positions (string length + display length) to the right
  // to move it offscreen right:
  for (int positionCounter = 0; positionCounter < 29; positionCounter++) {
    // scroll one position right:
    lcd.scrollDisplayRight();
    // wait a bit:
    delay(250);
  }

  // scroll 16 positions (display length + string length) to the left
  // to move it back to center:
  for (int positionCounter = 0; positionCounter < 16; positionCounter++) {
    // scroll one position left:
    lcd.scrollDisplayLeft();
    // wait a bit:
    delay(250);
  }

  // delay at the end of the full loop:
  delay(500);
  
}

