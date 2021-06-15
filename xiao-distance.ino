/*
 *Seeeduino XIAO + VL53L0Xレーザー測距 SDカード記録式データロガー Ver.1.0
 *by sakuraiphysics 20210610
 *レーザー式の距離測定を行い、OLEDに表示しながらSDに記録するやつです。
 *ハードはMicroSDを採用していますが、そりゃあ別にどっちでも動くと思います。
 *OLEDと測距センサーとはI2Cで通信し、SDはシリアルです。
 *SDカードへの書き込み開始と終了はタクトスイッチにより制御します。
 *
 *要改善事項
 *
 */

#define FREQ 20

#include <SPI.h>
#include <SD.h>
#include <Wire.h>

#include <TimerTC3.h>

#include <VL53L1X.h>

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

//ハードウェア関係
#define SCREEN_WIDTH 128 // OLED 幅ピクセル数
#define SCREEN_HEIGHT 64 // OLED 高さピクセル数
#define OLED_RESET     1 // OLED リセットピン
#define SCREEN_ADDRESS 0x3C //OLED I2Cアドレス
#define SW 3 //トグルスイッチのピンNo.
#define CS 7 //SDカードのChipSelect

VL53L1X sensor;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//グローバル変数系
bool noSD = false; //SDがない場合はnoSDモードで起動
bool prevSW; //前回のトグルスイッチ状態。
byte filecount; //SDカードのファイルNo.を格納する
int number; //SDデータカウント

//関数プロトタイプ泉源
void newfile();
void writing(int);
void oled(boolean, int);

void setup() {
  delay(50);
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SW, INPUT_PULLUP);
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
  
  //OLEDのアロケーション 失敗したらLEDが1チカを繰り返して起動しない
  delay(50);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed.");
    for(;;) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
      digitalWrite(LED_BUILTIN, LOW);
      delay(800);
    }
  }
  display.setTextColor(WHITE);
  display.setTextSize(1);
  
  //測距センサのイニシャライズ 失敗したらLEDが2チカを繰り返して起動しない
  sensor.setTimeout(500);
  if (!sensor.init()) {
    Serial.println("Failed to detect and initialize sensor.");
    display.clearDisplay();
    display.println("Sensor failed.");
    display.display();
    
    for(;;) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(150);
      digitalWrite(LED_BUILTIN, LOW);
      delay(150);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(150);
      digitalWrite(LED_BUILTIN, LOW);
      delay(550);
    }
  }

  sensor.setDistanceMode(VL53L1X::Medium);
  sensor.setMeasurementTimingBudget(40000);
  
  //SDカードスロットのアロケーション 失敗したらnoSDモードで起動
  if (!SD.begin(CS)) {
    noSD = true;
  }

  //測距センサをback-to-backモードで起動
  sensor.startContinuous(50);
  
  //TimerTC3初期化
    TimerTc3.initialize(1000000 / FREQ);
    TimerTc3.attachInterrupt(measurement);
}

void measurement() {

  //各パラメータを収集
  int distance = sensor.read();
  if (sensor.timeoutOccurred()) {
    Serial.println("Sensor Timeout.");
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.println("Sensor");
    display.println("timeout");
    display.display();
  }

  bool nowSW = digitalRead(SW);

  //distanceが8190なら、書き込みや表示のリフレッシュを全部すっとばす
  if(distance < 8000) {
    //SD書き込み OFFやnoSDならスキップ
    if (noSD == false && nowSW == false) {
  
      //スイッチONされて1回目は新規ファイル処理
      if (prevSW == true){
        newfile();
      }
  
      //データ書き込み処理
      writing(distance);
    }
  
    oled(nowSW, distance);
    
  }

  //numberをインクリメントし、prevSWを格納
  number++;
  prevSW = nowSW;
  
}

void newfile() {
  number = 0;
  File countFile = SD.open("count.txt", FILE_READ);
  if (countFile) {
    filecount = countFile.read();
    countFile.close();
    SD.remove("count.txt");
  } else {
    filecount = 0;
  }
  countFile = SD.open("count.txt", FILE_WRITE);
  countFile.write(filecount + 1);
  countFile.close();
}

void writing(int dist) {
  File dataFile = SD.open("DATA" + String(filecount) + ".csv", FILE_WRITE);
  if (dataFile) {
    dataFile.print(number / 20);
    dataFile.print(".");
    dataFile.print((number % 20) * 5);
    dataFile.print(",");
    dataFile.println(dist);
    dataFile.close();
  }  
}

void oled(boolean nowSW, int dist) {

  display.clearDisplay();
  display.setCursor(0,0);
  display.print("t = ");
  display.print(number / 20);
  display.print(".");
  display.println((number % 20) * 5);
  display.print("x = ");
  display.println(dist);

  display.setCursor(104,0);
  display.setTextSize(1);
  
  if (noSD == true) {
    display.print("noSD");
  } else {
    if (nowSW == false) {
      display.print("REC");
    } else {
      display.print("STOP");
    }
  }
  
  display.display();
  
}

void loop() {
  
  //タイマ割り込みで全部やるのでloopは空です。
  
}
