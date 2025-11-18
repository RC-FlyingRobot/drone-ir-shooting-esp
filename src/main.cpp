#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <IRrecv.h>

const char *ssid = "LAPTOP-7BRN7V2J 0408";
const char *password = "3401q+9H";

// --- 赤外線センサー設定 ---
const uint16_t RECV_PIN_38 = D2;           // センサー(38kHz)のピン
const uint16_t RECV_PIN_56 = D4;           // センサー(56kHz)のピン
const uint16_t LED_PIN_38 = D1;            // LED(38kHz)のピン
const uint16_t LED_PIN_56 = D3;            // LED(56kHz)のピン
const uint64_t HIT_CODE_TEST = 0x5CCDEDFE; // エアコンのリモコン
const uint32_t HIT_CODE_38 = 0x8F7807F;    // 赤外線(38kHz)
const uint32_t HIT_CODE_56 = 0xC34016E0;   // 赤外線(56kHz)
IRrecv irrecv38(RECV_PIN_38);
IRrecv irrecv56(RECV_PIN_56);
// センサーごとに decode_results を分離（競合・上書き防止）
decode_results results38;
decode_results results56;

// LED をノンブロッキング制御するための設定
const uint32_t LED_PULSE_MS = 100; // 点灯時間
bool led38On = false;
bool led56On = false;
unsigned long led38StartMs = 0;
unsigned long led56StartMs = 0;

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
  pinMode(LED_PIN_38, OUTPUT);
  digitalWrite(LED_PIN_38, LOW); // 初期状態は消灯
  pinMode(LED_PIN_56, OUTPUT);
  digitalWrite(LED_PIN_56, LOW); // 初期状態は消灯

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
  irrecv38.enableIRIn();
  irrecv56.enableIRIn(); // 56kHz用レシーバーも有効化
  Serial.println("IR Receivers (38kHz & 56kHz) Ready!");

  // WebSocketサーバーを開始
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
  Serial.println("WebSocket server started!");
}

void loop()
{
  // 38kHzの赤外線信号を受信したら
  if (irrecv38.decode(&results38))
  {
    Serial.print("[38kHz] Received Code: 0x");
    Serial.print(results38.value, HEX); // 16進数で値を表示

    Serial.print(", Bits: ");
    Serial.print(results38.bits); // 受信したビット数を表示
    Serial.print(", Type: ");
    Serial.print((int)results38.decode_type);
    Serial.println();

    // 38kHz用のLEDを点灯（ノンブロッキング制御）
    digitalWrite(LED_PIN_38, HIGH);
    led38On = true;
    led38StartMs = millis();

    // もし、38kHz用の当たり判定コードと一致したら
    // クロストーク対策: 56kHzの当たりコードに一致した場合は無視
    if (results38.value == HIT_CODE_56)
    {
      Serial.println("[38kHz] Ignored (matches 56kHz HIT code)");
    }
    else if (results38.value == HIT_CODE_38)
    {
      Serial.println("HIT DETECTED! (38kHz)");
      // 接続している全てのクライアントに "HIT_38" という文字列を送信
      ws.textAll("HIT_38");
    }

    // 次の信号を受信するためにリセット
    irrecv38.resume();
  }

  // 56kHzの赤外線信号を受信したら
  if (irrecv56.decode(&results56))
  {
    Serial.print("[56kHz] Received Code: 0x");
    Serial.print(results56.value, HEX); // 16進数で値を表示

    Serial.print(", Bits: ");
    Serial.print(results56.bits); // 受信したビット数を表示
    Serial.print(", Type: ");
    Serial.print((int)results56.decode_type);
    Serial.println();

    // 56kHz用のLEDを点灯（ノンブロッキング制御）
    digitalWrite(LED_PIN_56, HIGH);
    led56On = true;
    led56StartMs = millis();

    // もし、56kHz用の当たり判定コードと一致したら
    // クロストーク対策: 38kHzの当たりコードに一致した場合は無視
    if (results56.value == HIT_CODE_38)
    {
      Serial.println("[56kHz] Ignored (matches 38kHz HIT code)");
    }
    else if (results56.value == HIT_CODE_56)
    {
      Serial.println("HIT DETECTED! (56kHz)");
      // 接続している全てのクライアントに "HIT_56" という文字列を送信
      ws.textAll("HIT_56");
    }

    // 次の信号を受信するためにリセット
    irrecv56.resume();
  }

  // ノンブロッキングでLEDを自動消灯
  unsigned long now = millis();
  if (led38On && (now - led38StartMs >= LED_PULSE_MS))
  {
    digitalWrite(LED_PIN_38, LOW);
    led38On = false;
  }
  if (led56On && (now - led56StartMs >= LED_PULSE_MS))
  {
    digitalWrite(LED_PIN_56, LOW);
    led56On = false;
  }
}