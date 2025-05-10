#include <Arduino.h> // Arduino核心库

#include <vector> // C++ STL vector容器
#include <string> // C++ STL string类

#include <WiFi.h> // WiFi连接库
#include <driver/i2s.h> // I2S驱动库
#include <esp_heap_caps.h> // ESP32堆内存管理库
#include <Adafruit_NeoPixel.h> // NeoPixel RGB LED控制库
#include <U8g2lib.h> // U8g2 OLED显示库

#include "freertos/FreeRTOS.h" // FreeRTOS实时操作系统核心
#include "freertos/task.h" // FreeRTOS任务管理
#include "freertos/queue.h" // FreeRTOS队列管理
#include "freertos/semphr.h" // FreeRTOS信号量管理

#include "config.h" // 项目配置文件

// I2S引脚定义 - INMP441麦克风
#define I2S_WS_INMP441 4    // I2S Word Select (LRCL) 引脚
#define I2S_SD_INMP441 6    // I2S Serial Data (DIN) 引脚
#define I2S_SCK_INMP441 5   // I2S Serial Clock (BCLK) 引脚

// I2S引脚定义 - MAX98357A音频放大器
#define I2S_DIN_98357A 7    // I2S Serial Data (DIN) 引脚
#define I2S_BCLK_98357A 15  // I2S Bit Clock (BCLK) 引脚
#define I2S_LRC_98357A 16   // I2S Left Right Clock (LRCK) 引脚

// 按键引脚定义
#define buttonDown 18   // 音量减按键
#define buttonUp 9      // 音量加按键
#define buttonStart 13  // 开始/停止按键

// I2S配置参数
#define I2S_PORT_INMP441 I2S_NUM_0 // INMP441麦克风使用的I2S端口号
#define I2S_PORT_98357A I2S_NUM_1  // MAX98357A放大器使用的I2S端口号
#define SAMPLE_RATE 16000          // 音频采样率 (16kHz)
#define SAMPLE_BITS 16             // 音频采样位数 (16-bit)
#define BUFFER_SIZE 1024           // I2S DMA缓冲区大小 (样本数)

// 网络通信信号定义
#define START_VOICE_RECEIVE 0x01 // 开始接收语音信号
#define STOP_VOICE_RECEIVE 0x02  // 停止接收语音信号

// 板载OLED和NeoPixel LED引脚定义 (通常固定)
#define LED_PIN 48    // NeoPixel LED数据引脚
#define LED_COUNT 1   // NeoPixel LED数量

WiFiClient client; // TCP客户端对象

double volume = 0.3;  // 音频播放音量 (0.0 ~ 1.0)

// FreeRTOS任务和队列句柄 - 运行在核心0
QueueHandle_t networkQueue;     // 网络任务队列句柄
QueueHandle_t ledControlQueue;  // LED控制任务队列句柄
QueueHandle_t u8g2Queue;        // OLED显示任务队列句柄
TaskHandle_t networkTask;       // 网络任务句柄
TaskHandle_t rgbLedTask;        // RGB LED任务句柄
TaskHandle_t u8g2Task;          // OLED显示任务句柄
TaskHandle_t buttomTask;        // 按键处理任务句柄
SemaphoreHandle_t buttonMutex;  // 按键互斥锁句柄，用于保护音量变量

// U8g2 OLED显示相关变量和函数
// OLED消息类型枚举
enum u8g2_msg_type
{
  TEXT_SCROLL,       // 滚动文本
  TEXT_STATIC,       // 静态文本
  TEXT_STATIC_SLEEP, // 休眠状态静态文本
};

// OLED消息结构体
struct u8g2Message
{
  u8g2_msg_type type; // 消息类型
  char *upper;        // OLED上半部分显示文本
  char *lower;        // OLED下半部分显示文本
};

// OLED显示任务函数
void u8g2_oled(void *parameter)
{
  // 初始化U8g2 OLED对象 (SSD1306, 128x32, 软件I2C: SCL=42, SDA=41)
  U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, 42, 41, U8X8_PIN_NONE);
  u8g2.begin();                          // 初始化OLED
  u8g2.setFont(u8g2_font_wqy16_t_gb2312); // 设置中文字体 (文泉驿点阵宋体16x16 GB2312)
  char *upper = (char*)malloc(256);      // 上半部分文本缓冲区
  char *lower = (char*)malloc(256);      // 下半部分文本缓冲区
  memset(upper, 0, 256);                 // 清空缓冲区
  memset(lower, 0, 256);                 // 清空缓冲区
  memcpy(upper, "表情机器人", sizeof("表情机器人")); // 默认上半部分文本
  memcpy(lower, "爱思麦当劳制作", sizeof("爱思麦当劳制作")); // 默认下半部分文本
  u8g2_msg_type type = TEXT_STATIC_SLEEP; // 默认显示类型
  u8g2.setFontPosBaseline();             // 设置字体基线位置
  int16_t scrollX = 128;                 // 文本滚动起始X坐标
  int16_t textWidth = u8g2.getUTF8Width(lower); // 获取下半部分文本宽度

  while (true) // 任务主循环
  {
    u8g2Message msg;
    // 尝试从队列接收新的显示消息，超时时间10ms
    if (xQueueReceive(u8g2Queue, &msg, pdMS_TO_TICKS(10)) == pdPASS)
    {
      type = msg.type; // 更新显示类型
      // 复制新的文本内容，并释放旧消息中的内存
      memcpy(upper, msg.upper,strlen(msg.upper)+1);
      upper[strlen(msg.upper)] = '\0';
      free(msg.upper);
      memcpy(lower, msg.lower,strlen(msg.lower)+1);
      lower[strlen(msg.lower)] = '\0';
      free(msg.lower);
      textWidth = u8g2.getUTF8Width(lower); // 更新文本宽度
      if (type == TEXT_SCROLL) // 如果是滚动类型，重置滚动位置
      {
        scrollX = 128;
      }
    }
    u8g2.clearBuffer(); // 清空OLED缓冲区
    switch (type)       // 根据显示类型绘制文本
    {
    case TEXT_STATIC_SLEEP: // 休眠状态静态文本
      u8g2.drawUTF8(0, 15, upper); // 绘制上半部分
      u8g2.drawUTF8(128 - textWidth, 31, lower); // 绘制下半部分 (右对齐)
      break;
    case TEXT_STATIC: // 静态文本
      u8g2.drawUTF8(0, 15, upper); // 绘制上半部分
      u8g2.drawUTF8(0, 31, lower); // 绘制下半部分 (左对齐)
      break;
    case TEXT_SCROLL: // 滚动文本
      u8g2.drawUTF8(0, 15, upper); // 绘制上半部分 (静态)
      u8g2.drawUTF8(scrollX, 31, lower); // 绘制下半部分 (滚动)
      if (scrollX < -textWidth) // 如果文本完全滚出左边界
      {
        scrollX = 128; // 重置到右边界外侧
      }
      scrollX -= 16; // 文本向左滚动的步长 (滚动速度)
      break;
    }
    u8g2.sendBuffer(); // 将缓冲区内容发送到OLED显示
    
    vTaskDelay(50 / portTICK_PERIOD_MS); // 任务延时50ms
  }
}

// 更新OLED显示内容的函数
void updateText(const char* upper, const char* lower, u8g2_msg_type type)
{
  u8g2Message msg;
  msg.type = type;
  // 为消息文本动态分配内存并复制内容
  msg.upper = (char*)malloc(strlen(upper)+1);
  memcpy(msg.upper, upper, strlen(upper));
  msg.upper[strlen(upper)] = '\0'; // 添加字符串结束符
  msg.lower = (char*)malloc(strlen(lower)+1);
  memcpy(msg.lower, lower, strlen(lower));
  msg.lower[strlen(lower)] = '\0'; // 添加字符串结束符
  // 将消息发送到OLED任务队列，超时时间100ms
  xQueueSend(u8g2Queue, &msg, pdMS_TO_TICKS(100));
}

// RGB LED状态枚举
enum RGB_LED_STATE
{
  GREEN,        // 绿色
  RED,          // 红色
  YELLOW,       // 黄色
  ORANGE,       // 橙色
  PURPLE,       // 紫色
  BLUE_FLICKER, // 蓝色闪烁
};

// RGB LED控制任务函数
void RGB_LED(void *parameter)
{
  // 初始化NeoPixel对象
  Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

  uint8_t brightness = 50;        // LED亮度 (0-255)
  uint8_t flickerBrightness = 1;  // 闪烁状态下的亮度切换标志 (0或1)
  strip.begin();                  // 初始化NeoPixel库
  strip.show();                   // 关闭所有LED (初始状态)
  strip.setBrightness(brightness); // 设置LED亮度
  strip.setPixelColor(0, strip.Color(255, 255, 0)); // 初始设置为黄色
  strip.show();                   // 显示颜色
  RGB_LED_STATE currentState = YELLOW; // 当前LED状态

  while (true) // 任务主循环
  {
    RGB_LED_STATE newState;
    // 尝试从队列接收新的LED状态，非阻塞
    if (xQueueReceive(ledControlQueue, &newState, 0) == pdPASS)
    {
      currentState = newState; // 更新LED状态
    }
    switch (currentState) // 根据当前状态设置LED颜色
    {
    case GREEN:
      strip.setPixelColor(0, strip.Color(0, 255, 0)); // 绿色
      strip.show();
      break;

    case RED:
      strip.setPixelColor(0, strip.Color(255, 0, 0)); // 红色
      strip.show();
      break;

    case YELLOW:
      strip.setPixelColor(0, strip.Color(255, 255, 0)); // 黄色
      strip.show();
      break;

    case ORANGE:
      strip.setPixelColor(0, strip.Color(255, 165, 0)); // 橙色
      strip.show();
      break;

    case PURPLE:
      strip.setPixelColor(0, strip.Color(128, 0, 128)); // 紫色
      strip.show();
      break;

    case BLUE_FLICKER: // 蓝色闪烁
      flickerBrightness ^= 1; // 切换闪烁亮度标志
      // 根据标志设置蓝色或熄灭
      strip.setPixelColor(0, strip.Color(flickerBrightness * 0, flickerBrightness * 0, flickerBrightness * 255));
      strip.show();
      vTaskDelay(500 / portTICK_PERIOD_MS); // 闪烁间隔500ms
      continue; // 继续下一次闪烁循环，不执行下面的延时
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // 非闪烁状态下任务延时100ms
  }
}

// 更新LED状态的函数
void updateLedState(RGB_LED_STATE newState)
{
  // 将新的LED状态发送到LED控制任务队列，超时时间10ms
  xQueueSend(ledControlQueue, &newState, pdMS_TO_TICKS(10));
}

// 网络消息类型枚举
enum NetMsgType
{
  NET_SEND_AUDIO,  // 发送音频数据
  NET_SEND_SIGNAL, // 发送控制信号
};

// 网络消息结构体
struct NetMessage
{
  NetMsgType type; // 消息类型
  union // 联合体，根据消息类型存储不同数据
  {
    struct // 音频数据结构
    {
      int16_t *data; // 音频样本数据指针
      size_t bytes;  // 音频数据字节数
    } audioData;
    struct // 控制信号数据结构
    {
      uint16_t signal; // 控制信号值
    } signalData;
  };
};

// 网络任务函数 (处理数据发送)
void NetworkTaskFunction(void *parameter)
{
  NetMessage msg; // 网络消息

  while (true) // 任务主循环
  {
    // 尝试从网络队列接收消息，超时时间200ms
    if (xQueueReceive(networkQueue, &msg, pdMS_TO_TICKS(200)) == pdPASS)
    {
      switch (msg.type) // 根据消息类型处理
      {
      case NET_SEND_AUDIO: // 发送音频数据
      {
        uint8_t signal_type = 0x02; // 定义数据类型为音频 (0x02)
        uint32_t datalength = static_cast<uint32_t>(msg.audioData.bytes); // 音频数据长度
        // 发送数据类型头部
        client.write((const uint8_t *)&signal_type, sizeof(signal_type));
        // 发送数据长度头部
        client.write((const uint8_t *)&datalength, sizeof(datalength));
        size_t total_written = 0;
        //循环发送音频数据，直到全部发送完毕
        while (total_written < datalength)
        {
          int n = client.write(((uint8_t *)msg.audioData.data) + total_written,
                               datalength - total_written);
          if (n <= 0) // 如果发送失败或连接断开
            break;
          total_written += n; // 更新已发送字节数
        }
        free(msg.audioData.data); // 释放音频数据内存
        client.flush(); // 确保所有数据都已发送
        break;
      }
      case NET_SEND_SIGNAL: // 发送控制信号
      {
        uint8_t signal_type = 0x01; // 定义数据类型为信号 (0x01)
        uint32_t datalength = sizeof(uint16_t); // 信号数据长度 (固定为2字节)
        uint16_t signal = msg.signalData.signal; // 获取信号值
        // 发送数据类型头部
        client.write((const uint8_t *)&signal_type, sizeof(signal_type));
        // 发送数据长度头部
        client.write((const uint8_t *)&datalength, sizeof(datalength));
        // 发送信号数据
        client.write((const uint8_t *)&signal, datalength);
        client.flush(); // 确保所有数据都已发送
        break;
      }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // 任务延时10ms
  }
}

// 发送音频数据到网络任务队列
void sendAudioToNetwork(int16_t *samples, size_t bytes_size)
{
  NetMessage msg;
  msg.type = NET_SEND_AUDIO;
  // 为音频数据动态分配内存并复制内容
  msg.audioData.data = (int16_t *)malloc(bytes_size);
  memcpy(msg.audioData.data, samples, bytes_size);
  msg.audioData.bytes = bytes_size;
  // 将消息发送到网络任务队列，超时时间100ms
  xQueueSend(networkQueue, &msg, pdMS_TO_TICKS(100));
}

// 发送控制信号到网络任务队列
void sendSignalToNetwork(uint16_t signal)
{
  NetMessage msg;
  msg.type = NET_SEND_SIGNAL;
  msg.signalData.signal = signal;
  // 将消息发送到网络任务队列，超时时间100ms
  xQueueSend(networkQueue, &msg, pdMS_TO_TICKS(100));
}

// 按键处理任务函数
void buttom(void *parameter)
{
  pinMode(buttonUp, INPUT_PULLUP);   // 设置音量加按键为上拉输入
  pinMode(buttonDown, INPUT_PULLUP); // 设置音量减按键为上拉输入
  int buttonUpState = HIGH;          // 音量加按键状态
  int buttonDownState = HIGH;        // 音量减按键状态

  while (true) // 任务主循环
  {
    buttonUpState = digitalRead(buttonUp);     // 读取音量加按键状态
    buttonDownState = digitalRead(buttonDown); // 读取音量减按键状态

    if (buttonUpState == LOW) // 如果音量加按键被按下
    {
      // 获取互斥锁，保护音量变量的访问
      if (xSemaphoreTake(buttonMutex, portMAX_DELAY) == pdTRUE)
      {
        if (volume < 0.99) // 如果音量未达到最大值
        {
          volume += 0.1; // 增加音量
        }
        xSemaphoreGive(buttonMutex); // 释放互斥锁
      }
    }
    if (buttonDownState == LOW) // 如果音量减按键被按下
    {
      // 获取互斥锁
      if (xSemaphoreTake(buttonMutex, portMAX_DELAY) == pdTRUE)
      {
        if (volume > 0.01) // 如果音量未达到最小值
        {
          volume -= 0.1; // 减小音量
        }
        xSemaphoreGive(buttonMutex); // 释放互斥锁
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // 任务延时100ms，进行按键消抖和降低CPU占用
  }
}

// 从服务器接收音频数据相关
size_t voice_size = 1024 * 1024; // 预分配用于接收语音的缓冲区大小 (1MB样本，即2MB字节)
// 在SPIRAM中为接收的语音样本分配内存
int16_t *voice_samples = (int16_t *)heap_caps_malloc(voice_size * sizeof(int16_t), MALLOC_CAP_SPIRAM);

// 从TCP客户端接收语音数据
size_t receive_client_voice()
{
  size_t datalength = 0; // 期望接收的数据长度
  // 等待客户端数据可用
  while (!client.available())
  {
    delay(100); // 等待100ms
  }
  // 读取数据长度头部 (4字节)
  client.readBytes((uint8_t *)&datalength, sizeof(datalength));
  size_t samples_count = datalength / sizeof(int16_t); // 计算样本数量
  size_t total_read = 0;
  // 循环读取数据，直到接收完指定长度的数据
  while (total_read < datalength)
  {
    int n = client.read(((uint8_t *)voice_samples) + total_read, datalength - total_read);
    total_read += n;
  }

  double current_volume = 0.5; // 默认音量
  // 获取当前音量设置 (受互斥锁保护)
  if (xSemaphoreTake(buttonMutex, portMAX_DELAY) == pdTRUE)
  {
    current_volume = volume;
    xSemaphoreGive(buttonMutex);
  }
  // 根据当前音量调整接收到的语音样本的幅度
  for (size_t i = 0; i < samples_count; i++)
  {
    voice_samples[i] = (int16_t)(voice_samples[i] * current_volume);
  }
  return samples_count; // 返回接收到的样本数量
}

// 从TCP客户端接收文本数据
char *receive_client_text()
{
  size_t textlength = 0; // 期望接收的文本长度
  // 等待客户端数据可用
  while (!client.available())
  {
    delay(100);
  }
  // 读取文本长度头部 (4字节)
  client.readBytes((uint8_t *)&textlength, sizeof(textlength));
  // 为文本缓冲区动态分配内存 (+1用于字符串结束符)
  char *text_buffer = (char *)malloc(textlength + 1);
  size_t total_read = 0;
  // 循环读取数据，直到接收完指定长度的文本
  while (total_read < textlength)
  {
    int n = client.read(((uint8_t *)text_buffer) + total_read, textlength - total_read);
    if (n > 0)
    {
      total_read += n;
    }
  }
  text_buffer[textlength] = '\0'; // 添加字符串结束符
  return text_buffer; // 返回接收到的文本字符串 (调用者负责释放内存)
}

// 能量法语音活动检测 (VAD)
bool energe_vad(int16_t *samples, size_t size)
{
  int64_t tot_energy = 0; // 总能量
  // 计算音频帧的总能量 (平方和)
  for (size_t i = 0; i < size; i++)
  {
    tot_energy += (int32_t)samples[i] * samples[i];
  }
  int32_t energy = tot_energy / size; // 平均能量
  return energy > VAD_ENERGY_THRESHOLD; // 如果平均能量大于阈值，则认为有语音活动
}

// 核心0任务初始化函数
void core0_begin()
{
  // 创建各个任务所需的队列
  networkQueue = xQueueCreate(100, sizeof(NetMessage));       // 网络任务队列，容量100
  ledControlQueue = xQueueCreate(10, sizeof(RGB_LED_STATE)); // LED控制队列，容量10
  u8g2Queue = xQueueCreate(10, sizeof(u8g2Message));         // OLED显示队列，容量10

  // 创建并启动固定在核心0上的任务
  xTaskCreatePinnedToCore(
      NetworkTaskFunction, // 任务函数
      "NetTask",           // 任务名称
      8192,                // 任务堆栈大小 (字节)
      NULL,                // 传递给任务的参数
      3,                   // 任务优先级 (0-configMAX_PRIORITIES-1)
      &networkTask,        // 任务句柄
      0);                  // 核心ID (0)

  xTaskCreatePinnedToCore(
      RGB_LED,
      "RGB_LED_Task",
      4096,
      NULL,
      1,
      &rgbLedTask,
      0);

  xTaskCreatePinnedToCore(
      u8g2_oled,
      "u8g2_oled",
      16384, // OLED任务需要较大堆栈，特别是使用中文字库时
      NULL,
      2,
      &u8g2Task,
      0);

  xTaskCreatePinnedToCore(
      buttom,
      "buttom",
      4096,
      NULL,
      1,
      &buttomTask,
      0);
  Serial.println("Core0 tasks created"); // 串口打印核心0任务创建完成信息
}

// 网络初始化函数 (WiFi连接和TCP服务器连接)
void network_begin()
{
  Serial.println("Connecting to WiFi..."); // 串口提示正在连接WiFi
  // 使用config.h中定义的SSID和密码连接WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // 等待WiFi连接成功
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500); // 每500ms检查一次
  }
  Serial.println("WiFi connected"); // 串口提示WiFi已连接
  Serial.println("Establishing TCP connection..."); // 串口提示正在建立TCP连接
  // 使用config.h中定义的服务器主机和端口连接TCP服务器
  while (!client.connect(SERVER_HOST, SERVER_PORT))
  {
    delay(1000); // 每1000ms尝试一次
  }
  Serial.println("TCP connection established"); // 串口提示TCP连接已建立
}

// I2S驱动初始化函数
void i2s_begin()
{
  // INMP441麦克风的I2S配置
  i2s_config_t i2s_config_INMP441 = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX), // 主模式，接收
      .sample_rate = SAMPLE_RATE,                         // 采样率
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,       // 16位采样
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,        // 单声道 (左声道)
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,  // 标准I2S格式
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,           // 中断分配标志
      .dma_buf_count = 8,                                 // DMA缓冲区数量
      .dma_buf_len = BUFFER_SIZE,                         // 单个DMA缓冲区大小 (样本数)
      .use_apll = false,                                  // 不使用APLL时钟
      .tx_desc_auto_clear = false,                        // 发送描述符自动清除 (RX模式下无效)
      .fixed_mclk = 0                                     // 固定MCLK频率 (0表示自动)
  };

  // INMP441麦克风的I2S引脚配置
  i2s_pin_config_t pin_config_INMP441 = {
      .mck_io_num = I2S_PIN_NO_CHANGE,   // MCLK引脚 (不使用)
      .bck_io_num = I2S_SCK_INMP441,     // BCLK引脚
      .ws_io_num = I2S_WS_INMP441,       // WS (LRCL) 引脚
      .data_out_num = I2S_PIN_NO_CHANGE, // 数据输出引脚 (RX模式下不使用)
      .data_in_num = I2S_SD_INMP441      // 数据输入引脚
  };

  // MAX98357A音频放大器的I2S配置
  i2s_config_t i2s_config_98357A = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX), // 主模式，发送
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // 虽然是单声道放大器，但通常配置为左声道
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = BUFFER_SIZE,
      .use_apll = false,
      .tx_desc_auto_clear = true, // 发送模式下，发送完一个缓冲区后自动清除描述符
      .fixed_mclk = 0
  };

  // MAX98357A音频放大器的I2S引脚配置
  i2s_pin_config_t pin_config_98357A = {
      .mck_io_num = I2S_PIN_NO_CHANGE,
      .bck_io_num = I2S_BCLK_98357A,
      .ws_io_num = I2S_LRC_98357A,
      .data_out_num = I2S_DIN_98357A,    // 数据输出引脚
      .data_in_num = I2S_PIN_NO_CHANGE // 数据输入引脚 (TX模式下不使用)
  };

  // 安装并启动I2S驱动 (INMP441 - I2S0)
  i2s_driver_install(I2S_NUM_0, &i2s_config_INMP441, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config_INMP441);

  // 安装并启动I2S驱动 (MAX98357A - I2S1)
  i2s_driver_install(I2S_NUM_1, &i2s_config_98357A, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &pin_config_98357A);
  Serial.println("I2S driver installed"); // 串口提示I2S驱动已安装
}

// Arduino setup()函数，在程序启动时执行一次
void setup()
{
  Serial.begin(115200); // 初始化串口通信，波特率115200
  // 初始化接收语音样本的缓冲区 (虽然heap_caps_malloc已分配，这里memset确保初始为0)
  if (voice_samples != NULL) { // 确保内存已成功分配
      memset(voice_samples, 0, voice_size * sizeof(int16_t));
  }


  buttonMutex = xSemaphoreCreateMutex(); // 创建按键互斥锁
  pinMode(buttonStart, INPUT_PULLUP);    // 设置开始按键为上拉输入

  core0_begin();   // 初始化核心0上的任务
  network_begin(); // 初始化网络连接
  i2s_begin();     // 初始化I2S驱动
  delay(2000);     // 等待系统稳定
}

// 用于VAD和初始音频数据读取的缓冲区，大小为8个I2S DMA缓冲区
int16_t vad_samples[BUFFER_SIZE * 8];

// Arduino loop()函数，在setup()执行完毕后循环执行 (核心1)
void loop()
{
  // 检测开始按键是否被按下 (低电平有效)
  int open = digitalRead(buttonStart) == LOW;
  updateLedState(ORANGE); // 将LED设置为橙色 (待机状态)

  if (open) // 如果开始按键被按下
  {
    delay(2500); // 按键消抖或延时，等待用户释放
    updateLedState(RED); // LED变为红色 (准备录音)
    uint32_t last_activate = millis(); // 记录上次有语音活动的时间

    while (open) // 保持在激活状态，直到再次按下开始键或超时
    {
      size_t bytes_read = 0; // I2S读取到的字节数
      // 从麦克风读取一批音频数据用于VAD检测
      i2s_read(I2S_PORT_INMP441, vad_samples, BUFFER_SIZE * 8 * sizeof(int16_t), &bytes_read, portMAX_DELAY);
      // 进行VAD检测
      bool loud = energe_vad(vad_samples, bytes_read / sizeof(int16_t));

      if (loud) // 如果检测到语音活动
      {
        sendSignalToNetwork(START_VOICE_RECEIVE); // 发送开始接收语音信号给服务器
        sendAudioToNetwork(vad_samples, bytes_read); // 发送第一批检测到的音频数据
        updateText("正在聆听您的声音", "Hearing Voice", TEXT_STATIC); // OLED提示正在聆听
        updateLedState(GREEN); // LED变为绿色 (正在录音)

        int16_t samples[BUFFER_SIZE]; // 用于后续连续录音的缓冲区
        size_t total_send = 0;        // 已发送的总字节数
        total_send += bytes_read;     // 加上第一批数据

        uint32_t start_time = millis();     // 本次语音段开始时间
        uint32_t last_loud_time = start_time; // 上次检测到语音的时间

        // 持续录音和发送，直到静音超时或总时长超时
        while (true)
        {
          // 从麦克风读取音频数据
          i2s_read(I2S_PORT_INMP441, samples, BUFFER_SIZE * sizeof(int16_t), &bytes_read, portMAX_DELAY);
          sendAudioToNetwork(samples, bytes_read); // 发送音频数据
          loud = energe_vad(samples, bytes_read / sizeof(int16_t)); // VAD检测
          total_send += bytes_read; //累加发送字节数

          uint32_t current_time = millis(); // 当前时间
          if (loud) // 如果有语音
          {
            last_loud_time = current_time; // 更新上次有语音的时间
            // 检查是否超过单次语音激活最大持续时间
            if (current_time - start_time >= MAX_ACTIVATE_INTERVAL)
            {
              break; // 停止录音
            }
          }
          else // 如果没有语音 (静音)
          {
            // 检查是否超过静音检测最大间隔
            if (current_time - last_loud_time > MAX_VAD_INTERVAL)
            {
              break; // 停止录音
            }
          }
        }
        Serial.print("Total sent bytes: "); // 串口打印总发送字节数
        Serial.println(total_send);
        char ch[20];
        sprintf(ch, "%d", total_send); // 将发送字节数转为字符串
        updateText("TOTAL SEND", ch, TEXT_STATIC); // OLED显示发送字节数
        sendSignalToNetwork(STOP_VOICE_RECEIVE); // 发送停止接收语音信号给服务器

        updateText("少女祈祷中...", "Now Processing", TEXT_STATIC); // OLED提示正在处理
        updateLedState(BLUE_FLICKER); // LED变为蓝色闪烁 (等待服务器响应)

        // 接收服务器返回的语音数据
        size_t tot_length = receive_client_voice();
        Serial.print("Received voice data length: "); // 串口打印接收到的语音数据长度
        Serial.println(tot_length);

        // 接收服务器返回的文本数据
        char *reply_text = receive_client_text();
        Serial.print("接收到文本: "); // 串口打印接收到的文本
        Serial.println(reply_text);
        updateText("正在回复您的声音", reply_text, TEXT_SCROLL); // OLED显示回复文本 (滚动)
        free(reply_text); // 释放接收文本的内存

        updateLedState(PURPLE); // LED变为紫色 (正在播放回复语音)
        // 播放接收到的语音数据
        for (size_t i = 0; i < tot_length; i += BUFFER_SIZE)
        {
          size_t bytes_written = 0;
          if (i + BUFFER_SIZE < tot_length) // 如果剩余数据大于一个缓冲区
          {
            i2s_write(I2S_PORT_98357A, &voice_samples[i], BUFFER_SIZE * sizeof(int16_t), &bytes_written, portMAX_DELAY);
          }
          else // 播放剩余不足一个缓冲区的数据
          {
            i2s_write(I2S_PORT_98357A, &voice_samples[i], (tot_length - i) * sizeof(int16_t), &bytes_written, portMAX_DELAY);
          }
        }
        // 播放结束后，发送一些静音数据以确保DMA缓冲区被清空，避免残留声音
        int16_t silence[BUFFER_SIZE] = {0}; // 静音样本缓冲区
        for (int i = 0; i < 8; i++)
        {
          size_t bytes_written = 0;
          i2s_write(I2S_PORT_98357A, silence, BUFFER_SIZE * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        }

        updateLedState(RED); // LED变回红色 (准备下一次录音)
        last_activate = millis(); // 更新上次活动时间
      }
      else // 如果没有检测到语音活动 (初始VAD为静音)
      {
        // 检查是否超过无语音激活进入休眠的最大等待时间
        if (millis() - last_activate > MAX_REST_LIMIT)
        {
          open = false; // 退出激活状态 (相当于用户按了停止键)
          updateLedState(ORANGE); // LED变为橙色 (待机)
          updateText("表情机器人", "爱思麦当劳制作", TEXT_STATIC_SLEEP); // OLED显示默认休眠文本
        }
      }
      // 检查开始按键是否再次被按下以停止当前会话
      if (digitalRead(buttonStart) == LOW) {
          delay(50); // 消抖
          if (digitalRead(buttonStart) == LOW) {
              open = false; // 退出激活状态
              updateLedState(ORANGE);
              updateText("表情机器人", "爱思麦当劳制作", TEXT_STATIC_SLEEP);
          }
      }
    }
  }
  // loop()本身会不断循环，这里的延时可以降低CPU占用率
  // 但由于内部有 vTaskDelay 和 i2s_read 的阻塞，此处的延时可能不是必须的
  // 或者可以设置一个较小的值
  delay(10);
}