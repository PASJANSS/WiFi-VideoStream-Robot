#include <WebSocketsClient.h>
#include<WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "esp_camera.h"
#define CAMERA_MODEL_XIAO_ESP32S3
#include "camera_pins.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>

class Camera{
  public:
    unsigned long last = 0;
    int sendDelay;
    WebSocketsClient* ws;
    Camera(){}
    Camera(int _sendDelay, WebSocketsClient* _ws){
      sendDelay = _sendDelay;
      ws = _ws;
    }
    void Start(){
      camera_config_t config;
      config.ledc_channel = LEDC_CHANNEL_0;
      config.ledc_timer = LEDC_TIMER_0;
      config.pin_d0 = Y2_GPIO_NUM;
      config.pin_d1 = Y3_GPIO_NUM;
      config.pin_d2 = Y4_GPIO_NUM;
      config.pin_d3 = Y5_GPIO_NUM;
      config.pin_d4 = Y6_GPIO_NUM;
      config.pin_d5 = Y7_GPIO_NUM;
      config.pin_d6 = Y8_GPIO_NUM;
      config.pin_d7 = Y9_GPIO_NUM;
      config.pin_xclk = XCLK_GPIO_NUM;
      config.pin_pclk = PCLK_GPIO_NUM;
      config.pin_vsync = VSYNC_GPIO_NUM;
      config.pin_href = HREF_GPIO_NUM;
      config.pin_sccb_sda = SIOD_GPIO_NUM;
      config.pin_sccb_scl = SIOC_GPIO_NUM;
      config.pin_pwdn = PWDN_GPIO_NUM;
      config.pin_reset = RESET_GPIO_NUM;
      config.xclk_freq_hz = 20000000;
      config.pixel_format = PIXFORMAT_JPEG;
      config.frame_size = FRAMESIZE_VGA;
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
      config.fb_location = CAMERA_FB_IN_DRAM;

      esp_err_t err = esp_camera_init(&config);
      if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
      }
    }

    void Update(){
      unsigned long now = millis();
      if(now - last < sendDelay)
        return;
      last = now;
      camera_fb_t * fb = esp_camera_fb_get();
      ws->sendBIN(fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
};

class Diode{
  public:
    int pin;
    bool blink = false;
    bool shot = false;
    bool state = false;
    int cycle;
    unsigned long last = 0;
    Diode(){}
    Diode(int _pin){
      pin = _pin;
    }

    void Blink(int _cycle = 500){
      blink = true;
      shot = false;
      state = false;
      last = millis();
      Light(state);
      cycle = _cycle;
    }

    void Shot(int _cycle = 200){
      blink = false;
      shot = true;
      state = true;
      last = millis();
      Light(state);
      cycle = _cycle;
    }

    void Stop(){
      blink = false;
      shot = false;
    }

    void Light(bool _state){
      digitalWrite(pin, _state ? HIGH : LOW);
    }

    void Update(){
      if(blink || shot){
        unsigned long now = millis();
        if(now - last < cycle)
          return;
        last = now;
        state = !state;
        Light(state);
        if(shot){
          shot = false;
        }
      }
    }
};

class Button{
  public:
    int pin;
    unsigned long antiSpam = 5000;
    unsigned long last = -5000;
    Button(){}
    Button(int _pin){
      pin = _pin;
    }

    bool isClicked(){
      unsigned long now = millis();
      if(now - last < antiSpam) 
        return false;
      if(digitalRead(pin) == HIGH)
      {
        last = now;
        return true;
      }
      return false;
    }
};

Preferences prefs;

//filled by user in captive portal
String ssid;
String password;

//external server info for communication
WebSocketsClient ws;
const char* serverIp = "172.20.10.10";
const int serverPort = 8080;
const char* serverPath = "/ws/robot";

const unsigned long wifiTimeout = 10000;

//my own internal server info
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer server(80);

int errDiodePin = D10, commDiodePin = D9;
Diode errDiode, commDiode;

Camera cam;

Button button;
int buttonPin = D8;

void ErrorMessage(String msg){
  Serial.println(msg);
  errDiode.Shot();
}

void ConnectToServer(){
  commDiode.Blink(1000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  bool foundSSID = false;
  unsigned long startLoop = millis();
  //look for ssid inserted by user in captive portal
  do{
    if(millis() - startLoop > wifiTimeout)
      break;
    int n = WiFi.scanNetworks();
    Serial.println(n);
    for(int i = 0; i < n; i++){
      Serial.println(WiFi.SSID(i));
      if(WiFi.SSID(i) == ssid)
        foundSSID = true;
    }
    delay(1000);
  }while(!foundSSID);

  if(!foundSSID){
    CaptivePortal();
    return;
  }

  //connect to the network pointed by user in captive portal
  startLoop = millis();
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    if(millis() - startLoop > wifiTimeout)
      break;
    delay(500);
    Serial.print(".");
  }
  if(WiFi.status() != WL_CONNECTED){
    CaptivePortal();
    return;
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  commDiode.Stop();

  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("password", password);
  prefs.end();

  //connecting to api websocket for camera streaming
  ws.begin(serverIp, serverPort, serverPath);
  ws.onEvent(WsEvent);
}

void handleMainPage(){
  //opening page and streaming it as a response for a get query
  commDiode.Stop();
  Serial.println("Client detected");
  File file = LittleFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleSubmit(){
  //parsing api query's message
  Serial.println("Got submitted network data");
  commDiode.Shot();
  String body = server.arg("plain");
  String response = "";
  JsonDocument docIncoming;
  DeserializationError err = deserializeJson(docIncoming, body);
  if(err){
    response = "Couldn't parse json";
    ErrorMessage(response);
  }
  else{
    ssid = String(docIncoming["ssid"]);
    password = String(docIncoming["networkPassword"]);
    response = "Robot got your network data, connecting to wifi. This server will be closed";
  }

  JsonDocument docResponding;
  docResponding["message"] = response;
  serializeJson(docResponding, response);

  server.send(200, "application/json", response);
  if(err)
    return;

  Serial.println("Trying to connect pointed network");
  server.stop();

  dnsServer.stop();
  WiFi.softAPdisconnect(true);

  ConnectToServer();
}

void CaptivePortal(){
  //opening file system to run index.html for captive portal
  bool err = LittleFS.begin(true);
  if(!err){
    ErrorMessage("Błąd przy otwieraniu plików");
    return;
  }
  Serial.println("File system running");
  WiFi.disconnect();
  //showing own wifi for user to connect to
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Robot Kacpra");
  Serial.println("Showing WiFi");

  commDiode.Blink();

  //setting up a dns server to redirect all connections to esp32's ip and html page
  err = dnsServer.start(53, "*", apIP);
  if(!err){
    ErrorMessage("Couldn't run DNS server. Retrying in 2 sec");
    delay(2000);
    CaptivePortal();
    return;
  }
  Serial.println("DNS server running");

  //starting api that responds for '/' GET query by showing main html page and '/submit' POST query by getting inserted data into a robot
  server.on("/", HTTP_GET, handleMainPage);
  server.on("/submit", HTTP_POST, handleSubmit);
  //dns server redirects all domains into our ip, but not every path in a URL is handled (only / and /submit) so weird paths are also redirected to our main page
  server.onNotFound([]() {
    handleMainPage();
  });
  server.begin();
  Serial.println("Server running");
}

void Move(char key){
  Serial.println(String("got key ") + key);
}

void WsEvent(WStype_t type, uint8_t* payload, size_t length){
  String message = (char*)payload;
  if(type == WStype_TEXT)
    Move(message[0]);
  if(type == WStype_CONNECTED)
    Serial.println("WS connected!");
  if(type == WStype_DISCONNECTED)
    Serial.println("WS disconnected!");
}

void setup(){
  Serial.begin(115200);
  
  //setting up diodes
  errDiode = Diode(errDiodePin);
  commDiode = Diode(commDiodePin);
  
  button = Button(buttonPin);

  //trying to connect to remembered network. if it does not exist then i enable captive portal
  prefs.begin("wifi", true);
  if(prefs.isKey("ssid")){
    ssid = prefs.getString("ssid");
    password = prefs.getString("password");
    prefs.end();
    ConnectToServer();
  }
  else{
    CaptivePortal();
  }

  cam = Camera(33, &ws);
  cam.Start();
}

void loop(){
  dnsServer.processNextRequest();
  server.handleClient();
  ws.loop();

  cam.Update();
  errDiode.Update();
  commDiode.Update();
  if(button.isClicked()){
    CaptivePortal();
  }
}