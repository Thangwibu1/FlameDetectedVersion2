
#define BLYNK_TEMPLATE_ID "TMPL6kaLmF0Ic"
#define BLYNK_TEMPLATE_NAME "Test"
#define BLYNK_AUTH_TOKEN "tpGbf7IkKqeGMBbI2zRqmVmcaiFCqH8x"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WebServer.h>
#include <BlynkSimpleEsp32.h>

// --- THAY ĐỔI THÔNG TIN WIFI CỦA BẠN TẠI ĐÂY ---
const char* ssid = "Trung Ngoc";
const char* password = "25061972";
// ---------------------------------------------

// Khai báo chân kết nối
const int ledPin = 2;         // GPIO 2 (D2) cho LED 1
const int sensorPin = 33;     // GPIO 33 cho cảm biến analog chính (lửa/cảnh báo)
const int relayPin = 26;      // GPIO 26 cho module relay

const int led2Pin = 15;       // GPIO 15 cho LED cảnh báo gas
const int gasSensorPin = 32;  // GPIO 32 cho cảm biến khí gas

// Biến toàn cục
int gasDefaultValue = 0;

// Khởi tạo các server
WebServer server(80);

// Biến lưu trữ mã HTML và CSS của trang web
const char* htmlPage = R"(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta http-equiv="refresh" content="1">
    <title>ESP32 Dashboard</title>
    <style>
        body { font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; padding: 20px 0; background-color: #f0f2f5; color: #333; }
        .container { text-align: center; padding: 40px; background-color: white; border-radius: 12px; box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1); min-width: 340px; }
        h1 { color: #0056b3; margin-bottom: 30px; }
        .section { margin-bottom: 25px; padding-bottom: 25px; border-bottom: 1px solid #eee; }
        .section:last-of-type { border-bottom: none; margin-bottom: 0; padding-bottom: 0; }
        .section p { margin-bottom: 10px; font-size: 1.1em; }
        .status { font-size: 1.8em; font-weight: bold; padding: 12px 25px; border-radius: 8px; display: inline-block; min-width: 120px; }
        .status-on { color: #fff; background-color: #28a745; }
        .status-off { color: #fff; background-color: #6c757d; }
        .status-alert { color: #fff; background-color: #dc3545; }
        .sensor-value { font-size: 2em; font-weight: bold; color: #007bff; padding: 10px; display: inline-block; }
        .footer { margin-top: 30px; font-size: 0.9em; color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <h1>BẢNG ĐIỀU KHIỂN ESP32</h1>
        <div class="section"> <p>Trạng thái đèn LED 1:</p> <div class="status %LED_CLASS%">%LED_STATE%</div> </div>
        <div class="section"> <p>Trạng thái Relay:</p> <div class="status %RELAY_CLASS%">%RELAY_STATE%</div> </div>
        <div class="section"> <p>Giá trị cảm biến 1:</p> <div class="sensor-value">%ANALOG_VAL%</div> </div>
        <div class="section"> <p>Cảnh báo rò rỉ Gas:</p> <div class="status %GAS_LED_CLASS%">%GAS_LED_STATE%</div> </div>
        <div class="section"> <p>Giá trị cảm biến Gas:</p> <div class="sensor-value">%GAS_VAL%</div> </div>
        <p class="footer">Trang sẽ tự làm mới sau 1 giây.</p>
    </div>
</body>
</html>
)";

// Hàm xử lý yêu cầu đến trang chủ ("/")
void handleRoot() {
  String page = String(htmlPage);
  
  // Đọc giá trị
  int sensorValue = analogRead(sensorPin);
  int gasSensorValue = analogRead(gasSensorPin);

  // Thay thế placeholder
  page.replace("%LED_STATE%", digitalRead(ledPin) ? "BẬT" : "TẮT");
  page.replace("%LED_CLASS%", digitalRead(ledPin) ? "status-on" : "status-off");
  
  page.replace("%RELAY_STATE%", digitalRead(relayPin) == LOW ? "BẬT" : "TẮT");
  page.replace("%RELAY_CLASS%", digitalRead(relayPin) == LOW ? "status-on" : "status-off");
  
  page.replace("%GAS_LED_STATE%", digitalRead(led2Pin) ? "CÓ GAS" : "AN TOÀN");
  page.replace("%GAS_LED_CLASS%", digitalRead(led2Pin) ? "status-alert" : "status-on");

  page.replace("%ANALOG_VAL%", String(sensorValue));
  page.replace("%GAS_VAL%", String(gasSensorValue));
  
  server.send(200, "text/html", page);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// Hàm chứa logic Blynk
void runBlynkLogic() {
    int gasValue = analogRead(gasSensorPin);
    int sensorValue = analogRead(sensorPin);

    // Gửi dữ liệu lên Blynk
    Blynk.virtualWrite(V1, gasValue);
    Blynk.virtualWrite(V2, sensorValue); // V2 bây giờ đọc từ sensorPin

    // Logic cảnh báo gas
    if (abs(gasValue - gasDefaultValue) > 100) {
        digitalWrite(led2Pin, HIGH);
        if(Blynk.connected()){
            Blynk.logEvent("gas_leak", "Gas leak detected!");
        }
    } else {
        digitalWrite(led2Pin, LOW);
    }

    // Logic cảnh báo dựa trên sensorPin (trước đây là lửa)
    if (sensorValue < 1000) {
        if(Blynk.connected()){
             Blynk.logEvent("flame_detected","Alert detected on Sensor 1!"); // Đổi tên sự kiện cho phù hợp
        }
    }
}


void setup() {
  Serial.begin(9600);
  
  // Cấu hình chân
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(sensorPin, INPUT); 
  pinMode(led2Pin, OUTPUT);
  pinMode(gasSensorPin, INPUT);
  
  // Mặc định tắt các thiết bị
  digitalWrite(ledPin, LOW);
  digitalWrite(relayPin, LOW); // Tắt relay (kích hoạt mức thấp)
  digitalWrite(led2Pin, LOW);

  // Kết nối WiFi
  Serial.print("Dang ket noi den WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nDa ket noi WiFi!");
  Serial.print("Dia chi IP cua ESP32: ");
  Serial.println(WiFi.localIP());

  // Kết nối Blynk
  Blynk.config(BLYNK_AUTH_TOKEN);
  
  // Lấy giá trị gas mặc định sau khi ổn định
  delay(2000);
  gasDefaultValue = analogRead(gasSensorPin);
  Serial.print("Gia tri Gas mac dinh: ");
  Serial.println(gasDefaultValue);

  // Định tuyến Web Server
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web Server da khoi dong");
}

void loop() {
  // Chạy các dịch vụ nền
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }
  server.handleClient();
  
  // Chạy logic của Blynk
  runBlynkLogic();

  // --- Xử lý logic cho cảm biến chính và quản lý Relay ---
  int sensorValue1 = analogRead(sensorPin);

  // Logic cho LED 1 (cảnh báo chung)
  if (sensorValue1 < 1000) {
      digitalWrite(ledPin, HIGH);
      digitalWrite(relayPin, HIGH);
      delay(5000); // BẬT Relay
    } else {
      digitalWrite(relayPin, LOW); // TẮT Relay
      digitalWrite(ledPin, LOW);
  }

}
