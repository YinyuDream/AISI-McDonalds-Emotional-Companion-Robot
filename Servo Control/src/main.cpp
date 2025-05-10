#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// 创建两个 PCA9685 对象
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40); // PCA9685板1，地址0x40
Adafruit_PWMServoDriver pwm2 = Adafruit_PWMServoDriver(0x41); // PCA9685板2，地址0x41

// SG90舵机特定的脉冲宽度范围 (单位: 微秒)
#define SG90_MIN 500  // SG90舵机最小脉冲宽度
#define SG90_MAX 2500 // SG90舵机最大脉冲宽度
#define SG90_FREQ 50  // SG90舵机PWM频率 (Hz)

#define DELAY_TIME 50 // 舵机动作之间的延迟时间 (毫秒)

uint8_t state = 0; // 当前表情状态: 0:netural, 1:happiness, 2:sadness, 3:surprise
#define BLINK_INTERVAL 3000 // 自动眨眼间隔时间 (毫秒)
unsigned long previousBlinkMillis = 0; // 上一次眨眼的时间戳

// 设置SG90舵机角度
// servoNum: 舵机编号 (0-15)
// angle: 目标角度 (0-180)
// pca_board: PCA9685板编号 (1 或 2)
void setSG90Angle(uint8_t servoNum, uint16_t angle, uint8_t pca_board) {
  uint16_t pulse = map(angle, 0, 180, SG90_MIN, SG90_MAX);
  uint16_t ticks = pulse * (SG90_FREQ * 4096.0 / 1000000.0); // 将脉冲宽度转换为PCA9685的tick值
  if (pca_board == 1) {
    pwm1.setPWM(servoNum, 0, ticks);
  } else {
    pwm2.setPWM(servoNum, 0, ticks);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("21 SG90 Servo Control with 2x PCA9685 Initialized");
  pwm1.begin();
  pwm2.begin();
  pwm1.setPWMFreq(SG90_FREQ); // 设置PWM频率
  pwm2.setPWMFreq(SG90_FREQ); // 设置PWM频率
  delay(10); // 等待PCA9685稳定
}

// 自然表情
void netural()
{
  // --- PCA1 控制的舵机 ---
  // 4 (PCA1): 60 - 100 degrees, 眼球左右, 60: 向左, 100: 向右
  setSG90Angle(4, 80, 1); // 眼球居中或微向右
  delay(DELAY_TIME);
  // 5 (PCA1): 160 - 180 degrees, 眼球上下, 160: 向下, 180: 向上
  setSG90Angle(5, 170, 1); // 眼球居中或微向上
  delay(DELAY_TIME);
  // 6 (PCA1): 95 - 170 degrees, 左上眼皮, 95: 向下(闭合), 170: 向上(张开)
  setSG90Angle(6, 120, 1);
  delay(DELAY_TIME);
  // 7 (PCA1): 55 - 95 degrees, 左下眼皮, 55: 向下(张开), 95: 向上(闭合)
  setSG90Angle(7, 60, 1);
  delay(DELAY_TIME);
  // 8 (PCA1): 0 - 70 degrees, 右上眼皮, 0: 向上(张开), 70: 向下(闭合)
  setSG90Angle(8, 30, 1);
  delay(DELAY_TIME);
  // 9 (PCA1): 90 - 130 degrees, 右下眼皮, 90: 向上(闭合), 130: 向下(张开)
  setSG90Angle(9, 125, 1);
  delay(DELAY_TIME);
  
  // 12 (PCA1): 0 - 50 degrees, 左侧正眉毛, 0: 向上, 50: 向下
  setSG90Angle(12, 25, 1);
  delay(DELAY_TIME);
  // 13 (PCA1): 0 - 60 degrees, 左侧斜眉毛, 0: 向下, 60: 向上
  setSG90Angle(13, 30, 1); 
  delay(DELAY_TIME);
  // 14 (PCA1): 0 - 30 degrees, 右侧正眉毛, 0: 向下, 30: 向上
  setSG90Angle(14, 15, 1);
  delay(DELAY_TIME);
  // 15 (PCA1): 0 - 60 degrees, 右侧斜眉毛, 0: 向上, 60: 向下
  setSG90Angle(15, 30, 1);
  delay(DELAY_TIME);
  
  // --- PCA2 控制的舵机 ---
  // 6 (PCA2): 70 - 140 degrees, 左下嘴, 70: 向下, 140: 向上
  setSG90Angle(6, 110, 2);
  delay(DELAY_TIME);
  // 7 (PCA2): 0 - 70 degrees, 右下嘴, 0: 向上, 70: 向下
  setSG90Angle(7, 35, 2);
  delay(DELAY_TIME);
  // 9 (PCA2): 150 - 190 degrees, 左上嘴, 150: 向下, 190: 向上
  setSG90Angle(9, 180, 2);
  delay(DELAY_TIME);
  // 10 (PCA2):150 - 180 degrees, 右中嘴(upper), 150: 向上, 180: 向下
  setSG90Angle(10, 150, 2);
  delay(DELAY_TIME);
  // 11 (PCA2): 150-190 degrees, 左中嘴(upper), 150: 向下, 190: 向上
  setSG90Angle(11, 180, 2);
  delay(DELAY_TIME);
  // 12 (PCA2): 0 - 40 degrees, 右上嘴, 0: 向上, 40: 向下
  setSG90Angle(12, 20, 2);
  delay(DELAY_TIME);
  // 13 (PCA2): 60 - 100 degrees, 右中嘴(low), 60: 向上, 100: 向下
  setSG90Angle(13, 90, 2);
  delay(DELAY_TIME);
  // 14 (PCA2): 80 - 130 degrees, 嘴张闭, 80: 向上(闭合), 130: 向下(张开)
  setSG90Angle(14, 80, 2); // 嘴巴闭合
  delay(DELAY_TIME);
  // 15 (PCA2): 140-180 degrees, 左中嘴(low), 140: 向下, 180: 向上
  setSG90Angle(15, 140, 2);
  delay(DELAY_TIME);
}

// 开心表情
void happiness()
{
  // --- PCA1 控制的舵机 (眼部和眉毛) ---
  // 4 (PCA1): 60 - 100 degrees, 眼球左右, 60: 向左, 100: 向右
  setSG90Angle(4, 80, 1); 
  delay(DELAY_TIME);
  // 5 (PCA1): 160 - 180 degrees, 眼球上下, 160: 向下, 180: 向上
  setSG90Angle(5, 180, 1); // 眼睛向上看
  delay(DELAY_TIME);
  // 6 (PCA1): 95 - 170 degrees, 左上眼皮, 95: 向下(闭合), 170: 向上(张开)
  setSG90Angle(6, 150, 1); // 眼皮张开
  delay(DELAY_TIME);
  // 7 (PCA1): 55 - 95 degrees, 左下眼皮, 55: 向下(张开), 95: 向上(闭合)
  setSG90Angle(7, 55, 1); // 眼皮张开
  delay(DELAY_TIME);
  // 8 (PCA1): 0 - 70 degrees, 右上眼皮, 0: 向上(张开), 70: 向下(闭合)
  setSG90Angle(8, 20, 1);  // 眼皮张开
  delay(DELAY_TIME);
  // 9 (PCA1): 90 - 130 degrees, 右下眼皮, 90: 向上(闭合), 130: 向下(张开)
  setSG90Angle(9, 130, 1); // 眼皮张开
  delay(DELAY_TIME);
  
  // 12 (PCA1): 0 - 50 degrees, 左侧正眉毛, 0: 向上, 50: 向下
  setSG90Angle(12, 0, 1);  // 眉毛上扬
  delay(DELAY_TIME);
  // 13 (PCA1): 0 - 60 degrees, 左侧斜眉毛, 0: 向下, 60: 向上
  setSG90Angle(13, 60, 1); // 眉毛上扬
  delay(DELAY_TIME);
  // 14 (PCA1): 0 - 30 degrees, 右侧正眉毛, 0: 向下, 30: 向上
  setSG90Angle(14, 30, 1); // 眉毛上扬
  delay(DELAY_TIME);
  // 15 (PCA1): 0 - 60 degrees, 右侧斜眉毛, 0: 向上, 60: 向下
  setSG90Angle(15, 0, 1);  // 眉毛上扬
  delay(DELAY_TIME);
  
  // --- PCA2 控制的舵机 (嘴部) ---
  // 6 (PCA2): 70 - 140 degrees, 左下嘴, 70: 向下, 140: 向上
  setSG90Angle(6, 140, 2); //嘴角上扬
  delay(DELAY_TIME);
  // 7 (PCA2): 0 - 70 degrees, 右下嘴, 0: 向上, 70: 向下
  setSG90Angle(7, 0, 2);   //嘴角上扬
  delay(DELAY_TIME);
  // 9 (PCA2): 150 - 190 degrees, 左上嘴, 150: 向下, 190: 向上
  setSG90Angle(9, 190, 2);
  delay(DELAY_TIME);
  // 10 (PCA2):150 - 180 degrees, 右中嘴(upper), 150: 向上, 180: 向下
  setSG90Angle(10, 150, 2);
  delay(DELAY_TIME);
  // 11 (PCA2): 150-190 degrees, 左中嘴(upper), 150: 向下, 190: 向上
  setSG90Angle(11, 190, 2);
  delay(DELAY_TIME);
  // 12 (PCA2): 0 - 40 degrees, 右上嘴, 0: 向上, 40: 向下
  setSG90Angle(12, 0, 2);
  delay(DELAY_TIME);
  // 13 (PCA2): 60 - 100 degrees, 右中嘴(low), 60: 向上, 100: 向下
  setSG90Angle(13, 60, 2);
  delay(DELAY_TIME);
  // 14 (PCA2): 80 - 130 degrees, 嘴张闭, 80: 向上(闭合), 130: 向下(张开)
  setSG90Angle(14, 120, 2); // 嘴巴微笑张开
  delay(DELAY_TIME);
  // 15 (PCA2): 140-180 degrees, 左中嘴(low), 140: 向下, 180: 向上
  setSG90Angle(15, 180, 2);
  delay(DELAY_TIME);
}

// 伤心表情
void sadness()
{
  // --- PCA1 控制的舵机 (眼部和眉毛) ---
  // 4 (PCA1): 60 - 100 degrees, 眼球左右, 60: 向左, 100: 向右
  setSG90Angle(4, 80, 1);
  delay(DELAY_TIME);
  // 5 (PCA1): 160 - 180 degrees, 眼球上下, 160: 向下, 180: 向上
  setSG90Angle(5, 160, 1); // 眼睛向下看
  delay(DELAY_TIME);
  // 6 (PCA1): 95 - 170 degrees, 左上眼皮, 95: 向下(闭合), 170: 向上(张开)
  setSG90Angle(6, 100, 1); // 眼皮下垂
  delay(DELAY_TIME);
  // 7 (PCA1): 55 - 95 degrees, 左下眼皮, 55: 向下(张开), 95: 向上(闭合)
  setSG90Angle(7, 90, 1); 
  delay(DELAY_TIME);
  // 8 (PCA1): 0 - 70 degrees, 右上眼皮, 0: 向上(张开), 70: 向下(闭合)
  setSG90Angle(8, 60, 1);  // 眼皮下垂
  delay(DELAY_TIME);
  // 9 (PCA1): 90 - 130 degrees, 右下眼皮, 90: 向上(闭合), 130: 向下(张开)
  setSG90Angle(9, 95, 1);
  delay(DELAY_TIME);
  
  // 12 (PCA1): 0 - 50 degrees, 左侧正眉毛, 0: 向上, 50: 向下
  setSG90Angle(12, 50, 1); // 眉毛向下
  delay(DELAY_TIME);
  // 13 (PCA1): 0 - 60 degrees, 左侧斜眉毛, 0: 向下, 60: 向上
  setSG90Angle(13, 10, 1); // 眉毛呈八字
  delay(DELAY_TIME);
  // 14 (PCA1): 0 - 30 degrees, 右侧正眉毛, 0: 向下, 30: 向上
  setSG90Angle(14, 0, 1);  // 眉毛向下
  delay(DELAY_TIME);
  // 15 (PCA1): 0 - 60 degrees, 右侧斜眉毛, 0: 向上, 60: 向下
  setSG90Angle(15, 50, 1); // 眉毛呈八字
  delay(DELAY_TIME);
  
  // --- PCA2 控制的舵机 (嘴部) ---
  // 6 (PCA2): 70 - 140 degrees, 左下嘴, 70: 向下, 140: 向上
  setSG90Angle(6, 70, 2);  // 嘴角向下
  delay(DELAY_TIME);
  // 7 (PCA2): 0 - 70 degrees, 右下嘴, 0: 向上, 70: 向下
  setSG90Angle(7, 70, 2);  // 嘴角向下
  delay(DELAY_TIME);
  // 9 (PCA2): 150 - 190 degrees, 左上嘴, 150: 向下, 190: 向上
  setSG90Angle(9, 150, 2);
  delay(DELAY_TIME);
  // 10 (PCA2):150 - 180 degrees, 右中嘴(upper), 150: 向上, 180: 向下
  setSG90Angle(10, 180, 2);
  delay(DELAY_TIME);
  // 11 (PCA2): 150-190 degrees, 左中嘴(upper), 150: 向下, 190: 向上
  setSG90Angle(11, 150, 2);
  delay(DELAY_TIME);
  // 12 (PCA2): 0 - 40 degrees, 右上嘴, 0: 向上, 40: 向下
  setSG90Angle(12, 40, 2);
  delay(DELAY_TIME);
  // 13 (PCA2): 60 - 100 degrees, 右中嘴(low), 60: 向上, 100: 向下
  setSG90Angle(13, 100, 2);
  delay(DELAY_TIME);
  // 14 (PCA2): 80 - 130 degrees, 嘴张闭, 80: 向上(闭合), 130: 向下(张开)
  setSG90Angle(14, 90, 2); // 嘴巴微张向下
  delay(DELAY_TIME);
  // 15 (PCA2): 140-180 degrees, 左中嘴(low), 140: 向下, 180: 向上
  setSG90Angle(15, 140, 2);
  delay(DELAY_TIME);
}

// 惊讶表情
void surprise()
{
  // --- PCA1 控制的舵机 (眼部和眉毛) ---
  // 4 (PCA1): 60 - 100 degrees, 眼球左右, 60: 向左, 100: 向右
  setSG90Angle(4, 80, 1);
  delay(DELAY_TIME);
  // 5 (PCA1): 160 - 180 degrees, 眼球上下, 160: 向下, 180: 向上
  setSG90Angle(5, 180, 1); // 眼睛向上看
  delay(DELAY_TIME);
  // 6 (PCA1): 95 - 170 degrees, 左上眼皮, 95: 向下(闭合), 170: 向上(张开)
  setSG90Angle(6, 170, 1); // 眼皮完全张开
  delay(DELAY_TIME);
  // 7 (PCA1): 55 - 95 degrees, 左下眼皮, 55: 向下(张开), 95: 向上(闭合)
  setSG90Angle(7, 55, 1);  // 眼皮完全张开
  delay(DELAY_TIME);
  // 8 (PCA1): 0 - 70 degrees, 右上眼皮, 0: 向上(张开), 70: 向下(闭合)
  setSG90Angle(8, 0, 1);   // 眼皮完全张开
  delay(DELAY_TIME);
  // 9 (PCA1): 90 - 130 degrees, 右下眼皮, 90: 向上(闭合), 130: 向下(张开)
  setSG90Angle(9, 130, 1); // 眼皮完全张开
  delay(DELAY_TIME);
  
  // 12 (PCA1): 0 - 50 degrees, 左侧正眉毛, 0: 向上, 50: 向下
  setSG90Angle(12, 0, 1);  // 眉毛高扬
  delay(DELAY_TIME);
  // 13 (PCA1): 0 - 60 degrees, 左侧斜眉毛, 0: 向下, 60: 向上
  setSG90Angle(13, 60, 1); // 眉毛高扬
  delay(DELAY_TIME);
  // 14 (PCA1): 0 - 30 degrees, 右侧正眉毛, 0: 向下, 30: 向上
  setSG90Angle(14, 30, 1); // 眉毛高扬
  delay(DELAY_TIME);
  // 15 (PCA1): 0 - 60 degrees, 右侧斜眉毛, 0: 向上, 60: 向下
  setSG90Angle(15, 0, 1);  // 眉毛高扬
  delay(DELAY_TIME);
  
  // --- PCA2 控制的舵机 (嘴部) ---
  // 6 (PCA2): 70 - 140 degrees, 左下嘴, 70: 向下, 140: 向上
  setSG90Angle(6, 100, 2); 
  delay(DELAY_TIME);
  // 7 (PCA2): 0 - 70 degrees, 右下嘴, 0: 向上, 70: 向下
  setSG90Angle(7, 40, 2);
  delay(DELAY_TIME);
  // 9 (PCA2): 150 - 190 degrees, 左上嘴, 150: 向下, 190: 向上
  setSG90Angle(9, 170, 2);
  delay(DELAY_TIME);
  // 10 (PCA2):150 - 180 degrees, 右中嘴(upper), 150: 向上, 180: 向下
  setSG90Angle(10, 165, 2);
  delay(DELAY_TIME);
  // 11 (PCA2): 150-190 degrees, 左中嘴(upper), 150: 向下, 190: 向上
  setSG90Angle(11, 170, 2);
  delay(DELAY_TIME);
  // 12 (PCA2): 0 - 40 degrees, 右上嘴, 0: 向上, 40: 向下
  setSG90Angle(12, 20, 2);
  delay(DELAY_TIME);
  // 13 (PCA2): 60 - 100 degrees, 右中嘴(low), 60: 向上, 100: 向下
  setSG90Angle(13, 80, 2);
  delay(DELAY_TIME);
  // 14 (PCA2): 80 - 130 degrees, 嘴张闭, 80: 向上(闭合), 130: 向下(张开)
  setSG90Angle(14, 130, 2); // 嘴巴O型张开
  delay(DELAY_TIME);
  // 15 (PCA2): 140-180 degrees, 左中嘴(low), 140: 向下, 180: 向上
  setSG90Angle(15, 160, 2);
  delay(DELAY_TIME);
}

// 眨眼动作
void blink()
{
  // 闭眼动作
  // 6 (PCA1) 左上眼皮: 95 向下(闭合)
  // 7 (PCA1) 左下眼皮: 95 向上(闭合)
  // 8 (PCA1) 右上眼皮: 70 向下(闭合)
  // 9 (PCA1) 右下眼皮: 90 向上(闭合)
  setSG90Angle(6, 95, 1);    // 左上眼皮向下闭合
  setSG90Angle(7, 95, 1);    // 左下眼皮向上闭合
  setSG90Angle(8, 70, 1);    // 右上眼皮向下闭合
  setSG90Angle(9, 90, 1);    // 右下眼皮向上闭合
  delay(200); // 闭眼持续时间

  // 根据当前表情状态恢复眼皮位置
  if(state == 0) { // 自然
    setSG90Angle(6, 120, 1);
    setSG90Angle(7, 60, 1);
    setSG90Angle(8, 30, 1);
    setSG90Angle(9, 125, 1);
  } else if (state == 1) { // 开心
    setSG90Angle(6, 150, 1);
    setSG90Angle(7, 55, 1);
    setSG90Angle(8, 20, 1);
    setSG90Angle(9, 130, 1);
  } else if (state == 2) { // 伤心
    setSG90Angle(6, 100, 1);
    setSG90Angle(7, 90, 1);
    setSG90Angle(8, 60, 1);
    setSG90Angle(9, 95, 1);
  } else if (state == 3) { // 惊讶
    setSG90Angle(6, 170, 1);
    setSG90Angle(7, 55, 1);
    setSG90Angle(8, 0, 1);
    setSG90Angle(9, 130, 1);
  }
}

void loop() {
  unsigned long currentMillis = millis(); // 获取当前时间

  // --- 定时眨眼逻辑 ---
  if (currentMillis - previousBlinkMillis >= BLINK_INTERVAL) {
    previousBlinkMillis = currentMillis;
    blink(); 
  }

  // --- 串口指令处理 ---
  if(Serial.available()) {
    int incomingByte = Serial.read(); // 读取指令字节

    // 指令 0x02: 控制眼球运动 (需要额外两个字节数据 x, y)
    if(incomingByte == 0x02) {
      while(Serial.available() < 2) { // 等待接收完 x 和 y 坐标
        delay(10); 
      }
      int x_angle = Serial.read(); // 读取眼球左右角度
      int y_angle = Serial.read(); // 读取眼球上下角度
      setSG90Angle(4, x_angle, 1); // 控制眼球左右 (PCA1, Servo 4)
      setSG90Angle(5, y_angle, 1); // 控制眼球上下 (PCA1, Servo 5)
    }
    // 指令 0x10: 自然表情
    if(incomingByte == 0x10) {
      state = 0;
      netural();
    }
    // 指令 0x11: 开心表情
    if(incomingByte == 0x11) {
      state = 1;
      happiness();
    }
    // 指令 0x12: 伤心表情
    if(incomingByte == 0x12) {
      state = 2;
      sadness();
    }
    // 指令 0x13: 惊讶表情
    if(incomingByte == 0x13) {
      state = 3;
      surprise();
    }
    // 指令 0x21: 开始说话 (嘴部动画)
    if(incomingByte == 0x21) {
      int mouth_flag = 1; // 用于切换嘴部开合状态
      unsigned long lastMouthMoveTime = millis();
      unsigned long mouthMoveInterval = 250; // 嘴部开合动画的间隔时间

      while(true){ // 循环播放说话动画直到接收到停止指令
        unsigned long now = millis(); 

        // 非阻塞嘴部动作: 在80度(闭合)和120度(张开)之间交替
        if (now - lastMouthMoveTime >= mouthMoveInterval) {
          lastMouthMoveTime = now;
          // Servo 14 (PCA2): 80 - 130 degrees, 嘴张闭, 80: 向上(闭合), 130: 向下(张开)
          // 此处使用 80 和 120 进行动画
          setSG90Angle(14, 100 + mouth_flag * 20, 2); // 切换到 80 或 120 度
          mouth_flag = -mouth_flag; // 反转状态
        }

        // 检查停止说话指令 (0x22)
        if(Serial.available()) {
          int stopByte = Serial.read();
          if(stopByte == 0x22) {
            setSG90Angle(14, 80, 2); // 说话结束，嘴巴闭合
            netural(); // 恢复到自然表情
            break; // 退出说话动画循环
          }
        }
        // 在说话时也保持眨眼
        if (now - previousBlinkMillis >= BLINK_INTERVAL) {
           previousBlinkMillis = now;
           blink();
        }
        delay(5); // 短暂延迟，避免CPU占用过高
      }
    }
  }
}