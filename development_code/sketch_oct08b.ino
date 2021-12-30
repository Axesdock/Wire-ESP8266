#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoJson.h>

#include <WebSocketsClient.h>

#include <Hash.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

String mac = WiFi.macAddress();
int portmap[5]={0,5,4,14,12};

const char* ssid     = "ESP8266-Access-Point";
const char* password = "123456789";
StaticJsonDocument<200> doc;


  // Deserialize the JSON document
 

  void control(String operation, int port )
  {
    if(operation=="OFF"){
      digitalWrite(portmap[port],0);
      
    }
    else{
      Serial.print("Operation is on to port"+port);
      digitalWrite(portmap[port],1);
    }
  }

void connection(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED: {
      USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);

      // send message to server when Connected
      webSocket.sendTXT("{\"event\":\"connect\",\"mac\":\""+mac+"\"}");
    }
      break;
    case WStype_TEXT:{
      USE_SERIAL.printf("[WSc] get text: %s\n", payload);
      deserializeJson(doc, payload);
      String event=doc["event"];
      String operation=doc["operation"];
      int port=doc["port"];
      if(event=="control"){
        control(operation,port);
        
      }
        
      

      // send message to server
//       webSocket.sendTXT("message here");
    }
      break;
    
    case WStype_BIN:
      USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);

      // send data to server
//       webSocket.sendBIN(payload, length);
      break;
        case WStype_PING:
            // pong will be send automatically
            USE_SERIAL.printf("[WSc] get ping\n");
            break;
        case WStype_PONG:
            // answer to a ping we send
            USE_SERIAL.printf("[WSc] get pong\n");
            break;
    }

}


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);



const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP8266 Server</h2>
  <p>
    <span class="dht-labels">SSID</span> 
    <span id="ssid">%SSID%</span>
    
  </p>
  <p>
    <span class="dht-labels">Password</span>
    <span id="password">%PASSWORD%</span>
  </p>
</body>

</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "SSID"){
    return ssid;
  }
  else if(var == "PASSWORD"){
    return password;
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(portmap[1],OUTPUT);
  pinMode(portmap[2],OUTPUT);
  pinMode(portmap[3],OUTPUT);
  pinMode(portmap[4],OUTPUT);
 
  
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Start server
  server.begin();




//  STA


  WiFiMulti.addAP("Abhishek", "abhi1234");

  //WiFi.disconnect();
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }
 Serial.print("DONE");

  // server address, port and URL
  webSocket.begin("192.168.94.114", 8011, "/");

  // event handler
  webSocket.onEvent(connection);

  // use HTTP Basic Authorization this is optional remove if not needed
//  webSocket.setAuthorization("user", "Password");

  // try ever 5000 again if connection has failed
  webSocket.setReconnectInterval(5000);
  
  // start heartbeat (optional)
}
 
void loop(){  
  webSocket.loop();

}
