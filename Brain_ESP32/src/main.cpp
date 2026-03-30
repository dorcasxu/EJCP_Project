#include <Arduino.h>

#define MOTOR_RX_PIN 8  
#define MOTOR_TX_PIN 9  

HardwareSerial MotorSerial(1); 

void setup() {
  Serial.begin(115200); 
  MotorSerial.begin(9600, SERIAL_8N1, MOTOR_RX_PIN, MOTOR_TX_PIN); 
  
  Serial.println("\n=====================================");
  Serial.println("🚀 具身大脑已启动！准备注入【精准到位】参数！");
  
  delay(8000); 
  while(MotorSerial.available()) MotorSerial.read(); // 清理杂音

  Serial.println("\n✅ 8秒结束！开始恢复动力基因！");
  
  // 💉 1. 恢复力气：把最高电压放宽到 5V（2V太小克服不了摩擦力）
  MotorSerial.print("MLU5.0\n"); delay(200);
  
  // 💉 2. 恢复记忆：把速度环积分(I)恢复到 5.0！
  // 这样它走到 45度推不动时，会咬牙继续推，绝对死磕到 90度！
  MotorSerial.print("MVI5.0\n"); delay(200);

  // 💉 3. 增强弹簧刚度：1.0太软了，我们设为 4.0！
  // 既保留了被人捏时的柔顺感，又有足够的劲儿拉到目标！
  MotorSerial.print("MAP4.0\n"); delay(200);

  // 💉 4. 保持优雅：最高速度依然限制在 2.0，防止它暴冲和剧烈抖动！
  MotorSerial.print("MVL2.0\n"); delay(200);

  Serial.println("🎉 参数注入完毕！这次绝对指哪打哪！\n");
}

unsigned long lastMoveTime = 0;
bool toggle = false;

void loop() {
  // 原样打印反馈
  while (MotorSerial.available()) {
    char c = MotorSerial.read();
    Serial.write(c); 
  }
  
  if (millis() - lastMoveTime > 4000) {
    lastMoveTime = millis();
    Serial.println("\n-------------------------------------");
    if (toggle) {
      Serial.println("👉 大脑下令: 稳稳去 90度 (M1.57)");
      MotorSerial.print("M1.57\n"); 
    } else {
      Serial.println("👈 大脑下令: 稳稳回 0度 (M0)");
      MotorSerial.print("M0\n");    
    }
    toggle = !toggle;
  }
}
