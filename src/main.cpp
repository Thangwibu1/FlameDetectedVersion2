/*
 * Mã nguồn cho ESP32: Tạo Web Server để theo dõi trạng thái các thiết bị
 * Mô tả:
 * - Kết nối ESP32 vào mạng WiFi.
 * - Đọc giá trị từ cảm biến analog (chân 33) để điều khiển LED (chân 2) và Relay (chân 26).
 * - Đọc giá trị từ cảm biến khí gas (chân 32) để điều khiển LED cảnh báo (chân 15).
 * - Logic điều khiển:
 * + Nếu giá trị cảm biến analog < 300: LED 1 và Relay BẬT.
 * + Nếu giá trị cảm biến gas < 500: LED 2 (cảnh báo) BẬT.
 * - Tạo một Web Server hiển thị trạng thái của tất cả các thiết bị và giá trị cảm biến.
 * - Trang web tự động làm mới sau mỗi 1 giây.
 *
 * Hướng dẫn:
 * 1. Thay đổi `TEN_WIFI_CUA_BAN` và `MAT_KHAU_WIFI`.
 * 2. Kết nối các cảm biến và thiết bị vào đúng các chân đã khai báo.
 * 3. Nạp code vào board ESP32 và mở Serial Monitor để xem địa chỉ IP.
 */

#include <WiFi.h>
#include <WebServer.h>

// --- THAY ĐỔI THÔNG TIN WIFI CỦA BẠN TẠI ĐÂY ---
const char* ssid = "Trung Ngoc";
const char* password = "25061972";
// ---------------------------------------------

// Khai báo chân kết nối
const int ledPin = 2;         // GPIO 2 (D2) cho LED 1
const int sensorPin = 33;     // GPIO 33 cho cảm biến analog 1
const int relayPin = 26;      // GPIO 26 cho module relay

const int led2Pin = 15;       // GPIO 15 cho LED cảnh báo gas
const int gasSensorPin = 32;  // GPIO 32 cho cảm biến khí gas

// Tạo một đối tượng WebServer lắng nghe trên cổng 80
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
        body {
            font-family: Arial, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 20px 0;
            background-color: #f0f2f5;
            color: #333;
        }
        .container {
            text-align: center;
            padding: 40px;
            background-color: white;
            border-radius: 12px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
            min-width: 340px;
        }
        h1 {
            color: #0056b3;
            margin-bottom: 30px;
        }
        .section {
            margin-bottom: 25px;
            padding-bottom: 25px;
            border-bottom: 1px solid #eee;
        }
        .section:last-child {
            border-bottom: none;
            margin-bottom: 0;
            padding-bottom: 0;
        }
        .section p {
            margin-bottom: 10px;
            font-size: 1.1em;
        }
        .status {
            font-size: 1.8em;
            font-weight: bold;
            padding: 12px 25px;
            border-radius: 8px;
            display: inline-block;
            min-width: 120px;
        }
        .status-on {
            color: #fff;
            background-color: #28a745; /* Màu xanh lá */
        }
        .status-off {
            color: #fff;
            background-color: #6c757d; /* Màu xám */
        }
        .status-alert {
            color: #fff;
            background-color: #dc3545; /* Màu đỏ */
        }
        .sensor-value {
            font-size: 2em;
            font-weight: bold;
            color: #007bff;
            padding: 10px;
            display: inline-block;
        }
        .footer {
            margin-top: 30px;
            font-size: 0.9em;
            color: #666;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>BẢNG ĐIỀU KHIỂN ESP32</h1>
        
        <div class="section">
            <p>Trạng thái đèn LED 1:</p>
            <div class="status %LED_CLASS%">%LED_STATE%</div>
        </div>

        <div class="section">
            <p>Trạng thái Relay:</p>
            <div class="status %RELAY_CLASS%">%RELAY_STATE%</div>
        </div>

        <div class="section">
            <p>Giá trị cảm biến 1:</p>
            <div class="sensor-value">%ANALOG_VAL%</div>
        </div>

        <div class="section">
            <p>Cảnh báo rò rỉ Gas:</p>
            <div class="status %GAS_LED_CLASS%">%GAS_LED_STATE%</div>
        </div>

        <div class="section">
            <p>Giá trị cảm biến Gas:</p>
            <div class="sensor-value">%GAS_VAL%</div>
        </div>

        <p class="footer">Trang sẽ tự làm mới sau 1 giây.</p>
    </div>
</body>
</html>
)";

// Hàm xử lý yêu cầu đến trang chủ ("/")
void handleRoot() {
  // Đọc trạng thái hiện tại của các chân và cảm biến
  int ledState = digitalRead(ledPin);
  int relayState = digitalRead(relayPin);
  int sensorValue = analogRead(sensorPin);
  int gasLedState = digitalRead(led2Pin);
  int gasSensorValue = analogRead(gasSensorPin);
  
  // Tạo một bản sao của chuỗi HTML để chỉnh sửa
  String page = String(htmlPage);
  
  // --- Thay thế placeholder cho Nhóm 1 (Cảm biến & Relay) ---
  if (ledState == HIGH) {
    page.replace("%LED_STATE%", "BẬT");
    page.replace("%LED_CLASS%", "status-on");
  } else {
    page.replace("%LED_STATE%", "TẮT");
    page.replace("%LED_CLASS%", "status-off");
  }
  if (relayState == LOW) { // LOW là BẬT
    page.replace("%RELAY_STATE%", "BẬT");
    page.replace("%RELAY_CLASS%", "status-on");
  } else { // HIGH là TẮT
    page.replace("%RELAY_STATE%", "TẮT");
    page.replace("%RELAY_CLASS%", "status-off");
  }
  page.replace("%ANALOG_VAL%", String(sensorValue));

  // --- Thay thế placeholder cho Nhóm 2 (Cảm biến Gas) ---
  if (gasLedState == HIGH) {
    page.replace("%GAS_LED_STATE%", "CÓ GAS");
    page.replace("%GAS_LED_CLASS%", "status-alert");
  } else {
    page.replace("%GAS_LED_STATE%", "AN TOÀN");
    page.replace("%GAS_LED_CLASS%", "status-on");
  }
  page.replace("%GAS_VAL%", String(gasSensorValue));
  
  // Gửi trang web đã được cập nhật về cho trình duyệt
  server.send(200, "text/html", page);
}

// Hàm xử lý khi không tìm thấy trang
void handleNotFound() {
  String message = "Không tìm thấy file\n\n";
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

void setup() {
  // Bắt đầu giao tiếp Serial để debug
  Serial.begin(9600);
  
  // Cấu hình chân
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(sensorPin, INPUT); 
  pinMode(led2Pin, OUTPUT);
  pinMode(gasSensorPin, INPUT);
  
  // Mặc định tắt các thiết bị khi khởi động
  digitalWrite(ledPin, LOW);
  digitalWrite(relayPin, LOW); // Tắt relay (kích hoạt mức thấp)
  digitalWrite(led2Pin, LOW);

  // Bắt đầu kết nối WiFi
  Serial.println();
  Serial.print("Dang ket noi den WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Chờ cho đến khi kết nối thành công
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Thông báo kết nối thành công và in ra địa chỉ IP
  Serial.println("");
  Serial.println("Da ket noi WiFi!");
  Serial.print("Dia chi IP cua ESP32: ");
  Serial.println(WiFi.localIP());

  // Định tuyến các yêu cầu HTTP
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  // Bắt đầu Web Server
  server.begin();
  Serial.println("Web Server da khoi dong");
}

void loop() {

  digitalWrite(relayPin, LOW);  // Bật Relay
  // --- Xử lý logic cho Nhóm 1 ---
  int sensorValue = analogRead(sensorPin);
  if (sensorValue < 1000) {
    digitalWrite(ledPin, HIGH); // Bật LED 1
    digitalWrite(relayPin, HIGH);
    delay(5000); // Tắt Relay
  } else {
    digitalWrite(ledPin, LOW);  // Tắt LED 1
  }

  // --- Xử lý logic cho Nhóm 2 (Gas) ---
  int gasSensorValue = analogRead(gasSensorPin);
  if (gasSensorValue < 500) {
    digitalWrite(led2Pin, HIGH); // Bật LED cảnh báo
  } else {
    digitalWrite(led2Pin, LOW);  // Tắt LED cảnh báo
  }
  
  // Lắng nghe và xử lý các yêu cầu từ client
  server.handleClient();
}
