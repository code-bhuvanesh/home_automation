#include <WiFi.h>
#include <BluetoothSerial.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <time.h>

const char* authkey = "XY9FJBe2bADvNMduSwdzJXVlovzD12Vu";
const char* prefernceName = "myapp";

//time configuration
const char* ntpServer = "pool.ntp.org";
long  gmtOffset_sec = 5.5*3600;
int   daylightOffset_sec = 0;


//  XY9FJBe2bADvNMduSwdzJXVlovzD12Vu/JioFiber-EgRrr\ndeva9840raj
const int noPins = 10; //total no of pins used

BluetoothSerial SerialBT;
Preferences preferences;

bool* isDeviceTurnedOn;
String deviceTurnOnOffTime[noPins]; // on off time splitted by /
// int pins[] = {16,17,18,19,21,22,23,25,26,27, 32, 33};
int pins[] = {16,17,18,19,23,25,26,27, 32, 33};

int ledPin = 21;
bool ledON = false;
int rstBtnPin = 22;


AsyncWebServer server(80);
AsyncWebSocket socket("/ws");


const char* PARAM_INPUT_1 ="output";
const char* PARAM_INPUT_2 = "state";

bool bluetoothConnected = false;
bool WifiConnected = false;
bool bluetoothMode = true;


String readmsgFromBle();
String getPinsStatus();
String boolToString(bool value); 
String deviceTurnOnOffTimeTOJson();

void initializeAllPins();
void sendMsgBle(const String& msg);
void initBluetooth();
void saveWifiDetails();
void connectWifi(String ssid, String password);
void getWifiCreds();
bool checkWifiCredentialsAvailable();
void handleRequest(); // get and post methods
void notifyClients(String *response); // uses websockets
void turnOnOffDevice(int pinIndex, int state);
void autoTurnDevice();
void setDeviceTimes(int pinNo,String startTime, String endTime);
void notifyUserLed();
void resetBoard();




String boolToString(bool value) {
    return value ? "1" : "0";
}



String getTime()
{
  struct tm timeinfo;
  String timeStringFormatted = "";
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return timeStringFormatted;
  }
  // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  // Serial.println(&timeinfo, "%H:%M");

  char timeString[50];
  strftime(timeString, sizeof(timeString), "%H:%M", &timeinfo);
  timeStringFormatted = String(timeString);
  return timeStringFormatted;
}


String splitTimeString(const String &inputTimeString, int extratime) {
  // Find the position of the colon
  int colonPos = inputTimeString.indexOf(':');
  
  // Extract hours and minutes substrings
  String hoursStr = inputTimeString.substring(0, colonPos);
  String minutesStr = inputTimeString.substring(colonPos + 1);
  
  // Convert substrings to integers
  int hours = hoursStr.toInt();
  int minutes = minutesStr.toInt();

  minutes += extratime;

  return String(hours) + ":" + String(minutes);
}


void initializeAllPins() {
    pinMode(ledPin,OUTPUT);
    digitalWrite(ledPin, LOW);
    pinMode(rstBtnPin,INPUT_PULLUP);
    for(int pinIndex = 0; pinIndex < noPins; pinIndex++){
        deviceTurnOnOffTime[pinIndex] = "";
        pinMode(pins[pinIndex],OUTPUT);
        // digitalWrite(pins[pinIndex],LOW);
        digitalWrite(pins[pinIndex],LOW);
    }
    isDeviceTurnedOn = new bool[noPins]();  // Initialize all elements to false
}

void resetBoard(){
    int currentState = digitalRead(rstBtnPin);
    if(currentState == LOW){
        Serial.println("rst btn pressed");
        preferences.begin(prefernceName, false);
        Serial.println("erasing memory......");
        preferences.clear();
        delay(100);
        ESP.restart();
    }
}

void notifyUserLed(){
    // type 1 : in bluetooth paring mode
    // type 2 : bluetooth connected
    // type 3 : connecting to wifi network
    // type 4 : connected to wifi

    if(bluetoothMode){
        if(bluetoothConnected){
            digitalWrite(ledPin, HIGH);
        }
        else{
            digitalWrite(ledPin, HIGH);
            Serial.println("bluetooth not connected : ");
            delay(200);
            digitalWrite(ledPin, LOW);
            delay(200);
        }
    }
    else{
        if(WifiConnected){
            digitalWrite(ledPin, HIGH);
        }
        else{
            digitalWrite(ledPin, HIGH);
            delay(200);
            digitalWrite(ledPin, LOW);
            delay(200);
        }
    }
}

String getPinsStatus(){
    String response = "{";
    // Serial.println("getting response");
    for(int pinIndex = 0; pinIndex < noPins; pinIndex++){
        // Serial.print(String(pinIndex) + ", ");
        response += '"' + String(pinIndex) + '"' + ":" + boolToString(isDeviceTurnedOn[pinIndex]);
        if(pinIndex < noPins -1){
           response += ",";
        }
    }
    response += "}";
    
    Serial.println("response: \n" + response);
    return response;
    
}


String readmsgFromBle() {
    String msg;
    while (SerialBT.available()) {
        char nextChar = SerialBT.read();
        if (nextChar == '\n') break;
        msg += nextChar;
    }
    if(msg != ""){
        Serial.println("msg : " + msg);
    }
    return msg;
}

void sendMsgBle(const String& msg) {
    SerialBT.println(msg);
}


void initBluetooth() {
    bluetoothMode = true;
    SerialBT.register_callback([](esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
        if (event == ESP_SPP_SRV_OPEN_EVT) {
            Serial.println("Client Connected");
            bluetoothConnected = true;
            
        }
    });

    if (!SerialBT.begin("ESP32")) {
        Serial.println("An error occurred initializing Bluetooth");
        Serial.println("Restart the device");
    } else {
        Serial.println("Bluetooth initialized");
    }
}

void saveWifiDetails(String ssid, String password){
    preferences.putBool("wifi_details", true);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    Serial.println("Restarting in device...");
    preferences.putBool("isSoftwareRestart", true);
    preferences.end();
    delay(1000);
    // Restart ESP
    ESP.restart();
}


String deviceTurnOnOffTimeTOJson(){
    String out = "{";
    for(int i = 0; i < noPins; i++){
        out += String(i) + ":" + deviceTurnOnOffTime[i];
        Serial.println("d: " + deviceTurnOnOffTime[i]);
        if(i < noPins -1){
            out += ",";
        }
    }
    out += "}";

    return out;
}


void handleRequest(){
    server.on("/", HTTP_GET, [] (AsyncWebServerRequest *request) {
        Serial.println("called base url");
        request->send(200, "text/plain", "ESP32 home automation");

    });
    server.on("/update", HTTP_POST, [] (AsyncWebServerRequest *request) {
        String inputMessage1;
        String inputMessage2;

        if (request->hasHeader("Authorization")) {
            if(request->header("Authorization") == authkey){
                    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
                if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
                    inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
                    inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
                    digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
                    Serial.print("GPIO: ");
                    Serial.print(inputMessage1);
                    Serial.print(" - Set to: ");
                    Serial.println(inputMessage2);
                    turnOnOffDevice(inputMessage1.toInt(), inputMessage2.toInt());
                    String response = getPinsStatus();
                    notifyClients(&response);
                    request->send(200, "application/json", response);
                }
                else {
                    Serial.println("no params included");
                    request->send(200, "text/plain", "OK");
                }
            }
            else{
                Serial.println("access unauthorized");
                request->send(401, "text/plain", "Unauthorized");
            }

     }

   
  });
  server.on("/updatetime", HTTP_POST, [] (AsyncWebServerRequest *request) {
        String startTime;
        String endTime;
        int pinNo;

        if (request->hasHeader("Authorization")) {
            if(request->header("Authorization") == authkey){
                    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
                if (request->hasParam("starttime") && request->hasParam("endtime") && request->hasParam("pinno")) {
                    startTime = request->getParam("starttime")->value();
                    endTime = request->getParam("endtime")->value();
                    pinNo = request->getParam("pinno")->value().toInt();
                    Serial.print(" pinNo :" + String(pinNo) + " startTime: " + startTime + " endTime: " + endTime);
                    setDeviceTimes(pinNo, startTime, endTime);
                    String response = deviceTurnOnOffTimeTOJson();
                    String deviceStatus = getPinsStatus();
                    notifyClients(&deviceStatus);
                    request->send(200, "application/json", response);
                }
                else {
                    Serial.println("no params included");
                    request->send(200, "text/plain", "OK");
                }
            }
            else{
                Serial.println("access unauthorized");
                request->send(401, "text/plain", "Unauthorized");
            }

     }
  });
   server.begin();
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    String pinDetails = getPinsStatus();

    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            notifyClients(&pinDetails);
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            char* input2 = (char *) data;
            String message = String(input2);
            Serial.println("message: " + message);
        //   handleWebSocketMessage(arg, data, len);
            break;
        // case WS_EVT_PONG:
        // case WS_EVT_ERROR:
        //   break;
    }
}

void initWebSocket() {
  socket.onEvent(onEvent);
  server.addHandler(&socket);
}

void notifyClients(String *response) {
  socket.textAll(response->c_str());
}

void turnOnOffDevice(int pinIndex, int state){
    digitalWrite(pins[pinIndex], state);
    isDeviceTurnedOn[pinIndex] = state;
}

String previousTime = "";

void autoTurnDevice(){
    String currentTime = getTime();
    if(previousTime == currentTime) return;
    previousTime = currentTime; 
    for(int i; i < noPins; i++){
        // Serial.println(deviceTurnOnOffTime[i]);
        if(!deviceTurnOnOffTime[i].isEmpty()){
            int index = deviceTurnOnOffTime[i].indexOf("/");
            String startTime = deviceTurnOnOffTime[i].substring(0,index);
            index++;
            String endTime = deviceTurnOnOffTime[i].substring(index);
            if(startTime == currentTime)
            {
                Serial.println("turing on pin :" + String(i));
                // turnOnOffDevice(i, 0); //realy
                turnOnOffDevice(i, 1); 
                String allDeviceStatus = getPinsStatus();
                notifyClients(&allDeviceStatus);
            }

            if(endTime == currentTime){
                Serial.println("turing off pin :" + String(i));
                // turnOnOffDevice(i, 1); //realy
                turnOnOffDevice(i, 0);
                String allDeviceStatus = getPinsStatus();
                notifyClients(&allDeviceStatus);
            }
        }
    }

   
}

void setDeviceTimes(int pinNo,String startTime, String endTime){
    deviceTurnOnOffTime[pinNo] = startTime + "/" + endTime;
    Serial.println("set" + deviceTurnOnOffTime[pinNo]);
}

void connectWifi(String ssid, String password) {
    bluetoothMode = false; // switch to wifi mode to change the led
    WiFi.begin(ssid, password);

    Serial.println("ssid: " + ssid);
    Serial.println("password: " + password);
    Serial.print("Connecting to WiFi");
    int tryedTimes = 0;
    while (WiFi.status() != WL_CONNECTED) {
        if(tryedTimes > 30){
            break;
        }
        tryedTimes++;
        delay(1000);
        Serial.print(".");
        if (WiFi.status() == WL_CONNECT_FAILED) {
            Serial.println("\nWiFi connection failed: Incorrect password");
            sendMsgBle("WIFI wrong password");
            break;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi");
        Serial.println("server ip: http://" + WiFi.localIP().toString());
        // sendMsgBle("WIFI connected");
        WifiConnected = true;
        handleRequest();
        initWebSocket();
        socket.cleanupClients();
    }
}

void getWifiCreds() {
    String msg = readmsgFromBle();
    if(msg == authkey){
        String ssid = readmsgFromBle();
        String password = readmsgFromBle();
        Serial.println("SSID: " + ssid);
        Serial.println("Password: " + password);
        saveWifiDetails(ssid, password);

        // connectWifi(ssid, password);
    }
    else if(msg != ""){
        // Serial.println("msg : " + msg);
        Serial.println("wrong credentials");
    }
   
}



bool checkWifiCredentialsAvailable() {
    return preferences.getBool("wifi_details");
}

void setup() {
    Serial.begin(115200);
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println();
    initializeAllPins();
    preferences = Preferences();
    preferences.begin(prefernceName, false);

    
    //// for testing only

    // if(!preferences.getBool("isSoftwareRestart")){
    //     Serial.println("erasing memory......");
    //     preferences.clear();
    // }

    if (!checkWifiCredentialsAvailable()) {
        initBluetooth();
        while(!checkWifiCredentialsAvailable()) {
            // sendMsgBle("WIFI");
            notifyUserLed();
            getWifiCreds();
        }
    } else {
        String ssid = preferences.getString("ssid");
        String password = preferences.getString("password");
        connectWifi(ssid, password);
    }

    initializeAllPins();

    Serial.println( getPinsStatus());
    Serial.println("setup completed");
    Serial.println(getTime());


}


void loop() {
    autoTurnDevice();
    notifyUserLed();
    resetBoard();
    // delay(100);

}