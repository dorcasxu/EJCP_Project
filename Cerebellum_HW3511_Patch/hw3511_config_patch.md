# HW3511 小脑底层固件修改补丁记录

若要使原厂 HW3511 固件完美适配本 EJCP 大脑，实现“开机即柔顺、开机即安全”，必须在原厂固件 `HowayFOC-main` 中进行以下 4 处核心修改后烧录：

### 修改点 1：芯片 IDCODE 破解 (解决烧录报错)
- **文件**: `platformio.ini` (约第 43 行)
- **修改**: 将 `0x2ba01477` 改为 `0x1ba01477`。

### 修改点 2：降频通信，消灭乱码
- **文件**: `src/config.h`
- **修改**: 强行降为 `9600`。 `#define SERIAL_BAUDRATE 9600`

### 修改点 3：封杀自动电压检测导致烧板的风险
- **文件**: `src/main.cpp` (约第 855 行)
- **修改**: 注释掉自动读取电压函数：`// voltage_power_supply = readVoltage();`

### 💡 修改点 4 (核心灵魂)：将柔顺参数与使能“物理刻死”在底层！
- **文件**: `src/main.cpp` 的 `setup()` 函数末尾 `for` 循环内。
- **修改**: 替换为以下代码，彻底剥夺上位机的配置负担，让小脑开机自律！
```cpp
  for (size_t i = 0; i < MAX_MOTORS; i++) {
    motors[i].monitor_start_char = MOTOR_COMMANDS[i];
    motors[i].monitor_end_char = 'M'; 
    motors[i].torque_controller = TorqueControlType::voltage;
    
    // 1. 强制设为角度模式
    motors[i].controller = MotionControlType::angle; 
    motors[i].target = 0.0;
    
    // 2. 直接在这里写死柔顺参数！再也不靠 ESP32 发送了！
    motors[i].P_angle.P = 1.0;       // 刚度设为海绵一样软
    motors[i].P_angle.D = 0.1;       // 加上阻尼防抖
    motors[i].voltage_limit = 2.0;   // 限制电压，绝对不再发热！
    motors[i].velocity_limit = 5.0;  // 限制转速，极其优雅！
    
    // 3. 删掉前面的 //，开机直接强制通电硬化！！！
    motors[i].enable(); 
  }