#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

#define USE_SERIAL Serial

const char* ssid     = "axesdock";
const char* password = "123456789";

int portmap[5]={0,14,12,14,12};
int port_status[5] = {0,0,0,0,0};
int n_total_ports = 5;
int active_ports[4]={1,2,3,4};
int n_active_ports = 4;

StaticJsonDocument<200> json_obj;

String mac = WiFi.macAddress();

AsyncWebServer server(80);
WebSocketsClient webSocket;

bool ws_status = 0;
bool wifi_status = 0;
int wifi_status_code = -100;
String wifi_ip = "NC";

String read_from_eeprom(String data){
  String out;
  if(data == "SSID"){
    for (int i = 0; i < 32; ++i)
    {
      out += char(EEPROM.read(i));
    }
  }
  if(data =="PASSWORD"){
    for (int i = 32; i < 64; ++i)
    {
      out += char(EEPROM.read(i));
    }
  }
  if(data == "WS_URL"){
    for (int i = 64; i < 96; ++i)
    {
      out += char(EEPROM.read(i));
    }
  }
  if(data == "WS_PORT"){
    for (int i = 96; i < 104; ++i)
    {
      out += char(EEPROM.read(i));
    }
  }
    if(data == "OWNER_ID"){
    for (int i = 104; i < 138; ++i)
    {
      out += char(EEPROM.read(i));
    }
  }
  Serial.print("Reading :"+data+" from EEPROM with value = "+out+"\n\n\n");
  return out;
}

void write_to_eeprom(String attribute, String data)
{
  if(attribute == "SSID"){
    // Flush SSID with 0
    for (int i = 0; i < 32; i++)
      {
        EEPROM.write(i, 0);
      }
    // Write data
    for (int i = 0; i < data.length(); i++)
      {
        EEPROM.write(i, data[i]);
      }
  }

  if(attribute == "PASSWORD"){
    // Flush with 0
    for (int i = 0; i < 32; ++i)
      {
        EEPROM.write(32+i, 0);
      }
    // Write data
    for (int i = 0; i < data.length(); ++i)
      {
        EEPROM.write(32+i, data[i]);
      }
      Serial.print("password: "+data+"|\n\n\n");
  }

  if(attribute == "WS_URL"){
    // Flush with 0
    for (int i = 0; i < 32; ++i)
      {
        EEPROM.write(64+i, 0);
      }
    // Write data
    for (int i = 0; i < data.length(); ++i)
      {
        EEPROM.write(64+i, data[i]);
      }
  }

    if(attribute == "WS_PORT"){
    // Flush with 0
    for (int i = 0; i < 8; ++i)
      {
        EEPROM.write(96+i, 0);
      }
    // Write data
    for (int i = 0; i < data.length(); ++i)
      {
        EEPROM.write(96+i, data[i]);
      }
  }

  if(attribute == "OWNER_ID"){
    // Flush with 0
    for (int i = 0; i < 32; ++i)
      {
        EEPROM.write(104+i, 0);
      }
    // Write data
    for (int i = 0; i < data.length(); ++i)
      {
        EEPROM.write(104+i, data[i]);
      }
  }


  EEPROM.commit();
}

void control_event(String operation, int port )
  {
    if(operation=="OFF"){
      digitalWrite(portmap[port],0);
      port_status[port] = 0;
      Serial.printf("\n Turing OFF %d \n\n", port);
    }
    else{
      Serial.printf("\n Turning ON %d \n\n ",port);
      digitalWrite(portmap[port],1);
      port_status[port] = 1;
    }
  }

String owner_id;

void connection(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      ws_status = 0;
      break;
    case WStype_CONNECTED: {
      ws_status = 1;
      USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);
      String s = "{\"type\":\"CONNECTION\",\"mac\":\""+mac+"\",\"org\":\""+String(owner_id.c_str())+"\"}";
      Serial.println(s);
      webSocket.sendTXT(s);
    }
      break;
    case WStype_TEXT:{
      USE_SERIAL.printf("[WSc] get text: %s\n", payload);
      deserializeJson(json_obj, payload);
      String event=json_obj["event"];
      String operation=json_obj["operation"];
      int port=json_obj["port"];
      if(event=="CONTROL"){
        control_event(operation,port);
      }
    }
      break;
    }

}

String prep_json(String attribute, String data)
{
  return "\""+attribute+"\":\""+data+"\"";
}

String prep_json(String attribute, int* data, int len)
{
    String s = "";
    for(int i=0;i<len-1;i++){
        s = s + String(data[i])  + ",";
    }
    s = s+String(data[len-1]);
  return "\""+attribute+"\":["+s+"]";
}

String ssid_sta_mode;
String password_sta_mode;
String e_ws_url;
String e_ws_port;

void setup() {
  Serial.begin(115200);
  Serial.print("\n Init of Setup \n\n\n\n\n\n\n");
  EEPROM.begin(512); //Initialasing EEPROM
  delay(1000);
  Serial.print(mac);

  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(12, OUTPUT);
  
  ssid_sta_mode = read_from_eeprom("SSID");
  password_sta_mode = read_from_eeprom("PASSWORD");
  e_ws_url = read_from_eeprom("WS_URL");
  e_ws_port = read_from_eeprom("WS_PORT");
  owner_id =read_from_eeprom("OWNER_ID");

  
  //-----------------------------------Start of AP Mode------------------------------------------------------------------------
  Serial.print("\n Starting AP mode with SSID= "+String(ssid)+" and password= "+String(password)+" .\n\n\n");
  Serial.print(ssid);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.print("\n\n\n\n\n");


  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String content = "{";
    content = content + prep_json("ssid",String(ssid_sta_mode.c_str())) +",";
    content = content + prep_json("password",String(password_sta_mode.c_str())) +",";
    content = content + prep_json("ws_url",String(e_ws_url.c_str())) +",";
    content = content + prep_json("ws_port",String(e_ws_port.c_str())) +",";
    content = content + prep_json("wifi_ip",wifi_ip) +",";
    content = content + prep_json("owner_id",String(owner_id.c_str())) +",";
    content = content + prep_json("mac",mac) +",";
    content = content + prep_json("ws_connect",String(ws_status)) +",";
    content = content + prep_json("port_status",port_status, n_total_ports) +",";
     content = content + prep_json("active_ports",active_ports, n_active_ports) +",";
    content = content + prep_json("wifi_status_code",String(wifi_status_code)) +",";
    content = content + prep_json("wifi_connect",String(wifi_status)) +"}";
    request->send(200, "application/json",content);
  });

  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){

    Serial.println("Request");
    
    if (request->hasParam("ssid")) {
        String qssid = request->getParam("ssid")->value();
        write_to_eeprom("SSID",qssid);
    }

    if (request->hasParam("password")) {
        String qpassword = request->getParam("password")->value();
        write_to_eeprom("PASSWORD",qpassword);
    }

    if (request->hasParam("open")){
      String qopen = request->getParam("open")->value();
      write_to_eeprom("PASSWORD","");
      Serial.print("Open WiFi");
    }

    if (request->hasParam("wsurl")) {
        String qwsurl = request->getParam("wsurl")->value();
        write_to_eeprom("WS_URL",qwsurl);
    }

    if (request->hasParam("wsport")) {
        String qwsport = request->getParam("wsport")->value();
        write_to_eeprom("WS_PORT",qwsport);
    }

        if (request->hasParam("ownerid")) {
        String qownerid = request->getParam("ownerid")->value();
        write_to_eeprom("OWNER_ID",qownerid);
    }
    
    request->send(200, "application/json","{\"status\":200}");
  });

  server.on("/control", HTTP_GET, [](AsyncWebServerRequest *request){
    
    if (request->hasParam("port")) {
        String port = request->getParam("port")->value();
        String control = request->getParam("control")->value();
        if(control.toInt() == 0){
          control_event("OFF",port.toInt());
        }else{
          control_event("ON",port.toInt());
        }
      Serial.print("Control : "+port+" "+control+"\n\n\n");
        
    }

   
    request->send(200, "application/json","{\"status\":200}");
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json","{\"status\":200}");
    delay(1000);
    ESP.reset();
  });

  
   DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");   
  server.begin();
//-----------------------------------End of AP Mode------------------------------------------------------------------------




//-----------------------------------Start of STA Mode------------------------------------------------------------------------

  
  //WiFi.begin(ssid_sta_mode.c_str(), password_sta_mode.c_str());             // Connect to the network
  WiFi.begin(ssid_sta_mode.c_str(), password_sta_mode.c_str()); 
  Serial.print("Connecting to "+ssid_sta_mode+" password: "+password_sta_mode+"\n\n\n");
  Serial.print(ssid_sta_mode.c_str());
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print('@');
  }
  wifi_status = 1;
  wifi_status_code = WiFi.status();
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  wifi_ip = WiFi.localIP().toString();

  webSocket.begin(e_ws_url, e_ws_port.toInt(), "/");
  webSocket.onEvent(connection);
  webSocket.setReconnectInterval(5000);
}

//-----------------------------------End of STA Mode------------------------------------------------------------------------

void loop() {
  webSocket.loop();
  if(WiFi.status() != WL_CONNECTED){
    wifi_status=0;
    wifi_status_code = WiFi.status();
  }
}
