#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <Wire.h>
#include <Adafruit_INA219.h>


TinyGPSPlus gps; 
//SoftwareSerial ss(4, 5); 
SoftwareSerial ss(2, 0); 

WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "";  //
const char* password = ""; //

const char* mqtt_broker = "34.87.147.230";
const int mqtt_port = 1883;
const char* mqtt_user = "lohp";
const char* mqtt_password = "9oL.0p;/";

float latitude , longitude;
int year , month , date, hour , minute , second;
String date_str , time_str , lat_str , lng_str, sep;
int pm;

String clientId = "MQTT_Cagayan_de_Oro"; // enter unique Id at least 8 characters long and no space...

String chan = "lohp_esp_9oL.0p;/";

//payload
String sketch = "1.0.1_dev";
String st = " ";
String la = " ";
String lo = " ";
String da = " ";
String ti = " ";

float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float loadvoltage = 0;

Adafruit_INA219 ina219;

void setupWifi() {
  delay(10);
  Serial.println();
  Wire.begin(4, 5);
  uint32_t currentFrequency;
  ina219.begin();
 
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  // Print the IP address
  Serial.println(WiFi.localIP());
  sep = "<><><><>";
}


void connectToBroker() {
   while (!client.connected()) {
        Serial.println("Connecting to mqtt broker.....");
        if (client.connect(clientId.c_str(),mqtt_user, mqtt_password)) {
            Serial.println("MQTT broker connected.");
        } else {
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }
}

void readings(){
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  Serial.println("");
}

void setup()
{
  Serial.begin(115200);
  ss.begin(9600);
  setupWifi();
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  

}

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}


String formatDate(int date, int month, int year){
        String d;
        
        if (date < 10)
          d = '0';
        d += String(date);

        d += " / ";

        if (month < 10)
          d += '0';
        d += String(month);

        d += " / ";

        if (year < 10)
          d += '0';
        d += String(year);

        return d;
}

String formatTime(int h, int m, int s){
        String t;
        int p;
        m = (m + 60);
        if (m > 59)
        {
          m = m - 60;
          h = h + 1;
        }
        h = (h + 7) ;
        if (h > 23)
          h = h - 24;

        if (h >= 12)
          p = 1;
        else
          p = 0;

        h = h % 12;

        if (h < 10)
          t = '0';
        t += String(h);

        t += " : ";

        if (m < 10)
          t += '0';
        t += String(m);

        t += " : ";

        if (s < 10)
          t += '0';
        t += String(s);

        if (p == 1)
          t += " PM ";
        else
          t += " AM ";

  return t;
}


void loop()
{  
  readings();
  delay(1000);
  
  if(!client.connected()){
    Serial.println("Not connected to MQTT broker.Connecting...");
    connectToBroker();
  }
  client.loop();
  
  if(ss.available() <= 0) {
    Serial.println("No SoftwareSerial data available."); 
    
    st = "SoftWare Serial data not available.";
    
  }

  while (ss.available() > 0)
    if (gps.encode(ss.read()))
    {
      st = "Software serial data available.";
      if (gps.location.isValid())
      {
        latitude = gps.location.lat();
        
        lat_str = String(latitude , 6);
        
        longitude = gps.location.lng();
        
        lng_str = String(longitude , 6);

        Serial.println(lat_str);
        Serial.println(lng_str);
 
        la = lat_str;
        lo = lng_str;
  
        delay(100);
        
      }

      if (gps.date.isValid())
      {
        date_str = "";
        date = gps.date.day();
        month = gps.date.month();
        year = gps.date.year();

        date_str = formatDate(date, month, year);
      
        da = date_str;
        
      }

      if (gps.time.isValid())
      {
        time_str = "";
        hour = gps.time.hour();
        minute = gps.time.minute();
        second = gps.time.second();

        time_str = formatTime(hour, minute, second);
   
        ti = time_str;
       
      
      }
    } 
  Serial.println("Keep - alive."); 
  String ka = "keepalive" + sep + clientId + sep + "alive";
  client.publish(chan.c_str(), ka.c_str());
  delay(300);

  String payload = "payload" + sep + clientId + sep + sketch + sep + st + sep + la + sep + lo + sep + da + sep + ti; 
  client.publish(chan.c_str(), payload.c_str());
  delay(2000);

}
