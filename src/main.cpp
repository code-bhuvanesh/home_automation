#include <WiFi.h>
#include <BluetoothSerial.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* authkey = "XY9FJBe2bADvNMduSwdzJXVlovzD12Vu";
const char* prefernceName = "myapp";

//  XY9FJBe2bADvNMduSwdzJXVlovzD12Vu/JioFiber-EgRrr\ndeva9840raj

BluetoothSerial SerialBT;
Preferences preferences;

bool* isDeviceTurnedOn;
int pins[] = {16,17,18,19,21,22,23,25,26,27,32,33};
int noPins = 12; //total no of pins used


AsyncWebServer server(80);

const char* PARAM_INPUT_1 ="output";
const char* PARAM_INPUT_2 = "state";

String readmsgFromBle();
String getPinsStatus();

void initializeAllPins();
void sendMsgBle(const String& msg);
void initBluetooth();
void saveWifiDetails();
void connectWifi(String ssid, String password);
void getWifiCreds();
bool checkWifiCredentialsAvailable();
void handleRequest();
void turnOnOffDevice(int pinIndex, int state);
String boolToString(bool value); 

String boolToString(bool value) {
    return value ? "1" : "0";
}


void initializeAllPins() {
    for(int pinIndex = 0; pinIndex < noPins; pinIndex++){
        pinMode(pins[pinIndex],OUTPUT);
        digitalWrite(pins[pinIndex],LOW);
    }
    isDeviceTurnedOn = new bool[noPins]();  // Initialize all elements to false
}

String getPinsStatus(){
    String response = "{";
    // Serial.println("getting response");
    for(int pinIndex = 0; pinIndex < noPins; pinIndex++){
        // Serial.print(String(pinIndex) + ", ");
        response += String(pinIndex) + ":" + boolToString(isDeviceTurnedOn[pinIndex]) + ",";
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
    SerialBT.register_callback([](esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
        if (event == ESP_SPP_SRV_OPEN_EVT) {
            Serial.println("Client Connected");
            
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
                    request->send(200, "application/json", response);
                }
                else {
                    inputMessage1 = "No message sent";
                    inputMessage2 = "No message sent";
                    request->send(200, "text/plain", "OK");
                }
                
            }
            else{
                request->send(401, "text/plain", "Unauthorized");
            }

     }

   
  });
   server.begin();
}


void turnOnOffDevice(int pinIndex, int state){
    digitalWrite(pins[pinIndex], state);
    isDeviceTurnedOn[pinIndex] = state;
}

void connectWifi(String ssid, String password) {
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
        
        handleRequest();
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
            getWifiCreds();
        }
    } else {
        String ssid = preferences.getString("ssid");
        String password = preferences.getString("password");
        connectWifi(ssid, password);
    }

    initializeAllPins();

    Serial.println(getPinsStatus());

    Serial.println("setup completed");

}


void loop() {
    // manageUserRequest.handleRequest();
    delay(20);

}