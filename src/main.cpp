/*
 * Mã nguồn cho ESP32: Tạo Web Server để theo dõi trạng thái LED, cảm biến Analog và Relay
 * Mô tả:
 * - Kết nối ESP32 vào mạng WiFi.
 * - Đọc giá trị từ một cảm biến analog kết nối vào chân GPIO 33.
 * - Nếu giá trị analog < 300:
 * + Đèn LED ở chân D2 (GPIO 2) sẽ BẬT.
 * + Relay ở chân 26 sẽ được kích hoạt (xuất tín hiệu LOW).
 * - Ngược lại, đèn LED và Relay sẽ TẮT.
 * - Tạo một Web Server tại cổng 80.
 * - Khi truy cập vào địa chỉ IP của ESP32, một trang web sẽ hiển thị:
 * + Trạng thái hiện tại của đèn LED (BẬT/TẮT).
 * + Trạng thái hiện tại của Relay (BẬT/TẮT).
 * + Giá trị analog đo được từ cảm biến.
 * - Trang web sẽ tự động làm mới sau mỗi 1 giây để cập nhật thông tin.
 *
 * Hướng dẫn:
 * 1. Thay đổi `TEN_WIFI_CUA_BAN` và `MAT_KHAU_WIFI`.
 * 2. Kết nối LED vào chân D2 (GPIO 2).
 * 3. Kết nối cảm biến analog vào chân 33.
 * 4. Kết nối module relay vào chân 26.
 * 5. Nạp code vào board ESP32 và mở Serial Monitor để xem địa chỉ IP.
 */

#include <WiFi.h>
#include <WebServer.h>

// --- THAY ĐỔI THÔNG TIN WIFI CỦA BẠN TẠI ĐÂY ---
const char* ssid = "Trung Ngoc";
const char* password = "25061972";
// ---------------------------------------------

// Khai báo chân kết nối
const int ledPin = 2;     // GPIO 2 (D2) cho LED
const int sensorPin = 33; // GPIO 33 cho cảm biến analog
const int relayPin = 26;  // GPIO 26 cho module relay

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
            height: 100vh;
            margin: 0;
            background-color: #f0f2f5;
            color: #333;
        }
        .container {
            text-align: center;
            padding: 40px;
            background-color: white;
            border-radius: 12px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
            min-width: 320px;
        }
        h1 {
            color: #0056b3;
            margin-bottom: 30px;
        }
        .section {
            margin-bottom: 25px;
        }
        .section p {
            margin-bottom: 10px;
            font-size: 1.1em;
        }
        .status {
            font-size: 2em;
            font-weight: bold;
            padding: 15px 30px;
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
            background-color: #dc3545; /* Màu đỏ */
        }
        .sensor-value {
            font-size: 2.2em;
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
            <p>Trạng thái đèn LED:</p>
            <div class="status %LED_CLASS%">%LED_STATE%</div>
        </div>

        <div class="section">
            <p>Trạng thái Relay:</p>
            <div class="status %RELAY_CLASS%">%RELAY_STATE%</div>
        </div>

        <div class="section">
            <p>Giá trị cảm biến Analog:</p>
            <div class="sensor-value">%ANALOG_VAL%</div>
        </div>

        <p class="footer">Trang sẽ tự làm mới sau 1 giây.</p>
    </div>
</body>
</html>
)";

// Hàm xử lý yêu cầu đến trang chủ ("/")
void handleRoot() {
  // Đọc trạng thái hiện tại của các chân
  int ledState = digitalRead(ledPin);
  int relayState = digitalRead(relayPin); // Đọc trạng thái relay
  int sensorValue = analogRead(sensorPin);
  
  // Tạo một bản sao của chuỗi HTML để chỉnh sửa
  String page = String(htmlPage);
  
  // Thay thế placeholder của LED
  if (ledState == HIGH) {
    page.replace("%LED_STATE%", "BẬT");
    page.replace("%LED_CLASS%", "status-on");
  } else {
    page.replace("%LED_STATE%", "TẮT");
    page.replace("%LED_CLASS%", "status-off");
  }

  // Thay thế placeholder của Relay (Lưu ý: Relay kích hoạt mức THẤP)
  if (relayState == LOW) { // LOW là BẬT
    page.replace("%RELAY_STATE%", "BẬT");
    page.replace("%RELAY_CLASS%", "status-on");
  } else { // HIGH là TẮT
    page.replace("%RELAY_STATE%", "TẮT");
    page.replace("%RELAY_CLASS%", "status-off");
  }

  // Thay thế placeholder của cảm biến
  page.replace("%ANALOG_VAL%", String(sensorValue));
  
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
  pinMode(relayPin, OUTPUT); // Cấu hình chân relay là OUTPUT
  pinMode(sensorPin, INPUT); 
  
  // Mặc định tắt LED và Relay khi khởi động
  digitalWrite(ledPin, LOW);
  digitalWrite(relayPin, HIGH); // Tắt relay (kích hoạt mức thấp nên mức cao là tắt)

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
  // Đọc giá trị từ cảm biến
  int sensorValue = analogRead(sensorPin);
  digitalWrite(relayPin, HIGH); // Đặt Relay ở mức cao (tắt)
  // Điều khiển đèn LED và Relay dựa trên giá trị cảm biến
  if (sensorValue < 1000) {
    digitalWrite(ledPin, HIGH); // Bật LED
    digitalWrite(relayPin, LOW); 
    delay(5000); // Bật Relay (kích hoạt mức thấp)
  } else {
    digitalWrite(ledPin, LOW);  // Tắt LED // Tắt Relay
  }
  
  // Lắng nghe và xử lý các yêu cầu từ client
  server.handleClient();
}
