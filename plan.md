# LCD 按键无响应问题修复记录

## 问题描述

按钮按下后，串口日志显示按键事件被正常检测和处理（menu_index 正确切换），
但 LCD 屏幕完全没有视觉变化——初始菜单能显示，但后续按键不刷新。

## 根本原因

经过与正常项目（991_lvgl）对比，发现了**三个独立问题叠加**导致：

### 问题 1（根本原因）：缺少 LVGL 时钟源

项目使用 LVGL v9（managed_components/lvgl__lvgl），但从未设置 `lv_tick_inc()`。
LVGL 的 `lv_timer_handler()` 依赖 `lv_tick_get()` 判断内部定时器是否到期。
没有 tick 源，初始渲染后的增量脏区域检测和重绘机制完全失效。

正常项目（LVGL v8）通过 `esp_timer` 每 1ms 调用 `lv_tick_inc(1)`。

### 问题 2：DMA 回调与 `lv_display_flush_ready()` 的竞态

原 `flush_cb` 是异步的——启动 DMA 后立即返回，依赖 DMA 回调调用
`lv_display_flush_ready()`。如果回调未触发（竞态或硬件问题），
LVGL 永远认为上一次 flush 仍在进行中，拒绝调用后续 `flush_cb`。

### 问题 3：`spilcd_show_char` 中 `refresh_done_flag` 的竞态

`spilcd_show_char` 在调用 `esp_lcd_panel_draw_bitmap()` **之后**才设置
`refresh_done_flag = 0`。如果 DMA 极速完成，回调先把 flag 设为 1，
随即被覆写为 0，导致每次字符渲染额外等待 200ms 超时。

---

## 修改的文件

### 1. `components/LVGL/Lvgl_Port/lvgl_port.c`

#### 1a. 添加 LVGL 时基（核心修复）

```c
// 新增：LVGL tick 回调
static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(1);
}

// 在 lvgl_port_init() 中新增：
const esp_timer_create_args_t tick_timer_args = {
    .callback = &lvgl_tick_cb,
    .name = "lvgl_tick"
};
esp_timer_handle_t tick_timer = NULL;
ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000));
```

这确保 `lv_tick_get()` 返回递增的时间戳，LVGL 内部定时器正常工作。

#### 1b. `flush_cb` 改为同步模式

原来的异步流程：
```
flush_cb: 启动 DMA → 立即返回 → 依赖 DMA 回调调用 lv_display_flush_ready()
                                        ↓
                              如果回调未触发 → LVGL 永远等待 → 屏幕不刷新
```

修改后的同步流程：
```
flush_cb: 等待上一次 DMA → 重置 flag → 启动 DMA → 等待 DMA 完成 → 直接调用 lv_display_flush_ready()
```

关键变化：
- `lv_display_flush_ready()` 由 `flush_cb` 自己在 DMA 完成后直接调用
- DMA 回调中的 `lvgl_flush_ready_notify()` 改为空函数，避免双重通知
- 增加超时保护（500ms），防止 DMA 异常时死锁

#### 1c. 移除 `lvgl_keypad` 双重输入

移除了 `lvgl_keypad_init()` 调用和 group 绑定代码。

之前有两套独立的按键系统同时读取同一个物理按键：
- `key_task`（key.c）→ event_bus → LVGL task 处理
- `lvgl_keypad.c` → LVGL input device → lv_group 焦点

同一个按键被处理两次，造成状态混乱。移除后只保留 `key_task` + event_bus。

---

### 2. `components/BSP/SPILCD/spilcd.c`

#### 修复 `spilcd_show_char` 竞态条件

**修改前（有 bug）：**
```c
esp_lcd_panel_draw_bitmap(...);   // 先启动 DMA
refresh_done_flag = 0;            // 再重置 flag —— DMA 可能已经完成并设回 1
```

**修改后：**
```c
refresh_done_flag = 0;            // 先重置 flag
esp_lcd_panel_draw_bitmap(...);   // 再启动 DMA
```

确保 `refresh_done_flag` 在 DMA 启动前被清零，
回调设 1 后不会被意外覆写。

---

## 修改文件清单

| 文件 | 修改内容 |
|------|----------|
| `components/LVGL/Lvgl_Port/lvgl_port.c` | 添加 lv_tick_inc(1) 定时器；flush_cb 改同步；移除 lvgl_keypad |
| `components/BSP/SPILCD/spilcd.c` | 修复 spilcd_show_char 中 refresh_done_flag 竞态 |

## 验证结果

修改后串口日志确认：
1. 初始菜单渲染 4 次 flush_cb ✓
2. 按键后 menu_index 正确切换 ✓
3. LCD 屏幕正确响应按键并刷新 ✓

---

# 摄像头拍照、照片保存与相册浏览问题修复

## 问题描述

1. **拍照显示旧照片**：进入拍照模式后显示的是上一次的照片，需要连续操作三次才能看到新照片
2. **拍照后 MCU 崩溃重启**：按 BOOT/KEY0 拍照后，MCU 直接重启
3. **相册为空**：进入相册显示 0 张照片，实际 SD 卡中有照片
4. **相册浏览崩溃**：浏览照片时 `LoadProhibited` 崩溃
5. **新拍照片不在相册中**：拍完照片进入相册，数量不变

## 涉及文件

| 文件 | 修改内容 |
|------|----------|
| `components/BSP/CAMERA/camera.c` | 帧缓冲排空修复、同/异步保存、SPI 安全顺序 |
| `components/BSP/CAMERA/camera.h` | 新增 `camera_wait_save_done()` 声明 |
| `components/BSP/SD/sd_init.c` | `photo_count` 保存成功后才递增、0字节文件检测 |
| `components/BSP/SD/sd_init.h` | 新增 `sd_scan_photos()`、`sd_read_photo()` 声明 |
| `components/LVGL/Lvgl_Screens/lvgl_screens.c` | 相册图片显示修复、进入相册等待保存完成 |
| `components/LVGL/Lvgl_Screens/lvgl_screens.h` | 新增 gallery 浏览函数声明 |
| `components/LVGL/Lvgl_Port/lvgl_port.c` | 相册入口刷新显示、重置索引 |
| `components/Middlewares/HTTP/http_server.c` | HTTP 视频流加互斥锁保护 |
| `main/main.c` | `save_task` 优先级 1→4 |

---

## 根因分析

### Bug 1：拍照显示旧照片 + 需要点三次

**根因：** `fb_count=5` 但 `camera_takephoto()` 只排空 3 帧旧数据。

`CAMERA_GRAB_WHEN_EMPTY` 模式下，camera_task 处于 IDLE 时无人取帧，5 个缓冲全部保留过时帧。进入拍照时只排空 3 帧，剩余 2 帧旧数据。重新进入拍照模式时 5 个缓冲全是上一次会话的旧帧，需多次操作才能循环排出。

**修复：** 排空 `fb_count`（5）帧 + 等待 66ms（~2 帧 @30fps）让传感器输出稳定新帧。`PHOTO_SNAP` 进入拍照模式时同样处理。

### Bug 2：拍照后 MCU 崩溃重启（SPI 总线冲突）

**根因：** ESP32-S3 双核同时访问共享 SPI 外设。

LCD 和 SD 卡共用同一个 SPI 总线。`camera_takephoto()` 在保存之前发送 `EVT_CAM_PHOTO_DONE`，另一核的 LVGL 任务收到后立即标记 label 为脏 → `lv_timer_handler` → `flush_cb` → `esp_lcd_panel_draw_bitmap`（LCD SPI），与当前核的 `sd_save_jpeg` → `fclose`（SD SPI）同时发生。

```
Core 0: camera_task                Core 1: lvgl_port_task
  event_bus_send(PHOTO_DONE) ──→   收到事件 → label 脏
  sd_save_jpeg → SD SPI              lv_timer_handler → LCD SPI
       ↑＿＿＿＿＿＿ SPI 硬件冲突 ＿＿＿＿＿＿↑
  ASSERT: spi_ll_get_running_cmd(hw) == 0 → 系统崩溃
```

**修复：** 改为同步保存，先保存后发事件：

```
1. LCD 写屏完成（SPI 空闲）
2. sd_save_jpeg（SD SPI，独占）
3. 保存完成后才发送 EVT_CAM_PHOTO_DONE + EVT_SD_SAVE_DONE
4. LVGL 收到事件后才触发 LCD 刷新（SPI 空闲，安全）
```

同时将保存从异步队列改为 `camera_task` 内同步执行，利用其最高优先级（7）确保保存期间不被其他任务打断。

### Bug 3：相册浏览崩溃（lv_canvas_init_layer）

**根因：** `gallery_view_img` 是通过 `lv_img_create()` 创建的 **图片对象**，但 `show_gallery_photo()` 却对它调用 `lv_canvas_init_layer()` 等 **canvas 专属函数**。canvas 函数内部访问了图片对象不存在的扩展数据，导致 `LoadProhibited`（访问 `0x38` 非法地址）。

**修复：** 改用 LVGL v9 正确方式——构建 `lv_image_dsc_t` 描述符包装解码后的 RGB565 数据，通过 `lv_image_set_src()` 设置到 `lv_img` 对象上。

```c
// ✅ 正确：lv_img 用 lv_image_set_src
static lv_image_dsc_t gallery_img_dsc;
gallery_img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
gallery_img_dsc.header.w = outimg.width;
gallery_img_dsc.header.h = outimg.height;
gallery_img_dsc.data = gallery_decode_buf;
gallery_img_dsc.data_size = outimg.width * outimg.height * 2;
lv_image_set_src(gallery_view_img, &gallery_img_dsc);
```

### Bug 4：相册看不到新拍的照片

**根因：** 之前的异步保存流程：拍照 → 入队 save_queue → 立即返回 → 用户进相册 → `sd_scan_photos()` 扫描。若 `save_task` 还没来得及写入 SD 卡，扫描就找不到新文件。

加上 `save_task` 优先级为 1（最低），被 camera(7)、key(5)、lvgl(3)、fps(2) 所有任务压制，可能长时间得不到执行。

**修复：** 改为同步保存后此问题自然解决——进入相册时照片已经写入 SD 卡。同时保留 `camera_wait_save_done()` 作为额外保障。

### Bug 5：`sd_save_jpeg` 编号留空

**根因：** `photo_count++` 在 `fopen` 之前执行。若 `fopen` 失败，计数已递增但文件未创建，`sd_scan_photos()` 顺序扫描遇空即停。

```c
// ❌ 旧代码
photo_count++;  // 先递增
fopen(...);     // 失败了 → 编号留空，扫描中断
```

**修复：** 保存成功后才更新 `photo_count`。

### Bug 6：0 字节文件导致 malloc(0) 失败

**根因：** 之前的 SPI 崩溃导致部分照片文件创建了但未写入数据（0字节）。`sd_read_photo` 中 `malloc(0)` 返回 NULL，无法读取。

**修复：** `sd_read_photo` 增加 `*size == 0` 检测，提前返回错误。

---

## 踩坑事项

### 1. ESP32-S3 双核 SPI 共享冲突

**这是本次最隐蔽的坑。** LCD 和 SD 卡在同一个 SPI 主机（SPI2_HOST）上，通过不同 CS 引脚区分。ESP-IDF 的 SPI 驱动理论上支持多设备共享同一主机，但前提是所有设备都正确使用驱动的 `spi_device_acquire_bus()` / `spi_device_release_bus()` 机制。

实际情况是：
- LCD 通过 `esp_lcd_panel_draw_bitmap()` 直接写 SPI，不经过标准的 `spi_device_transmit`
- SD 卡通过 `sdspi_host_do_transaction` → `spi_device_polling_transmit` 访问 SPI
- 两者在双核上可能真正同时执行，导致硬件断言 `spi_ll_get_running_cmd(hw) == 0` 失败

**教训：** 在 ESP32-S3 上如果多个外设共享 SPI 总线：
1. 优先使用不同 SPI 主机（SPI2、SPI3）
2. 无法分主机时，确保所有 SPI 操作在**同一任务**内顺序执行，或在**同一核心**上运行
3. 不要在 SPI 操作期间通过事件总线通知其他任务（其他核的任务可能立即触发 LCD 刷新）
4. `refresh_done_flag` 只能做单核同步，不能替代跨核互斥锁

### 2. `CAMERA_GRAB_WHEN_EMPTY` 模式下的帧队列管理

此模式下，当所有缓冲都满时传感器**停止抓帧**。进入 IDLE 后如果不主动排空，队列中全是过时帧。
- 排空帧数必须 ≥ `fb_count`
- 排空后必须等待传感器重新产出新帧（至少 2 帧时间）
- `esp_camera_fb_get()` 是阻塞调用，队列空时会无限等待——循环次数要精确控制

### 3. LVGL v9 的 `lv_img` vs `lv_canvas`

LVGL v9 中这两个是**完全不同的对象类型**：
- `lv_img_create()` → `lv_image_set_src()` + `lv_image_dsc_t`
- `lv_canvas_create()` → `lv_canvas_init_layer()` + `lv_canvas_finish_layer()`

混用会导致访问非法内存。LVGL 内部对不同类型的对象有不同的扩展数据结构，canvas 函数访问的偏移量在 img 对象上对应的是完全不相关的内存。

### 4. 异步保存的"假安全感"

异步保存（队列 → 低优先级任务）看起来"不阻塞 UI"，但引入了多个问题：
- 保存状态不可知（保存成功/失败无反馈）
- 相册扫描时文件可能还未写入
- 低优先级任务可能被永久饥饿
- 引入跨核 SPI 竞争

**对于嵌入式拍照场景，同步保存是最可靠的方案。** 一张 QVGA JPEG 写入 SD 卡只需 10-50ms，完全在可接受范围内。

### 5. `malloc(0)` 在嵌入式平台的行为

标准 C 中 `malloc(0)` 的行为是实现定义的——可能返回 NULL，也可能返回非 NULL 但不可用的指针。ESP-IDF 的 picolibc 实现会返回 NULL。读取文件时务必先检查文件大小，避免 `malloc(0)`。

### 6. 事件发送顺序在双核系统中的重要性

`event_bus_send()` 是广播到所有消费者的队列。在双核系统上，事件可能被另一核**立即**处理。如果事件触发 SPI 操作（如 LVGL 刷新 LCD），而发送方仍在执行 SPI 操作（如 SD 卡写入），就会冲突。

**原则：先完成所有硬件操作，再发送事件通知其他任务。**

---

# 网页端拍照按钮（HTTP /capture）

## 问题描述

在监视页面（MJPEG 视频流）上添加"拍照"按钮，点击后通过 HTTP 请求触发 ESP32 拍照并保存到 SD 卡。

## 涉及文件

| 文件 | 修改内容 |
|------|----------|
| `components/Middlewares/HTTP/http_server.c` | 新增 `/capture` 路由；前端按钮+JS；设 display_mode |
| `components/BSP/CAMERA/camera.h` | 新增 `capture_done_sem` 信号量声明（后移除） |
| `components/BSP/CAMERA/camera.c` | 新增拍照完成信号量（后移除） |

## 踩坑事项

### 7. ESP-IDF HTTP 服务器的单线程阻塞陷阱

**根因：** ESP-IDF HTTP 服务器使用**单线程**处理所有请求。当 `/stream` 的 MJPEG handler 进入 `while(true)` 循环不断发送 JPEG 帧时，服务器线程永远困在该 handler 内，无法接收或处理任何新连接。

```
HTTP 服务器线程:
  while(true) {
    esp_camera_fb_get()          ← 阻塞等帧
    httpd_resp_send_chunk()      ← 发送 JPEG
    // 困在这里，/capture 请求永远等不到处理！
  }
```

**表现：** 前端 `fetch('/capture')` 发出后毫无反应，串口无 `"Capture requested"` 日志。

**修复：** 前端在发 `/capture` 请求前先停掉视频流（`img.src=''`），浏览器关闭流连接后 `stream_handler` 退出，服务器线程恢复空闲，200ms 后再发 fetch 请求，处理完成后再恢复流（`img.src='/stream'`）。

### 8. `img.src=''` 断流触发的一连串"假"错误

**表现：** 串口出现以下日志：
```
error in send : 104                    ← ECONNRESET，客户端关闭连接
Failed to send multipart boundary     ← 服务器还在发流数据
uri handler execution failed          ← stream_handler 异常退出
Method '2' not allowed for '/stream'  ← 浏览器尝试对 /stream 发 POST
```

**根因：** `img.src=''` 导致浏览器直接关闭 TCP 连接，但 ESP32 服务器此时可能仍在 `stream_handler` 内尝试发送下一帧。这些全部是**预期行为**，不是 bug。只有 `fetch('/capture')` 也失败时才需要调大断流后的等待延迟（`setTimeout` 的 ms 数）。

### 9. 网页拍照后 KEY2 无法返回菜单

**根因：** 网页拍照只发 `EVT_KEY_SHUTTER` 触发 `camera_task` 拍照和 LCD 写屏，但 `display_mode` 未改变。`lvgl_port_task` 中 KEY2 的返回逻辑是：

```c
else if (event_bus_get_display_mode() != DISPLAY_MODE_MENU)
    lv_scr_load(scr_menu);
```

若拍照前用户恰好在 MENU 页面（`display_mode == MENU`），拍照后 `display_mode` 仍是 MENU，KEY2 判断 `MENU != MENU` 为 false → **按键被忽略**。

**修复：** 在 `capture_handler` 中先执行 `event_bus_set_display_mode(DISPLAY_MODE_PHOTO)`，再发快门事件。这样 KEY2 判断 `PHOTO != MENU` 为 true → 正常返回菜单。

### 10. 信号量等待的延迟体感问题

最初方案用信号量让 `/capture` handler **等待拍照完成**（`xSemaphoreTake(sem, 5s)`）。拍照完整流程约 500ms（排空帧 165ms + 等待 66ms + 解码 + LCD 写屏 + SD 保存），HTTP 请求被阻塞半秒。

**修复：** 改为 fire-and-forget 模式——发完 `EVT_KEY_SHUTTER` 立即返回 `{"status":"ok"}`，不等待拍照完成。前端按钮即时反馈（变灰 + "Capturing..."），拍照在 ESP32 本地异步完成。

**代价：** HTTP 无法告知前端保存结果（成功/失败）。可接受——LCD 和串口有本地反馈。

## 最终方案流程

```
浏览器端:                                 ESP32 端:
  [Take Photo] 点击
    → 按钮变灰 "Capturing..."
    → img.src='' 断流                    → stream_handler 退出
    → setTimeout 200ms                    → 服务器线程空闲
    → fetch('/capture') ──────────────→  capture_handler:
                                          display_mode = PHOTO
                                          send EVT_KEY_SHUTTER
                                          返回 {"status":"ok"}
    ← ────────────────────────────────
    → 按钮显示 "Done!"
    → img.src='/stream' 恢复流 ──────→  stream_handler 重新运行
    → 1.5s 后按钮恢复
                                          camera_task 异步拍照
                                          排空帧 → 解码 → LCD → SD 保存
                                          
  ESP32 LCD: KEY2 → display_mode=PHOTO → 返回菜单 ✓
```
---

# AI 功能集成阶段总结

## 1. 开发环境与模型部署

- 统一使用 ESP-IDF 6.0.2，修正 manifest 版本限制。
- 接入 `espressif/coco_detect` 与 `espressif/esp-dl`。
- 将 `yolo11n.pt` 转换并部署为 `coco_detect_yolo11n_320_s8_v3.espdl`。
- 增加 `coco_det` 模型分区，大小为 3 MB，容纳约 2.86 MB 模型。
- menuconfig 启用 `FLASH_COCO_DETECT_YOLO11N_320_S8_V3`、`COCO_DETECT_YOLO11N_320_S8_V3` 和 `COCO_DETECT_MODEL_IN_FLASH_PARTITION`。

## 2. 人脸检测与双模式控制

- 接入 ESP-DL 人脸检测模型，支持多人脸、置信度和 RGB565 检测框绘制。
- 增加 `components/AI/detector_config.h`，用两个宏控制检测模式：

```c
#define AI_ENABLE_FACE_MODE 0
#define AI_ENABLE_YOLO11N_MODE 1
```

- 编译期强制两个宏必须恰好有一个为 `1`，人脸检测和 YOLO 不会同时初始化。

## 3. 摄像头、LCD 与异步 AI

- 将 AI 推理移到独立 `camera_ai_task`，摄像头采集、JPEG 解码和 LCD 刷新不直接等待推理。
- 使用 PSRAM 输入帧、帧互斥锁、结果互斥锁和忙状态管理。
- 使用 `CAMERA_GRAB_LATEST`，优先处理最新帧。
- AI 取帧间隔由每 3 帧调整为每 10 帧，减少 PSRAM 带宽争用。
- 新一轮 YOLO 推理开始时清除旧结果，避免检测框残留。
- 增加 LCD DMA 内部 bounce buffer，修复 SPI DMA 分配失败和花屏问题。
- HTTP MJPEG 使用持久化 PSRAM 缓冲区，减少反复 malloc/free。

## 4. YOLO 类别显示

- 增加 COCO 80 类别名称映射。
- 串口打印类别编号、类别名称、框坐标和置信度。
- LCD 框上显示类别名称和置信度，例如 `person 92%`。
- 复用 SPILCD 字体时改用 `extern` 声明，解决字体数组重复定义链接错误。

## 5. 双核推理优化

当前任务绑定关系：

| 核心 | 任务 |
|---|---|
| CPU0 | 摄像头、JPEG 解码、LCD、LVGL |
| CPU1 | AI 推理 |

ESP-DL 模型运行方式由默认单核改为：

```cpp
m_model->run(dl::RUNTIME_MODE_MULTI_CORE);
```

实测 YOLO11n 单次检测从约 17 秒降低到约 4 秒，说明双核路径已经生效。

## 6. 已知限制与后续方向

- 当前约 4 秒/次，约 0.25 FPS，仍不是实时目标检测。
- `FB-OVF` 表示摄像头帧缓冲区被占满，可能造成丢帧。
- `task_wdt` 表示推理占用时间过长，触发任务看门狗提示。
- 若需要更高帧率，应使用 160×160、192×192 或 224×224 的轻量模型，或将 YOLO 推理移至 PC/服务器。
# 当前代码整体架构说明

## 1. 分层结构

当前工程采用“硬件 BSP + 应用事件总线 + UI + AI + 网络中间件”的分层结构：

```text
app_main()
 ├─ BSP：摄像头、LCD、按键、SD、Wi-Fi、IO 扩展
 ├─ APP：event_bus / app_event
 ├─ LVGL：页面创建、页面切换、按键事件处理
 ├─ AI：人脸检测或 YOLO11n 目标检测
 └─ Middlewares：HTTP 页面、MJPEG 视频流、拍照接口、Wi-Fi 设置
```

各层的职责如下：

| 层 | 主要目录 | 职责 |
|---|---|---|
| 程序入口 | `main/main.c` | 初始化系统、创建 FreeRTOS 任务、选择 AI 模式 |
| 硬件 BSP | `components/BSP` | 封装摄像头、LCD、按键、SD 卡、Wi-Fi 和 GPIO |
| 应用通信 | `components/APP` | 定义事件类型并通过 event bus 广播事件 |
| UI | `components/LVGL` | LVGL 初始化、页面创建、菜单和设置页面 |
| AI | `components/AI` | 人脸/目标检测模型、预处理、结果和框绘制 |
| 网络中间件 | `components/Middlewares/HTTP` | HTTP 服务、视频流、拍照和 Wi-Fi 配置 |
| 资源 | `components/Resource` | LCD 图片和其他显示资源 |

## 2. 启动流程

`app_main()` 的初始化顺序为：

```text
NVS
 ↓
网络接口和事件循环
 ↓
event_bus
 ↓
SPI、I2C、IO 扩展、LCD
 ↓
摄像头和 SD 卡
 ↓
根据 detector_config.h 初始化人脸或 YOLO
 ↓
LVGL
 ↓
Wi-Fi 和 HTTP 服务
 ↓
创建 key、camera、AI、LVGL、FPS 任务
```

AI 模式在编译期决定，而不是运行时动态切换。`detector_config.h` 要求两个宏恰好一个为 `1`：

```c
#define AI_ENABLE_FACE_MODE 0
#define AI_ENABLE_YOLO11N_MODE 1
```

因此，未选中的模型不会在 `app_main()` 中初始化，也不会进入摄像头 AI 任务的推理分支。

## 3. FreeRTOS 任务和双核分工

| 任务 | 核心 | 优先级 | 主要工作 |
|---|---:|---:|---|
| `camera_task` | CPU0 | 7 | 获取摄像头帧、JPEG 解码、LCD 预览、拍照状态机 |
| `camera_ai_task` | CPU1 | 4 | 从共享 AI 帧缓冲区取帧并执行检测 |
| `lvgl_port_task` | CPU0 | 3 | LVGL 定时器、页面刷新和 UI 事件 |
| `key_task` | 未固定 | 5 | 扫描物理按键并发送应用事件 |
| `fps_task` | 未固定 | 2 | 统计录像预览帧率 |

摄像头和 LCD 任务固定在 CPU0，AI 任务固定在 CPU1。YOLO 推理内部使用：

```cpp
m_model->run(dl::RUNTIME_MODE_MULTI_CORE);
```

因此模型计算阶段会使用两个核心；摄像头任务仍负责维持显示流水线。

## 4. 摄像头预览流水线

预览模式下的单帧流程：

```text
esp_camera_fb_get()
 ↓
获取 JPEG 帧
 ↓
esp_jpeg_decode()
 ↓
输出 320×240 RGB565
 ↓
camera_process_ai()
 ├─ 必要时复制一帧到 PSRAM AI 缓冲区
 └─ 将上一轮检测结果绘制到当前帧
 ↓
按 40 行分块复制到内部 DMA 缓冲区
 ↓
esp_lcd_panel_draw_bitmap()
 ↓
等待 LCD DMA 完成
```

摄像头使用 `PIXFORMAT_JPEG`、`FRAMESIZE_QVGA` 和 `CAMERA_GRAB_LATEST`。`CAMERA_GRAB_LATEST` 的目的，是在处理速度不足时优先丢弃旧帧，而不是让显示长时间追赶历史帧。

## 5. AI 异步流水线

AI 不在 `camera_task` 中直接运行，而是采用生产者/消费者结构：

```text
camera_task
  └─ camera_process_ai()
       └─ 复制 RGB565 到 ai_frame_buf
              ↓
         ai_frame_pending = true
              ↓
camera_ai_task
  └─ 取出 ai_frame_buf
       ├─ 人脸模式：ai_face_detect_rgb565()
       └─ YOLO 模式：object_detect_rgb565()
              ↓
         写入结果数组
              ↓
camera_process_ai()
  └─ 读取结果快照并绘制检测框/标签
```

共享状态由以下对象保护：

- `ai_frame_mutex`：保护 AI 输入帧和 pending/busy 状态；
- `ai_result_mutex`：保护人脸或目标检测结果；
- `ai_frame_pending`：表示有待处理帧；
- `ai_frame_busy`：表示 AI 正在推理；
- `ai_object_request`：表示当前帧是否真正触发 YOLO 推理。

YOLO 推理不是每个显示帧都执行。当前代码用 `AI_DETECT_INTERVAL` 降低复制频率，并用 `OBJECT_DETECT_INTERVAL` 控制目标检测触发频率。推理期间不会阻塞 LCD 当前帧的发送。

## 6. 检测结果和显示

人脸结果使用 `ai_face_result_t`，目标结果使用 `object_detect_result_t`，两者都包含：

```text
x1, y1, x2, y2, score
```

YOLO 结果额外包含 `category`。`object_detect_category_name()` 将 COCO 类别编号转换为类别名称。

结果有两个输出：

1. 串口输出类别、框坐标、置信度和推理耗时；
2. 在 RGB565 帧上绘制矩形框和类别标签，然后随当前 LCD 帧显示。

标签绘制复用了 SPILCD 的 ASCII 字体数据，但 `camera.c` 只使用 `extern` 声明，避免字体数组重复定义。

## 7. 事件总线和页面切换

按键任务不直接操作 LVGL，而是发送应用事件：

```text
key_task
  ↓
event_bus_send()
  ↓
lvgl_port_task / camera_task / HTTP 消费者
```

典型事件包括：

- `EVT_KEY_BOOT`：进入当前菜单项；
- `EVT_KEY_BACK`：返回上一级；
- `EVT_KEY_SHUTTER`：拍照或触发拍摄；
- `EVT_DISP_ENTER_PREVIEW`：进入录像预览；
- `EVT_CAM_STOPPED`：摄像头停止后通知 LVGL 恢复菜单；
- `EVT_SD_SAVE_DONE`：照片保存完成。

返回菜单时，`camera_task` 先停止预览并发送 `EVT_CAM_STOPPED`，LVGL 收到确认后再刷新菜单，从而避免 LCD DMA 与摄像头预览同时操作显示设备。

## 8. HTTP 数据路径

HTTP 服务提供三类主要功能：

```text
/          设置页或监控页面
/stream    MJPEG 视频流
/capture   触发拍照
```

HTTP 视频流单独获取摄像头 JPEG 帧，不直接使用 LCD 的 RGB565 缓冲区。浏览器访问 `/capture` 时通过事件总线触发本地拍照流程，拍照和 SD 保存由摄像头任务完成。

## 9. 内存分工

| 缓冲区 | 位置 | 用途 |
|---|---|---|
| 摄像头 JPEG 帧 | PSRAM | OV5640 输出的 JPEG 数据 |
| `jpeg_decode_buf` | DMA 兼容内存/必要时 PSRAM | JPEG 解码到 RGB565 |
| `camera_lcd_dma_buf` | 内部 DMA SRAM | LCD SPI DMA 分块发送 |
| `ai_frame_buf` | PSRAM | AI 异步推理输入 |
| YOLO 模型和中间张量 | Flash + PSRAM/内部 RAM | 模型权重和 ESP-DL 推理 |

内部 SRAM 主要留给 DMA、任务栈和实时控制；大图像和模型数据优先使用 PSRAM。LCD DMA 不能直接依赖普通 PSRAM 缓冲区，因此增加了内部 bounce buffer。

## 10. 当前架构的主要限制

- YOLO11n 320 模型在 ESP32-S3 上仍属于低帧率推理，实测双核后约 4 秒/次；
- 双核推理会占用 CPU0，推理期间 LCD 可能出现短暂降帧；
- `FB-OVF` 表示摄像头帧缓冲区被占满，意味着采集速度高于消费速度；
- `task_wdt` 表示一次推理持续时间过长，可能触发任务看门狗提示；
- 如需实时检测，应使用更小输入尺寸模型，或将推理迁移到 PC/服务器。
# 代码解耦补充：摄像头 AI 子模块

为避免 `camera.c` 同时承担摄像头驱动、JPEG 解码、LCD DMA、AI 队列和检测结果绘制等过多职责，已将 AI 相关逻辑拆分为：

| 文件 | 职责 |
|---|---|
| `components/BSP/CAMERA/camera.c` | 摄像头初始化、帧获取、JPEG 解码、LCD 分块发送、拍照状态机 |
| `components/BSP/CAMERA/camera_ai.c` | AI 缓冲区、互斥锁、异步推理任务、结果快照、检测框和标签绘制 |
| `components/BSP/CAMERA/camera_ai.h` | AI 子模块的初始化、帧处理和任务入口接口 |
| `components/BSP/CAMERA/camera.h` | 摄像头公共接口，并包含 `camera_ai.h` |

新的调用关系为：

```text
init_camera()
 └─ camera_ai_init()

camera_record()
 └─ camera_ai_process_frame()
      ├─ 投递最新 RGB565 帧
      └─ 绘制上一轮检测结果

main.c
 └─ camera_ai_task()
      └─ camera_ai.c 内部执行人脸或 YOLO 推理
```

这样摄像头主文件不再直接依赖人脸/目标检测的数据结构和同步变量。今后替换模型、修改检测频率或增加检测后处理时，主要修改 `camera_ai.c`，不会影响摄像头采集和 LCD DMA 状态机。
# 最近一次修改总结：OV5640 自动对焦与摄像头解耦

## 1. OV5640 自动对焦接入

- 确认正点原子 OV5640 模组支持 AF 镜头和内部 AF 固件。
- 使用 ESP-IDF 6.0.2 自带的 `esp_camera_af` API，不再手写 AF 固件数组和寄存器下载流程。
- 启用配置：

```text
CONFIG_CAMERA_AF_SUPPORT=y
```

- 摄像头 SCCB 引脚配置为：

```c
#define CAM_PIN_SIOD GPIO_NUM_39
#define CAM_PIN_SIOC GPIO_NUM_38
```

- 摄像头初始化后自动执行一次单次对焦。
- 初始化时使用 5 秒 AF 超时，并打印对焦状态：

```text
CAM: OV5640 autofocus: raw=0x10 focused=1 busy=0
```

## 2. I2C 总线冲突修复

启用 AF 后，摄像头会真正使用 SCCB I2C1。原来的 `myiic_init()` 同时初始化了 I2C0 和 I2C1，导致：

```text
I2C bus id(1) has already been acquired
camera: Camera probe failed
```

现在的总线分工为：

```text
I2C0：IO41/42 → XL9555
I2C1：IO39/38 → OV5640 SCCB 和 AF
```

`myiic_init()` 只初始化 I2C0，I2C1 交由 `esp_camera` 驱动创建和管理。

## 3. 录像模式 KEY0 单次对焦

新增应用事件：

```c
EVT_KEY_FOCUS
```

按键行为现在为：

| 页面 | KEY0 行为 |
|---|---|
| 录像预览 | 触发一次自动对焦 |
| 拍照页面 | 拍照 |
| 其他页面 | 保持原有逻辑 |

调用流程：

```text
KEY0
 ↓
EVT_KEY_FOCUS
 ↓
camera_task
 ↓
camera_autofocus_once()
 ↓
esp_camera_af_trigger()
 ↓
esp_camera_af_wait()
```

成功时串口输出：

```text
KEY: Sending EVT_KEY_FOCUS
CAM: Starting OV5640 single autofocus
CAM: Autofocus completed: raw=0x10 focused=1 busy=0
```

录像页面提示文字同步改为：

```text
KEY0:Focus  KEY2:Back
```

## 4. 摄像头 AI 文件解耦

原来的 `camera.c` 同时负责摄像头、LCD、AI 缓冲区、检测任务和标签绘制，已拆分为：

| 文件 | 职责 |
|---|---|
| `camera.c` | 摄像头初始化、取帧、JPEG 解码、LCD DMA、拍照状态机和 AF 控制 |
| `camera_ai.c` | AI 缓冲区、异步推理、结果同步、检测框和标签绘制 |
| `camera_ai.h` | AI 子模块接口 |
| `camera.h` | 摄像头公共接口，并包含 AI 接口 |

主要接口：

```c
bool camera_ai_init(void);
void camera_ai_process_frame(uint8_t *rgb565, uint16_t width, uint16_t height);
void camera_ai_task(void *arg);
void camera_autofocus_once(void);
```

## 5. CMake 构建修复

为保证拆分后的 `camera_ai.c` 和其他 BSP 源文件全部参与编译，BSP 改为递归收集所有 C 文件：

```cmake
file(GLOB_RECURSE src_files
     "${CMAKE_CURRENT_LIST_DIR}/*.c")

idf_component_register(
    SRCS ${src_files}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES ${requires}
)
```

没有使用 `CONFIGURE_DEPENDS`，因为 ESP-IDF 6.0.2 的组件需求扫描脚本不支持该参数。

## 6. MCU 采集、PC HTTP 推理最终方案

本项目最终将 YOLO 目标检测从 ESP32-S3 MCU 迁移到 PC 端。MCU 不再加载
ESP-DL、COCO Detect 或 YOLO 模型，只负责摄像头采集、LCD 预览和 JPEG 网络传输。

### 6.1 总体数据流

```text
OV5640 摄像头
    ↓
ESP32-S3 esp_camera_fb_get()
    ↓ JPEG
ESP32 HTTP GET /frame
    ↓ image/jpeg
PC detect_server.py
    ↓ OpenCV 解码
Ultralytics YOLO yolo11n.pt
    ↓
检测框、类别、置信度
    ├─ /latest.json：结构化检测结果
    └─ /latest.jpg：绘制检测框后的 JPEG
    ↓
PC 浏览器页面显示
```

### 6.2 MCU 端职责

- 使用 OV5640 采集 QVGA JPEG 图像。
- 继续负责 LCD 本地预览、Wi-Fi 连接和摄像头状态机。
- 通过 `GET /frame` 返回一张最新 JPEG 图像。
- `/frame` 获取摄像头帧后复制 JPEG 数据，再归还摄像头帧缓冲，避免网络发送期间占用摄像头缓冲区。
- 不再创建 `camera_ai_task`，不再调用 `object_detect_init()`、`object_detect_rgb565()` 或 `camera_ai_process_frame()`。
- `camera_ai.c` 保留在仓库中作为历史参考，但已从 BSP 编译列表排除。

ESP32 端核心接口：

```http
GET /frame
Content-Type: image/jpeg
```

`/stream` 仍可用于原有 MJPEG 预览，但 PC YOLO 检测使用一次请求一帧的 `/frame`，避免长连接占用 ESP32 HTTP 服务线程。

### 6.3 PC 端职责

PC 端代码位于 `pc_server/`：

```text
pc_server/
├── detect_server.py
├── requirements.txt
└── yolo11n.pt
```

`detect_server.py` 启动后台检测循环，执行以下操作：

1. 请求 ESP32 的 `/frame` 接口。
2. 使用 OpenCV 将 JPEG 解码为图像。
3. 使用 Ultralytics 加载 `yolo11n.pt` 并执行 YOLO 推理。
4. 在图像上绘制类别、置信度和检测框。
5. 更新最新的 JPEG 图像和 JSON 检测结果。

PC HTTP 接口：

```text
/                  检测结果网页
/latest.jpg        绘制检测框后的最新 JPEG
/latest.json       最新检测结果 JSON
```

典型 JSON 格式：

```json
{
  "status": "ok",
  "detections": [
    {
      "class_id": 0,
      "label": "person",
      "score": 0.91,
      "x1": 35,
      "y1": 20,
      "x2": 180,
      "y2": 239
    }
  ]
}
```

### 6.4 构建资源调整

- 从 `main/idf_component.yml` 和 `dependencies.lock` 移除 `coco_detect`、`human_face_detect` 和 `esp-dl` 依赖。
- 从 `sdkconfig` 和 `sdkconfig.defaults` 移除 COCO YOLO 模型配置。
- 从 `partitions-16MiB.csv` 移除 `coco_det` 模型分区，并将释放的空间并入 `vfs` 分区。
- 将旧 `components/AI` 注册为空组件，避免 ESP-IDF 自动扫描该目录时继续解析已经删除的模型依赖。
- BSP 编译列表排除 `components/BSP/CAMERA/camera_ai.c`。

### 6.5 验证结果

验证顺序如下：

1. ESP-IDF 6.0.2 下固件可以重新配置、编译和烧录。
2. ESP32 成功连接 Wi-Fi，并通过串口输出实际 IP 地址。
3. 浏览器访问 `http://ESP32_IP/frame` 可以显示摄像头 JPEG 图像。
4. PC 端启动 `detect_server.py` 后可以持续获取 `/frame`。
5. 访问 PC 的 `/latest.json` 可以获得检测结果。
6. 访问 PC 的 `/latest.jpg` 或主页可以看到 YOLO 检测框。
7. 停止 PC 服务后，ESP32 的 `/frame` 仍然可以工作，证明 ESP32 只负责采集，YOLO 推理已经在 PC 端完成。

### 6.6 后续扩展

当前采用 PC 主动拉取模式，优点是 MCU 改动小、调试简单。若以后需要让 MCU 根据检测结果控制 LCD、继电器或舵机，可以在此基础上增加 `frame_id`，并采用 PC 返回 JSON 或 PC 向 MCU 推送检测结果的方式实现闭环控制。

---

# 低功耗系统设计补充

## 1. 设计目标

低功耗设计不是简单地调用一次睡眠函数，而是由电源策略、任务调度、外设挂起/恢复和唤醒源共同组成：

~~~text
应用状态判断
    ↓
资源引用计数确认没有活跃外设
    ↓
停止 HTTP、Wi-Fi、摄像头、SD、LCD 和 LVGL
    ↓
释放 CPU 频率和禁止睡眠锁
    ↓
FreeRTOS Tickless Idle 自动进入 Light Sleep
    ↓
GPIO0/其他唤醒源触发
    ↓
恢复系统锁和外设
    ↓
回到 ACTIVE 状态
~~~

设计原则：

- 任务不直接操作全局睡眠状态，由 power_manager 统一仲裁。
- 外设使用资源引用计数，任何活跃资源都可以阻止进入低功耗。
- 进入低功耗前先停止业务，再关闭外设电源或卸载文件系统。
- 唤醒后按照依赖关系恢复外设，不能在中断中执行 SD、Wi-Fi 或 LCD 初始化。
- Light Sleep 用于保持 CPU 上下文的运行时省电；Deep Sleep 只作为显式的系统级休眠，唤醒后会重新启动应用。

## 2. 低功耗模块划分

| 模块 | 位置 | 主要职责 |
|---|---|---|
| 电源状态机 | components/Power/power_manager | 管理 ACTIVE、IDLE、BLE_STANDBY、DEEP_SLEEP 状态 |
| 电源策略 | components/Power/power_policy | 配置 CPU 频率、自动 Light Sleep、空闲超时和唤醒 GPIO |
| 应用挂起恢复 | main/power_hooks.c | 按顺序停止和恢复 HTTP、LVGL、摄像头、SD、Wi-Fi、LCD |
| 按键唤醒 | components/BSP/KEY/key.c | 处理 GPIO0/BOOT 唤醒和普通按键事件 |
| ESP-IDF PM | esp_pm_configure() | 动态调频和 FreeRTOS Tickless Idle 自动 Light Sleep |

## 3. 状态机设计

当前状态定义为：

~~~c
POWER_STATE_ACTIVE
POWER_STATE_IDLE
POWER_STATE_BLE_STANDBY
POWER_STATE_DEEP_SLEEP_PENDING
POWER_STATE_DEEP_SLEEP
~~~

状态关系：

~~~text
ACTIVE ──空闲超时──> IDLE ──BOOT/GPIO──> ACTIVE
  │                    │
  │                    └──────────────> BLE_STANDBY
  │
  └─显式请求──> DEEP_SLEEP_PENDING ──> DEEP_SLEEP
                                      │
                                      └─唤醒后重新启动应用
~~~

IDLE 表示应用已经完成外设挂起并允许自动 Light Sleep；它不等同于已经执行了 esp_deep_sleep_start()。当前工程不会自动进入 Deep Sleep。

## 4. 进入低功耗的条件

自动进入 IDLE 的条件必须全部满足：

1. 当前状态为 POWER_STATE_ACTIVE。
2. idle_timeout_ms 大于 0，并且超过设定的无操作时间。
3. s_resource_mask == 0，没有任务持有摄像头、LCD DMA、SD、Wi-Fi、HTTP、OTA 或 BLE 连接资源。
4. 已注册 prepare_idle 挂钩。
5. 所有外设挂起操作成功。

当前策略参数位于：

~~~text
components/Power/power_policy/power_policy.h
~~~

当前空闲超时为 15000 ms。常用调整方式：

~~~c
#define POWER_POLICY_DEFAULT_IDLE_TIMEOUT_MS 15000  // 15 秒
#define POWER_POLICY_DEFAULT_IDLE_TIMEOUT_MS 30000  // 30 秒
#define POWER_POLICY_DEFAULT_IDLE_TIMEOUT_MS 0      // 禁止自动进入低功耗
~~~

业务任务在开始使用外设时调用 power_manager_acquire_resource(resource, "owner")，使用完成后调用 power_manager_release_resource(resource)，避免摄像头采集、SD 写入或 HTTP 传输过程中被挂起。

## 5. 进入 IDLE 时的外设处理顺序

当前 main/power_hooks.c 中的处理顺序为：

~~~text
1. 停止 HTTP server
2. 暂停 LVGL task 和 LVGL tick timer
3. 设置摄像头 suspended，关闭 OV5640 PWDN
4. 卸载 SD 卡文件系统
5. 停止 Wi-Fi
6. 关闭 LCD 显示并关闭 LCD 电源
7. 设置 POWER_STATE_IDLE
8. 释放 ESP_PM_CPU_FREQ_MAX
9. 释放 ESP_PM_NO_LIGHT_SLEEP
~~~

第 7 步之后，FreeRTOS 只有在没有可运行任务时才会真正进入自动 Light Sleep。app_main 的延时任务仍可能周期性唤醒 CPU，因此当前方案不会达到绝对零唤醒功耗。

## 6. Light Sleep 技术实现

工程使用以下 ESP-IDF 配置：

~~~ini
CONFIG_PM_ENABLE=y
CONFIG_FREERTOS_USE_TICKLESS_IDLE=y
CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP=3
~~~

运行时配置：

~~~c
esp_pm_config_t pm_config = {
    .max_freq_mhz = 240,
    .min_freq_mhz = 40,
    .light_sleep_enable = true,
};

esp_pm_configure(&pm_config);
~~~

电源锁的作用：

- ESP_PM_CPU_FREQ_MAX：业务活动期间保持 CPU 最高频率。
- ESP_PM_NO_LIGHT_SLEEP：外设或关键操作期间禁止自动 Light Sleep。
- 资源引用计数：从应用层阻止错误的电源切换。

ESP-IDF 的自动 Light Sleep 依赖 FreeRTOS Tickless Idle；系统没有可运行任务且没有禁止睡眠锁时，CPU 才进入睡眠。[ESP32-S3 低功耗模式说明](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/low-power-mode/low-power-mode-soc.html)

## 7. GPIO0/BOOT 唤醒设计

GPIO0 具有两个角色：

1. 普通 BOOT 按键输入。
2. ESP32-S3 的启动模式选择脚。

当前 Light Sleep 使用专用 GPIO 唤醒接口：

~~~c
gpio_wakeup_enable(GPIO_NUM_0, GPIO_INTR_LOW_LEVEL);
esp_sleep_enable_gpio_wakeup();
~~~

由于当前配置开启了：

~~~ini
CONFIG_PM_SLP_DISABLE_GPIO=y
~~~

自动睡眠会关闭所有 GPIO。GPIO0 在配置为唤醒源时必须排除出该关闭范围：

~~~c
gpio_sleep_sel_dis(GPIO_NUM_0);
~~~

否则系统虽然可以进入 IDLE，但按下 BOOT 后 GPIO0 不会触发唤醒。[ESP32-S3 Kconfig 说明](https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32s3/api-reference/kconfig.html)

按键任务在低功耗等待期间使用低电平中断，保证按键已经拉低后仍能通知任务；恢复 ACTIVE 后切回下降沿中断，避免按键持续按下造成中断风暴。

正常唤醒流程：

~~~text
BOOT 按下，GPIO0 = 0
    ↓
GPIO 唤醒 Light Sleep
    ↓
key_task 收到通知
    ↓
power_manager_request_active()
    ↓
恢复 LCD、LVGL、摄像头、SD、Wi-Fi、HTTP
    ↓
输出 BOOT wakeup: active state restored
~~~

GPIO0 仍然是启动模式脚。如果在 GPIO0 为低时同时触发 EN/RESET，芯片可能进入 Download Boot，这属于硬件启动模式行为，不是 Light Sleep 唤醒。

## 8. 唤醒和外设恢复

当前恢复顺序为：

~~~text
LCD
  ↓
LVGL
  ↓
Camera
  ↓
SD mount
  ↓
Wi-Fi start
  ↓
HTTP server start
~~~

BOOT 唤醒后的恢复不能在 GPIO ISR 中完成。ISR 只负责发送任务通知，实际恢复在任务上下文中执行。

SD 挂载、Wi-Fi 启动和 HTTP server 创建都属于重栈操作。当前 key_task 栈已经调整为 4096 个栈字，不能恢复为原来的 2048 个栈字，否则可能出现：

~~~text
A stack overflow in task key_task has been detected.
rst:0xc (RTC_SW_CPU_RST)
~~~

长期优化方向是增加独立的 power_resume_task，让 key_task 只发送唤醒请求，不在按键任务中直接执行 SD、Wi-Fi 和 HTTP 初始化。

## 9. Deep Sleep 边界

Deep Sleep 不由空闲超时自动触发，只能通过 power_manager_request_deep_sleep() 显式请求。

进入 Deep Sleep 前需要：

1. 确认所有资源引用已经释放。
2. 调用 prepare_deep_sleep 停止应用外设。
3. 使用 esp_sleep_enable_ext1_wakeup_io() 配置 RTC GPIO 唤醒。
4. 释放 CPU 和禁止睡眠锁。
5. 调用 esp_deep_sleep_start()。

Deep Sleep 唤醒后会重新执行启动流程，属于一次新的 MCU 启动；Light Sleep 唤醒则应该继续执行原来的上下文，不应出现完整 bootloader 日志。

## 10. USB 调试限制

ESP32-S3 原生 USB Serial/JTAG 在 Light Sleep 期间可能失去响应或重新枚举。因此工程当前保留：

~~~ini
CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y
~~~

该配置适合调试时保持 USB 连接稳定，但 USB Serial/JTAG 连接期间可能禁止自动 Light Sleep。真正测试低功耗时，建议使用外部 USB-UART；产品版本可以根据是否需要原生 USB 调试决定是否关闭该配置。

## 11. 日志和验证要求

启动时打印复位原因：

~~~c
ESP_LOGI("MAIN", "reset_reason=%d", esp_reset_reason());
~~~

低功耗功能验证顺序：

1. 将超时设置为 5～15 秒，确认出现 POWER: entered idle state。
2. 按下并释放 BOOT，确认出现 BOOT wakeup: active state restored。
3. 确认 LCD、摄像头、SD、Wi-Fi 和 HTTP 可以恢复。
4. 连续测试进入/退出低功耗至少 20 次。
5. 测试 SD 卡未挂载时拍照不会崩溃，恢复后可以重新挂载。
6. 测试按住 BOOT 时不会触发重复事件或进入下载模式。
7. 分别记录 ACTIVE、IDLE 和唤醒恢复期间的电流。

关键日志应类似：

~~~text
POWER: entered idle state
KEY: BOOT wakeup: active state restored
CAM: camera resumed
SD: SD card mounted at /sdcard
wifi: WiFi resumed
HTTP: HTTP server started
~~~

如果出现完整启动日志，应进一步检查 reset_reason；如果出现 RTC_SW_CPU_RST，优先检查 panic、任务栈和看门狗；如果出现 Brownout，则优先检查电源和唤醒时外设的瞬态电流。

## 12. BLE 与低功耗的后续接口

当前 POWER_STATE_BLE_STANDBY 和 POWER_RESOURCE_BLE_CONNECTION 已为 BLE 预留，但 BLE 协议栈和广播/连接业务尚未接入。

接入 BLE 后建议：

- BLE 广播期间进入 BLE_STANDBY，不要关闭 BLE 控制器。
- BLE 已连接时持有 POWER_RESOURCE_BLE_CONNECTION，禁止进入会影响连接的深度休眠。
- BLE 断开且无其他业务时释放资源，回到普通 IDLE。
- BLE 事件回调只发送电源事件，不直接操作 LCD、SD 或摄像头。
- 若需要维持连接，应使用 BLE modem sleep/协议栈省电机制，而不是直接关闭 BLE 所有时钟。
---

# OTA 升级系统架构与实现技术补充

## 1. 设计目标

本项目采用网页端推送固件的 OTA 方案。浏览器直接把新的 `.bin` 固件上传到 ESP32-S3，设备将固件写入当前运行分区之外的 OTA 分区，完成校验后切换启动分区并重启。

OTA 设计需要同时解决以下问题：

1. 不能覆盖当前正在运行的固件。
2. 固件传输过程中断电或网络中断时，设备仍然可以启动旧固件。
3. 固件必须经过大小、格式和 SHA-256 校验。
4. 新固件首次启动失败时需要自动回滚。
5. OTA 上传期间不能进入低功耗，也不能让摄像头、LCD、SD 卡等外设并发破坏升级过程。
6. 网页端需要提供上传入口、进度显示和升级结果反馈。

## 2. 当前分区和 A/B 结构

当前工程已经使用双 OTA 分区，不需要重新设计分区表：

| 分区 | 地址 | 大小 | 作用 |
|---|---:|---:|---|
| `ota_0` | `0x10000` | `0x400000` | A 固件分区 |
| `ota_1` | `0x410000` | `0x400000` | B 固件分区 |
| `otadata` | `0x810000` | `0x2000` | 保存当前启动分区和 OTA 状态 |
| `vfs` | `0x820000` | `0x6E0000` | 文件系统 |
| `storage` | `0xF00000` | `0x100000` | 设备数据存储 |

A/B 分区的基本规则是：

```text
当前运行 ota_0
    -> 新固件写入 ota_1
    -> 校验通过
    -> otadata 选择 ota_1
    -> 重启

当前运行 ota_1
    -> 新固件写入 ota_0
    -> 校验通过
    -> otadata 选择 ota_0
    -> 重启
```

固件不会直接覆盖当前运行分区。`esp_ota_get_next_update_partition()` 负责选择非当前分区，`esp_ota_set_boot_partition()` 负责更新下一次启动目标。

工程当前已经打开：

```ini
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y
```

因此新固件首次启动时会处于 `ESP_OTA_IMG_PENDING_VERIFY` 状态。应用完成自检后调用 `esp_ota_mark_app_valid_cancel_rollback()`，否则 Bootloader 可以在下一次启动时回滚到旧版本。

## 3. 总体软件架构

```text
浏览器 OTA 页面
        |
        | POST /api/ota/upload
        | 固件二进制 + Manifest HTTP Headers
        v
HTTP Server
        |
        | 参数读取、Token 校验、接收分块
        v
OTA Manager
        |
        | 状态机、互斥锁、低功耗资源锁
        | esp_ota_begin/write/end
        | SHA-256 流式计算
        v
非当前 OTA 分区 ota_0 / ota_1
        |
        | esp_ota_set_boot_partition
        v
重启 -> Bootloader -> 新固件
                         |
                         | 自检成功：确认新固件
                         | 自检失败/未确认：自动回滚
```

模块职责如下：

| 模块 | 位置 | 职责 |
|---|---|---|
| HTTP OTA 接口 | `components/Middlewares/HTTP/http_server.c` | 网页、上传接口、状态接口、响应处理 |
| OTA 管理器 | `components/OTA/OTA_Manager` | OTA 事务状态机、分区写入、哈希校验、启动分区切换 |
| Manifest 校验 | `components/OTA/OTA_Manifest` | 产品名、版本、固件大小、SHA-256 格式校验 |
| OTA 策略 | `components/OTA/OTA_Policy` | 固件产品限制、分区大小限制、Token 校验 |
| 低功耗协调 | `components/Power/power_manager` | OTA 期间禁止进入 Idle/Light Sleep |
| 启动确认 | `main/main.c` | 新固件启动后的自检和回滚确认 |

OTA Manager 与 HTTP Server 分离，后续可以复用同一套 OTA 核心实现来支持 HTTPS 拉取 OTA、BLE 触发 OTA 或串口 OTA。

## 4. HTTP 上传协议

当前网页上传采用原始二进制请求体，不使用 multipart，减少 ESP32 端解析和内存开销。

### 4.1 上传接口

```http
POST /api/ota/upload
Content-Type: application/octet-stream
Content-Length: <firmware_size>
X-OTA-Product: 25_1_camera
X-OTA-Version: 1.0.1
X-OTA-SHA256: <64 hexadecimal characters>
X-OTA-Token: <optional token>

<firmware binary body>
```

固件大小使用 HTTP `Content-Length`，Manifest 元数据使用 HTTP Header 传递。这样不需要在设备端引入完整 JSON 解析器，也不需要额外占用文件系统保存 Manifest。

### 4.2 状态接口

```http
GET /api/ota/status
```

返回 OTA 当前状态、已接收字节数、总字节数、版本和错误码，例如：

```json
{
  "state": "receiving",
  "received": 524288,
  "total": 1399808,
  "version": "1.0.1",
  "error": 0
}
```

### 4.3 网页端处理

网页端完成以下工作：

1. 选择固件 `.bin` 文件。
2. 输入版本号和可选 Token。
3. 在浏览器端计算 SHA-256。
4. 使用 `XMLHttpRequest` 上传原始固件并显示进度。
5. 上传成功后等待设备重启。

网页端计算哈希可以避免设备端先保存完整固件再计算摘要。设备端仍然会重新对接收数据进行 SHA-256 计算，因此不能只信任浏览器发送的结果。

## 5. OTA Manager 状态机

当前 OTA 状态定义为：

```text
IDLE
  -> PREPARING
  -> RECEIVING
  -> VERIFYING
  -> READY_TO_REBOOT

任意阶段发生错误
  -> FAILED
  -> 下一次新的上传可以重新进入 PREPARING
```

### 5.1 PREPARING

进入准备阶段时执行：

1. 检查是否已有其他 OTA 事务。
2. 调用 `power_manager_request_active()`，确保系统不处于低功耗状态。
3. 获取非当前运行分区。
4. 校验产品名、版本号、固件大小和 SHA-256 字符串。
5. 检查固件大小不能超过目标 OTA 分区大小。
6. 获取 `POWER_RESOURCE_OTA`。
7. 调用 `esp_ota_begin()` 擦除并准备目标分区。
8. 初始化 mbedTLS SHA-256 上下文。

### 5.2 RECEIVING

HTTP Server 每次收到一段数据后调用：

```c
ota_manager_write(buffer, received_length);
```

OTA Manager 同时执行：

```text
esp_ota_write()       -> 写入 Flash
mbedtls_sha256_update -> 更新摘要
received_size         -> 更新进度
```

接收缓存放在堆上，不放在 HTTP Server 任务栈上，避免再次出现任务栈溢出问题。

### 5.3 VERIFYING

上传结束后执行：

1. 检查实际接收大小是否等于 `Content-Length`。
2. 完成 SHA-256 计算。
3. 将实际 SHA-256 与 Manifest 中的 SHA-256 比较。
4. 调用 `esp_ota_end()` 校验 ESP-IDF 应用镜像格式。

任何一步失败都调用 `esp_ota_abort()`，保持旧固件为有效启动目标。

### 5.4 READY_TO_REBOOT

所有校验成功后：

1. 调用 `esp_ota_set_boot_partition()`。
2. 释放 OTA 资源锁。
3. 返回网页成功响应。
4. 延迟一小段时间，确保 HTTP 响应发出。
5. 调用 `esp_restart()`。

## 6. 校验和安全技术

### 6.1 SHA-256 的作用

SHA-256 用于检查固件在浏览器到设备传输过程中是否发生损坏或截断。设备端对实际写入的每个数据块计算摘要，不能只依赖浏览器端提供的摘要。

### 6.2 `esp_ota_end()` 的作用

`esp_ota_end()` 会检查写入内容是否是合法的 ESP 应用镜像。它与 SHA-256 的职责不同：

| 校验 | 主要作用 |
|---|---|
| 固件大小 | 防止越界和截断 |
| SHA-256 | 检查传输内容是否与 Manifest 一致 |
| `esp_ota_end()` | 检查 ESP 应用镜像格式和完整性 |
| Secure Boot | 检查固件是否由可信密钥签名 |

当前工程没有开启 Secure Boot，因此 SHA-256 只能解决完整性校验，不能防止攻击者上传自己构造的合法镜像。产品化时还需要配置签名固件、Secure Boot V2 和必要的 Flash Encryption。

### 6.3 Token 策略

`components/OTA/OTA_Policy/OTA_Policy.h` 中的 `OTA_UPLOAD_TOKEN` 默认为空，方便局域网调试。正式设备应修改为设备专属 Token：

```c
#define OTA_UPLOAD_TOKEN "device-specific-token"
```

Token 不是固件签名的替代方案。正式方案应同时使用访问控制和 Secure Boot。

## 7. OTA 与低功耗的协调

OTA 上传属于长时间网络和 Flash 操作，不能进入低功耗。

资源关系如下：

```text
OTA HTTP 请求开始
    -> 持有 POWER_RESOURCE_HTTP
    -> OTA Manager 持有 POWER_RESOURCE_OTA
    -> power_manager_can_sleep() = false
    -> 保持 Wi-Fi、HTTP 和 CPU 活跃
    -> 上传/校验/切换分区
    -> 返回响应
    -> 重启或释放资源
```

为了避免摄像头和 Flash 更新过程并发运行，网页上传开始时会：

1. 关闭浏览器中的 MJPEG stream。
2. 发送 `EVT_DISP_BACK` 请求停止摄像头预览。
3. 等待摄像头释放 `POWER_RESOURCE_CAMERA`。
4. 如果摄像头、LCD DMA 或 SD 仍被占用，返回 `409 Conflict`，不开始 OTA。

OTA 不应复用普通的低功耗 `prepare_idle()` 流程，因为该流程会停止 HTTP Server，而 HTTP Server 正是 OTA 上传通道。OTA 应保持 Wi-Fi/HTTP 活跃，只暂停可能影响 Flash/总线安全的业务外设。

## 8. 启动确认和自动回滚

新固件启动后的确认流程如下：

```text
Bootloader 启动新固件
    -> 新固件标记为 PENDING_VERIFY
    -> main.c 初始化核心模块
    -> ota_manager_confirm_running_image()
    -> esp_ota_mark_app_valid_cancel_rollback()
```

如果新固件在确认前发生崩溃、看门狗复位或再次重启，Bootloader 不会继续信任该版本，而是回滚到旧 OTA 分区。

启动确认应放在必要外设和关键任务创建完成之后，不能一进入 `app_main()` 就立即确认，否则摄像头、SD、Wi-Fi 或 LVGL 初始化失败也可能被错误地认为是成功升级。

## 9. 失败场景处理

| 场景 | 处理方式 |
|---|---|
| 缺少版本或 SHA-256 Header | 返回 `400 Bad Request` |
| Token 错误 | 返回 `401 Unauthorized` |
| 摄像头/LCD/SD 正在使用 | 返回 `409 Conflict` |
| 固件超过 OTA 分区 | 拒绝写入 |
| HTTP 中断 | `esp_ota_abort()`，保留旧固件 |
| SHA-256 不匹配 | 丢弃目标分区内容，不切换启动分区 |
| `esp_ota_end()` 失败 | 不切换启动分区 |
| 写入后重启失败 | Bootloader 按回滚策略恢复旧固件 |
| OTA 成功但未确认 | 下一次启动自动回滚 |

## 10. 当前实现涉及的主要知识点

1. ESP-IDF `app_update` 组件和 `esp_ota_ops` API。
2. OTA A/B 分区和 `otadata` 启动选择机制。
3. FreeRTOS 互斥锁、任务、堆内存和资源引用计数。
4. HTTP Server 的 Header、Content-Length、分块接收和响应时序。
5. mbedTLS SHA-256 流式摘要计算。
6. ESP 应用镜像格式校验和 Bootloader 回滚机制。
7. OTA 与低功耗状态机之间的资源锁协调。
8. Flash 写入期间摄像头、LCD DMA、SD 等外设的并发控制。
9. Secure Boot、Flash Encryption、签名固件和完整性校验之间的区别。
10. 浏览器端文件读取、哈希计算、XHR 上传进度和设备重启后的连接恢复。

## 11. 实现文件清单

| 文件 | 作用 |
|---|---|
| `components/OTA/CMakeLists.txt` | 加入 Manager、Manifest、Policy 和 Power 依赖 |
| `components/OTA/OTA_Manager/OTA_Manager.c/.h` | OTA 核心状态机和分区操作 |
| `components/OTA/OTA_Manifest/OTA_Manifest.c/.h` | Manifest 字段和格式校验 |
| `components/OTA/OTA_Policy/OTA_Policy.c/.h` | 产品、大小和 Token 策略 |
| `components/Middlewares/CMakeLists.txt` | HTTP 组件依赖 OTA |
| `components/Middlewares/HTTP/http_server.c` | 网页 OTA UI、上传接口和状态接口 |
| `main/main.c` | OTA Manager 初始化和新固件启动确认 |

## 12. OTA 验证计划

正式烧录测试前，应依次验证：

1. 上传正确固件，确认设备切换到另一 OTA 分区并成功重启。
2. 上传错误 SHA-256，确认返回失败且旧固件仍可启动。
3. 上传截断文件，确认不会切换启动分区。
4. 上传超过 `0x400000` 的文件，确认被拒绝。
5. 在 OTA 写入过程中断电，确认旧固件可以启动。
6. 让新固件在启动确认前主动复位，确认 Bootloader 回滚。
7. 在摄像头预览、SD 写入和低功耗状态下分别测试上传行为。
8. 连续执行多次 A/B 交替升级，确认 `ota_0` 和 `ota_1` 可以反复切换。
9. 开启 Token 后测试无 Token、错误 Token 和正确 Token。
10. 最后再增加 Secure Boot/签名固件测试，验证未经授权的镜像无法启动。
