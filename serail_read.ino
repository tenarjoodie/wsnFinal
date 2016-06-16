#include <HttpClient.h>
#include <LTask.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <LDateTime.h>

#define WIFI_AP "LGD838"
#define WIFI_PASSWORD "0928459979"
#define WIFI_AUTH LWIFI_WPA  // choose from LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP.

#define DEVICEID "Dm00fpiz" // Input your deviceId
#define DEVICEKEY "aMA3gQnDGn0XNXF5" // Input your deviceKey
#define SITE_URL "api.mediatek.com"
#define per 50
#define per1 3

#define LED_PIN 13

unsigned int rtc;
unsigned int lrtc;
unsigned int rtc1;
unsigned int lrtc1;

char port[4]={0};
char connection_info[21]={0};
char ip[21]={0};             
int portnum;
int val = 0;
String tcpdata = String(DEVICEID) + "," + String(DEVICEKEY) + ",0";
String tcpcmd_led_on = "ledControl,1";
String tcpcmd_led_off = "ledControl,0";
String upload_led;

LWiFiClient c;
LWiFiClient c2;

char lightData[30];
int len = 0;

char lightValue[5];
int lightValue_int = 0;

HttpClient http(c2);

void setup(){

    LTask.begin();
    LWiFi.begin();
    Serial.begin(9600);
    Serial1.begin(115200);

    while(!Serial1){
        Serial1.begin(115200);
        Serial.println("Serial1 not begin");
        delay(1000);
    }    
    
    while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD))){
      Serial.println("Re-Connecting to AP");
      delay(1000);
    }  
    while (!c2.connect(SITE_URL, 80)){
      Serial.println("Re-Connecting to API WebSite");
      delay(1000);
    }
    pinMode(LED_PIN, OUTPUT);
    getconnectInfo();
    connectTCP();
}


void getconnectInfo(){
      //calling RESTful API to get TCP socket connection
      c2.print("GET /mcs/v2/devices/");
      c2.print(DEVICEID);
      c2.println("/connections.csv HTTP/1.1");
      c2.print("Host: ");
      c2.println(SITE_URL);
      c2.print("deviceKey: ");
      c2.println(DEVICEKEY);
      c2.println("Connection: close");
      c2.println();
      
      delay(500);
    
      int errorcount = 0;
      while (!c2.available()){
        Serial.println("waiting HTTP response: ");
        Serial.println("errorcount: "+errorcount);
        errorcount += 1;
        if (errorcount > 10) {
          c2.stop();
          return;
        }
        delay(100);
      }
      int err = http.skipResponseHeaders();
    
      int bodyLen = http.contentLength();
      Serial.print("Content length is: ");
      Serial.println(bodyLen);
      Serial.println();
      char c;
      int ipcount = 0;
      int count = 0;
      int separater = 0;
      while(c2){
          int v = c2.read();
          if (v != -1){
            c = v;
            Serial.print(c);
            connection_info[ipcount]=c;
            if(c==',')
              separater=ipcount;
            ipcount++;    
          }
          else{
            Serial.println("no more content, disconnect");
            c2.stop();    
          }
      }
      Serial.print("The connection info: ");
      Serial.println(connection_info);
      int i;
      for(i=0;i<separater;i++){  
          ip[i]=connection_info[i];
      }
      int j=0;
      separater++;
      for(i=separater;i<21 && j<5;i++){  
         port[j]=connection_info[i];
         j++;
      }
      Serial.println("The TCP Socket connection instructions:");
      Serial.print("IP: ");
      Serial.println(ip);
      Serial.print("Port: ");
      Serial.println(port);
      portnum = atoi (port);
      Serial.println(portnum);

} //getconnectInfo

void connectTCP(){
  //establish TCP connection with TCP Server with designate IP and Port
  c.stop();
  Serial.println("Connecting to TCP");
  Serial.println(ip);
  Serial.println(portnum);
  while (0 == c.connect(ip, portnum)){
    Serial.println("Re-Connecting to TCP");    
    delay(1000);
  }  
  Serial.println("send TCP connect");
  c.println(tcpdata);
  c.println();
  Serial.println("waiting TCP response:");
} //connectTCP

void heartBeat(){
  c.print("Send TCP heartBeat");
  c.print(lightData);
  c.println();
    
} //heartBeat

void uploadstatus(String channelID, int channelValue){
  //calling RESTful API to upload datapoint to MCS to report LED status
  Serial.println("calling connection");
  LWiFiClient c2;  

  while (!c2.connect(SITE_URL, 80)){
    Serial.println("Re-Connecting to WebSite");
    delay(1000);
  }
  delay(100);
  String data = channelID + ",," + channelValue;
  Serial.println("data: "+data);
  int thislength = data.length();
  HttpClient http(c2);
  c2.print("POST /mcs/v2/devices/");
  c2.print(DEVICEID);
  c2.println("/datapoints.csv HTTP/1.1");
  c2.print("Host: ");
  c2.println(SITE_URL);
  c2.print("deviceKey: ");
  c2.println(DEVICEKEY);
  c2.print("Content-Length: ");
  c2.println(thislength);
  c2.println("Content-Type: text/csv");
  c2.println("Connection: close");
  c2.println();
  c2.println(data);
  
  delay(500);

  int errorcount = 0;
  while (!c2.available()){
    Serial.print("waiting HTTP response: ");
    Serial.println("errorcount: "+errorcount);
    errorcount += 1;
    if (errorcount > 10) {
      c2.stop();
      return;
    }
    delay(100);
  }
  int err = http.skipResponseHeaders();

  int bodyLen = http.contentLength();
  Serial.print("Content length is: ");
  Serial.println(bodyLen);
  Serial.println();
  while (c2)
  {
    int v = c2.read();
    if (v != -1){
      Serial.print(char(v));
    }
    else{
      Serial.println("no more content, disconnect");
      c2.stop();

    }
    
  }
}


void loop(){

  String tcpcmd="";
  while (c.available()){
      int v = c.read();
      if (v != -1){
        Serial.print((char)v);
        tcpcmd += (char)v;
        if (tcpcmd.substring(40).equals(tcpcmd_led_on)){
          digitalWrite(13, HIGH);
          Serial.print("Switch LED ON ");
          tcpcmd="";
        }
        else if(tcpcmd.substring(40).equals(tcpcmd_led_off)){  
          digitalWrite(13, LOW);
          Serial.print("Switch LED OFF");
          tcpcmd="";
        }
      }
   }

   int k = 0;
   while(Serial1.available()){    // 接收資料
          char c = Serial1.read();
          
          if( c >= 48 && c <= 57){
              lightValue[k] = c;
              k++;
          }
          else if( c == '\n'){
              lightValue[k] = '\0';
              lightValue_int = atoi(lightValue);
              k = 0;
          }
          
         lightData[len] = c; 
         len++;
         if(c == '\n'){
              lightData[len] = '\0';
              Serial.print(lightData);
              len = 0;
         }
   }
   
   LDateTime.getRtc(&rtc);
   if ((rtc - lrtc) >= per) {
       heartBeat();
       lrtc = rtc;
   }
   //Check for report datapoint status interval
   LDateTime.getRtc(&rtc1);
   if ((rtc1 - lrtc1) >= per1) {
      int ledState = digitalRead(13);
      uploadstatus("ledState", ledState);
      delay(500);
      uploadstatus("lightState", lightValue_int);
      lrtc1 = rtc1;
   }
}
