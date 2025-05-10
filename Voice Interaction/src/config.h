#ifndef CONFIG_H
#define CONFIG_H

// WiFi 配置
#define WIFI_SSID "YOUR_WIFI_SSID"          // WiFi网络名称 (SSID)
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"  // WiFi网络密码

// 服务器配置
#define SERVER_HOST "YOUR_SERVER_IP_ADDRESS" // 服务器IP地址或域名
#define SERVER_PORT 5000                     // 服务器端口号

// VAD (Voice Activity Detection) 参数
#define MAX_VAD_INTERVAL 2000       // 静音检测最大间隔 (ms) - 超过此时间未检测到语音活动，则认为是一段静默
#define MAX_ACTIVATE_INTERVAL 30000 // 单次语音激活最大持续时间 (ms) - 语音活动超过此时间，则强制结束本次激活
#define MAX_REST_LIMIT 30000        // 无语音激活进入休眠的最大等待时间 (ms) - 在此时间内无任何语音激活，设备可能进入休眠模式
#define VAD_ENERGY_THRESHOLD 10000  // VAD 能量阈值 - 用于判断是否检测到语音活动的能量界限

#endif // CONFIG_H