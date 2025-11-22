#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <IRrecv.h>

const char *ssid = "LAPTOP-7BRN7V2J 0408";
const char *password = "3401q+9H";

// --- 赤外線センサー設定 ---
const uint16_t RECV_PIN = D1;                      // センサーのピン
const uint16_t LED_1_PIN = D2;                     // LEDのピン
const uint16_t LED_2_PIN = D3;                     // LEDのピン
const uint64_t HIT_CODE_TEST = 0x5555555555555555; // エアコンのリモコン
const uint32_t HIT_CODE = 0x8F7807F;               // 赤外線(38kHz)
IRrecv irrecv(RECV_PIN);
decode_results results;

// --- WebSocketサーバー設定 ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // /ws パス

// --- クールダウン設定 ---
unsigned long lastHitTime = 0;
const unsigned long HIT_COOLDOWN = 500; // 0.5秒間のクールダウン

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
  pinMode(LED_1_PIN, OUTPUT);
  pinMode(LED_2_PIN, OUTPUT);
  digitalWrite(LED_1_PIN, LOW); // 初期状態は消灯
  digitalWrite(LED_2_PIN, LOW); // 初期状態は消灯

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

  // mDNS設定
  if (MDNS.begin("esp32-drone"))
  {
    Serial.println("mDNS responder started: esp32-drone.local");
  }

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
  ws.cleanupClients();

  // 赤外線信号を受信したら
  if (irrecv.decode(&results))
  {
    Serial.print("Received Code: 0x");
    Serial.print(results.value, HEX); // 16進数で値を表示

    Serial.print(", Bits: ");
    Serial.print(results.bits); // 受信したビット数を表示
    Serial.print(", Type: ");

    // プロトコル名を文字列で表示
    Serial.print((int)results.decode_type);
    Serial.println();

    // LEDを点灯
    // digitalWrite(LED_1_PIN, HIGH); // デバッグ用
    // digitalWrite(LED_2_PIN, HIGH); // デバッグ用

    // もし当たり判定のコードと一致したら
    if (results.value == HIT_CODE_TEST)
    {
      if (millis() - lastHitTime > HIT_COOLDOWN)
      {
        Serial.println("HIT DETECTED (REMOTE)!");
        // 接続している全てのクライアントに "HIT_REMOTE" という文字列を送信
        ws.textAll("HIT_REMOTE");
        lastHitTime = millis();

        // LEDを点灯
        digitalWrite(LED_1_PIN, HIGH);
        digitalWrite(LED_2_PIN, HIGH);
      }
    }
    else if (results.value == HIT_CODE)
    {
      if (millis() - lastHitTime > HIT_COOLDOWN)
      {
        Serial.println("HIT DETECTED (IR)!");
        // 接続している全てのクライアントに "HIT_IR" という文字列を送信
        ws.textAll("HIT_IR");
        lastHitTime = millis();

        // LEDを点灯
        digitalWrite(LED_1_PIN, HIGH);
        digitalWrite(LED_2_PIN, HIGH);
      }
    }

    // 0.1 秒待ってからLEDを消灯
    delay(100);
    digitalWrite(LED_1_PIN, LOW);
    digitalWrite(LED_2_PIN, LOW);

    // 次の信号を受信するためにリセット
    irrecv.resume();
  }
}