#include <TridentTD_LineNotify.h>
#include <HTTPClient.h>
// #define SSID        "true_home2G_4ab"                                     //ใส่ ชื่อ Wifi ที่จะเชื่อมต่อ
// #define PASSWORD    "245yf4ef"                                   //ใส่ รหัส Wifi
#define SSID        "Mm"                                     //ใส่ ชื่อ Wifi ที่จะเชื่อมต่อ
#define PASSWORD    "mmimmiii" 
#define LINE_TOKEN  "ZeCgGyo2lKFC89DTmjlXQg2MBV2NIX9Q2jF5uR8Vz95" //ใส่ รหัส TOKEN ที่ได้มาจากข้างบน

int tem = 0;
int hum = 0;
int ledPin = 13;
int analogPin = 4; //ประกาศตัวแปร ให้ analogPin แทนขา analog ขาที่4 
int val = 0;
// String servername ="http://192.168.2.36:8000/main-board/";
// String servername ="http://10.70.3.77:8000/main-board/";
String servername ="http://172.20.10.3:8000/main-board/";

#include "DHTesp.h" // Click here to get the library: http://librarymanager/All#DHTesp
#include <Ticker.h>

#ifndef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP32 ONLY!)
#error Select ESP32 board.
#endif

DHTesp dht;
void tempTask(void *pvParameters);
bool getTemperature();
void triggerGetTemp();
void va();

TaskHandle_t tempTaskHandle = NULL;
Ticker tempTicker;
ComfortState cf;

bool tasksEnabled = false;
int dhtPin = 23;

bool initTemp() {
  byte resultValue = 0;
  // Initialize temperature sensor
  dht.setup(dhtPin, DHTesp::DHT11);
  Serial.println("DHT initiated");

  // Start task to get temperature
  xTaskCreatePinnedToCore(
    tempTask,                       /* Function to implement the task */
    "tempTask ",                    /* Name of the task */
    4000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    5,                              /* Priority of the task */
    &tempTaskHandle,                /* Task handle. */
    1);                             /* Core where the task should run */

  if (tempTaskHandle == NULL) {
    Serial.println("Failed to start task for temperature update");
    return false;
  } else {
    // Start update of environment data every 20 seconds
    tempTicker.attach(1, triggerGetTemp);
  }
  return true;
}
void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}
void handleNotFound() {

  String message = "File Not Found\n\n";

  message += "URI: ";

  message += server.uri();

  message += "\nMethod: ";

  message += (server.method() == HTTP_GET) ? "GET" : "POST";

  message += "\nArguments: ";

  message += server.args();

  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {

    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);


}


void triggerGetTemp() {
  if (tempTaskHandle != NULL) {
    xTaskResumeFromISR(tempTaskHandle);
  }
}

void tempTask(void *pvParameters) {
  Serial.println("tempTask loop started");
  while (1) // tempTask loop
  {
    if (tasksEnabled) {
      // Get temperature values
      getTemperature();
    }
    // Got sleep again
    vTaskSuspend(NULL);
  }
}

bool getTemperature() {
  // Reading temperature for humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  TempAndHumidity newValues = dht.getTempAndHumidity();
  // Check if any reads failed and exit early (to try again).
  if (dht.getStatus() != 0) {
    Serial.println("DHT11 error status: " + String(dht.getStatusString()));
    return false;
  }

  float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  float cr = dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);

  Serial.println(" temperature : " + String(newValues.temperature) + " °C  " + "  moisture : " + String(newValues.humidity) + " %");
  // LINE.notify(" อุณหภูมิ : " + String(newValues.temperature) + " °C  " + "  ความชื้น : " + String(newValues.humidity) + " %");
  tem = newValues.temperature;
  hum = newValues.humidity;

  return true;
}

void Va() {
  val = analogRead(analogPin);  //อ่านค่าสัญญาณ analog ขา23
  Serial.println(val); 
  if (val > 900) { // ค่า 900 สามารถกำหนดปรับได้ตามค่าแสงในห้องต่าง
    digitalWrite(ledPin, HIGH); // สั่งให้ LED ติดสว่าง
    LINE.notify("ไฟติด");
  } else {
    digitalWrite(ledPin, LOW); // สั่งให้ LED ดับ
  }
  delay(1000);
}
void setup() {
  pinMode(ledPin, OUTPUT);  // sets the pin as output
  pinMode(analogPin,INPUT);
  Serial.begin(115200);
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(LINE.getVersion());

  WiFi.begin(SSID, PASSWORD);
  Serial.printf("WiFi connecting to %s\n",  SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(400);
  }
  Serial.printf("\nWiFi connected\nIP : ");
  Serial.println(WiFi.localIP());

  // กำหนด Line Token
  LINE.setToken(LINE_TOKEN);
  LINE.notify("Welcome");
 
  initTemp();
  // Signal end of setup() to tasks
  tasksEnabled = true;  Serial.println("DHT ESP32 example with tasks");
  // initTemp();
  // Signal end of setup() to tasks
 

}

void loop() {
  HTTPClient http;
  server.handleClient();
  http.begin(servername);
  val = analogRead(analogPin);  //อ่านค่าสัญญาณ analog ขา23
  Serial.println(val); 

    // Wait 2 seconds to let system settle down
  delay(2000);
    // Enable task that will read values from the DHT sensor
  tasksEnabled = true;
  if (tempTaskHandle != NULL) {
    vTaskResume(tempTaskHandle);
  }
  http.addHeader("Content-Type", "application/json");
  bool light = false;
  if(val==0){
    light=true;
  }
  int httpResponseCode = http.POST("{\"temp\":\"" + String(tem) + "\",\"moisture\":\""+String(hum)+"\",\"lighting\":\""+ light +"\"}");
  Serial.println(httpResponseCode);

  yield();
  LINE.notify("     temperature : " + String(tem) + "°C" + "  moisture : " + String(hum) + " %");
  delay(1 * 60 * 1000); //

  if (tem > 40) {
    String LineText;
    String string1 = "อุณหภูมิเกินกำหนด ";
    LineText = string1 ;
    // LINE.notify("        temperature : " + String(tem) + "°C" + "  moisture : " + String(hum) + " %");
    Serial.print("Line ");
    Serial.println(LineText);
    LINE.notify(LineText);
  }
  if (hum > 60) {
    String LineText;
    String string2 = "ความชื้นเกินกำหนด ";
    LineText = string2 ;
    // LINE.notify("        temperature : " + String(tem) + "°C" + "  moisture : " + String(hum) + " %");
    Serial.print("Line ");
    Serial.println(LineText);
    LINE.notify(LineText);
  }
}