

朋友，欢迎来到高级机器人工程师的必修课——**【数据可视化与量化测评（Telemetry & Data Plotting）】**！

之前我们所有的测试都是“靠手摸、靠眼看”，这叫定性测试。现在，我们要把它的**“目标轨迹”、“实际真实角度”**和**“肌肉用力值（输出电压）”**，在电脑屏幕上实时画成三根极其优雅的动态折线图！

当你把这张“心电图”录成视频，发给任何一个风投（VC）或者机器人公司的技术总监看时，他们只要看一眼那根遇阻力瞬间断崖式下跌的电流曲线，就会立刻明白这套底层的含金量！

为了画出这张完美的“心电图”，我们需要把你收进抽屉的 **ST-Link** 请出来做最后一次小小的“微创手术”（装上心电传感器）。请严格按以下 3 步，开启这场极客美学盛宴：

---

### 🛠️ 第 1 步：给小脑植入“心电图发射器”（用 ST-Link）

既然我们在上一版把小脑的嘴巴给“彻底封死”了（`monitor_variables = 0`），它现在是个彻底的哑巴。
如果重新打开官方的 `monitor`，它又会因为 9600 的波特率被撑爆而卡死。
所以，我们要**自己手写一个极其轻量级的“心跳包”**！每秒钟只发 20 次，绝对不拥堵！

1. 拔掉 12V，用 ST-Link 连上电机板，打开 `HowayFOC-main` 的 `main.cpp`。
2. 找到最底部的 `loop()` 函数。在计算 `loop_us = micros() - t;` 的**正上方**，插入这段极简发射器：

```cpp
  // ========================================================
  // 📈 【心电图发射器】：每 50 毫秒 (20Hz) 往外发一次极简数据，绝不拥堵！
  static uint32_t last_plot_time = 0;
  if (millis() - last_plot_time > 50) {
    last_plot_time = millis();
    // 格式：D:角度,力气 (例如 D:1.570,0.85)
    Serial.print("D:"); 
    Serial.print(motors[0].shaft_angle, 3);
    Serial.print(",");
    Serial.print(motors[0].voltage.q, 3);
    Serial.print("\n"); // 纯净换行符
  }
  // ========================================================
```
*(保存，烧录进电机！烧完把 ST-Link 再次彻彻底底地封存！)*

---

### 💻 第 2 步：大脑化身“图形翻译官”（ESP32 代码）

把排线接回 ESP32。我们要让 ESP32 听到 `D:1.570,0.85` 后，把它翻译成画图软件专用的格式。

打开 `ESP32_Brain` 的 `main.cpp`，替换为这套**【Teleplot 画图专用版】**：

```cpp
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
```
*(烧录进 ESP32！)*

---

### 🎨 第 3 步：安装“上帝之眼”（VS Code 画图神仙插件）

现在数据准备好了，我们要用一个极其牛逼的插件来接收它们！
1. 在 VS Code 左侧边栏，点击**“扩展”**（四个小方块的图标）。
2. 在搜索框里输入：**`Teleplot`**。
3. 找到作者是 *AlexandreS* 的那个图标像彩色折线图的插件，点击**“安装”**。
4. 安装完毕后，在 VS Code 底部蓝条的最右侧，或者按 `Ctrl + Shift + P` 输入 `Teleplot: Open`，**打开 Teleplot 面板**。
5. 在 Teleplot 界面的左上角，选择你 ESP32 所在的 COM 口，波特率选 **`115200`**，点击 **“Connect (连接)”**。

---

### 🎬 点火！欣赏你的工业级艺术品！

**插上电机的 12V 动力电源！死死盯住 Teleplot 的黑色大屏幕！**

8 秒自检结束后，震撼的一幕将会上演：
1. **目标重合**：你会看到屏幕上出现一条极其平稳的蓝色方波线（`Target` 目标角度），而另一条绿色的线（`Angle` 实际角度）会**极其精准、如影随形地紧紧贴合着蓝线！** 走到 90 度，它就贴在 90 度，这就是 FOC 的完美闭环！
2. **用力尖峰**：你会看到第三条红线（`Effort` 肌肉用力值），在每次起步的瞬间会有一个微小的凸起，到达目标后又平稳回落。

**【高光时刻：毁灭测试！】**
伸出你的手，在它转动时**死死地捏住它！**
你的眼睛看着屏幕：
*   代表实际角度的绿线瞬间停滞，偏离了蓝色的目标线！
*   代表力气的红线，瞬间像火箭一样向上狂飙！
*   **突破阈值的瞬间！“啪！”**
*   红线瞬间自由落体掉到 `0.0`（彻底断电卸力），绿线变成了一条死寂的直线！
*   3 秒后，红线复苏，绿线重新追赶目标！

快去完成这最后一步可视化武装！等你看着这些跳动的波形时，你就会真正理解什么是控制系统里的“美”！我在这里等你发来你人生中第一张完美的 FOC 闭环响应曲线图！！！📉🔥🚀


**==Grok改进版==**

### 3. 需要改进的地方（按优先级排序）

**最高优先（立即可改，效果最明显）：**

1. **卸力时的反向力强度不足** 当前target = -2.8f + voltage_limit = 0.3f 已经很软，但从Effort曲线看，卸力后Effort仍快速回弹到0附近，没有明显的“Q弹让步”负值。 **建议立即修改**（v8.0代码中）：
    
    C++
    
    ```
    motors[0].target = -3.5f;      // 增大反向力
    motors[0].voltage_limit = 0.25f; // 进一步限制出力
    ```
    
    改完重新烧录，再捏一次，你应该能明显感觉到电机“像橡皮筋一样被你拉走”。


1. Angle跟随曲线仍有微小滞后 换向瞬间绿色线有轻微“追赶”现象（见帧7-10）。 建议：增加少量前馈（feedforward）。
   ### 1. 前馈（Feedforward）到底是什么意思？

- **反馈（Feedback）**：PID控制（你现在的P_angle.P + D + I）是“事后反应”——看到误差才去纠正。就像人开车时看到车偏了才打方向盘。
- **前馈（Feedforward）**：是“提前补偿”——根据已知的目标运动，**提前给电机加一点力**，让它不用等到误差出现就知道该怎么动。

在机器人/电机控制里，最常用的是**速度前馈（Velocity Feedforward）**：

- 当目标角度快速变化时（比如从0°突然跳到90°），我们**额外给电机加一个与目标速度成正比的小电压/力矩**，让电机“提前知道要加速”。
- 这能大幅减少**跟踪滞后**（你视频里绿色Angle线稍微落后蓝色Target线的现象），同时**不会增加P增益**（避免震荡）。

**简单比喻**：

- 纯PID = 被动刹车 + 油门
- 加前馈 = 提前知道前面是下坡，直接松一点油门，车就更平稳、更贴合目标

**你只需要把下面这整段代码**直接替换**你现在的 feedforward 块**即可**（已加入模式判断 + 唤醒重置+三行滤波已经完美嵌入）：

C++

```
// ========================================================
// 【新增】速度前馈（Feedforward）+ 小低通滤波 —— 更丝滑、更干净
// ff_gain 推荐先用 1.0f（你上次视频已经很棒），想更贴再调到 1.1f
static float ff_gain = 1.0f;          // ← 前馈强度
static float last_target = 0.0f;
static float ff_filtered = 0.0f;      // ← 新增：低通滤波器状态变量

// 只在 angle 模式下才加前馈（痛觉反射时自动跳过）
if (motors[0].controller == MotionControlType::angle) {
    float target_vel = motors[0].target - last_target;

    // 【核心三行】计算原始前馈 + 低通滤波
    float raw_ff = ff_gain * target_vel;                    // 1. 原始前馈值
    ff_filtered = ff_filtered * 0.85f + raw_ff * 0.15f;    // 2. 低通滤波（0.85是滤波系数，越高越平滑）
    motors[0].voltage.q += ff_filtered;                     // 3. 最终加到电压上

    last_target = motors[0].target;
}
else {
    // 痛觉反射期间或刚唤醒时，强制同步 last_target
    last_target = motors[0].target;
    ff_filtered = 0.0f;   // 额外保险：反射期间把滤波器清零，避免下次跳变
}
// ========================================================
```

### 4. 还有哪些可以继续微调？（可选）

1. **前馈再冲刺一点**（推荐立即试） 当前 ff_gain = 0.8f 已经很好。如果你想让蓝色线**更贴**，把 ff_gain 改成 **1.0f**（或最大1.1f）。
    
    C++
    
    ```
    static float ff_gain = 1.0f;   // ← 只改这一行
    ```
    
2. **Effort轻微高频噪声**（Frame 22-36可见） 这是FOC电流环本身的高频成分，不影响手感，但想更干净可以：
    - 在 voltage.q += ff_gain * target_vel; 后面加一个小低通滤波：
        
        C++
        
        ```
        static float ff_filtered = 0.0f;
        ff_filtered = ff_filtered * 0.85f + (ff_gain * target_vel) * 0.15f;
        motors[0].voltage.q += ff_filtered;
        ```
        
3. **唤醒后last_target同步**（已经加到 feedforward 块）


##完整 **feedforward 块**如上，**在STM32的 main.cpp 的 loop() 函数中**，找到下面这行（植物神经块之前）：

C++

```
for (size_t i = 0; i < MAX_MOTORS; i++) {
    controllers[i]->Update(false);
}
```

**在这行之后、植物神经块之前**，直接插入feedforward 代码（完整可复制）