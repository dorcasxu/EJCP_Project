#include <Arduino.h>

#define MOTOR_RX_PIN 8  
#define MOTOR_TX_PIN 9  

HardwareSerial MotorSerial(1); 

void setup() {
  Serial.begin(115200); 
  MotorSerial.begin(9600, SERIAL_8N1, MOTOR_RX_PIN, MOTOR_TX_PIN); // 极稳 9600
  
  Serial.println("\n=====================================");
  Serial.println("🚀 大脑 V_FINAL：坚守基线架构！");
  Serial.println("✅ 取消一切捂嘴操作，原样监听小脑的真实回音！");
  Serial.println("⏳ 死等 8 秒...");
  delay(8000); 
  Serial.println("✅ 开始极简坐标调度！\n");
}

unsigned long lastMoveTime = 0;
bool toggle = false;

void loop() {
  // 💡 【核心修复】：放开捂嘴的手！原原本本打印小脑说的每一句话！
  // 哪怕是乱码，也是极其珍贵的诊断线索！
  while (MotorSerial.available()) {
    char c = MotorSerial.read();
    Serial.write(c); 
  }
  
  // ⏱️ 每 4 秒下发一次纯净的指令
  if (millis() - lastMoveTime > 4000) {
    lastMoveTime = millis();
    Serial.println("\n-------------------------------------");
    if (toggle) {
      Serial.println("👉 大脑下令: 去 90度");
      MotorSerial.print("M1.57\n"); 
    } else {
      Serial.println("👈 大脑下令: 回 0度");
      MotorSerial.print("M0\n");    
    } 
    toggle = !toggle;
  }
}