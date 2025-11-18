#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <IRrecv.h>

const char *ssid = "LAPTOP-7BRN7V2J 0408";
const char *password = "3401q+9H";

// --- 赤外線センサー設定 ---
const uint16_t kRecvPin = D2;  // センサーのピン
const uint16_t kLedPin_1 = D1; // LEDのピン
const uint16_t kLedPin_2 = D3; // LEDのピン
// const uint64_t HIT_CODE = 0x5555555555555555; // エアコンのリモコン
const uint32_t HIT_CODE = 0x8F7807F; // 赤外線(38kHz)
IRrecv irrecv(kRecvPin);
decode_results results;

// --- WebSocketサーバー設定 ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // /ws パス

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.println("Client connected");
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.println("Client disconnected");
  }
}

void setup()
{
  Serial.begin(115200);

  // LEDピンを出力モードに設定
  pinMode(kLedPin_1, OUTPUT);
  digitalWrite(kLedPin_1, LOW); // 初期状態は消灯
  pinMode(kLedPin_2, OUTPUT);
  digitalWrite(kLedPin_2, LOW); // 初期状態は消灯

  // Wi-Fi接続
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // 赤外線受信を開始
  irrecv.enableIRIn();
  Serial.println("IR Receiver Ready!");

  // WebSocketサーバーを開始
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
  Serial.println("WebSocket server started!");
}

void loop()
{
  // 赤外線信号を受信したら
  if (irrecv.decode(&results))
  {
    Serial.print("Received Code: 0x");
    Serial.print(results.value, HEX); // 16進数で値を表示

    Serial.print(", Bits: ");
    Serial.print(results.bits); // 受信したビット数を表示
    Serial.print(", Type: ");
    // プロトコル名を文字列で表示（ライブラリ関数が見つからないため数値で表示）
    Serial.print((int)results.decode_type);
    Serial.println();

    // LEDを点灯
    digitalWrite(kLedPin_1, HIGH);
    digitalWrite(kLedPin_2, HIGH);

    // もし、当たり判定のコードと一致したら
    if (results.value == HIT_CODE)
    {
      Serial.println("HIT DETECTED!");
      // 接続している全てのクライアントに "HIT" という文字列を送信
      ws.textAll("HIT");

      // LEDを点灯
      // digitalWrite(kLedPin, HIGH);
    }

    // 0.1秒待ってからLEDを消灯（視認しやすくするため）
    delay(100);
    digitalWrite(kLedPin_1, LOW);
    digitalWrite(kLedPin_2, LOW);

    // 次の信号を受信するためにリセット
    irrecv.resume();
  }
}