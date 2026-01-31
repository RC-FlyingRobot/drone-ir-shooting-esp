#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <IRrecv.h>
#include <Preferences.h>

const char *ssid = "LAPTOP-7BRN7V2J 0408";
const char *password = "3401q+9H";

// --- ピン周りの設定 --- //
const uint16_t RECV_PIN = D1;  // センサーのピン
const uint16_t LED_1_PIN = D2; // 緑色LEDのピン1
const uint16_t LED_2_PIN = D3; // 緑色LEDのピン2

// ヒット判定を行うコードのリスト (64bit対応)
const uint64_t HIT_CODES[] = {
    0x5555555555555555, // エアコンのリモコン
    0x8F7807F,          // 赤外線(38kHz) 正規コード
    0xB301BD1F,         // 頻出パターン1
    0x55EEECD,          // 頻出パターン2
    0xCADB61A9          // 頻出パターン3
};
const int HIT_CODES_COUNT = sizeof(HIT_CODES) / sizeof(HIT_CODES[0]);

IRrecv irrecv(RECV_PIN); // 赤外線受信オブジェクト
decode_results results;  // 受信結果オブジェクト

// --- データ保存関係 --- //
Preferences preferences;
int currentScore = 0;       // 現在のスコア
bool isGameRunning = false; // ゲーム中かどうかのフラグ

// --- クールダウンタイム設定 --- //
unsigned long lastHitTime = 0;          // 最後にヒットを検出した時間
const unsigned long HIT_COOLDOWN = 200; // クールダウン時間(ミリ秒)

void setup()
{
  Serial.begin(115200); // USBシリアル開始

  // LEDピン初期化
  pinMode(LED_1_PIN, OUTPUT);
  digitalWrite(LED_1_PIN, LOW);
  pinMode(LED_2_PIN, OUTPUT);
  digitalWrite(LED_2_PIN, LOW);

  // 赤外線開始
  irrecv.enableIRIn();

  // 保存領域を開く(名前空間: "game", 読み書きモード: false)
  preferences.begin("game", false);

  // 電源喪失対策として, 前回の続きからスコアを読み込む
  // デフォルトは0
  currentScore = preferences.getInt("score", 0);

  Serial.println("System Ready. Current Saved Score: " + String(currentScore));
}

void loop()
{
  // USBシリアルからのコマンド受信
  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    command.trim(); // 改行コード除去

    // ゲーム開始時
    if (command == "START")
    {
      // スコアをリセットして保存
      currentScore = 0;
      preferences.putInt("score", currentScore); // 保存
      isGameRunning = true;
      Serial.println("OK:STARTED");

      // 合図用のLED点滅(3回)
      for (int i = 0; i < 3; i++)
      {
        digitalWrite(LED_1_PIN, HIGH);
        digitalWrite(LED_2_PIN, HIGH);
        delay(100);
        digitalWrite(LED_1_PIN, LOW);
        digitalWrite(LED_2_PIN, LOW);
        delay(100);
      }
    }

    // ゲーム終了時
    else if (command == "FINISH")
    {
      // 現在のスコアを返す
      isGameRunning = false;
      Serial.println("SCORE:" + String(currentScore));
    }
  }

  // 赤外線受信時
  if (irrecv.decode(&results))
  {
    // デバッグ用出力(USBがつながっている時だけ見える)
    Serial.println(resultToHexidecimal(&results));

    // ゲーム中でなくても, ヒットしたらカウントして保存する仕様にする
    // USBを抜いている間は isGameRunning フラグがリセットされる可能性があるため

    bool isHit = false;
    for (int i = 0; i < HIT_CODES_COUNT; i++)
    {
      if (results.value == HIT_CODES[i])
      {
        isHit = true;
        break;
      }
    }

    if (isHit && (millis() - lastHitTime > HIT_COOLDOWN))
    {
      // スコア加算
      currentScore++;

      // 即座にフラッシュメモリに保存
      // これにより, この瞬間に電源が落ちてもデータは守られる
      preferences.putInt("score", currentScore);

      lastHitTime = millis();

      // LED点灯
      digitalWrite(LED_1_PIN, HIGH);
      digitalWrite(LED_2_PIN, HIGH);
      delay(100);
      digitalWrite(LED_1_PIN, LOW);
      digitalWrite(LED_2_PIN, LOW);

      // デバッグ出力(USBがつながっている時だけ見える)
      Serial.println("HIT! Saved Score: " + String(currentScore));
    }

    irrecv.resume();
  }
}