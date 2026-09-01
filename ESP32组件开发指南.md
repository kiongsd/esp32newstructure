# ESP32-S3 组件开发指南

基于正点原子 ESP32-S3 开发板（OV5640 + ST7789 LCD + XL9555 IO扩展器）的开发总结。

---

## 一、项目结构

```
99_OV5640/
├── CMakeLists.txt                  # 项目根 CMakeLists
├── main/
│   ├── CMakeLists.txt              # main 组件构建配置
│   ├── idf_component.yml           # 组件注册表依赖声明
│   └── main.c                      # 程序入口
├── components/
│   ├── BSP/                        # 板级支持包（自定义组件）
│   │   ├── CMakeLists.txt
│   │   ├── LED/                    # LED 驱动
│   │   ├── MYIIC/                  # I2C 驱动
│   │   ├── MYSPI/                  # SPI 驱动
│   │   ├── XL9555/                 # IO扩展器驱动
│   │   └── SPILCD/                 # LCD 驱动
│   └── Middlewares/                # 中间件（自定义组件）
│       ├── CMakeLists.txt
│       └── OV5640/                 # 摄像头驱动
├── managed_components/             # 自动下载的注册表组件（勿手动修改）
│   ├── espressif__esp32-camera/
│   ├── espressif__esp_jpeg/
│   ├── jbrilha__esp_lcd_st7789/
│   └── ...
└── sdkconfig                       # 项目配置
```

---

## 二、两种添加组件的方式

### 方式1：组件注册表（官方/第三方组件）

适用场景：使用乐鑫官方或第三方已发布的组件。

**步骤：**

1. 在 [ESP-IDF 组件注册表](https://components.espressif.com/) 搜索组件
2. 在 `main/idf_component.yml` 中声明依赖：

```yaml
dependencies:
  espressif/esp32-camera: "^2.1.7"
  jbrilha/esp_lcd_st7789: "^1.0.2"
```

3. 执行 `idf.py build`，自动下载到 `managed_components/` 目录
4. 在本地组件的 `CMakeLists.txt` 中引用（注意 `/` 变 `__`）：

```cmake
set(requires
    driver
    esp_lcd
    espressif__esp32-camera)    # 组件名中 "/" 替换为 "__"
```

5. 在代码中 `#include "xxx.h"` 使用

### 方式2：本地组件（自定义驱动）

适用场景：自己编写的硬件驱动、业务逻辑模块。

**步骤：**

1. 在 `components/` 下创建目录：

```
components/my_driver/
├── CMakeLists.txt
├── my_driver.c
└── my_driver.h
```

2. 编写 `CMakeLists.txt`：

```cmake
idf_component_register(SRCS "my_driver.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_lcd)
```

- `SRCS`：源文件
- `INCLUDE_DIRS`：头文件目录（`.` 表示当前目录，会导出给其他组件使用）
- `REQUIRES`：依赖的其他组件（编译时链接）

3. 在上层组件中引用：

```cmake
# main/CMakeLists.txt
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES BSP Middlewares)
```

---

## 三、组件依赖链

```
idf_component.yml          → 声明"我要下载这个组件"
CMakeLists.txt REQUIRES    → 声明"我的代码编译时依赖它"
#include "xxx.h"           → 实际使用（代码中引用）
```

三步缺一不可。

### 依赖传递

```
main → BSP → driver, esp_lcd
main → Middlewares → BSP, espressif__esp32-camera
```

依赖是传递的：main 依赖 BSP，BSP 依赖 driver，那么 main 中也能使用 driver 的头文件。

### CMakeLists.txt 中 include_dirs 的作用

- `src_dirs`：告诉构建系统哪些目录有源文件需要编译
- `include_dirs`：告诉构建系统哪些目录的头文件需要**导出**给其他组件使用

如果一个目录在 `src_dirs` 中但不在 `include_dirs` 中，源文件会被编译，但其他组件找不到它的头文件。

---

## 四、本项目的依赖关系

### 组件注册表依赖（idf_component.yml）

| 组件 | 版本 | 用途 |
|------|------|------|
| espressif/esp32-camera | ^2.1.7 | OV5640 摄像头驱动 |
| jbrilha/esp_lcd_st7789 | ^1.0.2 | ST7789 LCD 驱动 |
| espressif/esp_lcd_touch | ^1.2.1 | 触摸屏驱动 |
| lvgl/lvgl | ^9.5.0 | GUI 框架 |
| espressif/esp_lvgl_port | ^2.8.0~1 | LVGL 移植层 |

### 本地组件依赖

```
main
 ├── BSP (LED, MYIIC, MYSPI, SPILCD, XL9555)
 │    ├── driver
 │    └── esp_lcd
 └── Middlewares (OV5640)
      ├── BSP
      ├── driver
      ├── esp_lcd
      └── espressif__esp32-camera
```

### 两条 I2C 总线

| 总线 | 端口 | 引脚 | 用途 |
|------|------|------|------|
| I2C_NUM_0 | XL9555 | IO41=SDA, IO42=SCL | 控制 LCD 的 RST/PWR、OV5640 的 RESET/PWDN |
| I2C_NUM_1 | OV5640 SCCB | IO38=SCL, IO39=SDA | 配置 OV5640 寄存器（由 esp_camera 自动初始化） |

---

## 五、初始化顺序

```c
void app_main(void)
{
    led_init();           // LED 初始化
    myiic_init();         // I2C_NUM_0 初始化（XL9555 通信）
    my_spi_init();        // SPI2 总线初始化（LCD/SD 卡）
    xl9555_init();        // XL9555 IO扩展器初始化（控制 RST/PWR/PWDN）
    spilcd_init();        // LCD 初始化（挂载到 SPI2，通过 XL9555 控制电源）
    camera_init();        // 摄像头初始化（通过 XL9555 控制电源，自动初始化 I2C_NUM_1）
    
    while (1)
    {
        camera_record();  // 采集一帧 → 显示到 LCD → 释放缓冲
    }
}
```

初始化顺序有依赖关系，不能随意调换：
- `myiic_init()` 必须在 `xl9555_init()` 之前（XL9555 依赖 I2C_NUM_0）
- `my_spi_init()` 必须在 `spilcd_init()` 之前（LCD 依赖 SPI 总线）
- `xl9555_init()` 必须在 `spilcd_init()` 和 `camera_init()` 之前（LCD 和摄像头的 RST/PWDN 通过 XL9555 控制）

---

## 六、数据流

```
OV5640 摄像头
    │
    │ DVP 接口（8-bit 并行）
    │ IO4-IO18, IO45=PCLK, IO47=VSYNC, IO48=HREF
    ▼
PSRAM 双缓冲（RGB565, 320x240）
    │
    │ esp_camera_fb_get() 获取帧数据
    ▼
CPU 拷贝到 LCD 缓冲区
    │
    │ esp_lcd_panel_draw_bitmap()
    ▼
SPI2 总线（IO11=MOSI, IO12=SCK）
    │
    │ CS=IO21, DC=IO40
    ▼
ST7789 LCD 显示
```

---

## 七、常见问题与解决

### 1. 头文件找不到

检查 CMakeLists.txt 中是否把该组件加入了 `include_dirs` 和 `REQUIRES`。

### 2. 重复符号错误（链接时）

同一个源文件被多个组件编译。检查是否有多个 CMakeLists.txt 包含了同一个目录的源文件。解决方法：只在一个组件中编译，其他组件通过 `REQUIRES` 依赖它。

### 3. I2C/SPI 端口冲突

同一个 I2C 端口不能被初始化两次。如果库会自动初始化某个端口，就不要手动初始化它。

### 4. 宏定义冲突

多个头文件定义了同名宏（如 `CAM_PIN_SIOD`）。后定义的会覆盖先定义的。解决方法：只在一个地方定义，其他地方引用。

### 5. 编译器内部错误（ICE）

```
internal compiler error: Segmentation fault
```

这是 GCC 编译器自身的 bug，不是代码问题。解决方法：
```bash
idf.py fullclean
idf.py build
```

---

## 八、添加新组件的 Checklist

- [ ] 在 `idf_component.yml` 中声明注册表依赖（如有）
- [ ] 创建组件目录和 `CMakeLists.txt`
- [ ] 在 `CMakeLists.txt` 中正确设置 `SRCS`、`INCLUDE_DIRS`、`REQUIRES`
- [ ] 确保头文件目录在 `include_dirs` 中（否则其他组件找不到）
- [ ] 确保依赖的组件在 `REQUIRES` 中
- [ ] 在上层组件的 `CMakeLists.txt` 中添加对该组件的 `REQUIRES`
- [ ] 在代码中 `#include` 对应头文件
- [ ] 检查初始化顺序是否正确
- [ ] 检查是否有宏定义冲突
- [ ] 编译测试
