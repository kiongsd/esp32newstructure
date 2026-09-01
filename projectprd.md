# ESP32-S3 OV5640 智能摄像头项目 PRD

## 项目概述

- **目标**: 基于正点原子 ESP32-S3 开发板构建智能摄像头系统
- **硬件配置**:
  - ESP32-S3 开发板 (PSRAM OCT 80MHz, Flash 16MB)
  - OV5640 摄像头模块 (DVP 接口)
  - SPI LCD 屏幕 (240x320, **ST7789**, 支持触摸)
  - SD 卡槽 (SPI接口，与LCD共用SPI总线)
  - XL9555 IO扩展器 (I2C接口，控制LCD/OV5640的复位和电源)
- **平台**: ESP-IDF v5.4.4
- **项目路径**: `D:\espproject\99_OV5640`

---

## 分阶段 PRD

### Phase 1: 基础采集与显示 (当前目标)

**目标**: 实现 OV5640 拍摄 + LCD 实时显示

**功能**:
- OV5640 初始化 (DVP 接口, RGB565, 240x320)
- SPI LCD 初始化 (ST7789)
- 实时循环: 摄像头采集 → LCD 显示
- 帧率目标: ≥15 FPS
- 串口输出 FPS 统计

**交付物**:
- 可运行固件，LCD 实时显示摄像头画面
- 清晰代码结构，便于后续扩展

---

### Phase 2: SD 卡图像存储

**目标**: 实现拍照存储功能

**功能**:
- SD 卡初始化 (SPI 或 SDMMC 接口)
- 文件系统挂载 (FAT32)
- 按键触发拍照
- JPEG 编码后保存到 SD 卡
- 文件命名: `IMG_0001.jpg`, `IMG_0002.jpg`...
- 串口打印保存路径

**存储路径**: `/sdcard/DCIM/`

---

### Phase 3: WiFi 图传 (远程查看)

**目标**: 通过浏览器远程查看摄像头画面

**功能**:
- WiFi AP 模式 (创建热点) 或 STA 模式 (连接路由器)
- HTTP MJPEG 流服务器
- 浏览器访问 `http://192.168.x.x` 查看实时画面
- 同时保持 LCD 本地显示
- 拍照按钮同步到网页端

---

### Phase 4: 触控 & GUI

**目标**: 图形化操作界面

**功能**:
- LVGL 集成
- 模式切换 UI:
  - 实时预览模式
  - 拍照模式
  - 相册浏览模式
  - 设置模式

---

### Phase 5: 智能识别 (深度学习)

**目标**: 集成 AI 功能

**功能**:
- 人脸识别 (ESP-WHO)
  - 检测人脸位置
  - 人脸特征匹配
  - 人脸数据库管理
- 目标检测 (ESP-DL)
  - YOLO11/YOLO26 模型
  - COCO 数据集类别识别
- 颜色分类 (自定义模型)
  - 训练颜色分类模型
  - 实时颜色识别
- 结果显示在 LCD 上 (边框/标签)

**依赖组件**:
- `espressif/esp-dl`
- `espressif/esp-who`

---

### Phase 6: 高级功能

**目标**: 完善产品功能

**功能**:
- 运动检测 (帧差法)
- 定时拍照 (延时摄影)
- OTA 固件升级
- MQTT 推送通知
- 图片上传到云端

---

## 硬件连接说明 (已确认)

### SPI 总线 (LCD 与 SD 卡共用)

| 信号 | GPIO | 说明 |
|------|------|------|
| MOSI | IO11 | SPI 数据输出 |
| MISO | IO13 | SPI 数据输入 |
| SCK | IO12 | SPI 时钟 |

### SPI LCD (ST7789, 240x320)

| 信号 | GPIO | 说明 |
|------|------|------|
| MOSI | IO11 | SPI 数据 (共用总线) |
| SCK | IO12 | SPI 时钟 (共用总线) |
| CS | IO21 | 片选 |
| DC | IO40 | 数据/命令选择 |
| RST | XL9555 IO1_2 | 复位 (通过IO扩展器) |
| PWR | XL9555 IO1_3 | 电源控制 (通过IO扩展器) |

> LCD 屏幕支持触摸，但触摸IC型号待确认 (可能是CST816S)

### SD 卡 (SPI 接口)

| 信号 | GPIO | 说明 |
|------|------|------|
| MOSI | IO11 | SPI 数据 (共用总线) |
| MISO | IO13 | SPI 数据 (共用总线) |
| SCK | IO12 | SPI 时钟 (共用总线) |
| CS | IO2 | 片选 |

### OV5640 DVP 接口

| 信号 | GPIO | 说明 |
|------|------|------|
| D0 | IO4 | 数据位0 |
| D1 | IO5 | 数据位1 |
| D2 | IO6 | 数据位2 |
| D3 | IO7 | 数据位3 |
| D4 | IO15 | 数据位4 |
| D5 | IO16 | 数据位5 |
| D6 | IO17 | 数据位6 |
| D7 | IO18 | 数据位7 |
| PCLK | IO45 | 像素时钟 (输入) |
| HREF | IO48 | 水平参考 (输入) |
| VSYNC | IO47 | 垂直同步 (输入) |
| SIOD (SDA) | IO39 | SCCB/I2C 数据 (使用I2C_NUM_1) |
| SIOC (SCL) | IO38 | SCCB/I2C 时钟 (使用I2C_NUM_1) |
| RESET | XL9555 IO0_5 | 复位 (通过IO扩展器) |
| PWDN | XL9555 IO0_4 | 电源控制 (通过IO扩展器) |
| XCLK | GPIO_NUM_NC | 由esp32-camera通过LEDC自动配置 |

> **注**: 官方案例中XCLK、SIOD、SIOC设置为GPIO_NUM_NC，实际SIOD/SIOC在myiic.h中定义为IO39/IO38，使用I2C_NUM_1端口

### XL9555 IO 扩展器 (I2C 接口)

| 信号 | GPIO | 说明 |
|------|------|------|
| SDA | IO41 | I2C 数据 |
| SCL | IO42 | I2C 时钟 |
| INT | IO40 | 中断输出 (与LCD DC共用) |
| I2C 地址 | 0x20 | 7位地址 |

### XL9555 IO 分配表

| IO编号 | 功能 | 说明 |
|--------|------|------|
| IO0_0 | AP_INT | 音频中断 |
| IO0_1 | QMA_INT | 加速度计中断 |
| IO0_2 | SPK_EN | 扬声器使能 |
| IO0_3 | BEEP | 蜂鸣器 |
| IO0_4 | OV_PWDN | OV5640电源控制 |
| IO0_5 | OV_RESET | OV5640复位 |
| IO0_6 | GBC_LED | RGB LED |
| IO0_7 | GBC_KEY | 按键 |
| IO1_0 | LCD_BL | LCD背光 |
| IO1_1 | CT_RST | 触摸芯片复位 |
| IO1_2 | SLCD_RST | LCD复位 |
| IO1_3 | SLCD_PWR | LCD电源 |
| IO1_4 | KEY3 | 按键3 |
| IO1_5 | KEY2 | 按键2 |
| IO1_6 | KEY1 | 按键1 |
| IO1_7 | KEY0 | 按键0 |

---

## Phase 1 实现步骤 (方式B: 组件化)

### Step 1: 创建项目文件结构

```
99_OV5640/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── main.c
│   ├── pin_config.h          # 公共引脚定义
│   └── idf_component.yml     # 组件依赖声明
├── components/
│   ├── xl9555/               # IO扩展器组件
│   │   ├── CMakeLists.txt
│   │   ├── xl9555.c
│   │   └── xl9555.h
│   ├── camera/               # 摄像头组件
│   │   ├── CMakeLists.txt
│   │   ├── camera_init.c
│   │   └── camera_init.h
│   └── lcd/                  # LCD组件
│       ├── CMakeLists.txt
│       ├── lcd_init.c
│       └── lcd_init.h
├── sdkconfig
└── partitions-16MiB.csv
```

### Step 2: 创建引脚定义 (`main/pin_config.h`)

集中管理所有GPIO引脚定义，包括：
- SPI引脚 (MOSI, MISO, SCK)
- LCD引脚 (CS, DC)
- I2C引脚 (SDA, SCL)
- OV5640数据和控制引脚

### Step 3: 创建 XL9555 组件 (`components/xl9555/`)

实现IO扩展器驱动：
- I2C初始化 (IO41=SDA, IO42=SCL)
- 读写XL9555寄存器
- 控制LCD的RST和PWR
- 控制OV5640的RESET和PWDN

### Step 4: 创建 Camera 组件 (`components/camera/`)

依赖 xl9555 组件，实现：
- 通过XL9555控制OV5640的RESET和PWDN
- 配置DVP接口GPIO (IO4-IO18, IO45, IO47, IO48, IO38, IO39)
- 初始化esp32-camera驱动
- 设置RGB565格式，QVGA分辨率 (320x240)
- 分配PSRAM双缓冲

### Step 5: 创建 LCD 组件 (`components/lcd/`)

依赖 xl9555 组件，实现：
- 通过XL9555控制LCD的RST和PWR
- 初始化SPI总线 (IO11=MOSI, IO12=SCK)
- 创建ST7789面板 (IO21=CS, IO40=DC)
- 配置显示方向 (竖屏240x320)
- 控制背光

### Step 6: 更新主程序 (`main/main.c`)

```c
app_main() {
    i2c_init();         // 初始化I2C
    xl9555_init();      // 初始化IO扩展器
    camera_init();      // 初始化摄像头
    lcd_init();         // 初始化LCD
    lcd_set_backlight(true);  // 开启背光
    
    while(1) {
        camera_capture();     // 采集图像
        lcd_display_image();  // 显示到LCD
        // FPS统计
    }
}
```

### Step 7: 更新构建配置

- 更新 `main/CMakeLists.txt`: 添加组件依赖
- 更新 `CMakeLists.txt`: 添加components目录到EXTRA_COMPONENT_DIRS
- 更新 `main/idf_component.yml`: 添加esp32-camera和esp_lcd_st7789依赖

### Step 8: 编译验证

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

---

## 关键文件清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `main/pin_config.h` | 新建 | 公共GPIO引脚定义 |
| `main/main.c` | 修改 | 主程序入口 |
| `main/CMakeLists.txt` | 修改 | 添加组件依赖 |
| `main/idf_component.yml` | 修改 | 添加esp32-camera和esp_lcd_st7789依赖 |
| `components/xl9555/xl9555.c` | 新建 | XL9555 IO扩展器驱动 |
| `components/xl9555/xl9555.h` | 新建 | XL9555头文件 |
| `components/xl9555/CMakeLists.txt` | 新建 | XL9555组件构建配置 |
| `components/camera/camera_init.c` | 新建 | OV5640初始化代码 |
| `components/camera/camera_init.h` | 新建 | 摄像头头文件 |
| `components/camera/CMakeLists.txt` | 新建 | 摄像头组件构建配置 |
| `components/lcd/lcd_init.c` | 新建 | SPI LCD初始化代码 |
| `components/lcd/lcd_init.h` | 新建 | LCD头文件 |
| `components/lcd/CMakeLists.txt` | 新建 | LCD组件构建配置 |

---

## 需要用户确认的信息

1. **触摸IC型号** — 确认触摸芯片型号 (CST816S/FT6336等)，以便Phase 4实现触摸功能

---

## 验证方案

| 测试项 | 预期结果 | 优先级 |
|--------|----------|--------|
| 编译通过 | 无错误 | P0 |
| 启动正常 | 串口输出初始化日志 | P0 |
| XL9555 初始化 | I2C通信正常，读取IO状态成功 | P0 |
| LCD 背光亮 | 通过XL9555控制LCD电源和背光 | P0 |
| 摄像头初始化 | SCCB 读取 OV5640 ID 成功 | P0 |
| 实时画面 | LCD 显示摄像头画面 | P0 |
| 帧率 ≥15FPS | 串口打印帧率 | P1 |

---

## 计划文件位置

**计划文件路径**: `D:\espproject\99_OV5640\plan.md`

执行时可随时查阅此文件。

---

## 参考代码

### 官方Camera示例 (主要参考)
路径: `D:\espproject\25_1_camera`

该示例实现了完整的OV5640摄像头采集+LCD显示功能，包含：
- `main/main.c` - 摄像头初始化和主循环
- `components/BSP/XL9555/` - XL9555 IO扩展器驱动
- `components/BSP/MYIIC/` - I2C驱动 (IO41=SDA, IO42=SCL)
- `components/BSP/MYSPI/` - SPI驱动 (IO11=MOSI, IO12=SCK, IO13=MISO)
- `components/BSP/SPILCD/` - ST7789 LCD驱动

### 官方LCD示例
路径: `D:\espproject\codeans\10_spilcd`

可参考其中的BSP组件实现。
