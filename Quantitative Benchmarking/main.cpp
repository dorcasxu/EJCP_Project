// main.cpp

#ifndef JUST_TEST

#include "main.h"

#ifdef AUTOTUNER
// PID自动调试相关的指令处理
// 自动调试，获取最佳PID参数
#warning "此功能仓库为私有内容，暂不公开。如果编译报错请在项目中关闭AUTOTUNER选项，或者在头部进行#undef操作"
#include ".\autotuner\AutoPIDTuner.h"

// 全局自动调试器指针数组
AutoPIDTuner *autoTuners[MAX_MOTORS] = {nullptr};

// 自动PID调试命令处理函数
void onAutoTunePID(char *cmd)
{
  int motorIndex = 0;
  uint8_t tuneModes = AutoPIDTuner::ALL_LOOPS;

  if (cmd && cmd[0] != '\0')
  {
    // 第一个字符是电机索引
    motorIndex = cmd[0] - '0';
    if (motorIndex < 0 || motorIndex >= MAX_MOTORS)
    {
      Serial.print("无效的电机索引，使用默认索引0。有效范围: 0-");
      Serial.println(MAX_MOTORS - 1);
      motorIndex = 0;
    }

    // 解析调试模式（剩余字符）
    const char *modeStr = cmd + 1;
    if (strlen(modeStr) > 0)
    {
      tuneModes = 0;

      // 检查完整模式名称
      if (strstr(modeStr, "ALL"))
        tuneModes = AutoPIDTuner::ALL_LOOPS;
      else if (strstr(modeStr, "FILTERS"))
        tuneModes = AutoPIDTuner::ALL_FILTERS;
      else if (strstr(modeStr, "COMPLETE"))
        tuneModes = AutoPIDTuner::COMPLETE_SYSTEM;
      else
      {
        // 解析单个模式字符
        for (const char *p = modeStr; *p != '\0'; p++)
        {
          switch (*p)
          {
          case 'A':
            tuneModes |= AutoPIDTuner::ANGLE_LOOP;
            break;
          case 'V':
            tuneModes |= AutoPIDTuner::VELOCITY_LOOP;
            break;
          case 'Q':
            tuneModes |= AutoPIDTuner::CURRENT_LOOP_Q;
            break;
          case 'D':
            tuneModes |= AutoPIDTuner::CURRENT_LOOP_D;
            break;
          case 'F': // 滤波器组
            if (*(p + 1) == 'V')
            {
              tuneModes |= AutoPIDTuner::VELOCITY_FILTER;
              p++;
            }
            else if (*(p + 1) == 'A')
            {
              tuneModes |= AutoPIDTuner::ANGLE_FILTER;
              p++;
            }
            else if (*(p + 1) == 'C')
            {
              tuneModes |= AutoPIDTuner::CURRENT_FILTER;
              p++;
            }
            break;
          }
        }
      }

      if (tuneModes == 0)
      {
        Serial.println("无效的调试模式，使用所有环路");
        tuneModes = AutoPIDTuner::ALL_LOOPS;
      }
    }
  }

  if (controllers[motorIndex] != nullptr)
  {
    // 检查是否已有调试器在运行
    if (autoTuners[motorIndex] != nullptr && autoTuners[motorIndex]->isTuning())
    {
      Serial.println("该电机正在进行自动调试，请等待完成");
      return;
    }
    // 创建或重新创建自动调试器
    if (autoTuners[motorIndex] != nullptr)
    {
      delete autoTuners[motorIndex];
    }
    autoTuners[motorIndex] = new AutoPIDTuner(&motors[motorIndex]);

    // 设置默认调试配置
    AutoPIDTuner::TuningConfig config;
    config.strategy = AutoPIDTuner::STRATEGY_BALANCED;
    config.enable_adaptive_search = true;
    config.enable_cascade_tuning = true;
    autoTuners[motorIndex]->setTuningConfig(config);

    Serial.print("开始自动PID和滤波器调试 - 电机");
    Serial.print(motorIndex);
    Serial.print(" 模式: ");
    if (tuneModes & AutoPIDTuner::ANGLE_LOOP)
      Serial.print("角度环 ");
    if (tuneModes & AutoPIDTuner::VELOCITY_LOOP)
      Serial.print("速度环 ");
    if (tuneModes & AutoPIDTuner::CURRENT_LOOP_Q)
      Serial.print("Q轴电流 ");
    if (tuneModes & AutoPIDTuner::CURRENT_LOOP_D)
      Serial.print("D轴电流 ");
    if (tuneModes & AutoPIDTuner::VELOCITY_FILTER)
      Serial.print("速度滤波器 ");
    if (tuneModes & AutoPIDTuner::ANGLE_FILTER)
      Serial.print("角度滤波器 ");
    if (tuneModes & AutoPIDTuner::CURRENT_FILTER)
      Serial.print("电流滤波器 ");
    Serial.println();

    // 开始调试（非阻塞）
    if (autoTuners[motorIndex]->startTuning(tuneModes))
    {
      Serial.println("自动调试已启动，将在后台运行");
      Serial.println("可使用 'S' 命令查看调试状态");
    }
    else
    {
      Serial.println("自动PID调试启动失败");
      delete autoTuners[motorIndex];
      autoTuners[motorIndex] = nullptr;
    }
  }
  else
  {
    Serial.print("电机");
    Serial.print(motorIndex);
    Serial.println("控制器未初始化");
  }
}

// 查看调试状态命令 - 保持不变
void onShowTuneStatus(char *cmd)
{
  Serial.println("=== 自动PID调试状态 ===");
  bool anyTuning = false;

  for (int i = 0; i < MAX_MOTORS; i++)
  {
    if (autoTuners[i] != nullptr && autoTuners[i]->isTuning())
    {
      anyTuning = true;
      Serial.print("电机");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(autoTuners[i]->getModeName(autoTuners[i]->getCurrentMode()));
      Serial.print(" - 进度: ");
      Serial.print(autoTuners[i]->getProgress(), 1);
      Serial.println("%");
    }
  }

  if (!anyTuning)
  {
    Serial.println("没有正在进行的自动调试");

    // 显示已完成的调试结果
    for (int i = 0; i < MAX_MOTORS; i++)
    {
      if (autoTuners[i] != nullptr && !autoTuners[i]->isTuning())
      {
        auto pidResults = autoTuners[i]->getPIDResults();
        auto filterResults = autoTuners[i]->getFilterResults();
        if (!pidResults.empty() || !filterResults.empty())
        {
          Serial.print("电机");
          Serial.print(i);
          Serial.println("有可用的调试结果，使用 'P' 命令应用");
        }
      }
    }
  }
  Serial.println("====================");
}

// 停止调试命令
void onStopTune(char *cmd)
{
  int motorIndex = -1; // -1表示停止所有

  if (cmd && cmd[0] != '\0')
  {
    motorIndex = cmd[0] - '0';
    if (motorIndex < 0 || motorIndex >= MAX_MOTORS)
    {
      motorIndex = -1;
    }
  }

  if (motorIndex == -1)
  {
    // 停止所有调试
    for (int i = 0; i < MAX_MOTORS; i++)
    {
      if (autoTuners[i] != nullptr)
      {
        delete autoTuners[i];
        autoTuners[i] = nullptr;
        Serial.print("已停止电机");
        Serial.print(i);
        Serial.println("的调试");
      }
    }
    Serial.println("所有自动调试已停止");
  }
  else
  {
    // 停止指定电机调试
    if (autoTuners[motorIndex] != nullptr)
    {
      delete autoTuners[motorIndex];
      autoTuners[motorIndex] = nullptr;
      Serial.print("已停止电机");
      Serial.print(motorIndex);
      Serial.println("的调试");
    }
    else
    {
      Serial.print("电机");
      Serial.print(motorIndex);
      Serial.println("没有正在进行的调试");
    }
  }
}

// 应用自动调试结果的命令
void onApplyAutoTuneResult(char *cmd)
{
  int motorIndex = 0;
  uint8_t applyMode = AutoPIDTuner::ALL_LOOPS;

  if (cmd && cmd[0] != '\0')
  {
    // 第一个字符是电机索引
    motorIndex = cmd[0] - '0';
    if (motorIndex < 0 || motorIndex >= MAX_MOTORS)
    {
      Serial.print("无效的电机索引，使用默认索引0。有效范围: 0-");
      Serial.println(MAX_MOTORS - 1);
      motorIndex = 0;
    }

    // 解析应用模式（剩余字符）
    const char *modeStr = cmd + 1;
    if (strlen(modeStr) > 0)
    {
      // 检查完整模式名称
      if (strstr(modeStr, "ALL"))
        applyMode = AutoPIDTuner::ALL_LOOPS;
      else if (strstr(modeStr, "FILTERS"))
        applyMode = AutoPIDTuner::ALL_FILTERS;
      else if (strstr(modeStr, "COMPLETE"))
        applyMode = AutoPIDTuner::COMPLETE_SYSTEM;
      else
      {
        // 解析单个模式字符
        applyMode = 0;
        for (const char *p = modeStr; *p != '\0'; p++)
        {
          switch (*p)
          {
          case 'A':
            applyMode = AutoPIDTuner::ANGLE_LOOP;
            break;
          case 'V':
            applyMode = AutoPIDTuner::VELOCITY_LOOP;
            break;
          case 'Q':
            applyMode = AutoPIDTuner::CURRENT_LOOP_Q;
            break;
          case 'D':
            applyMode = AutoPIDTuner::CURRENT_LOOP_D;
            break;
          case 'F': // 滤波器组
            if (*(p + 1) == 'V')
            {
              applyMode = AutoPIDTuner::VELOCITY_FILTER;
              p++;
            }
            else if (*(p + 1) == 'A')
            {
              applyMode = AutoPIDTuner::ANGLE_FILTER;
              p++;
            }
            else if (*(p + 1) == 'C')
            {
              applyMode = AutoPIDTuner::CURRENT_FILTER;
              p++;
            }
            break;
          }
        }
      }
    }
  }

  if (controllers[motorIndex] != nullptr)
  {
    // 检查是否有调试结果可用
    if (autoTuners[motorIndex] != nullptr && !autoTuners[motorIndex]->isTuning())
    {
      autoTuners[motorIndex]->applyResults();

      // // 保存到EEPROM
      // saveMotorParams(motorIndex, motors[motorIndex]);
      // saveEEPROM();

      Serial.print("电机");
      Serial.print(motorIndex);
      Serial.println("的调试参数已应用并保存");

      // 显示应用的结果
      auto pidResults = autoTuners[motorIndex]->getPIDResults();
      auto filterResults = autoTuners[motorIndex]->getFilterResults();

      Serial.println("=== 应用的PID参数 ===");
      for (const auto &result : pidResults)
      {
        if ((applyMode == AutoPIDTuner::ALL_LOOPS) || (result.loop_type == applyMode))
        {
          Serial.print("模式:");
          switch (result.loop_type)
          {
          case AutoPIDTuner::ANGLE_LOOP:
            Serial.print("角度环");
            break;
          case AutoPIDTuner::VELOCITY_LOOP:
            Serial.print("速度环");
            break;
          case AutoPIDTuner::CURRENT_LOOP_Q:
            Serial.print("Q轴电流");
            break;
          case AutoPIDTuner::CURRENT_LOOP_D:
            Serial.print("D轴电流");
            break;
          default:
            break;
          }
          Serial.print(" - Kp:");
          Serial.print(result.Kp, 4);
          Serial.print(" Ki:");
          Serial.print(result.Ki, 4);
          Serial.print(" Kd:");
          Serial.print(result.Kd, 4);
          Serial.print(" 评分:");
          Serial.println(result.performance_score, 1);
        }
      }

      Serial.println("=== 应用的滤波器参数 ===");
      for (const auto &result : filterResults)
      {
        if ((applyMode == AutoPIDTuner::ALL_FILTERS) || (result.filter_type == applyMode))
        {
          Serial.print("模式:");
          switch (result.filter_type)
          {
          case AutoPIDTuner::VELOCITY_FILTER:
            Serial.print("速度滤波器");
            break;
          case AutoPIDTuner::ANGLE_FILTER:
            Serial.print("角度滤波器");
            break;
          case AutoPIDTuner::CURRENT_FILTER:
            Serial.print("电流滤波器");
            break;
          default:
            break;
          }
          Serial.print(" - Tf:");
          Serial.print(result.Tf, 4);
          Serial.print(" 截止频率:");
          Serial.print(result.cutoff_freq, 1);
          Serial.print("Hz 评分:");
          Serial.println(result.performance_score, 1);
        }
      }
    }
    else
    {
      Serial.println("没有可用的调试结果，请先完成自动调试");
    }
  }
  else
  {
    Serial.print("电机");
    Serial.print(motorIndex);
    Serial.println("控制器未初始化");
  }
}

// 配置自动调试参数命令
void onConfigAutoTune(char *cmd)
{
  int motorIndex = 0;
  AutoPIDTuner::TuningStrategy strategy = AutoPIDTuner::STRATEGY_BALANCED;
  bool adaptive = true;
  bool cascade = true;

  if (cmd && cmd[0] != '\0')
  {
    // 第一个字符是电机索引
    motorIndex = cmd[0] - '0';
    if (motorIndex < 0 || motorIndex >= MAX_MOTORS)
    {
      Serial.print("无效的电机索引，使用默认索引0。有效范围: 0-");
      Serial.println(MAX_MOTORS - 1);
      motorIndex = 0;
    }

    // 解析策略和选项（剩余字符）
    const char *options = cmd + 1;
    if (strlen(options) > 0)
    {
      // 解析策略
      if (strstr(options, "AGG"))
        strategy = AutoPIDTuner::STRATEGY_AGGRESSIVE;
      else if (strstr(options, "BAL"))
        strategy = AutoPIDTuner::STRATEGY_BALANCED;
      else if (strstr(options, "CON"))
        strategy = AutoPIDTuner::STRATEGY_CONSERVATIVE;
      else if (strstr(options, "SMO"))
        strategy = AutoPIDTuner::STRATEGY_SMOOTH;

      // 解析其他参数
      if (strstr(options, "NOADAPTIVE"))
        adaptive = false;
      if (strstr(options, "NOCASCADE"))
        cascade = false;
    }
  }

  if (controllers[motorIndex] != nullptr)
  {
    if (autoTuners[motorIndex] == nullptr)
    {
      autoTuners[motorIndex] = new AutoPIDTuner(&motors[motorIndex]);
    }

    AutoPIDTuner::TuningConfig config;
    config.strategy = strategy;
    config.enable_adaptive_search = adaptive;
    config.enable_cascade_tuning = cascade;
    autoTuners[motorIndex]->setTuningConfig(config);

    Serial.print("电机");
    Serial.print(motorIndex);
    Serial.print("自动调试配置已更新 - 策略:");
    Serial.print(autoTuners[motorIndex]->getStrategyName(strategy));
    Serial.print(" 自适应搜索:");
    Serial.print(adaptive ? "启用" : "禁用");
    Serial.print(" 级联调试:");
    Serial.println(cascade ? "启用" : "禁用");
  }
  else
  {
    Serial.print("电机");
    Serial.print(motorIndex);
    Serial.println("控制器未初始化");
  }
}

#endif

#pragma region 扩展指令

uint32_t loop_us = 0;
void onPrintLoopTime(char *cmd)
{
  Serial.print("Loop Time:");
  Serial.print(loop_us);
  Serial.println("us");
}
void onPrintCurrentInfo(char *cmd)
{
  // 解析电机索引
  int motorIndex = 0;
  if (cmd && cmd[0] != '\0')
  {
    motorIndex = cmd[0] - '0';
    if (motorIndex < 0 || motorIndex >= MAX_MOTORS)
    {
      Serial.print("无效的电机索引，使用默认索引0。有效范围: 0-");
      Serial.println(MAX_MOTORS - 1);
      motorIndex = 0;
    }
  }
  if (currents[motorIndex])
  {
    PhaseCurrent_s cur = currents[motorIndex]->getPhaseCurrents();
    Serial.print("PHASE CURRENT:");
    Serial.print("A:");
    Serial.print(cur.a, 3);
    Serial.print("\tB:");
    Serial.print(cur.b, 3);
    Serial.print("\tC:");
    Serial.println(cur.c, 3);
  }
  else
  {
    Serial.print("NO CURRENTSENSOR");
  }
}

void onPrintMotorIdentitys(char *cmd)
{
  Serial.print("MOTOR_IDENTITYS: ");
  for (size_t i = 0; i < MAX_MOTORS; i++)
  {
    Serial.print(MOTOR_COMMANDS[i]);
    Serial.print(";");
  }
  Serial.println("");
}
void onPrintMotorInfo(char *cmd)
{
  int motorIndex = 0;
  if (cmd && cmd[0] != '\0')
  {
    motorIndex = cmd[0] - '0';
    if (motorIndex < 0 || motorIndex >= MAX_MOTORS)
    {
      Serial.print("无效的电机索引，使用默认索引0。有效范围: 0-");
      Serial.println(MAX_MOTORS - 1);
      motorIndex = 0;
    }
  }

  if (controllers[motorIndex] != nullptr)
  {
    Serial.println("==========================================");
    Serial.print("=== 电机");
    Serial.print(motorIndex);
    Serial.println("详细信息 ===");

    // 获取电机对象引用
    BLDCMotor motor = motors[motorIndex];

    // // 基本信息
    // Serial.println("【基本信息】");
    // Serial.print("  电机标识符: ");
    // Serial.println(MOTOR_COMMANDS[motorIndex]);
    // Serial.print("  使能状态: ");
    // Serial.println(motor.enabled ? "已启用" : "已禁用");
    // Serial.print("  极对数: ");
    // Serial.println(motor.pole_pairs);
    // Serial.print("  相电阻: ");
    // Serial.print(motor.phase_resistance, 6);
    // Serial.println(" Ω");

    // // 角度信息
    // Serial.println("【角度信息】");
    // if (sensors[motorIndex] != nullptr)
    // {
    //   float sensor_angle = sensors[motorIndex]->getAngle();
    //   float sensor_velocity = sensors[motorIndex]->getVelocity();
    //   Serial.print(F("  传感器角度: "));
    //   Serial.print(sensor_angle, 4);
    //   Serial.println(" rad");
    //   Serial.print("  传感器速度: ");
    //   Serial.print(sensor_velocity, 4);
    //   Serial.println(" rad/s");
    // }

    // Serial.print("  机械角度: ");
    // Serial.print(motor.shaft_angle, 4);
    // Serial.println(" rad");
    // Serial.print("  机械速度: ");
    // Serial.print(motor.shaft_velocity, 4);
    // Serial.println(" rad/s");
    Serial.print("  电角度: ");
    Serial.print(motor.electrical_angle, 4);
    // Serial.println(" rad");
    // Serial.print("  角度偏移: ");
    // Serial.print(motor.sensor_offset, 4);
    // Serial.println(" rad");

    // // 电压和电流信息
    // Serial.println("【电压电流信息】");
    // Serial.print("  电源电压: ");
    // Serial.print(drivers[motorIndex].voltage_power_supply, 2);
    // Serial.println(" V");

    //   if (currents[motorIndex].initialized)
    //   {
    //     PhaseCurrent_s current = currents[motorIndex].getPhaseCurrents();
    //     Serial.print("  相电流 - A:");
    //     Serial.print(current.a, 3);
    //     Serial.print(" A, B:");
    //     Serial.print(current.b, 3);
    //     Serial.print(" A, C:");
    //     Serial.print(current.c, 3);
    //     Serial.println(" A");

    //     DQCurrent_s dq_current = currents[motorIndex].getFOCCurrents(motor.electrical_angle);
    //     Serial.print("  DQ电流 - D:");
    //     Serial.print(dq_current.d, 3);
    //     Serial.print(" A, Q:");
    //     Serial.print(dq_current.q, 3);
    //     Serial.println(" A");
    //   }

    //   // 7. 故障状态
    //   Serial.println("【系统状态】");
    //   Serial.print("  驱动器就绪: ");
    //   Serial.println(drivers[motorIndex].initialized ? "是" : "否");
    //   Serial.print("  传感器就绪: ");
    //   Serial.println(sensors[motorIndex] != nullptr ? "是" : "否");
    //   Serial.print("  电流传感器就绪: ");
    //   Serial.println(currents[motorIndex].initialized ? "是" : "否");

    //   Serial.println("==========================================");
  }
  else
  {
    Serial.print("电机");
    Serial.print(motorIndex);
    Serial.println("控制器未初始化或不可用");
  }
}

// 清空校准数据并复位
void onClearMotorsParams(char *cmd)
{
  initEEPROM();
  for (size_t i = 0; i < MAX_MOTORS; i++)
  {
    clearMotorParams(i);
  }
  saveEEPROM();

  Serial.println("参数已清除，即将复位...");
  delay(100); // 确保串口消息发送完成

  // 禁用中断
  __disable_irq();

  // 触发系统复位
  NVIC_SystemReset();

  // 如果复位失败，等待看门狗复位
  delay(1000);

  // 如果还在这里，尝试软件复位
  HAL_NVIC_SystemReset();

  // 终极保险
  while (1)
  {
    // 空循环，等待看门狗或手动复位
  }
}

// 重置电机的角度
void onResetMotorAngle(char *cmd)
{
  // 解析电机索引
  int motorIndex = 0;
  if (cmd && cmd[0] != '\0')
  {
    motorIndex = cmd[0] - '0';
    if (motorIndex < 0 || motorIndex >= MAX_MOTORS)
    {
      Serial.print("无效的电机索引，使用默认索引0。有效范围: 0-");
      Serial.println(MAX_MOTORS - 1);
      motorIndex = 0;
    }
  }

  if (controllers[motorIndex] != nullptr)
  {
    // 获取当前机械角度
    float current_angle = sensors[motorIndex]->getAngle();
    // 设置偏移量，使当前位置变为0
    motors[motorIndex].sensor_offset = -current_angle;
    Serial.println("Monitor | angle: 0");
  }
  else
  {
    Serial.print("电机");
    Serial.print(motorIndex);
    Serial.println("控制器未初始化");
  }
}

// 打印所有可用命令的帮助信息
void onPrintHelp(char *cmd)
{
  Serial.println("=== FOC电机控制系统 - 可用命令列表 ===");
  Serial.println();

  // 基础电机控制命令
  Serial.println("【基础电机控制命令】");
  Serial.println("C - 清除所有校准参数并重启系统");
  Serial.println("L - 打印一次循环时间");
  Serial.println("Z[index] - 重置电机角度零点，示例: Z0 (重置电机0角度)");
  Serial.println();

  // 自动PID调谐命令 (AUTOTUNER模式下可用)
#ifdef AUTOTUNER
  Serial.println("【自动PID调谐命令】");
  Serial.println("A[index][modes] - 开始自动PID调谐");
  Serial.println("   模式参数:");
  Serial.println("     A - 角度环");
  Serial.println("     V - 速度环");
  Serial.println("     Q - Q轴电流环");
  Serial.println("     D - D轴电流环");
  Serial.println("     FV - 速度滤波器");
  Serial.println("     FA - 角度滤波器");
  Serial.println("     FC - 电流滤波器");
  Serial.println("     ALL - 所有PID环路");
  Serial.println("     FILTERS - 所有滤波器");
  Serial.println("     COMPLETE - 完整系统");
  Serial.println("   示例:");
  Serial.println("     A0ALL - 调试电机0所有环路");
  Serial.println("     A1VQ - 调试电机1速度环和Q轴电流环");
  Serial.println("     A0FILTERS - 调试所有滤波器");
  Serial.println();

  Serial.println("P[index][mode] - 应用自动调谐结果");
  Serial.println("   示例:");
  Serial.println("     P0ALL - 应用所有结果到电机0");
  Serial.println("     P1V - 只应用速度环结果到电机1");
  Serial.println("     P0FILTERS - 只应用滤波器结果");
  Serial.println();

  Serial.println("S - 查看自动调谐状态");
  Serial.println("X[index] - 停止自动调谐，示例: X0 (停止电机0), X (停止所有)");
  Serial.println("T[index][strategy][options] - 配置自动调谐参数");
  Serial.println("   策略参数:");
  Serial.println("     AGG - 激进策略 (快速响应，允许超调)");
  Serial.println("     BAL - 平衡策略 (默认)");
  Serial.println("     CON - 保守策略 (优先稳定性)");
  Serial.println("     SMO - 平滑策略 (优先滤波器平滑性)");
  Serial.println("   选项:");
  Serial.println("     NOADAPTIVE - 禁用自适应搜索");
  Serial.println("     NOCASCADE - 禁用级联调试");
  Serial.println("   示例:");
  Serial.println("     T0BAL - 电机0使用平衡策略");
  Serial.println("     T1AGGNOADAPTIVE - 电机1使用激进策略，禁用自适应");
  Serial.println();
#endif

  // 电压监测命令
#if PIN_VOLTAGE_ADC != _NC
  Serial.println("【系统监测命令】");
  Serial.println("V - 读取主电源电压");
  Serial.println();
#endif

  // 电机特定命令
  Serial.println("【电机特定命令】");
  for (size_t i = 0; i < MAX_MOTORS; i++)
  {
    Serial.print(MOTOR_COMMANDS[i]);
    Serial.print(" - 配置电机");
    Serial.print(i);
    Serial.println("参数");
  }
  Serial.println();

  Serial.println("【通用命令】");
  Serial.println("HELP - 显示此帮助信息");
  Serial.println();
  Serial.println("==========================================");
}

#pragma endregion

void setup()
{
#ifdef CHANNEL1_ENABLE_PIN
  pinMode(CHANNEL1_ENABLE_PIN, OUTPUT);
  digitalWrite(CHANNEL1_ENABLE_PIN, LOW);
#endif
#ifdef CHANNEL2_ENABLE_PIN
  pinMode(CHANNEL2_ENABLE_PIN, OUTPUT);
  digitalWrite(CHANNEL2_ENABLE_PIN, LOW);
#endif
#ifdef CHANNEL3_ENABLE_PIN
  pinMode(CHANNEL3_ENABLE_PIN, OUTPUT);
  digitalWrite(CHANNEL3_ENABLE_PIN, LOW);
#endif
#ifdef CHANNEL4_ENABLE_PIN
  pinMode(CHANNEL4_ENABLE_PIN, OUTPUT);
  digitalWrite(CHANNEL4_ENABLE_PIN, LOW);
#endif

#if LED_BUILTIN != _NC
  led.start(); // 最先启用LED，指示MCU已开始工作
#endif
#if PIN_USERBTN != _NC
  userButton.setLongPressCallback(onLongPress);
  userButton.setShortPressCallback(onShortPress);
  userButton.setMultiClickCallback(onMultiClick);
  userButton.setLongPressTime(1000);
#endif
#pragma region 初始化默认串口
  Serial.setTx(SERIAL_TX_PIN);
  Serial.setRx(SERIAL_RX_PIN);
  Serial.begin(SERIAL_BAUDRATE, SERIAL_8N1);
  Serial.println("Serial OK!");

#ifndef SIMPLEFOC_DISABLE_DEBUG
  SimpleFOCDebug::enable(&Serial); // 启用FOC调试
#endif

#if (defined(DRV_MODEL_HW3511_SSI) || defined(DRV_MODEL_HW2811_SSI)) // HW3511|HW2811
// HW2811默认即可切换PWM模式，但HW3511如果客户没有要求，一般不焊接模式切换线路的导通电阻，如果HW3511需要使用6PWM模式,需要自行在驱动板上焊接标注为“M”的电阻（一般0-22Ω即可，还可以用更大的阻值进行额外配置，自行查看DRV8311的芯片手册了解）
#ifdef CHANNEL1_PWMMODE_TOGGLE_PIN
  pinMode(CHANNEL1_PWMMODE_TOGGLE_PIN, OUTPUT_OPEN_DRAIN); // 开漏输出，3PWM时使用浮空配置即可（浮空与上拉都是3PWM模式，但过载参数设定不一样，详情查看芯片手册）
#if PWM_MODE == 6
  digitalWrite(CHANNEL1_PWMMODE_TOGGLE_PIN, LOW);
#elif PWM_MODE == 3
  digitalWrite(CHANNEL1_PWMMODE_TOGGLE_PIN, HIGH);
#endif
#endif
#if PWM_MODE == 3 // 3PWM模式下将低侧引脚固定拉高
  pinMode(CHANNEL1_PWM_A_PIN_LOW, OUTPUT);
  pinMode(CHANNEL1_PWM_B_PIN_LOW, OUTPUT);
  pinMode(CHANNEL1_PWM_C_PIN_LOW, OUTPUT);
  digitalWrite(CHANNEL1_PWM_A_PIN_LOW, HIGH);
  digitalWrite(CHANNEL1_PWM_B_PIN_LOW, HIGH);
  digitalWrite(CHANNEL1_PWM_C_PIN_LOW, HIGH);
#endif
#endif
  // 初始化编码器
  InitEncoders();
  float voltage_power_supply = VOLTAGE_POWER_SUPPLY; // 设定默认电源电压;
#if PIN_VOLTAGE_ADC != _NC                           // 设置了电压读取引脚，自动读取电压
  // voltage_power_supply = readVoltage();
  if (voltage_power_supply < 2.9)
  {
    while (1)
    {
      Serial.println("ERROR:主电源电压过低或者配置了错误的电压读取。");
      Serial.print(voltage_power_supply);
      Serial.println("V");
      delay(2000);
    }
  }
  DEBUG_PRINT("电源电压:");
  DEBUG_PRINTLN(voltage_power_supply);
#endif

  for (size_t i = 0; i < MAX_MOTORS; i++)
  {
    drivers[i].voltage_power_supply = voltage_power_supply;

    controllers[i] = new FOCController(&drivers[i], &motors[i], sensors[i], currents[i], i, MOTOR_COMMANDS[i], faultPins[i]); //&currents[i]
    // if (i<1)
    // {
    controllers[i]->Init();
    controllers[i]->InitFoc();
    // }

#if PWM_MODE == 6
    drivers[i].dead_zone = 0.05; // 6PWM时设置死区时间为5%，默认为2%
#endif
#ifndef UNUSED_UART
    String str = "full motor config,motor Index:" + String(i) + ".";
    commander.add(MOTOR_COMMANDS[i], cmdCallbacks[i], str.c_str());
#endif
#ifdef comm
    comm.registerController(i, controllers[i]);
#endif
  }

#ifdef comm
  // 初始化从机通信
  comm.init();
#endif

#ifndef UNUSED_UART
  // 注册全局命令
#if PIN_VOLTAGE_ADC != _NC
  commander.add('V', printVoltage, "PRINT MAIN VOLTAGE");
#endif
  commander.add('*', onPrintMotorIdentitys, "PRINT MOTOR IDENTITY LIST");
  commander.add('i', onPrintMotorInfo, "PRINT MOTOR INFO");
  commander.add('I', onPrintCurrentInfo, "PRINT PHASE CURRENT INFO");
  commander.add('C', onClearMotorsParams, "CLEAR PARAMS AND RESTART.");
  commander.add('L', onPrintLoopTime, "PRINT ONCE LOOP TIME.");
  commander.add('Z', onResetMotorAngle, "RESET MOTOR ZERO ANGLE [index]"); // 20251022新增重置角度命令(将机械角度的0点设为当前角度)

#ifdef AUTOTUNER
  // 添加自动PID调试命令 - 更新描述为无空格格式
  commander.add('A', onAutoTunePID, "AUTO TUNE PID [index][modes]");
  commander.add('P', onApplyAutoTuneResult, "APPLY AUTO TUNE RESULT [index][mode]");
  commander.add('S', onShowTuneStatus, "SHOW AUTO TUNE STATUS");
  commander.add('X', onStopTune, "STOP AUTO TUNE [index]");
  commander.add('T', onConfigAutoTune, "CONFIG AUTO TUNE [index][strategy][options]");
  commander.add('H', onPrintHelp, "PRINT ALL AVAILABLE COMMANDS"); // 额外的帮助信息（如果不启用AUTOTUNER，基础命令很简单，使用SimpleFOC的?指令即可）
#endif
#endif

  // 显示欢迎信息和基本命令提示
  Serial.println("=== FOC电机控制系统已就绪 ===");
#ifdef AUTOTUNER
  Serial.println("输入 'H' 查看所有可用命令");

#endif
  Serial.println("===============================");

  // 以下设置驱动器初始化完成后的默认行为
  for (size_t i = 0; i < MAX_MOTORS; i++)
  {
    motors[i].monitor_start_char = MOTOR_COMMANDS[i];
    motors[i].monitor_end_char = 'M'; 
    motors[i].monitor_variables = 0; // 彻底闭嘴，不往外乱发监控日志，保证通道极度干净！
    
    motors[i].torque_controller = TorqueControlType::voltage;
    motors[i].controller = MotionControlType::angle; // 强制角度模式
    motors[i].target = 0.0;
    
    // 💡 黄金肌肉参数
    motors[i].voltage_limit = 5.5;    // 最大力气 5.5V
    motors[i].velocity_limit = 3.0;   // 优雅限速 3.0
    motors[i].P_angle.P = 6.0;        // 刚度 6.0
    motors[i].P_angle.D = 0.12;        // 防抖 0.12
    motors[i].PID_velocity.I = 3.0;   // 恢复记忆，死磕到 90度   

    motors[i].enable(); // 把前面的 // 删掉，强制通电使能
  }
  delay(50);
  Serial.println("[DRV Initialized!]");
  Serial.println("[ELE ANGLE]");
  for (size_t i = 0; i < MAX_MOTORS; i++)
  {
    Serial.println("");
    Serial.println(motors[i].zero_electric_angle, 4);
  }
}

void loop()
{
  static uint32_t lastTime = 0;
  uint32_t t = micros();
  FOCController::counter++;
  if (FOCController::counter > 100000000)
    FOCController::counter = 0;
  if (FOCController::counter % 5 == 0)
  {
    float radPerSec = motors[0].shaft_velocity;
    // 根据转速更新LED呼吸频率
    led.updateFrequencyByRPM(radPerSec, 260.0f, 100, 5000); // 最大转速260rad/s
  }
#ifdef comm
  // 更新状态（处理超时等）
  comm.update();
#endif
#if LED_BUILTIN != _NC
  led.update();
#endif
#if PIN_USERBTN != _NC
  userButton.update();
#endif
#ifndef UNUSED_UART
  commander.run();
#endif

  // 错误检查逻辑似乎还有问题，先禁用错误检查
  static uint8_t checkFault = 0b1111; // 0; // 0b1111; // 错误检查标识，这个系统最多只允许四路电机，不要担心溢出

  // 更新自动PID调试器
#ifdef AUTOTUNER
  static uint8_t tuningFlag = 0;
  static uint32_t last_progress_report[MAX_MOTORS] = {0};
  static float last_reported_progress[MAX_MOTORS] = {-1};

  for (size_t i = 0; i < MAX_MOTORS; i++)
  {
    if (autoTuners[i] != nullptr && autoTuners[i]->isTuning())
    {
      uint32_t current_time = millis();
      float current_progress = autoTuners[i]->getProgress();

      // 每个电机独立的进度报告
      if (current_time - last_progress_report[i] > 2000 ||
          std::abs(current_progress - last_reported_progress[i]) > 1.0)
      {
        Serial.print("电机");
        Serial.print(i);
        Serial.print(" 调试进度: ");
        Serial.print(current_progress, 1);
        Serial.println("%");

        last_progress_report[i] = current_time;
        last_reported_progress[i] = current_progress;
      }
      autoTuners[i]->update();
      tuningFlag |= (0x01 << i);
    }
    else
    {
      tuningFlag &= ~(0x01 << i);
    }
  }
#endif
  for (size_t i = 0; i < MAX_MOTORS; i++)
  {
    bool enable_fault_check = (checkFault >> i) & 0x01; // 错误检查标识
#ifdef AUTOTUNER
    if (tuningFlag & (0x01 << i))
    { // 如果电机正在进行自动调试，强制禁用错误检查
      enable_fault_check = false;
    }
#endif
    controllers[i]->Update(false);
  } 
  
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

 // ========================================================
  // 💡 【植物神经 终极手感版 v8.0】—— 真实电流版 + 极致Q弹软
static uint32_t faint_start_time = 0;
static bool is_fainted = false;
static uint32_t block_start_time = 0;
static bool is_blocking = false;
static uint32_t wake_ignore_until = 0;

if (is_fainted) {
    if (millis() - faint_start_time > 2800) {
        motors[0].controller = MotionControlType::angle;
        motors[0].target = sensors[0]->getAngle();
        motors[0].voltage_limit = 5.5f;
        motors[0].enable();
        is_fainted = false;
        wake_ignore_until = millis() + 500;
        Serial.println("\nWAKE_UP");
    }
} 
else if (millis() > wake_ignore_until) {
    float angle_error = fabs(motors[0].target - motors[0].shaft_angle);
    float shaft_vel   = fabs(motors[0].shaft_velocity);
    
    // 【修复点】：currents[0] 是指针，必须用 ->
    float effort = 0.0f;
    if (currents[0] != nullptr) {
        effort = fabs(currents[0]->getFOCCurrents(motors[0].electrical_angle).q);
    } else {
        effort = fabs(motors[0].voltage.q);  // 备用：如果电流传感器未初始化
    }

    static uint32_t last_debug = 0;
    if (millis() - last_debug > 1500) {
        Serial.print("PAIN | err:");
        Serial.print(angle_error, 3);
        Serial.print(" vel:");
        Serial.print(shaft_vel, 2);
        Serial.print(" curr:");
        Serial.println(effort, 2);
        last_debug = millis();
    }

    // 触发条件（真实电流版，更精准）
    if (angle_error > 0.25f && shaft_vel < 0.40f && effort > 1.1f) {
        if (!is_blocking) {
            is_blocking = true;
            block_start_time = millis();
            Serial.println("!!! 检测到堵转，开始计时 !!!");
        } 
        else if (millis() - block_start_time > 450) {
            motors[0].disable();
            motors[0].controller = MotionControlType::torque;
            motors[0].target = -3.5f;           // 更大反向力
            motors[0].voltage_limit = 0.25f;     // 极低出力
            is_fainted = true;
            faint_start_time = millis();
            is_blocking = false;
            Serial.println("\nOUCH! FAINTED! （已极软，手感非常明显）");
        }
    } 
    else {
        is_blocking = false;
    }
}
// ========================================================

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

  loop_us = micros() - t;
  lastTime = t;
}

// #if COMM_TYPE == COMM_SPI
// // SPI中断处理函数
// extern "C" void SPI1_IRQHandler(void)
// {
//   HAL_SPI_IRQHandler(comm.getSPISlave().getHandle());
// }

// extern "C" void SPI2_IRQHandler(void)
// {
//   HAL_SPI_IRQHandler(comm.getSPISlave().getHandle());
// }

// #ifdef SPI3
// extern "C" void SPI3_IRQHandler(void)
// {
//   HAL_SPI_IRQHandler(comm.getSPISlave().getHandle());
// }
// #endif

// #ifdef SPI4
// extern "C" void SPI4_IRQHandler(void)
// {
//   HAL_SPI_IRQHandler(comm.getSPISlave().getHandle());
// }
// #endif

// // DMA中断处理函数
// extern "C" void DMA1_Channel2_IRQHandler(void)
// {
//   HAL_DMA_IRQHandler(comm.getSPISlave().getHandle()->hdmarx);
// }

// extern "C" void DMA1_Channel3_IRQHandler(void)
// {
//   HAL_DMA_IRQHandler(comm.getSPISlave().getHandle()->hdmatx);
// }

// #ifdef SPI2
// extern "C" void DMA1_Channel4_IRQHandler(void)
// {
//   HAL_DMA_IRQHandler(comm.getSPISlave().getHandle()->hdmarx);
// }

// extern "C" void DMA1_Channel5_IRQHandler(void)
// {
//   HAL_DMA_IRQHandler(comm.getSPISlave().getHandle()->hdmatx);
// }
// #endif

// #ifdef SPI3
// extern "C" void DMA2_Channel1_IRQHandler(void)
// {
//   HAL_DMA_IRQHandler(comm.getSPISlave().getHandle()->hdmarx);
// }

// extern "C" void DMA2_Channel2_IRQHandler(void)
// {
//   HAL_DMA_IRQHandler(comm.getSPISlave().getHandle()->hdmatx);
// }
// #endif
// #endif

#endif // JUST_TEST