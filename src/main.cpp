#define BLYNK_TEMPLATE_ID "TMPL6RatetRbw"
#define BLYNK_TEMPLATE_NAME "Flame Gas Warning"
#define BLYNK_AUTH_TOKEN "qMAMg8LShVD1XBW7XzOing_-WvqBzwlS"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WebServer.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <UrlEncode.h>

// --- THAY ĐỔI THÔNG TIN CỦA BẠN TẠI ĐÂY ---
const char *ssid = "Theeanhs";
const char *password = "14032005";

// --- URL GOOGLE SCRIPT ---
const char *G_SCRIPT_URL = "https://script.google.com/macros/s/AKfycbwV7dBY5gvMLbRPz53vmjE7-rTOqxVbu46nIZRnzYt5RnBz5NZnyHFKuSo2Hbh8YYHDLA/exec";
// ---------------------------------------------

// Khai báo chân kết nối
const int ledPin = 2;     // GPIO 2 (D2) cho LED 1
const int sensorPin = 33; // GPIO 33 cho cảm biến analog chính (lửa/cảnh báo)
const int relayPin = 26;  // GPIO 26 cho module relay

const int led2Pin = 15;      // GPIO 15 cho LED cảnh báo gas
const int gasSensorPin = 32; // GPIO 32 cho cảm biến khí gas

// --- Chân cho các linh kiện mới ---
const int fanPin = 27;   // GPIO 22 cho quạt
const int lightPin = 23; // GPIO 23 cho đèn

// Biến toàn cục
int gasDefaultValue = 0;

// Biến toàn cục cho dữ liệu thời tiết
String weather_temperature = "N/A";
String weather_description = "N/A";

// --- Biến cho việc ghi lịch sử mỗi 30 phút ---
const unsigned long logInterval = 10000; // 30 phút = 1,800,000 mili giây

// Khởi tạo các server
WebServer server(80);

// Biến lưu trữ mã HTML và CSS của trang web chính
const char *htmlPage = R"(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta http-equiv="refresh" content="2">
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
        .nav-link { display: inline-block; margin: 5px; padding: 10px 20px; background-color: #007bff; color: white; text-decoration: none; border-radius: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>BẢNG ĐIỀU KHIỂN ESP32</h1>
        <div class="section"> <p>Trạng thái đèn Buzzer:</p> <div class="status %LED_CLASS%">%LED_STATE%</div> </div>
        <div class="section"> <p>Trạng thái Relay:</p> <div class="status %RELAY_CLASS%">%RELAY_STATE%</div> </div>
        <div class="section"> <p>Giá trị cảm biến 1:</p> <div class="sensor-value">%ANALOG_VAL%</div> </div>
        <div class="section"> <p>Cảnh báo rò rỉ Gas:</p> <div class="status %GAS_LED_CLASS%">%GAS_LED_STATE%</div> </div>
        <div class="section"> <p>Giá trị cảm biến Gas:</p> <div class="sensor-value">%GAS_VAL%</div> </div>
        <div>
            <a href="/weather" class="nav-link">Xem Thời Tiết</a>
            <a href="/control" class="nav-link">Điều Khiển Thiết Bị</a>
        </div>
        <p class="footer">Trang sẽ tự làm mới sau 2 giây.</p>
    </div>
</body>
</html>
)";

// Biến lưu trữ mã HTML và CSS của trang thời tiết
const char *weatherPage = R"(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Thời Tiết - ESP32</title>
    <style>
        body { font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; padding: 20px 0; background-color: #e2f3ff; color: #333; }
        .container { text-align: center; padding: 40px; background-color: white; border-radius: 12px; box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1); min-width: 340px; }
        h1 { color: #0056b3; margin-bottom: 30px; }
        .weather-info { margin-bottom: 20px; }
        .weather-info p { margin: 10px 0; font-size: 1.2em; }
        .temperature { font-size: 4em; font-weight: bold; color: #ff8c00; }
        .description { font-size: 1.5em; color: #555; }
        .location { font-size: 1.2em; color: #777; margin-bottom: 25px; }
        .nav-link { display: inline-block; margin-top: 20px; padding: 10px 20px; background-color: #007bff; color: white; text-decoration: none; border-radius: 5px; }
        .footer { margin-top: 30px; font-size: 0.9em; color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <h1>THÔNG TIN THỜI TIẾT</h1>
        <p class="location">Thành phố Hồ Chí Minh</p>
        <div class="weather-info">
            <p class="temperature">%TEMPERATURE% &deg;C</p>
            <p class="description">%WEATHER_DESCRIPTION%</p>
        </div>
        <a href="/" class="nav-link">Về Trang Chính</a>
        <p class="footer">Dữ liệu được cung cấp bởi Open-Meteo.</p>
    </div>
</body>
</html>
)";

// --- MÃ HTML CHO TRANG ĐIỀU KHIỂN MỚI ---
const char *controlPage = R"(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta http-equiv="refresh" content="2">
    <title>Điều Khiển - ESP32</title>
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
        .nav-link { display: inline-block; margin-top: 20px; padding: 10px 20px; background-color: #007bff; color: white; text-decoration: none; border-radius: 5px; }
        .control-button { display: inline-block; margin-top: 10px; padding: 12px 25px; color: white; text-decoration: none; border-radius: 5px; font-size: 1em; background-color: #17a2b8; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ĐIỀU KHIỂN THIẾT BỊ</h1>
        <div class="section">
            <p>Trạng thái Quạt:</p>
            <div class="status %FAN_CLASS%">%FAN_STATE%</div>
            <a href="/toggleFan" class="control-button">BẬT/TẮT QUẠT</a>
        </div>
        <div class="section">
            <p>Trạng thái Đèn:</p>
            <div class="status %LIGHT_CLASS%">%LIGHT_STATE%</div>
            <a href="/toggleLight" class="control-button">BẬT/TẮT ĐÈN</a>
        </div>
        <a href="/" class="nav-link">Về Trang Chính</a>
    </div>
</body>
</html>
)";


// --- TASK ĐỂ GHI DỮ LIỆU GOOGLE SHEETS SONG SONG ---
void logToSheetsTask(void *pvParameters) {
  Serial.println("Task ghi du lieu Google Sheets da bat dau tren Core 0.");
  
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      
      int gasValue = analogRead(gasSensorPin);
      String url = String(G_SCRIPT_URL) + "?gasValue=" + String(gasValue);

      Serial.print("[Task] Dang gui du lieu dinh ky den Google Sheets: ");
      Serial.println(gasValue);
      
      // Đặt timeout cho kết nối HTTP để tránh bị treo
      http.begin(url);
      http.setTimeout(10000); // Timeout 10 giây
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      
      int httpCode = http.GET();

      if (httpCode > 0) {
        Serial.printf("[Task] Google Sheets response code: %d\n", httpCode);
        // Phải đọc response để giải phóng bộ đệm
        http.getString(); 
      } else {
        Serial.printf("[Task] Loi khi gui den Google Sheets: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.println("[Task] Mat ket noi WiFi, bo qua viec ghi du lieu.");
    }

    vTaskDelay(logInterval / portTICK_PERIOD_MS);
  }
}


// Hàm chuyển đổi mã thời tiết WMO thành mô tả
String getWeatherDescription(int code)
{
  switch (code)
  {
  case 0: return "Trời quang";
  case 1: return "Gần quang";
  case 2: return "Mây rải rác";
  case 3: return "Nhiều mây";
  case 45: return "Sương mù";
  case 48: return "Sương mù đọng";
  case 51: return "Mưa phùn nhẹ";
  case 53: return "Mưa phùn vừa";
  case 55: return "Mưa phùn dày";
  case 61: return "Mưa nhỏ";
  case 63: return "Mưa vừa";
  case 65: return "Mưa to";
  case 80: return "Mưa rào nhẹ";
  case 81: return "Mưa rào vừa";
  case 82: return "Mưa rào to";
  case 95: return "Dông";
  default: return "Không xác định";
  }
}

// Hàm lấy dữ liệu thời tiết từ API
void getWeatherData()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin("https://api.open-meteo.com/v1/forecast?latitude=10.82&longitude=106.63&current_weather=true");
    http.setTimeout(10000); // Timeout 10 giây
    int httpCode = http.GET();
    if (httpCode > 0)
    {
      if (httpCode == HTTP_CODE_OK)
      {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        float temp = doc["current_weather"]["temperature"];
        int code = doc["current_weather"]["weathercode"];
        weather_temperature = String(temp, 1);
        weather_description = getWeatherDescription(code);
        Serial.println("Lay du lieu thoi tiet thanh cong.");
      }
    }
    else
    {
      Serial.printf("Loi khi goi API, ma loi: %s\n", http.errorToString(httpCode).c_str());
      weather_temperature = "N/A";
      weather_description = "Lỗi kết nối";
    }
    http.end();
  }
  else
  {
    Serial.println("Mat ket noi WiFi, khong the lay du lieu thoi tiet.");
    weather_temperature = "N/A";
    weather_description = "Mất WiFi";
  }
}

// Hàm xử lý yêu cầu đến trang chủ ("/")
void handleRoot()
{
  String page = String(htmlPage);
  int sensorValue = analogRead(sensorPin);
  int gasSensorValue = analogRead(gasSensorPin);
  page.replace("%LED_STATE%", digitalRead(ledPin) ? "BẬT" : "TẮT");
  page.replace("%LED_CLASS%", digitalRead(ledPin) ? "status-on" : "status-off");
  page.replace("%RELAY_STATE%", digitalRead(relayPin) == HIGH ? "BẬT" : "TẮT");
  page.replace("%RELAY_CLASS%", digitalRead(relayPin) == HIGH ? "status-on" : "status-off");
  page.replace("%GAS_LED_STATE%", digitalRead(led2Pin) ? "CÓ GAS" : "AN TOÀN");
  page.replace("%GAS_LED_CLASS%", digitalRead(led2Pin) ? "status-alert" : "status-on");
  page.replace("%ANALOG_VAL%", String(sensorValue));
  page.replace("%GAS_VAL%", String(gasSensorValue));
  server.send(200, "text/html", page);
}

// Hàm xử lý yêu cầu đến trang thời tiết ("/weather")
void handleWeather()
{
  getWeatherData();
  String page = String(weatherPage);
  page.replace("%TEMPERATURE%", weather_temperature);
  page.replace("%WEATHER_DESCRIPTION%", weather_description);
  server.send(200, "text/html", page);
}

// CÁC HÀM XỬ LÝ CHO TRANG ĐIỀU KHIỂN
void handleControl()
{
  String page = String(controlPage);
  page.replace("%FAN_STATE%", !digitalRead(fanPin) ? "BẬT" : "TẮT");
  page.replace("%FAN_CLASS%", !digitalRead(fanPin) ? "status-on" : "status-off");
  page.replace("%LIGHT_STATE%", digitalRead(lightPin) ? "BẬT" : "TẮT");
  page.replace("%LIGHT_CLASS%", digitalRead(lightPin) ? "status-on" : "status-off");
  server.send(200, "text/html", page);
}

void handleToggleFan()
{
  digitalWrite(fanPin, !digitalRead(fanPin));
  server.sendHeader("Location", "/control", true);
  server.send(302, "text/plain", "");
}

void handleToggleLight()
{
  digitalWrite(lightPin, !digitalRead(lightPin));
  server.sendHeader("Location", "/control", true);
  server.send(302, "text/plain", "");
}

void handleNotFound()
{
  server.send(404, "text/plain", "Not found");
}

// Hàm chứa logic cảnh báo của Blynk
void runBlynkLogic()
{
  int gasValue = analogRead(gasSensorPin);
  int sensorValue = analogRead(sensorPin);
  if (gasValue > 650)
  {
    digitalWrite(led2Pin, HIGH);
    if (Blynk.connected())
    {
      Blynk.logEvent("gas_leak", "Gas leak detected!");
    }
  }
  else
  {
    digitalWrite(led2Pin, LOW);
  }
  
}


void setup()
{
  Serial.begin(9600); // Tăng tốc độ baud để xem log khởi động chi tiết

  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(gasSensorPin, INPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(lightPin, OUTPUT);

  digitalWrite(ledPin, LOW);
  digitalWrite(relayPin, HIGH);
  digitalWrite(led2Pin, LOW);
  digitalWrite(lightPin, LOW);
  digitalWrite(fanPin, HIGH); // Đặt fanPin thành HIGH để quạt TẮT khi khởi động

  Serial.print("Dang ket noi den WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  // SỬA LỖI: Thêm timeout cho kết nối WiFi để không bị treo vô hạn
  unsigned long startConnectTime = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (millis() - startConnectTime > 20000) { // Timeout sau 20 giây
        Serial.println("\nKhong the ket noi WiFi!");
        // Cân nhắc khởi động lại hoặc chạy ở chế độ offline
        return; 
    }
  }
  Serial.println("\nDa ket noi WiFi!");
  Serial.print("Dia chi IP cua ESP32: ");
  Serial.println(WiFi.localIP());

  Blynk.config(BLYNK_AUTH_TOKEN);

  delay(2000);
  gasDefaultValue = analogRead(gasSensorPin);
  Serial.print("Gia tri Gas mac dinh: ");
  Serial.println(gasDefaultValue);

  server.on("/", handleRoot);
  server.on("/weather", handleWeather);
  server.on("/control", handleControl);
  server.on("/toggleFan", handleToggleFan);
  server.on("/toggleLight", handleToggleLight);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web Server da khoi dong");

  // SỬA LỖI: Tăng kích thước stack cho task để tránh lỗi tràn bộ nhớ
  // khi thực hiện các yêu cầu HTTPS.
  xTaskCreatePinnedToCore(
      logToSheetsTask,   // Hàm task sẽ chạy
      "LogToSheetsTask", // Tên của task (dùng để debug)
      8192,              // Kích thước stack (tăng từ 4096 lên 8192)
      NULL,              // Tham số truyền vào task (không dùng)
      1,                 // Độ ưu tiên của task (1 là thấp)
      NULL,              // Handle của task (không dùng)
      0);                // Chạy task trên Core 0 (lõi xử lý phụ)
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Blynk.run();
  }
  server.handleClient();
  runBlynkLogic();
  
  int sensorValue1 = analogRead(sensorPin);
  if (sensorValue1 < 1000)
  {
    digitalWrite(ledPin, HIGH);
    digitalWrite(relayPin, LOW);
    if (Blynk.connected())
    {
      Blynk.logEvent("flame_detected", "Alert detected on Sensor 1!");
    }
    delay(5000);
  }
  else
  {
    digitalWrite(ledPin, LOW);
    digitalWrite(relayPin, HIGH);
  }
  
  // Thêm một delay nhỏ để nhường thời gian xử lý cho các tác vụ khác
  delay(10); 
}
