/****Wii Remote Status Lights and Button Functionality****/

/** Wii Remote LED 1 on means Disconnected
/** Wii Remote LED 4 on means Wii remote connected
/** Button 1 initializes IR and turns it ON. Press more than once if it doesn't read IR the first time
/** Button 2 gets status request to see if IR is initialized (only used with Serial Monitor)
/** Button A turns toggles IR Emitters ON/OFF
/** Button B Enables/Disables motor controller
/** Button + initializes the cart, turns on IR, and begins reading data 
/** Button - turns all functionality off, but remians connected
/** DOWN is used for debugging, outputs raw IR data (only used wit hSerial Monitor)
/** HOME turns everything off and disconnects

/********************************************************************************************************/

/** RED LED -- IR out of range
/** BLUE LED -- Person too close
/** Green LED -- Person furthur than 3 feet
*/

/******************************************************************************************************/
/***X and Y in the program refers to the x and y coordinates of the points the IR camera is tracking***/
/***********************X ranges from 0 - 1023, and Y ranges from 0 - 767******************************/
/******************************************************************************************************/

#include <Wii.h>
USB Usb;
BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so
/* You can create the instance of the class in two ways */
//WII Wii(&Btd,PAIR); // This will start an inquiry and then pair with your Wiimote - you only have to do this once
WII Wii(&Btd); // After that you can simply create the instance like so and then press any button on the Wiimote

#define DEBUG
#define DEBUG_Y
#define EXTRADEBUG_X_Y
#define RAWVALUES


/**Y threshhold and miscellaneous values. Thess values are used to change the linear distance away from the cart at which it activates**/
#define Y_THRESH_CONSTANT 30
#define CENTER_Y_DIST 300
int MIN_Y_DIST = CENTER_Y_DIST;
#define MIN_Y_DIST_THRESH_LOW (CENTER_Y_DIST + Y_THRESH_CONSTANT) 
#define MIN_Y_DIST_THRESH_HIGH (CENTER_Y_DIST - Y_THRESH_CONSTANT)

//** X threshhold and miscellaneous values. These values are used for tracking the lateral movements of the Wii remote**/ 
//**LEFT or _1 refers to the left IR emiiters WHEN FACING THE CART
//**RIGHT or _2 refers to the right IR emiiters WHEN FACING THE CART
#define X_THRESH_CONSTANT 5 
#define X_1_CENTER 300   //360
#define X_2_CENTER 720
#define X_1_CENTER_THRESH 65
#define X_2_CENTER_THRESH 65

#define X_1_THRESH_LOW (X_1_CENTER - X_1_CENTER_THRESH)
#define X_1_THRESH_HIGH (X_1_CENTER + X_1_CENTER_THRESH)
#define X_2_THRESH_LOW (X_2_CENTER - X_2_CENTER_THRESH)
#define X_2_THRESH_HIGH (X_2_CENTER + X_2_CENTER_THRESH)

#define X_1_OVERTURN 900
#define X_2_OVERTURN 123

#define IR_outofrange_thresh 50

//**BLUE LED 
#define LED_pin_close 3 //pin used for BLUE LED indicating too close
#define DIM 20 //pwm light intensity for BLUE LED indicating cart is too close

//**GREEN LED steady when moving
#define LED_pin_moving 2 //pin used for LED indicating moving forward

//**RED LED blink when no IR in range
#define LED_pin_outofrange 9 //pin used for LED indicating out of range

//**GREEN if WiiMote connected
#define LED_pin_WiiMoteConnected 0


/****Motor Pins****/ 
//**Right refers to cart right
//**Left refers to cart left
#define MOTOR_RIGHT_ENABLE 7
#define MOTOR_RIGHT 6 //PWM pin!!! 
#define MOTOR_LEFT_ENABLE 8
#define MOTOR_LEFT 5 //PWM pin!!!

#define IR_TRANSISTOR_R 1
#define IR_TRANSISTOR_L 4

#define TYPICAL 1 //State variable. not used in this rev
#define X1_LAST_VISIBLE 2  //State variable. not used in this rev
#define X2_LAST_VISIBLE 3  //State variable. not used in this rev

/* x-axis hysteresis threshhold values */
int x1_thresh_low;
int x1_thresh_high;
int x2_thresh_low;
int x2_thresh_high;

int initialize; //initialization variable used to inialize program once
int state; //state variable. not used in this rev

bool printAngle;
bool readIR;
bool debug_1;

/**The following are used to keep track of which side of the cart the Wii remote goes out of range**/
/**Only some of them are used in the current rev**/
bool IR_x2_outofrange;
bool x2_last_visible;
bool IR_outofrange;
bool y_last_visible = false;
bool out_in = false;
bool IR_x2_interrupt;
bool x1_left_last_visible;
bool x2_right_last_visible;
bool IR_x_outofrange;

void setup() {
  //Serial.begin(115200); //pins 0 and 1 control important functions, they will not work if Serial.begin is called
  pinMode(LED_pin_outofrange, OUTPUT);
  pinMode(IR_TRANSISTOR_R, OUTPUT);
  pinMode(IR_TRANSISTOR_L, OUTPUT);
  pinMode(LED_pin_close, OUTPUT);
  pinMode(LED_pin_moving, OUTPUT);
  pinMode(LED_pin_WiiMoteConnected, OUTPUT);
  pinMode(MOTOR_RIGHT_ENABLE, OUTPUT);
  pinMode(MOTOR_RIGHT, OUTPUT);
  pinMode(MOTOR_LEFT_ENABLE, OUTPUT);
  pinMode(MOTOR_LEFT, OUTPUT);
  
  PORTD = 0x00; //set pins 0 - 7 low
  PORTB &= ~0x03; //set pin 8 and 9 low
  
  if (Usb.Init() == -1) {
   // Serial.print(F("\r\nOSC did not start"));
    while(1); //halt
  }
  Serial.print(F("\r\nWiimote Bluetooth Library Started"));
}
void loop() {  
  Usb.Task();
  
  if(Wii.wiimoteConnected) { //function that returns whether Wii remote connects with bluetooth shield
    digitalWrite(LED_pin_WiiMoteConnected, HIGH); //status LED
    
    if(Wii.getButtonClick(HOME)){ // Turns all cart funcionality off and disconnects
      PORTD = 0x00; //set pins 0 - 7 low
      PORTB &= ~0x03; //set pin 8 and 9 low
      digitalWrite(LED_pin_close, LOW); //this won't turn off without this for some reason
      disableIR(); //function that runs IR on/off
      Wii.disconnect(); // Disconnect the Wiimote - it will establish the connection again since the Wiimote automatically reconnects
      }
      
    else {
       
      if(Wii.getButtonClick(ONE)) //initialize IR camera
        Wii.IRinitialize();
      
       if(Wii.getButtonClick(TWO)) //check status request. Returns if IR is intialized or not (Serial Monitor only)
        Wii.statusRequestPublic();      
      
       if(Wii.getButtonClick(A)){ //toggles IR emitters
         digitalWrite(IR_TRANSISTOR_R, (digitalRead(IR_TRANSISTOR_R))^1); 
         delay(100);
         digitalWrite(IR_TRANSISTOR_L, (digitalRead(IR_TRANSISTOR_L))^1);
       }
       
       if(Wii.getButtonClick(B)) { //toggles motor enables, useful for debugging and acts as a quick kill switch
        digitalWrite(MOTOR_RIGHT_ENABLE, (digitalRead(MOTOR_RIGHT_ENABLE))^1);
        digitalWrite(MOTOR_LEFT_ENABLE, (digitalRead(MOTOR_LEFT_ENABLE))^1);
        Serial.println("MOTOR CONTROLLER toggled");
       }
       
       if(Wii.getButtonClick(PLUS)){ //Initializes different things and overall activates the cart
         Wii.setAllOff();
         Wii.setLedOn(LED4);
         readIR = true; //All cart functionality code only runs if readIR = true      
         disableIR();
         enableIR();
         digitalWrite(MOTOR_RIGHT_ENABLE, HIGH);
         digitalWrite(MOTOR_LEFT_ENABLE, HIGH);
         initialize = 0; //used in IR processing below to simply run a section of code once
         state = TYPICAL; //state varialbe not used in current rev
         }   
         
       if(Wii.getButtonClick(MINUS)){ //turns all cart functionality off, but leaves Wii remote connected to Arduino
         Wii.setAllOff();
         Wii.setLedOn(LED1);
         readIR = false;
         PORTD = 0x00; //set pins 0 - 7 low
         PORTB &= ~0x03; //set pin 8 low
         digitalWrite(LED_pin_close, LOW); //For some reason PORTD and PORT B won't turn this one off
         disableIR(); //Port style won't work
       }
       
#ifdef RAWVALUES   //Used for debugging, returns raw values from IR camera (two brightest points: x and y coordinates and size)
       if(Wii.getButtonClick(DOWN)){
         debug_1 = !debug_1;
       }
          if(debug_1){
               initialize = 0;
               Serial.print(F("\r\n y1: "));
               Serial.print(Wii.getIRy1());
               Serial.print(F("\t y2: "));
               Serial.print(Wii.getIRy2());
               Serial.print(F("\t x1: "));
               Serial.print(Wii.getIRx1());
               Serial.print(F("\t x2: "));
               Serial.print(Wii.getIRx2());              
               Serial.print(F("\t s1:"));
               Serial.print(Wii.getIRs1());
               Serial.print(F("\t s2:"));
               Serial.print(Wii.getIRs2());
            }
#endif
               
/****************************************************************/
/****************** IR processing program below *****************/
/****************************************************************/
        /**enter/exit IR loop if PLUS or MINUS button is pressed**/
        /**All code below this (except for functions at the bottom) is inside this if statement)**/
        
        if(readIR){ 
        
              if(initialize == 0){//set the thresh hold to the current wii position the first time through
                  x1_thresh_low = (Wii.getIRx1() - X_THRESH_CONSTANT);
                  x1_thresh_high = (Wii.getIRx1() + X_THRESH_CONSTANT);
                  x2_thresh_low = (Wii.getIRx2() - X_THRESH_CONSTANT);
                  x2_thresh_high = (Wii.getIRx2() + X_THRESH_CONSTANT); 
#ifdef DEBUG
                  Serial.print(F("\r\n x1 initial high threshold: "));
                  Serial.print(x1_thresh_high);
                  Serial.print(F("\t x2 initial low threshold: "));
                  Serial.print(x1_thresh_low);
#endif
                  initialize = 1;
                  /**set out-of-range state variables to false. See variable declaration for explination**/
                  IR_x2_outofrange = false;
                  x2_last_visible = false;
                  IR_outofrange = false;
                  IR_x_outofrange = false;
                  x1_left_last_visible = false;
                  x2_right_last_visible = false;
              }
/*********the following if statements are used to blink the IR LED's when the IR comes back in range*************/

              if(Wii.getIRx2() < 100){ // This tracks if the Wii remote goes out of range to the right of the cart. 100 is the x coordinate
                x2_last_visible = true;
                IR_x2_outofrange = false;
              }
              
              if(out_in){//true if going out of range, false if coming into range
                if((Wii.getIRy1() < 100) || (Wii.getIRy2() < 100)){
                  y_last_visible = true;
                  IR_outofrange = false;
                } 
              } 
/**********************************************************************************************************************/

              /*Turn motor off if IR is out of range*/
              if((Wii.getIRy1() == 1023) && (Wii.getIRy2() == 1023)){    
#ifdef DEBUG
                Serial.println("IR out of range - motor controller turned off");
#endif          
                /*Turn on RED LED if no IR is detected*/
                digitalWrite(LED_pin_moving, LOW);
                digitalWrite(LED_pin_close, LOW);
                digitalWrite(LED_pin_outofrange, HIGH);
                 
                /* Turn off motors */
                digitalWrite(MOTOR_LEFT, LOW);
                digitalWrite(MOTOR_RIGHT,LOW);
                 
                /*Get new threshhold values for when wii comes back into frame*/ 
                x1_thresh_low = (Wii.getIRx1() - X_THRESH_CONSTANT);
                x1_thresh_high = (Wii.getIRx1() + X_THRESH_CONSTANT);
                x2_thresh_low = (Wii.getIRx2() - X_THRESH_CONSTANT);
                x2_thresh_high = (Wii.getIRx2() + X_THRESH_CONSTANT); 
                
                IR_x2_outofrange = true;
                IR_outofrange = true;
                out_in = false;
                IR_x_outofrange = true;
              }
              
              
       /*******If IR in range, proceed********/
              else{               
                digitalWrite(LED_pin_outofrange, LOW);
                
                if(IR_outofrange && y_last_visible){ //true if IR is out of range and it went out of range in the y direction
                  disableIR();
                  enableIR();
                  IR_outofrange = false;
                  y_last_visible = false;
                }

       /******Person is within 3 feet, do not move*******/
                if((Wii.getIRy1() < MIN_Y_DIST) || (Wii.getIRy2() < MIN_Y_DIST)){  
                  
#ifdef DEBUG_Y
                  Serial.print(F("\r\n Person is within 3 feet: y1, y2: "));
                  Serial.print(Wii.getIRy1());
                  Serial.print(F("\t, "));
                  Serial.print(Wii.getIRy2());
#endif
                  /*Turn on BLUE LED if within 3 feet*/
                  digitalWrite(LED_pin_outofrange, LOW); 
                  digitalWrite(LED_pin_moving, LOW); 
                  analogWrite(LED_pin_close, DIM); 
               
                  /* Turn off motors */
                  digitalWrite(MOTOR_LEFT, LOW); 
                  digitalWrite(MOTOR_RIGHT, LOW);
               
                  /*Get new threshhold values*/
                  MIN_Y_DIST = MIN_Y_DIST_THRESH_LOW; 
                  x1_thresh_low = (Wii.getIRx1() - X_THRESH_CONSTANT);
                  x1_thresh_high = (Wii.getIRx1() + X_THRESH_CONSTANT);
                  x2_thresh_low = (Wii.getIRx2() - X_THRESH_CONSTANT);
                  x2_thresh_high = (Wii.getIRx2() + X_THRESH_CONSTANT);       
                }
                                
         /*****If person is farther than 3 feet, move*******/   
                  else if((Wii.getIRy1() > MIN_Y_DIST) || (Wii.getIRy2() > MIN_Y_DIST)){  
                    
                    out_in = true; //keeps track of if the wii remote is going out of range or in range
                    
                    /* Blinks IR LED's if x2 goes out and into range, this is an attempt to establish x_1 IR emitter first in the IR camera*/
                    if(x2_last_visible && IR_x2_outofrange){
                      disableIR();
                      enableIR();
                      x2_last_visible = false;
                      IR_x2_outofrange = false;
                      IR_x2_interrupt = true;
                    }
#ifdef DEBUG_Y
                    Serial.print(F("\r\n Person is furthur than 3 feet: y1, y2: "));
                    Serial.print(Wii.getIRy1());
                    Serial.print(F("\t, "));
                    Serial.print(Wii.getIRy2());
#endif
                    /*Turn on GREEN LED if within 3 feet*/
                    digitalWrite(LED_pin_outofrange, LOW); 
                    digitalWrite(LED_pin_close, LOW);  
                    digitalWrite(LED_pin_moving, HIGH); 
                          
                    MIN_Y_DIST = MIN_Y_DIST_THRESH_HIGH; //get new y threshhold value        
              
            /***if position moves above or below the x thresh hold: proceed***/
                    if(((Wii.getIRx1() > x1_thresh_high) || (Wii.getIRx1() < x1_thresh_low)) || ((Wii.getIRx2() > x2_thresh_high) || (Wii.getIRx2() < x2_thresh_low))){ 
#ifdef EXTRADEBUG_X_Y                
                      Serial.print(F("\r\n y1: "));
                      Serial.print(Wii.getIRy1());
                      Serial.print(F("\t y2: "));
                      Serial.print(Wii.getIRy2());
                      Serial.print(F("\t x1: "));
                      Serial.print(Wii.getIRx1());
                      Serial.print(F("\t x2: "));
                      Serial.print(Wii.getIRx2());
                      Serial.print(F("\t s1: "));
                      Serial.print(Wii.getIRs1());
                      Serial.print(F("\t s2: "));
                      Serial.print(Wii.getIRs2());
#endif                       

                        /*if x1 and x2 is centered turn both motors on at 100% speed*/
                        if((Wii.getIRx1() < X_1_THRESH_HIGH) && (Wii.getIRx2() > X_2_THRESH_LOW)){
                          analogWrite(MOTOR_LEFT, 255);
                          analogWrite(MOTOR_RIGHT, 255);
                          Serial.println("CENTERED");
                        }
                        /*if turning too far right, start turning left to correct it*/
                        else if(Wii.getIRx2() < X_2_OVERTURN){ 
                            analogWrite(MOTOR_RIGHT, 255);
                            analogWrite(MOTOR_LEFT, map(Wii.getIRx2(), 0, X_2_OVERTURN, 0, 115)); //maps the pwm signal from 0 to 115 
                            Serial.println("correcting left");
                            Serial.println(map(Wii.getIRx2(), 0, X_2_OVERTURN, 0, 115));
                        }
                        /*if turning too far left, start turning right to correct it. Note: only this case has to check two values because they range 0-1023 right to left*/
                        else if((Wii.getIRx1() > X_1_OVERTURN) && (Wii.getIRx1() != 1023)){ 
                            analogWrite(MOTOR_LEFT, 255);
                            analogWrite(MOTOR_RIGHT, map(Wii.getIRx1(), X_1_OVERTURN, 1023, 108, 0)); //maps the pwm signal from 0 to 115 
                            Serial.println("correcting right");
                            Serial.println(map(Wii.getIRx1(), X_1_OVERTURN, 1023, 115, 0));
                        }
                        /*turn right*/
                        else if(Wii.getIRx2() < (X_2_THRESH_LOW)){ 
                          analogWrite(MOTOR_LEFT, 255);
                          analogWrite(MOTOR_RIGHT, map(Wii.getIRx2(), 0, X_2_THRESH_LOW, 0, 150)); //maps the pwm signal and slows down the appropriate motor
                          Serial.println("turning RIGHT");
                          Serial.println(map(Wii.getIRx2(), 0, X_2_THRESH_LOW , 0, 150));
                          
                          x2_right_last_visible = true;
                        }
                        /*turn left*/
                        else if(Wii.getIRx1() > (X_1_THRESH_HIGH)){ 
                          analogWrite(MOTOR_RIGHT, 255);
                          analogWrite(MOTOR_LEFT, map(Wii.getIRx1(), X_1_THRESH_HIGH, 1023, 150, 0)); //maps the pwm signal and slows down the appropriate motor
                          Serial.println("turning LEFT");
                          Serial.println(map(Wii.getIRx1(), X_1_THRESH_HIGH, 1023, 150, 0));
                            
                          x1_left_last_visible = true;
                        }
                        
                   
                      x1_thresh_low = (Wii.getIRx1() - X_THRESH_CONSTANT);
                      x1_thresh_high = (Wii.getIRx1() + X_THRESH_CONSTANT);
                      x2_thresh_low = (Wii.getIRx2() - X_THRESH_CONSTANT);
                      x2_thresh_high = (Wii.getIRx2() + X_THRESH_CONSTANT);
                    
                    }
                }
              }
          
        }//closes if(IR) statement
        
      }//else
   }//if wii.connected
}//loop

void enableIR()
{
  delay(200);
  digitalWrite(IR_TRANSISTOR_R, HIGH);
  delay(200);
  digitalWrite(IR_TRANSISTOR_L, HIGH);
}

void disableIR()
{
  digitalWrite(IR_TRANSISTOR_R, LOW);
  digitalWrite(IR_TRANSISTOR_L, LOW);
}


