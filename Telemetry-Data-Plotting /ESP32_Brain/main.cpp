#include <Arduino.h>

#define MOTOR_RX_PIN 8  
#define MOTOR_TX_PIN 9  

HardwareSerial MotorSerial(1); 

void setup() {
  Serial.begin(115200); 
  MotorSerial.begin(9600, SERIAL_8N1, MOTOR_RX_PIN, MOTOR_TX_PIN); 
  
  Serial.println("\n=====================================");
  Serial.println("🚀 大脑已启动！画图模式开启！");
  Serial.println("⏳ 等待 8 秒开机...");
  delay(8000); 
  while(MotorSerial.available()) MotorSerial.read();
  Serial.println("✅ 准备就绪，开始输出心电图...");
}

unsigned long lastMoveTime = 0;
bool toggle = false;

void loop() {
  // 1. 🎧 接收心跳数据并画图
  while (MotorSerial.available()) {
    String msg = MotorSerial.readStringUntil('\n');
    msg.trim(); 
    
    if (msg.startsWith("D:") && msg.length() > 3) {
      // 提取心跳包数据
      String data = msg.substring(2);
      int commaIdx = data.indexOf(',');
      if (commaIdx > 0) {
        String angle = data.substring(0, commaIdx);
        String effort = data.substring(commaIdx + 1);
        
        // 📈 【画图专用格式】：在变量名前加 '>'
        Serial.print(">Angle:"); Serial.println(angle);
        Serial.print(">Effort:"); Serial.println(effort);
      }
    } 
    else if (msg.length() > 1) {
      // 如果是 OUCH 或 WAKE_UP 等惨叫，原样打印
      Serial.print("🤖 "); Serial.println(msg);
    }
  }
  
  // 2. ⏱️ 每 4 秒下达目标，并把目标也画到图上！
  if (millis() - lastMoveTime > 4000) {
    lastMoveTime = millis();
    if (toggle) {
      MotorSerial.print("M1.57\n"); 
      Serial.println(">Target:1.57"); // 📈 把目标线也画出来！
    } else {
      MotorSerial.print("M0\n");    
      Serial.println(">Target:0.0");  // 📈 把目标线也画出来！
    }
    toggle = !toggle;
  }
}