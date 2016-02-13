#include <TimerOne.h>
#include <SPI.h>
#include <EEPROM.h>
#include <boards.h>
#include <RBL_nRF8001.h>
#include "Boards.h"
#define S0     6   // Pin's define
#define S1     5
#define S2     4
#define S3     3
#define OUT    2 
int lightSensorPin = A0; // light sensor is connected to A0 pin.
int key=0;
int   g_count = 0;    // count the frequecy
int   g_array[3];     // store the RGB value
int   g_flag = 0;     // filter of RGB queue
float g_SF[3];        // save the RGB Scale factor
int lightness = 0;   //record the light when doing White Balance
int currentlightness=0; // record the current light when get the RGB value
float lightChange=0;  // store the scale of light change;
boolean WB=0; // as a lock to identify whether the system is in WB mode or RGB task mode
unsigned char buf[256] = {0};
unsigned char len = 0;
static byte buf_len = 0;
int button=1;
int R=0;
int G=0;
int B=0;


// the setup methord
 void setup()
 {
    TSC_Init();
   Serial.begin(57600);
   pinMode(lightSensorPin, INPUT);
   pinMode(key, INPUT);
   button=digitalRead(key);
   Timer1.initialize();             // defaulte is 1s
   Timer1.attachInterrupt(TSC_Callback); // read from color sensor each second
   attachInterrupt(0, TSC_Count, RISING); 
   ble_begin(); // a methord to get ble ready to work, from library "RBL_nRF8001.h"  
   delay(4000);
   doingWB();
 }
 
 //a methord to send int value to BLE connected device
 void ble_write_int(int value)
{
   int b=value/100%10+48;
   int s=value/10%10+48;
   int g=value%10+48;
   byte values[]= {b,s,g};
   ble_write_string(values,3);
   
}

//a methord to send String value to BLE connected device
void ble_write_String(String string)
{
  int len=string.length();
  //byte[] buf=null;
 
  string.getBytes(buf, len+1);
  ble_write_string(buf,(uint8_t)(len+1));

}

//a methord to send String value in the form of bytes to BLE connected device
void ble_write_string(byte *bytes, uint8_t len)
{
    if (buf_len + len > 256)
  {
    for (int j = 0; j < 15000; j++)
    ble_do_events();
    buf_len = 0;
  }
  
  for (int j = 0; j < len; j++)
  {
    ble_write(bytes[j]);
    buf_len++;
  }
    
  if (buf_len == 256)
  {
    for (int j = 0; j < 15000; j++)
    ble_do_events(); 
    buf_len = 0;
  } 
}

//a methord of doing White Balance for the color sensor
 void doingWB()
 {  
   g_SF[0] = 255.0/ g_array[0];     //R Scale factor
   g_SF[1] = 255.0/ g_array[1] ;    //G Scale factor
   g_SF[2] = 255.0/ g_array[2] ;//B Scale factor   actual job is to WB the sensor.
   //ble_write_String("These are RGB scales: R="+String(g_SF[0]*100)+"; G= "+String(g_SF[1]*100)+"; B= "+String(g_SF[2]*100));
  // ble_do_events();
    ble_write_String("White Balance done");
    ble_do_events();
    WB=0;
    lightness = analogRead(lightSensorPin);// use a int variable light to store the analog input of light sensor  
 }
 
 // a methord to initial the color sensor
 void TSC_Init()
{
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT); 
  digitalWrite(S0, HIGH);// OUTPUT FREQUENCY SCALING 2%
  digitalWrite(S1, HIGH); 
}

// Select the filter color, control the color sensor to select which color's value to input to the Arduino
void TSC_FilterColor(int Level01, int Level02)
{
  if(Level01 != 0)
    Level01 = HIGH;
    
  if(Level02 != 0)
    Level02 = HIGH;
    
  digitalWrite(S2, Level01); 
  digitalWrite(S3, Level02); 
}

// a methord to increase the counter periodly
void TSC_Count()
{
  g_count ++ ;
}

// a methord for doing White Balance
void TSC_Callback()
{
  switch(g_flag)
  {
  case 0:
      Serial.println("->WB Start");
      TSC_WB(LOW, LOW);              //Filter without Red         
      break;
    case 1:
      Serial.print("->Frequency R=");
      Serial.println(g_count);
      g_array[0] = g_count;
      TSC_WB(HIGH, HIGH);            //Filter without Green        
      break;    
    case 2:
      Serial.print("->Frequency G=");        
      Serial.println(g_count);
      g_array[1] = g_count;
      TSC_WB(LOW, HIGH);             //Filter without Blue
      break; 
    case 3:
      Serial.print("->Frequency B=");
      Serial.println(g_count);
      Serial.println("->WB End");
      g_array[2] = g_count;
      TSC_WB(HIGH, LOW);             //Clear(no filter)   
      break;
    default:     
      g_count = 0;        
      break;  
  }
} 

//A methord for White Balance
 void TSC_WB(int Level0, int Level1)  
 {
   g_count = 0;
   g_flag ++;
   TSC_FilterColor(Level0, Level1);
   Timer1.setPeriod(1000000);
 }
 
 void reWB()
 {
  WB=0;
       ble_write_String("Begin to reWB");
       ble_do_events();
       delay(2000); 
       doingWB();
 }
 
 
 // the main function
 void loop()
 {   
    g_flag = 0;
    currentlightness = analogRead(lightSensorPin); 
    lightChange=100*(currentlightness-lightness)/lightness;
    //ble_write_String("******");
    //ble_do_events();
    //ble_write_String("*lightness Information*");
    //ble_do_events(); 
    ble_write_String("The current lightness is: "+String(currentlightness)); 
    ble_do_events();
    ble_write_String(" The initial lightness is: "+String(lightness));
    ble_do_events();
    ble_write_String("Change of environment lightness:" );
    ble_do_events();
    ble_write_String(String(lightChange)+"%");
    ble_do_events();
    
    if(lightChange>1||lightChange<-1)
    {
      WB=1;
     }
     
    delay(2000);
    
    if(WB==0)
    {
    delay(2000);
    WB=0;
    R=int(g_array[0] * g_SF[0]);
    G=int(g_array[1] * g_SF[1]);
    B=int(g_array[2] * g_SF[2]);
    ble_write_String("***These are RGB task***");
    ble_do_events();
    ble_write_String("R= "+String(R));
    ble_do_events();
    ble_write_String("G= "+ String(G));
    ble_do_events();
    ble_write_String("B= "+ String(B));
    ble_do_events();  
    while(R-G>10&&R-B>10)
    {
      ble_write_String("RED");
      ble_do_events();
      break;
    } 
    while(B-R>10&&B-G>10)
    {
      ble_write_String("BLUE");
      ble_do_events();
      break;
    }
    while(G-R>10&&G-B>10)
    {
      ble_write_String("GREEN");
      ble_do_events();
      break;
    } 
    while(G-R<10&&G-B<10&&R-B<10)
    {
      ble_write_String("WHITE");
      ble_do_events();
      break;
    } 
    
  }
    if(WB==1)
    {
    reWB();
    }
    
    
 }
