
//**************************************************************//
//  Name    : LED Quilt Controller, Siggraph                    //
//  Author  : Saman Tehrani                                     //
//  Date    : 17 July, 2015                                     //
//  Version : 1.0                                               //
//  Notes   : Code for using an array of chained 74HC595        //
//          : Shift Register, Shiftout function implemented by  //
//          : Carlyn Maw, Tom Igoe                              //
//          : Retrives data from Xbee radio modules.            //
//**************************************************************//

#include <SoftwareSerial.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

//Xbee is connected to the Arduino on pins [8(TX),9(RX)].
SoftwareSerial XBee(8,9); 

//MATRIX VARIABLES
//-------------------
//CS Pin
int pinCS = 10;
int numberOfHorizontalDisplays = 1;
int numberOfVerticalDisplays = 1;
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
int wait = 200;
const int pinRandom = A0;
const int length = 8;
int x[length], y[length];
int ptr, nextPtr;
String tape = "Parsons";
int spacer = 1;
int width = 7 + spacer; // The font width is 5 pixels
int waitWait = 50;
int waitSnake = 50;

int mode_time = 20;
bool modeStarted = false;
int currentMode = -1;
int timer = 0;
int randomPixel = -1;

// states on data reception
typedef enum {  NONE, ID_FETCHED, MODE_FETCHED} dataReadingStates;
// reception state
dataReadingStates dataReadState;
// data reception temp value
int dataReadingTempValue;
//last Received information
int latestReceivedID = -1;
int latestReceivedMode = -1;
int commandedMode = 0;
// quilt modes

typedef enum{ WAITING_MODE,V_LINE_MODE,H_LINE_MODE,INOUT_MODE,SNAKE_MODE,RANDOM_BLOCK_MODE,BLINK_ALL_MODE,MODES_TAIL} lightingMode;
//Request's Buffer Element Struct
struct request{
    int ID;
    lightingMode mode; 
};

// Request's Buffer Size
#define queueSize 20
// Request's Buffer
request *requestQueue = new request[queueSize];

// Request's Buffer Tail Pos
int queueTail = 0;
int insertedID = -1;


void setup(){
  matrix.setIntensity(15);
  
  dataReadState = NONE;
  dataReadingTempValue = 0;

  
  Serial.begin(9600);
  
  XBee.begin(9600);

  // Reset all variables
  for ( ptr = 0; ptr < length; ptr++ ) {
    x[ptr] = numberOfHorizontalDisplays * 8 / 2;
    y[ptr] = numberOfVerticalDisplays * 8 / 2;
  }
  nextPtr = 0;

  randomSeed(analogRead(pinRandom)); // Initialize random generator

}

void reset(){
  while(XBee.available() > 0)
    XBee.read();
  randomPixel = -1;
  int insertedID = -1;
  modeStarted = false;
  currentMode = -1;
  timer = 0;
  dataReadingTempValue = 0;
  latestReceivedID = -1;
  latestReceivedMode = -1;
  queueTail = 0; 
  matrix.setIntensity(15);
  
  dataReadState = NONE;
  dataReadingTempValue = 0;
  
   for ( ptr = 0; ptr < length; ptr++ ) {
    x[ptr] = numberOfHorizontalDisplays * 8 / 2;
    y[ptr] = numberOfVerticalDisplays * 8 / 2;
  }
  nextPtr = 0;

  randomSeed(analogRead(pinRandom));
}
void acknowledgeHandshaking(){
  XBee.write("HAAAA");
}

bool insertRequestIntoQueue(){
  request tempRequest;
  tempRequest.ID = latestReceivedID; 
  if ( tempRequest.ID == insertedID ){
      return false;
  }
  if(  (lightingMode)latestReceivedMode < MODES_TAIL ){ // mode doesn't exist
    
    tempRequest.mode = (lightingMode)latestReceivedMode;
    commandedMode = (lightingMode)latestReceivedMode;
    informTheRequestToBeRun( commandedMode );
    if( queueTail < queueSize ){
      requestQueue[queueTail++] = tempRequest;
      insertedID = tempRequest.ID;
      return true;
    }
  }

  return false;
}

void confirmRequestReception(){
  char message [5];
  sprintf (message,"I%04d", latestReceivedID);
  XBee.write(message);
  
 while(XBee.available() > 0)
    XBee.read();
}
        
void storeID (const unsigned int value)
{
  latestReceivedID = value;
} 

void storeMode (const unsigned int value)
{
  latestReceivedMode = value;
 
} 

void handlePreviousNumber ()
{
  switch (dataReadState){
  case ID_FETCHED:
    storeID (dataReadingTempValue);
    break;
  case MODE_FETCHED:
    storeMode (dataReadingTempValue);
    break;
  }  
  dataReadingTempValue = 0; 
} 

void readIncomingByte (const byte incomingByte)
{
 
  Serial.println(incomingByte);
  if (isDigit (incomingByte) ){
    
      dataReadingTempValue *= 10;
      dataReadingTempValue += incomingByte - '0';
      
  }else{
      handlePreviousNumber ();
    switch (incomingByte){
    case 'I'://HANDSHAKING
      reset();
      acknowledgeHandshaking();
      break;  
    case 'H':
      if( dataReadState == NONE ){
        dataReadState = ID_FETCHED;
      }else{
        dataReadState = NONE;
      }
      break;
    case 'M':
    
     if( dataReadState == ID_FETCHED ){
        dataReadState = MODE_FETCHED;
      }else{
        dataReadState = NONE;
      }
      break;
    case 'E':
    
      if( dataReadState == MODE_FETCHED ){
        // if request was inserted, queue might be full in which case the request will be dropped!
        if( insertRequestIntoQueue() ){
          
          confirmRequestReception(); 
        }else{
            // alert queue full 
        }
        //reset temp variables
        latestReceivedID = -1;
        latestReceivedMode = -1;
        
      }
      dataReadState = NONE;
      break;
    default:
      dataReadState = NONE;
      break;
    
    }  
  } 
} 

void informTheRequestToBeRun( int ID ){
  char message [5];
  sprintf (message,"U%04d", ID);
  XBee.write(message);
}
void waitingModeFunction(){
   for ( int x = 0; x < matrix.width() - 1; x++ ) {
    matrix.fillScreen(LOW);
    matrix.drawLine(x, 0, matrix.width() - 1 - x, matrix.height() - 1, HIGH);
    matrix.write(); // Send bitmap to display
    delay(waitWait);
  }

  for ( int y = 0; y < matrix.height() - 1; y++ ) {
    matrix.fillScreen(LOW);
    matrix.drawLine(matrix.width() - 1, y, 0, matrix.height() - 1 - y, HIGH);
    matrix.write(); // Send bitmap to display
    delay(waitWait);
  } 
}
void vLineModeFunction(){
  for ( int x = 0; x < matrix.width(); x++ ) {
    matrix.fillScreen(LOW);
    matrix.drawLine(x, 0, x, matrix.height() - 1, HIGH);
    matrix.write(); 
    delay(wait);
  }
}
    
void hLineModeFunction(){
  for ( int x = 0; x < matrix.height() ; x++ ) {
    matrix.fillScreen(LOW);
    matrix.drawLine(0, x,  matrix.height() - 1,x, HIGH);
    matrix.write(); 
    delay(wait);
  }
}
    
void inoutModeFunction(){
  
    matrix.fillScreen(LOW);
    matrix.drawLine(0,0,0,7, HIGH);
    matrix.drawLine(0,7,7,7, HIGH);
    matrix.drawLine(7,7,7,0, HIGH);
    matrix.drawLine(7,0,0,0, HIGH);
    matrix.write();
    delay(wait);

    matrix.fillScreen(LOW);
    matrix.drawLine(1,1,1,6, HIGH);
    matrix.drawLine(1,6,6,6, HIGH);
    matrix.drawLine(6,6,6,1, HIGH);
    matrix.drawLine(6,1,1,1, HIGH);
    matrix.write();
    delay(wait);

    matrix.fillScreen(LOW);
    matrix.drawLine(2,2,2,5, HIGH);
    matrix.drawLine(2,5,5,5, HIGH);
    matrix.drawLine(5,5,5,2, HIGH);
    matrix.drawLine(5,2,2,2, HIGH);
    matrix.write();
    delay(wait);
    
    matrix.fillScreen(LOW);
    matrix.drawLine(3,3,3,4, HIGH);
    matrix.drawLine(3,4,4,4, HIGH);
    matrix.drawLine(4,4,4,3, HIGH);
    matrix.drawLine(4,3,3,3, HIGH);
    matrix.write();
    delay(wait);
    
    matrix.fillScreen(LOW);
    matrix.drawLine(2,2,2,5, HIGH);
    matrix.drawLine(2,5,5,5, HIGH);
    matrix.drawLine(5,5,5,2, HIGH);
    matrix.drawLine(5,2,2,2, HIGH);
    matrix.write();
    delay(wait);
    matrix.fillScreen(LOW);
    matrix.drawLine(1,1,1,6, HIGH);
    matrix.drawLine(1,6,6,6, HIGH);
    matrix.drawLine(6,6,6,1, HIGH);
    matrix.drawLine(6,1,1,1, HIGH);
    matrix.write();
    delay(wait);
}
    
boolean occupied(int ptrA) {
  for ( int ptrB = 0 ; ptrB < length; ptrB++ ) {
    if ( ptrA != ptrB ) {
      if ( equal(ptrA, ptrB) ) {
        return true;
      }
    }
  }

  return false;
}

int next(int ptr) {
  return (ptr + 1) % length;
}

boolean equal(int ptrA, int ptrB) {
  return x[ptrA] == x[ptrB] && y[ptrA] == y[ptrB];
}
void blinkAllFunction(){
    matrix.fillScreen(HIGH);
    matrix.write();
    delay(wait);
    matrix.fillScreen(LOW);
    matrix.write();
    delay(wait);
}
void randomBlockFunction(){
  for( int i=0;i<4 ; i++ ){
  int temp = random(0,64);
  while( temp == randomPixel ){
   temp = random(0,64);
  }
  randomPixel = temp;
  
  matrix.fillScreen(LOW);
  matrix.drawPixel(randomPixel/8,randomPixel%8,HIGH);
  matrix.write();
  delay(wait);
  }
  
}
void  snakeModeFunction(){
  for(int z = 0 ; z< 17 ;z++ ){
   ptr = nextPtr;
  nextPtr = next(ptr);

  matrix.drawPixel(x[ptr], y[ptr], HIGH); // Draw the head of the snake
  matrix.write(); // Send bitmap to display

  delay(waitSnake);

  if ( ! occupied(nextPtr) ) {
    matrix.drawPixel(x[nextPtr], y[nextPtr], LOW); // Remove the tail of the snake
  }

  for ( int attempt = 0; attempt < 10; attempt++ ) {

    // Jump at random one step up, down, left, or right
    switch ( random(4) ) {
    case 0: x[nextPtr] = constrain(x[ptr] + 1, 0, matrix.width() - 1); y[nextPtr] = y[ptr]; break;
    case 1: x[nextPtr] = constrain(x[ptr] - 1, 0, matrix.width() - 1); y[nextPtr] = y[ptr]; break;
    case 2: y[nextPtr] = constrain(y[ptr] + 1, 0, matrix.height() - 1); x[nextPtr] = x[ptr]; break;
    case 3: y[nextPtr] = constrain(y[ptr] - 1, 0, matrix.height() - 1); x[nextPtr] = x[ptr]; break;
    }

    if ( ! occupied(nextPtr) ) {
      break; // The spot is empty, break out the for loop
    }
  }
  }
}
void runMode( int mode ) {
  switch( mode ){
    case (int)WAITING_MODE:
      waitingModeFunction();
    break;
    case (int)V_LINE_MODE:
      vLineModeFunction();
    break;
    case (int)H_LINE_MODE:
      hLineModeFunction();
    break;
    case (int)INOUT_MODE:
      inoutModeFunction();
    break;
    case (int)SNAKE_MODE:
      snakeModeFunction();
    break;
    case (int)RANDOM_BLOCK_MODE:
      randomBlockFunction();
    break;
    case (int)BLINK_ALL_MODE:
      blinkAllFunction();
    break;
    default:
    break;
  }
}
struct request popFromRequestQueue(){
  request temp = requestQueue[0];

  for( int i=0;i<queueTail-1;i++ ){
    requestQueue[i] = requestQueue[i+1];
  }
  queueTail--;

  return temp;
}
void loop(){
  if (XBee.available() > 0)
  {
    readIncomingByte(XBee.read());
  }
  /* runMode() function applied the command instantly,
   *  with no consideration for the buffer.
   */
  runMode(commandedMode);
  
  /* the lower code is for queueing model, 
   * it runs each mode for at least 20 seconds 
   * and pops the next mode if available from queue.
   */
/*if(! modeStarted ){ 
  
    if( queueTail <= 0 ){ // queue is empty
      runMode((int)WAITING_MODE);
    }else{
      
      
      request toBeRun = popFromRequestQueue(); // head of the FIFO queue
      
      currentMode = (int)toBeRun.mode;
      
      modeStarted = true;
      timer=0;
      if( currentMode == SNAKE_MODE  )
        matrix.fillScreen(LOW);

    }
}else{
  if( timer == mode_time ){
    if( queueTail <= 0 )
      informTheRequestToBeRun( 0 ); 
    else
      informTheRequestToBeRun( requestQueue[0].ID ); 
  }
  if ( timer++ > mode_time ){
    modeStarted = false;
    
  }
  runMode(currentMode);
}
 */
    
}














