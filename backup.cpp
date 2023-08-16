#include <Arduino.h>
#include "time.h"

#include <WiFi.h>
// Replace with your network credentials
const char* ssid = "JioFiber-EgRrr";
const char* password = "deva9840raj";
// Set web server port number to 80
WiFiServer server(80);



// Variable to store the HTTP request
String header;
// Auxiliary variables to store the current output state
String output16State = "off";
String output21State = "off";
String output19State = "off";
String output17State = "off";
// Assign output variables to GPIO pins
const int output16 = 16;
const int output21 = 21;
const int output17 = 17;
const int output19 = 19;
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

WiFiClient client;

// void controlOnOFF(String no, int pinNo);



const char* ntpServer = "pool.ntp.org";
long  gmtOffset_sec = 5.5*3600;
int   daylightOffset_sec = 0;

String timeToOn = "20:00";
String timeToOff = "20:01";

String getLocalTime()
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



void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output16, OUTPUT);
  pinMode(output17, OUTPUT);
  pinMode(output21, OUTPUT);
  pinMode(output19, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output16, LOW);
  digitalWrite(output17, LOW);
  digitalWrite(output21, LOW);
  digitalWrite(output19, LOW);
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\noffset: "); Serial.println(daylightOffset_sec);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime();

  timeToOn = splitTimeString(getLocalTime(), 1);
  timeToOff = splitTimeString(getLocalTime(), 2);

  Serial.println("time to on :" + timeToOn);
  Serial.println("time to off :" + timeToOff);


  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void controlOnOFF(String no, int pinNo){
    if (header.indexOf(no+"/off") >= 0) {
        Serial.println(no);
        Serial.println(pinNo);
        if(pinNo == 16){

        output16State = "on";
        }
        else if(pinNo == 21){

        output21State = "on";
        }
        else if(pinNo == 17){

        output17State = "on";
        }
        else if(pinNo == 19){

        output19State = "on";
        }
        digitalWrite(pinNo, LOW);
        Serial.println("on pin no is : " + pinNo);
    } else if (header.indexOf(no+"/on") >= 0) {
        Serial.println(no);
        if(pinNo == 16){

        output16State = "off";
        }
        else if(pinNo == 21){

        output21State = "off";
        }
        else if(pinNo == 17){

        output17State = "off";
        }
        else if(pinNo == 19){

        output19State = "off";
        }
        Serial.println("off pin no is : " + pinNo);
        digitalWrite(pinNo, HIGH);
    }
}

void OnOffUi(int pinNo, String pinstatus){
if (output16State=="off") {
  client.println("<p><a href=\"/16/off\"><button class=\"button button2\">OFF</button></a></p>");
} else {
  client.println("<p><a href=\"/16/on\"><button class=\"button\">ON</button></a></p>");
}
}

void autOnOff(){
 
  if(getLocalTime() == timeToOn && output19State == "off"){
    Serial.println("time to on");
    output19State = "on";
    digitalWrite(19, HIGH);
  }
  if(getLocalTime() == timeToOff && output19State == "on"){
    output19State = "off";
    Serial.println("time to off");
    digitalWrite(19, LOW);
  }
}

void setOnOffTiming(){
  if(header.indexOf("?") > 0){
    int startIndex = header.indexOf("? ") + 1;
    int endIndex = header.indexOf(",");
    timeToOn = header.substring(startIndex, endIndex);
    startIndex = endIndex + 1;
    endIndex = header.indexOf("e");
    timeToOff = header.substring(startIndex, endIndex);
    Serial.println("new on time: " + timeToOn);
    Serial.println("new off time: " + timeToOff);
  }
  else{
    Serial.println("not found : " + header);

  }
}

void loop(){
  delay(100);
  // getLocalTime();
  autOnOff();
  client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();  
                   // read a byte, then
        Serial.write(c);   
        header += c;                 // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            controlOnOFF("GET /16", 16);
            controlOnOFF("GET /21", 21);
            controlOnOFF("GET /19", 19);
            controlOnOFF("GET /17", 17);

            setOnOffTiming();
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");
            // Display current state, and ON/OFF buttons for GPIO 16  
            client.println("<p>GPIO 16 - State " + output16State + "</p>");
            // If the output16State is off, it displays the ON button      
            if (output16State=="off") {
              client.println("<p><a href=\"/16/off\"><button class=\"button button2\">OFF</button></a></p>");
            } else {
              client.println("<p><a href=\"/16/on\"><button class=\"button\">ON</button></a></p>");
            }
            if (output21State=="off") {
              client.println("<p><a href=\"/21/off\"><button class=\"button button2\">OFF</button></a></p>");
            } else {
              client.println("<p><a href=\"/21/on\"><button class=\"button\">ON</button></a></p>");
            }
            if (output19State=="off") {
              client.println("<p><a href=\"/19/off\"><button class=\"button button2\">OFF</button></a></p>");
            } else {
              client.println("<p><a href=\"/19/on\"><button class=\"button\">ON</button></a></p>");
            }
            if (output17State=="off") {
              client.println("<p><a href=\"/17/off\"><button class=\"button button2\">OFF</button></a></p>");
            } else {
              client.println("<p><a href=\"/17/on\"><button class=\"button\">ON</button></a></p>");
            }

            // client.println("<form action='/submit' method='post'>");
            client.println("<label for='text1'>Text 1:</label>");
            client.println("<input type='text' id='text1' name='text1' value='" + timeToOn +"'><br><br>");
            client.println("<label for='text2'>Text 2:</label>");
            client.println("<input type='text' id='text2' name='text2' value='" + timeToOff +"'><br><br>");
            client.println("<button id=\"submit\">submit</button>");
            // client.println("</form>");
            client.println("<script>\nconst textInput1 = document.getElementById('text1');\ntextInput2 = document.getElementById('text2');\nconst submitButton = document.getElementById('submit');\nsubmitButton.addEventListener('click', function() {\nwindow.location.href =\"http://192.168.29.120/?\" + textInput1.value+\",\"+textInput2.value + \"e\";\n});\n</script>");


              

            client.println("</body></html>");
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  

}