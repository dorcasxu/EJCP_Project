# 🤖 EJCP 具身智能柔性关节项目 - 终极状态同步档案 V1.0
**（此文档包含该项目所有物理真理与底层基因，AI 助手请严格以此为基准，绝不可偏离！）**

## 1. 硬件架构与绝对物理基线
- **项目目标**：实现无力矩传感器的“物理柔顺控制”与“瞬间碰撞卸力（植物神经反射）”。
- **下位机 (小脑)**：HW3511 2804 智能无刷模组 (包含 STM32 + DRV8311 + 磁编码器)。
- **上位机 (大脑)**：ESP32-S3-DevKitC-1-N16R8。
- **物理接口**：严格使用 HW3511 上的 `PORT2` 接口。
- **接线规范 (核心避坑)**：
  - **红线 (3.3V)**：绝对悬空，严禁接入 ESP32！
  - **蓝线 (GND)**：接 ESP32 的 `GND`（必须共地）。
  - **绿线 (电机TX)**：接 ESP32 的 `8` 号引脚 (RX)。
  - **黄线 (电机RX)**：接 ESP32 的 `9` 号引脚 (TX)。
  - 电机采用独立 12V 动力电源供电。

## 2. 通信协议基线 (血泪踩坑总结)
- **波特率**：大小脑双向 UART 通信**严格锁定为 9600**（115200 会导致面包板严重丢包与电平翻转，绝不可改回 115200）。
- **指令格式**：电机原厂 Commander 极度严苛，接收的指令**必须以 `\n` 结尾**，绝对禁止发送 `\r\n`，否则会触发 `Warn: \r detected` 导致死锁不动。

## 3. 下位机 (小脑) 底层源码重写记录
*注：已通过 ST-Link 修改了出厂固件 `HowayFOC` 的 `main.cpp`，彻底摒弃了上位机调参，将安全与柔性基因物理刻死。*

**修改 A：`setup()` 尾部（固化柔性与使能）**
```cpp
for (size_t i = 0; i < MAX_MOTORS; i++) {
  motors[i].monitor_start_char = MOTOR_COMMANDS[i];
  motors[i].monitor_end_char = 'M'; 
  
  // 核心：彻底关闭原生监控输出，释放 10000Hz 算力，防串口拥堵卡死
  motors[i].monitor_variables = 0; 

  motors[i].torque_controller = TorqueControlType::voltage;
  motors[i].controller = MotionControlType::angle; // 开机即进入角度模式
  motors[i].target = 0.0;
  
  // 黄金阻抗参数
  motors[i].voltage_limit = 5.0;    // 力矩拉满
  motors[i].velocity_limit = 3.0;   // 优雅限速，防阶跃暴冲
  motors[i].P_angle.P = 4.0;        // 刚度适中
  motors[i].P_angle.D = 0.1;        // 阻尼防抖
  motors[i].PID_velocity.I = 2.0;   // 恢复积分，克服摩擦力，死磕到目标位置
  
  motors[i].enable(); // 开机强制通电锁死
}
```

**修改 B：`loop()` 尾部（底层植物神经：痛觉反射卸力）**
```cpp
static uint32_t faint_start_time = 0;
static bool is_fainted = false;
static int pain_counter = 0; 

if (is_fainted) {
  if (millis() - faint_start_time > 3000) { // 瘫痪 3 秒后复活
    motors[0].enable(); 
    motors[0].target = sensors[0]->getAngle(); // 醒来定在原地防暴冲
    is_fainted = false;
    Serial.println("\nWAKE_UP");
  }
} else {
  // 当内部用力 > 4.0V，且速度近乎为 0 (被死死卡住)
  if (abs(motors[0].voltage.q) > 4.0f && abs(motors[0].shaft_velocity) < 0.5f) {
    pain_counter++;
    if (pain_counter > 1000) { // 持续卡死 0.1 秒
      motors[0].disable(); // 瞬间软瘫卸力！
      is_fainted = true;
      faint_start_time = millis();
      pain_counter = 0;
      Serial.println("\nOUCH! FAINTED!"); 
    }
  } else {
    if (pain_counter > 0) pain_counter--; 
  }
}
```

## 4. 上位机 (大脑) 极简源码
*注：ESP32 仅作为透传网关与坐标调度器，不包含任何逻辑耦合。*
```cpp
#include <Arduino.h>
#define MOTOR_RX_PIN 8  
#define MOTOR_TX_PIN 9  
HardwareSerial MotorSerial(1); 

void setup() {
  Serial.begin(115200); 
  MotorSerial.begin(9600, SERIAL_8N1, MOTOR_RX_PIN, MOTOR_TX_PIN); // 对齐 9600
  Serial.println("🚀 EJCP 大脑就绪。");
}

unsigned long lastMoveTime = 0;
bool toggle = false;

void loop() {
  // 打印小脑惨叫 (OUCH/WAKE_UP)
  while (MotorSerial.available()) Serial.write(MotorSerial.read()); 
  
  if (millis() - lastMoveTime > 4000) {
    lastMoveTime = millis();
    if (toggle) MotorSerial.print("M1.57\n"); // 纯净的 \n 结尾
    else MotorSerial.print("M0\n");    
    toggle = !toggle;
  }
}
```

## 5. 调试进度与当前卡点（Context）
当前系统已完美解决：I2C死锁、红线短路、115200丢包、缓冲溢出、稳态误差卡死、自检期间发指令失效等所有底层物理 BUG。
**项目已进入最后的“运动精调与痛觉阈值标定”阶段。**
=======================================================