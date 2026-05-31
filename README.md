# EJCP (Embodied Joint Control Platform)
A 6-DOF, low-cost, high-precision desktop robotic arm designed as a physical data collection terminal for TinyML and imitation learning algorithms (e.g., ACT).

## 📺 Project Demo & Community Showcase
- **Zero-Latency Master-Slave Teleoperation System (0:11 Video)**: 

https://github.com/user-attachments/assets/5cbcba76-0f2d-49d3-8a41-b010c96eaf8a

- **Reddit Robotics Community Discussion (6.3K+ Views)**: https://www.reddit.com/r/robotics/comments/1sy2xor/i_built_a_zerolatency_masterslave_teleoperation/

### 🛠 3D Mechanical Design Overview (PTC Creo)
EJCP 3D Arm Design

<img width="3508" height="2480" alt="0-00" src="https://github.com/user-attachments/assets/0133ff82-05ed-435a-b88e-e47348e5b19f" />


*Featuring the dual-cantilever modular joints, GT2 timing belt reduction system, and integrated BLDC motors with magnetic encoders.*

---

## 🚀 Advanced Technical Progress Report (Phase 3 Re-architecture)

### 1. Powertrain & Mechanical Overhaul
- **Heavy-Duty Joints (J1 Base, J2 Shoulder, J3 Elbow)**: Upgraded to higher-torque **4015 brushless motors** (24-slot 22-pole, 11 pole pairs, 12V-24V, 0.35Nm rated torque) utilizing a 1:5 GT2 timing belt reduction system.
- **Wrist Joints (J4 Pitch, J5 Roll)**: Maintained lightweight **2804 motors** (12-slot 14-pole, 7 pole pairs).
- **Structure**: Modeled using Top-Down assembly design in PTC Creo. Replaced 3D-printed links with **2020 European standard aluminum profiles** to enhance structural rigidity.
- **Kinematic Optimization**: Implemented a **"Z-shape folding offset compensation"** assembly design to align the end-effector's rotation axis perfectly with the J1 base center vertical baseline, eliminating lateral bending moments and simplifying Inverse Kinematics (IK) solvers.

### 2. Electronics & Sensor Upgrade
- **Drivers**: Integrated **ELESAI ESP32 single-channel 20A high-current driver boards** (3x IR2104 + 6 discrete NMOS FETs) to safely handle the high loads of 4015 motors.
- **Feedback Resolution**: Upgraded high-load joints to **MT6701 magnetic encoders (14-bit precision)**, providing a 4x resolution increase compared to AS5600 (12-bit).
- **End-Effector (J6)**: Equipped with a **FEETECH STS3215 12V 30kg serial bus servo** coupled with a 3D-printed parallel guide gripper, supporting position and current back-reading for force-feedback.

### 3. Control Algorithms & EMI Mitigation
- **Unwrap Algorithm**: Programmed a continuous multi-turn angular expansion algorithm on the central controller to eliminate motor reversal/instability when the absolute encoder crosses the $0 \sim 2\pi$ boundary on the 1:5 belt-reduction system.
- **Gravity Feedforward Compensation**: Introduced an active control law $V_{ff} = G_{coeff} \cdot \cos(\theta)$ to cancel out static gravity effects, achieving a smooth, weightless "space-floating" manual feel for high-quality imitation learning datasets.
- **EMI Hardware Defense**: Implemented 100nF ceramic decoupling capacitors on each MT6701 VCC-GND line, lowered I2C to 100kHz for enhanced noise immunity, and isolated SDA/SCL via twisted-pair routing away from 20A motor phase lines.
