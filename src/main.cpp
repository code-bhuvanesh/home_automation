#include "WiFi.h"
#include "BluetoothSerial.h"

// Replace with your network credentials
const char* ssid = "JioFiber-EgRrr";
const char* password = "deva9840raj";

const char* apikey = "XY9FJBe2bADvNMduSwdzJXVlovzD12Vu";


BluetoothSerial SerialBT;


const int pinNos[] = {16, 17,19, 21};
bool* isDeviceTurnedOn;

//bluetooth mac address for esp32 : C8:F0:9E:A6:C4:66

void initIsDeviceTurnedOn(){
    // i size should match length of pinNos
    isDeviceTurnedOn = new bool[4];
    for(int i = 0; i < 4; i++){
        isDeviceTurnedOn[i] = false;
    }
}

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
  if(event == ESP_SPP_SRV_OPEN_EVT){
    Serial.println("Client Connected");
  }
}


String readmsgFromBle(){
    //read msg from bluetooth
    String msg;
    while (SerialBT.available())
    {
        char nextChar = SerialBT.read();
        if(nextChar == '\n') break;
        msg += nextChar;
    }
    String msgApikey = msg.substring(0,msg.indexOf("/"));
    if(msgApikey == apikey){
        return msg.substring(msg.indexOf("/") + 1);
    }
    return "connot send wifi credentials";
}

void sendMsgBle(String msg){
    SerialBT.println(msg);
}

void initBluetooth(){
    SerialBT.register_callback(callback);
 
    if(!SerialBT.begin("ESP32")){
        Serial.println("An error occurred initializing Bluetooth");
        Serial.println("Restart the device");
    }else{
        Serial.println("Bluetooth initialized");
    }
}

bool bluetothConnected = false;


void getWifiCreds(){
    String msg = readmsgFromBle(); 
    if(msg != "connot send wifi credentials"){
        ssid = msg.substring(0,msg.indexOf("\n")).c_str(); 
        Serial.println("ssid : " + String(ssid));
        password = msg.substring(msg.indexOf("\n") + 1).c_str();
        Serial.println("ssid : " + String(msg));
    }
}


void connectWifi(){
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
        if (WiFi.status() == WL_CONNECT_FAILED) {
            Serial.println("\nWiFi connection failed: Incorrect password");
            sendMsgBle("WIFI wrong password");
            break; // Exit the loop if incorrect password
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi");
        sendMsgBle("WIFI connected");
    }
}



bool checkWifiCredentialsAvailable(){
    return false;
}



void setup(){
    Serial.begin(115200);
    initIsDeviceTurnedOn();

    if(checkWifiCredentialsAvailable()){
        connectWifi();
    }
    else{
        initBluetooth();
    }

}


void loop(){
    if(!checkWifiCredentialsAvailable()){
        sendMsgBle("WIFI");
        getWifiCreds();
        connectWifi();
    }
    delay(20);
}