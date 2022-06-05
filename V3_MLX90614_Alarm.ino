/*********************************************************************
This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

This example is for a 128x64 size display using I2C to communicate
3 pins are required to interface (2 I2C and one reset)

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/
#include <ESP8266WiFi.h>
#include <SimpleTimer.h>
#include <Wire.h>
#include <ESP_Adafruit_SSD1306.h>
#include <SparkFunMLX90614.h> // SparkFunMLX90614 Arduino library
#include <WiFiClientSecure.h>

const char* ssid     = "mars";
const char* password = "1nn0v@t10n";
//const char* ssid     = "neptune";
//const char* password = "1010101010";
//const char* ssid     = "KK04";
//const char* password = "22224444";
//const char* host = "cp-laos.cpf.co.th";
//const String url = "http://cp-laos.cpf.co.th/process/bnp/logger.php";
const char* host = "iotlogger.cpf.co.th";
const String url = "http://iotlogger.cpf.co.th/process/bnp/logger.php";

const int httpPort = 443;
const char* fingerprint = "39 C7 07 19 7A E8 4E 67 6C 8A B1 D7 ED 00 74 11 5F 4E 9C BE";   //*.cpf.co.th

const byte HouseID = 5;

const int SHDN =  10;

int timetoreset = 1440;   //reset all anyway
int Threshold = 90;
#define OLED_RESET 4
#define GREEN_LED 0
#define RED_LED 14
#define SOUND 12

Adafruit_SSD1306 display(OLED_RESET);
int linecount = 0;
SimpleTimer timer;

String HTTPDATA;

IRTherm therm; // Create an IRTherm object to interact with throughout
float temp[20];
float temp_inrange[20];
void setup()   {   

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(SOUND, OUTPUT);

  pinMode(SHDN, OUTPUT);
  digitalWrite(SHDN, LOW);
  
  digitalWrite(GREEN_LED, LOW); 
  digitalWrite(RED_LED, HIGH); 
  digitalWrite(SOUND, LOW); 
  delay(500);
  digitalWrite(GREEN_LED, HIGH); 
  digitalWrite(RED_LED, LOW); 
  digitalWrite(SOUND, LOW); 
  delay(500);
  digitalWrite(GREEN_LED, LOW); 
  digitalWrite(RED_LED, LOW); 
  digitalWrite(SOUND, HIGH); 
  delay(500);

  digitalWrite(GREEN_LED, LOW); 
  digitalWrite(RED_LED, LOW); 
  digitalWrite(SOUND, LOW); 
  
  delay(500);
  
  digitalWrite(SHDN, HIGH);
  delay(2000);// very importan delay for stable vcc
  
  Serial.begin(9600); 
  
  Serial.println("\n**********Reset***********");

  Serial.println("\n**********OLED Initial***********");
  LCDInitial(); 

  Serial.println("\n**********WiFi Initial***********");
  WiFiInitial(); 
  
  delay(1000);

  Serial.println("\n**********Thermal IR Sensor Initial***********");
  therm.begin(); // Initialize thermal IR sensor
  therm.setUnit(TEMP_C); // Set the library's units to Farenheit
  
  Serial.println("\n**********Start***********");
  display.clearDisplay();display.setCursor(0,0);display.display();
  
  timer.setInterval(1000, Timer1Sec);  //time to Reset

  HTTP_Connect(url+"?Initial="+HouseID); 
 
  //ESP.wdtDisable(); //It is not disable function but it like initial function
  ESP.wdtEnable(50000); 
  ESP.wdtDisable(); //It is not disable function but it like initial function

  display.setTextSize(4);
  
}

void loop() { 
  static bool emtycheck= 1;
  ESP.wdtFeed();
 
  timer.run(); 
  

  if (therm.read()) // On success, read() will return 1, on fail 0.
  {
    Serial.print("Temp: " + String(therm.object(), 2));Serial.write('°');Serial.println("C");

          for(int i=0;i<19;i++)
          {
            temp[i]=temp[i+1];
          }
          temp[19] = therm.object();
      
          float temp_max = 0;
          float temp_min = 200;

          for(int i =0;i<=19;i++)
          {
              if(temp[i]>temp_max){temp_max = temp[i];}
              if(temp[i]<temp_min){temp_min = temp[i];}
          }
          if(temp_max<70)
          {
              emtycheck = 1;
          }

          if(temp_min>70 && temp_max>70 && emtycheck ==1)
          {
            emtycheck = 0;
      
              for(int i = 0;i<=19;i++){
              temp_inrange[i] = temp[i];
              }
           }

           temp_min = 200;
           for(int i =17;i<=19;i++)
            {
                if(temp[i]<temp_min){temp_min = temp[i];}
            }
            
            if(temp_min>70 && temp_min<120){
              display.clearDisplay();display.setCursor(0,10);display.print(String(temp_min, 1));display.println('*');display.display();
            }
            else{
              display.clearDisplay();display.setCursor(0,10);display.println(String(temp_min, 1));display.display();
            }

  }
  else{
    while(1){}
  }
  
}


void didplayprintline(String s)
{
  static int linecount = 0;
  
      if(linecount>=8)
      {
        linecount=0;
        display.clearDisplay();
        display.setCursor(0,0);
      }
      
      display.println(s);
      display.display();
      linecount++;
}


void LCDInitial(){
  //display.ssd1306_command(0xA0 | 0x1);
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.setRotation(2);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  Wire.begin(4, 5);
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen. 
  //display.display();
  //delay(500);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.setTextColor(WHITE);
  
}

void WiFiInitial(){
  byte mac[6];
  WiFi.mode(WIFI_STA);
  display.clearDisplay();display.setCursor(0,0); display.println("WiFi");display.print("MAC:");
  Serial.println("");Serial.println("WiFi");Serial.print("MAC:");
  
  WiFi.macAddress(mac);
  for(int i = 0; i < 6; i++)
  {
    Serial.print(mac[i], HEX);
    display.print(mac[i], HEX);
    if(i<5)
    {
      Serial.print(":");
      display.print(":");
    }
  }

  Serial.println();Serial.print("Connecting to ");Serial.println(ssid);
  display.println();display.print("SSID:"); display.println(ssid);display.display();

  WiFi.begin(ssid, password);
  int i = 0;
  if(WiFi.status() != WL_CONNECTED){
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        display.print(".");display.display();
        i++;
        if(i>20){
          break;
          }
      }
    display.println("");
  }

  if(i>20){
  Serial.println("");Serial.println("Connect Fail!");
  display.println(""); display.println("Connect Fail!");display.display();
  }
  else{
  Serial.println("");Serial.println("Connected");Serial.print("IP:");Serial.println(WiFi.localIP());
  display.println(""); display.println("Connected");display.print("IP: ");display.println(WiFi.localIP());display.display();
  }

}


const int transfertimeref = 1;
int transfertime = transfertimeref;
int time1mincount = 0;
bool alarmsts = 0; //0 alarm off , 1=alarm on
void Timer1Sec(void)
{
  time1mincount++;

  if(alarmsts==0){
    Serial.println("Green ON");
    
  digitalWrite(GREEN_LED, HIGH); 
  digitalWrite(RED_LED, LOW); 
  digitalWrite(SOUND, LOW);
  
  }
  else{
    if(time1mincount%2==0){
      Serial.println("Red ON");
      
      digitalWrite(GREEN_LED, LOW); 
      digitalWrite(RED_LED, HIGH); 
      digitalWrite(SOUND, HIGH);
  
    }else{
      Serial.println("Red OFF");
      
      digitalWrite(GREEN_LED, LOW); 
      digitalWrite(RED_LED, LOW); 
      digitalWrite(SOUND, LOW);
    }
  }
  if(time1mincount>=60)
  {
   time1mincount = 0;
   
   Serial.print("*****************Time to reset:");
   Serial.println(timetoreset); 

    if(--timetoreset==0)
    {
      //ESP.restart(); //SW reset
      Serial.print("**********************************Mid night reset********************************");
      while(1){}
    }
    
    if(--transfertime==0)
    {
        transfertime = transfertimeref;

        processdata();
    }
  }
}
void processdata()
{
   if(temp_inrange[0]==0)
   {
      for(int i = 0;i<=19;i++)
      {
          temp_inrange[i] = temp[i];
      }
    }
    
  //float sortValue[20] = {51.73,51.77,51.8,51.89,51.9,51.9,51.84,51.84,51.83,51.86,51.86,51.8,51.8,51.77,51.8,51.89,51.89,51.67,51.73,51.84};
  //float sortValue[20] = {114.86,103.3,103.3,110.12,115.23,107.49,118.27,109.46,109.46,105.46,126.68,108.74,107.61,107.61,134.33,144.08,127.23,133.35,143.33,143.34};
   //float sortValue[20] = {125.77,125.77,113.51,111.74,129.57,113.68,112.29,112.29,127.27,112.24,116.27,117.9,111.36,111.36,126.12,138.61,117.36,112.49,112.49,132.29};
  //float sortValue[20] = {120.27,111.12,110.92,110.92,123.23,110.95,115.4,114.51,109.79,109.79,124.42,112.34,109.92,127.34,136.41,136.41,114.55,110.68,127.95,112.84};
        /*for(int i=0;i<20;i++){
              temp_inrange[i] = sortValue[i];
        }*/

        
        sort(temp_inrange,20);
        for(int i=0;i<20;i++){
            Serial.print(temp_inrange[i]);
            Serial.print(",");
        }

          Serial.println();
          Serial.print("buntemp : ");
          Serial.println(temp_inrange[5]);
          /*
          int data = (int)((temp_inrange[5] + 39.65)*100);
          byte dataL = data & 0xFF;
          byte dataH = (data>>8)&0xFF;
          HTTPDATA="";
          HTTPDATA = Hex2String(HouseID) + Hex2String(0x01) + Hex2String(0x01) + Hex2String(dataH) + Hex2String(dataL);
          Serial.println(HTTPDATA);
          */
          int data = (int)((temp_inrange[0] + 39.65)*100);
          byte dataL = data & 0xFF;
          byte dataH = (data>>8)&0xFF;
          HTTPDATA="";
          HTTPDATA = Hex2String(HouseID) + Hex2String(0x01) + Hex2String(0x01) + Hex2String(dataH) + Hex2String(dataL);
      
          for(int i = 1;i<=19;i++)
          {
          data = (int)((temp_inrange[i] + 39.65)*100);
          dataL = data & 0xFF;
          dataH = (data>>8)&0xFF;
          HTTPDATA = HTTPDATA + Hex2String(dataH) + Hex2String(dataL);
          }
 
          HTTP_Connect(url +"?data=" + HTTPDATA);
          
        if(temp_inrange[5]>=70 && temp_inrange[19]<=150)
        {
          if(temp_inrange[5]>=Threshold){
            alarmsts = 0;
          }
          else{
            alarmsts = 1;
          }
        }

          for(int i = 0;i<=19;i++){
            
                temp_inrange[i] = 0;
          }
  
}
void sort(float a[], int Size){
  for(int i=0; i<(Size-1);i++){
    for(int o=0; o<(Size-(i+1));o++){
      if(a[o]>a[o+1]){
        float t = a[o];
        a[o] = a[o+1];
        a[o+1]=t;
      }
    }
  }
}


String Hex2String(byte s)
{
     if(s <16)
     {
       return("0"+String(s,HEX));
      }
      else{
        return(String(s,HEX));
      }
}
/**************************************************
 *  Function: HTTP_Connect();
 * 
 * Description:
 * 
 * 
/**************************************************/
void HTTP_Connect(String data_str)
{
  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClientSecure client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

  Serial.print("Requesting : ");
  Serial.println(String("GET ") + data_str + "%" + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  
  // This will send the request to the server
  client.print(String("GET ") + data_str + "%" + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(500);
  
  // Read all the lines of the reply from server and print them to Serial
  String line;
  while(client.available()){
    line = client.readStringUntil('\r');
    Serial.print(line);
    checkecho(line);
  }
  Serial.println("closing connection");
}

void checkecho(String line)
{
    if(line.substring(1, 2)=="#")
    { 
      int H = line.substring(2, 4).toInt();
      int M = line.substring(4, 6).toInt();
      int t =  1440-(H*60+M);
      
      if(t>10)
      {
        timetoreset = t;
      }
      
      Serial.println("");
      Serial.print("Time to reset ");Serial.print(timetoreset);Serial.println(" minutes");

      Threshold = line.substring(9, 12).toInt();
      Serial.print("Threshold = "); Serial.println(Threshold);
    }
}



