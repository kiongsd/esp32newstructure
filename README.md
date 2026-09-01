/**
 ***************************************************************************************************
 * 实验简介
 * 实验名称：摄像头实验
 * 实验平台：正点原子 ESP32-S3开发板
 * 实验目的：学习如何驱动摄像头模块

 ***************************************************************************************************
 * 硬件资源及引脚分配
 * 1 LED
     LED - IO1
 * 2 正点原子 1.3/2.4 寸SPILCD模块
 * 3, XL9555-->ESP32S3 IO
 *      INT-->IO0
 *      SDA-->IO41
 *      CLK-->IO42
 * 4, CAMERA-->ESP32S3 IO
 *      OV_D0-->IO4
 *      OV_D1-->IO5
 *      OV_D2-->IO6
 *      OV_D3-->IO7
 *      OV_D4-->IO15
 *      OV_D5-->IO16
 *      OV_D6-->IO17
 *      OV_D7-->IO18
 *      OV_VSYNC-->IO47
 *      OV_HREF-->IO48
 *      OV_PCLK-->IO45
 *      OV_SCL-->IO38
 *      OV_SDA-->IO39
 *      OV_PWDN-->IO扩展4(OV_PWDN)
 *      OV_RESET-->IO扩展5(OV_RESET)
 * 
 ***************************************************************************************************
 * 实验现象
 * 1 本实验代码,开机的时候先初始化XL9555IO扩展芯片，然后复位并开启摄像头，接着对摄像头及LCD进行初始化，
 *   最后调用函数显示摄像头数据到LCD显示屏上。
 * 2 LED闪烁，指示程序正在运行

 ***************************************************************************************************
 * 注意事项
 * 无
 
 ***********************************************************************************************************
 * 公司名称：广州市星翼电子科技有限公司（正点原子）
 * 电话号码：020-38271790
 * 传真号码：020-36773971
 * 公司网址：www.alientek.com
 * 购买地址：zhengdianyuanzi.tmall.com
 * 技术论坛：http://www.openedv.com/forum.php
 * 最新资料：www.openedv.com/docs/index.html
 *
 * 在线视频：www.yuanzige.com
 * B 站视频：space.bilibili.com/394620890
 * 公 众 号：mp.weixin.qq.com/s/y--mG3qQT8gop0VRuER9bw
 * 抖    音：douyin.com/user/MS4wLjABAAAAi5E95JUBpqsW5kgMEaagtIITIl15hAJvMO8vQMV1tT6PEsw-V5HbkNLlLMkFf1Bd
 ***********************************************************************************************************
 */


 components/lvgl/lvgl_screens.h
#ifndef __LVGL_SCREENS_H
#define __LVGL_SCREENS_H
#include "lvgl.h"
#include "esp_err.h"
lv_obj_t *create_menu_screen(void);
lv_obj_t *create_preview_screen(void);
lv_obj_t *create_photo_screen(void);
lv_obj_t *create_gallery_screen(void);
lv_obj_t *create_gallery_view_screen(void);
lv_obj_t *create_settings_screen(void);
void update_menu_highlight(uint8_t index);
void update_canvas(uint8_t *buf);
void show_photo_result(uint8_t *jpeg_buf, uint32_t size);
void update_gallery_thumbnail(uint8_t index);
void show_gallery_photo(uint8_t index);
void show_save_success(void);
void show_save_fail(esp_err_t err);
#endif /* __LVGL_SCREENS_H */
components/lvgl/lvgl_screens.c
#include "lvgl_screens.h"
#include "lcd_resource.h"
#include "app_event.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <stdio.h>
#include <string.h>
static const char *TAG = "SCREENS";
static lv_obj_t *menu_icons[4] = {NULL};
static lv_obj_t *menu_highlight = NULL;
static lv_obj_t *preview_canvas = NULL;
static lv_obj_t *preview_fps_label = NULL;
static lv_obj_t *photo_canvas = NULL;
static lv_obj_t *photo_status_label = NULL;
static lv_obj_t *gallery_img = NULL;
static lv_obj_t *gallery_index_label = NULL;
static lv_obj_t *gallery_view_img = NULL;
static lv_obj_t *settings_ssid_label = NULL;
static lv_obj_t *settings_pass_label = NULL;
static lv_obj_t *settings_ip_label = NULL;
static lv_color_t rgb565_to_lv(uint16_t c)
{
    lv_color_t color;
    color.green = (c >> 5) & 0x3F;
    color.red   = (c >> 11) & 0x1F;
    color.blue  = c & 0x1F;
    return color;
}
/* ---- 主菜单 ---- */
lv_obj_t *create_menu_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), 0);
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "ESP32-S3 Camera");
    lv_obj_set_style_text_color(title, lv_color_hex(0xe94560), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    const char *icon_texts[] = {"录像", "拍照", "图库", "设置"};
    const lv_color_t icon_colors[] = {
        lv_color_hex(0x0f3460),
        lv_color_hex(0x533483),
        lv_color_hex(0x2b9348),
        lv_color_hex(0xe94560),
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *item = lv_obj_create(scr);
        lv_obj_set_size(item, 60, 80);
        lv_obj_align(item, LV_ALIGN_TOP_LEFT, 20 + i * 75, 60);
        lv_obj_set_style_bg_color(item, icon_colors[i], 0);
        lv_obj_set_style_border_width(item, 2, 0);
        lv_obj_set_style_border_color(item, lv_color_hex(0x16213e), 0);
        lv_obj_set_style_radius(item, 8, 0);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text(label, icon_texts[i]);
        lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
        lv_obj_center(label);
        menu_icons[i] = item;
    }
    menu_highlight = lv_obj_create(scr);
    lv_obj_set_size(menu_highlight, 64, 84);
    lv_obj_align(menu_highlight, LV_ALIGN_TOP_LEFT, 18, 58);
    lv_obj_set_style_bg_opa(menu_highlight, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_highlight, 3, 0);
    lv_obj_set_style_border_color(menu_highlight, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_radius(menu_highlight, 10, 0);
    lv_obj_clear_flag(menu_highlight, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "KEY3:上  KEY1:下  BOOT:确认");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    return scr;
}
void update_menu_highlight(uint8_t index)
{
    if (index < 4 && menu_highlight) {
        lv_obj_align(menu_highlight, LV_ALIGN_TOP_LEFT, 18 + index * 75, 58);
    }
}
/* ---- 录像预览 ---- */
lv_obj_t *create_preview_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    preview_canvas = lv_canvas_create(scr);
    lv_obj_center(preview_canvas);
    lv_color_t *cbuf = heap_caps_malloc(FRAME_WIDTH * FRAME_HEIGHT * sizeof(lv_color_t),
                        MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (cbuf == NULL) {
        ESP_LOGE(TAG, "Preview canvas buf alloc failed [%s]", app_err_to_name(APP_ERR_LVGL_BUF));
    }
    lv_canvas_set_buffer(preview_canvas, cbuf, FRAME_WIDTH, FRAME_HEIGHT, LV_COLOR_FORMAT_RGB565);
    preview_fps_label = lv_label_create(scr);
    lv_label_set_text(preview_fps_label, "FPS: --");
    lv_obj_set_style_text_color(preview_fps_label, lv_color_hex(0x00ff00), 0);
    lv_obj_align(preview_fps_label, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "KEY2: 返回主菜单");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
    return scr;
}
void update_canvas(uint8_t *buf)
{
    if (preview_canvas == NULL || buf == NULL) return;
    uint16_t *src = (uint16_t *)buf;
    lv_color_t *dst = lv_canvas_get_buf(preview_canvas);
    uint32_t pixel_count = FRAME_WIDTH * FRAME_HEIGHT;
    for (uint32_t i = 0; i < pixel_count; i++) {
        dst[i] = rgb565_to_lv(src[i]);
    }
    lv_obj_invalidate(preview_canvas);
}
/* ---- 拍照模式 ---- */
lv_obj_t *create_photo_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    photo_canvas = lv_canvas_create(scr);
    lv_obj_center(photo_canvas);
    lv_color_t *cbuf = heap_caps_malloc(FRAME_WIDTH * FRAME_HEIGHT * sizeof(lv_color_t),
                        MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    lv_canvas_set_buffer(photo_canvas, cbuf, FRAME_WIDTH, FRAME_HEIGHT, LV_COLOR_FORMAT_RGB565);
    photo_status_label = lv_label_create(scr);
    lv_label_set_text(photo_status_label, "");
    lv_obj_set_style_text_color(photo_status_label, lv_color_hex(0x00ff00), 0);
    lv_obj_align(photo_status_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "KEY0:拍照  KEY2:返回");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
    return scr;
}
void show_photo_result(uint8_t *jpeg_buf, uint32_t size)
{
    if (photo_status_label) {
        lv_label_set_text(photo_status_label, "拍照成功!");
    }
    ESP_LOGI(TAG, "Photo result shown (%u bytes)", size);
}
/* ---- SD 卡图库 ---- */
lv_obj_t *create_gallery_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), 0);
    gallery_img = lv_img_create(scr);
    lv_obj_center(gallery_img);
    gallery_index_label = lv_label_create(scr);
    lv_label_set_text(gallery_index_label, "0 / 0");
    lv_obj_set_style_text_color(gallery_index_label, lv_color_hex(0xffffff), 0);
    lv_obj_align(gallery_index_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "KEY3:上一张  KEY1:下一张\nBOOT:大图  KEY2:返回");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -30);
    return scr;
}
void update_gallery_thumbnail(uint8_t index)
{
    if (gallery_index_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u", index + 1);
        lv_label_set_text(gallery_index_label, buf);
    }
}
/* ---- 大图查看 ---- */
lv_obj_t *create_gallery_view_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    gallery_view_img = lv_img_create(scr);
    lv_obj_center(gallery_view_img);
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "KEY2: 返回图库");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
    return scr;
}
void show_gallery_photo(uint8_t index)
{
    ESP_LOGI(TAG, "Showing full photo %u", index);
}
/* ---- HTTP 配置 ---- */
lv_obj_t *create_settings_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), 0);
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "WiFi / HTTP Config");
    lv_obj_set_style_text_color(title, lv_color_hex(0xe94560), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    settings_ssid_label = lv_label_create(scr);
    lv_label_set_text(settings_ssid_label, "SSID: ESP32_Camera");
    lv_obj_set_style_text_color(settings_ssid_label, lv_color_hex(0xffffff), 0);
    lv_obj_align(settings_ssid_label, LV_ALIGN_TOP_LEFT, 20, 50);
    settings_pass_label = lv_label_create(scr);
    lv_label_set_text(settings_pass_label, "PASS: 12345678");
    lv_obj_set_style_text_color(settings_pass_label, lv_color_hex(0xffffff), 0);
    lv_obj_align(settings_pass_label, LV_ALIGN_TOP_LEFT, 20, 80);
    settings_ip_label = lv_label_create(scr);
    lv_label_set_text(settings_ip_label, "IP: connecting...");
    lv_obj_set_style_text_color(settings_ip_label, lv_color_hex(0xffffff), 0);
    lv_obj_align(settings_ip_label, LV_ALIGN_TOP_LEFT, 20, 110);
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "KEY2: 返回主菜单");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    return scr;
}
void show_save_success(void)
{
    if (photo_status_label) {
        lv_label_set_text(photo_status_label, "保存成功!");
    }
}
void show_save_fail(esp_err_t err)
{
    if (photo_status_label) {
        char buf[64];
        snprintf(buf, sizeof(buf), "保存失败: %s", app_err_to_name(err));
        lv_label_set_text(photo_status_label, buf);
    }
    ESP_LOGE(TAG, "Save failed [%s]", app_err_to_name(err));
}
components/BSP/KEY/key.c
#include "key.h"
#include "camera.h"
#include "event_bus.h"
#include "app_event.h"
volatile uint8_t led_mode = 0;
void key_task(void *arg)
{
    app_event_t evt;
    uint8_t key;
    uint32_t tick_count = 0;
    while (1) {
        key = xl9555_key_scan(0);
        if (key) {
            display_mode_t mode = event_bus_get_display_mode();
            switch (key) {
            case BOOT_PRES:
                if (mode == DISPLAY_MODE_MENU || mode == DISPLAY_MODE_GALLERY) {
                    evt.type = EVT_KEY_BOOT;
                } else if (mode == DISPLAY_MODE_PHOTO) {
                    evt.type = EVT_KEY_SHUTTER;
                } else {
                    evt.type = EVT_KEY_BOOT;
                }
                evt.err = APP_OK;
                event_bus_send(&evt);
                break;
            case KEY3_PRES:
                if (mode == DISPLAY_MODE_MENU || mode == DISPLAY_MODE_GALLERY) {
                    evt.type = EVT_KEY_UP;
                    evt.err = APP_OK;
                    event_bus_send(&evt);
                }
                break;
            case KEY1_PRES:
                if (mode == DISPLAY_MODE_MENU || mode == DISPLAY_MODE_GALLERY) {
                    evt.type = EVT_KEY_DOWN;
                    evt.err = APP_OK;
                    event_bus_send(&evt);
                }
                break;
            case KEY2_PRES:
                evt.type = EVT_KEY_BACK;
                evt.err = APP_OK;
                event_bus_send(&evt);
                break;
            case KEY0_PRES:
                if (mode == DISPLAY_MODE_PHOTO) {
                    evt.type = EVT_KEY_SHUTTER;
                    evt.err = APP_OK;
                    event_bus_send(&evt);
                }
                break;
            }
        }
        if (led_mode == 2) {
            tick_count++;
            if (tick_count >= 100) { LED0_TOGGLE(); tick_count = 0; }
        } else if (led_mode == 3) {
            tick_count++;
            if (tick_count >= 200) { LED0_TOGGLE(); tick_count = 0; }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
components/BSP/CAMERA/camera.c（关键改动部分）
在现有代码基础上，需要修改以下函数。以下是完整替换的函数：
// 在文件顶部 #include 区域新增:
#include "event_bus.h"
#include "lcd_resource.h"
#include "app_event.h"
// 修改 camera_record():
void camera_record(void)
{
    camera_res_acquire();
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == NULL) {
        camera_res_release();
        ESP_LOGE("CAM", "Frame capture failed [%s]", app_err_to_name(APP_ERR_CAM_CAPTURE));
        return;
    }
    esp_jpeg_image_cfg_t jpeg_cfg = {
        .indata = fb->buf,
        .indata_size = fb->len,
        .outbuf = jpeg_decode_buf,
        .outbuf_size = 320 * 240 * 2,
        .out_format = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale = JPEG_IMAGE_SCALE_0,
        .flags = { .swap_color_bytes = 1 },
        .advanced = {
            .working_buffer = jpeg_work_buf,
            .working_buffer_size = 3800,
        },
    };
    esp_jpeg_image_output_t outimg;
    esp_err_t ret = esp_jpeg_decode(&jpeg_cfg, &outimg);
    if (ret != ESP_OK) {
        ESP_LOGE("CAM", "JPEG decode failed [%s]: %s", app_err_to_name(APP_ERR_CAM_DECODE), esp_err_to_name(ret));
        esp_camera_fb_return(fb);
        camera_res_release();
        return;
    }
    esp_camera_fb_return(fb);
    camera_res_release();
    uint8_t write_idx = frame_write_idx;
    memcpy(frame_bufs[write_idx].buf, jpeg_decode_buf, FRAME_BUF_SIZE);
    frame_bufs[write_idx].ready = true;
    frame_write_idx = (write_idx + 1) % 2;
    app_event_t evt = {0};
    evt.type = EVT_CAM_FRAME_READY;
    evt.data.frame.buf = frame_bufs[write_idx].buf;
    evt.data.frame.w = outimg.width;
    evt.data.frame.h = outimg.height;
    event_bus_send(&evt);
}
// 修改 camera_takephoto():
void camera_takephoto(void)
{
    camera_fb_t *fb;
    camera_res_acquire();
    for (int i = 0; i < 3; i++) {
        fb = esp_camera_fb_get();
        if (fb != NULL) esp_camera_fb_return(fb);
    }
    fb = esp_camera_fb_get();
    if (fb == NULL) {
        camera_res_release();
        ESP_LOGE("CAM", "Photo capture failed [%s]", app_err_to_name(APP_ERR_CAM_CAPTURE));
        return;
    }
    size_t jpeg_size = fb->len;
    uint8_t *buf = heap_caps_malloc(jpeg_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (buf == NULL) {
        ESP_LOGE("CAM", "Photo buffer alloc failed [%s]", app_err_to_name(APP_ERR_FRAME_BUF));
        esp_camera_fb_return(fb);
        camera_res_release();
        return;
    }
    memcpy(buf, fb->buf, jpeg_size);
    esp_jpeg_image_cfg_t jpeg_cfg = {
        .indata = fb->buf,
        .indata_size = fb->len,
        .outbuf = jpeg_decode_buf,
        .outbuf_size = 320 * 240 * 2,
        .out_format = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale = JPEG_IMAGE_SCALE_0,
        .flags = {.swap_color_bytes = 1},
        .advanced = {.working_buffer = jpeg_work_buf, .working_buffer_size = 3800},
    };
    esp_jpeg_image_output_t outimg;
    esp_err_t dec_ret = esp_jpeg_decode(&jpeg_cfg, &outimg);
    esp_camera_fb_return(fb);
    camera_res_release();
    if (dec_ret != ESP_OK) {
        ESP_LOGE("CAM", "Photo decode failed [%s]: %s", app_err_to_name(APP_ERR_CAM_DECODE), esp_err_to_name(dec_ret));
        free(buf);
        return;
    }
    while (save_in_progress) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    int wait_timeout = 500;
    while (refresh_done_flag != 1 && wait_timeout-- > 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    app_event_t photo_evt = {0};
    photo_evt.type = EVT_CAM_PHOTO_DONE;
    photo_evt.data.photo.buf = buf;
    photo_evt.data.photo.size = jpeg_size;
    event_bus_send(&photo_evt);
    save_req_t req = { .buf = buf, .size = jpeg_size };
    if (xQueueSend(save_queue, &req, 0) != pdTRUE) {
        ESP_LOGW("CAM", "Save queue full, saving synchronously");
        sd_save_jpeg(buf, jpeg_size);
        free(buf);
    }
}
// 修改 camera_task():
void camera_task(void *arg)
{
    app_event_t evt;
    camera_state_t state = CAM_STATE_IDLE;
    while (1) {
        while (event_bus_receive(&evt, 0)) {
            switch (evt.type) {
            case EVT_DISP_ENTER_PREVIEW:
            case EVT_DISP_ENTER_PHOTO:
                state = CAM_STATE_PREVIEW;
                break;
            case EVT_KEY_SHUTTER:
                if (state == CAM_STATE_PREVIEW)
                    state = CAM_STATE_PHOTO;
                break;
            case EVT_DISP_BACK:
            case EVT_DISP_ENTER_GALLERY:
            case EVT_DISP_ENTER_SETTINGS:
                state = CAM_STATE_IDLE;
                break;
            }
        }
        switch (state) {
        case CAM_STATE_PREVIEW:
            camera_record();
            break;
        case CAM_STATE_PHOTO:
            camera_takephoto();
            state = CAM_STATE_PREVIEW;
            break;
        case CAM_STATE_IDLE:
            vTaskDelay(pdMS_TO_TICKS(10));
            break;
        }
    }
}
components/BSP/CAMERA/camera.h（新增状态枚举）
在现有枚举后新增：
typedef enum {
    CAM_STATE_IDLE = 0,
    CAM_STATE_PREVIEW,
    CAM_STATE_PHOTO,
} camera_state_t;
main/main.c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "my_spi.h"
#include "myiic.h"
#include "xl9555.h"
#include "spilcd.h"
#include "camera.h"
#include "key.h"
#include <stdio.h>
#include "esp_timer.h"
#include "sd_init.h"
#include "my_wifi.h"
#include "http_server.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "event_bus.h"
#include "lvgl_port.h"
#include "lcd_resource.h"
#include "app_event.h"
static const char *TAG = "MAIN";
void fps_task(void *arg)
{
    int64_t last_time = 0;
    int frame_count = 0;
    while (1) {
        if (camera_mode == CAMERA_MODE_PREVIEW) {
            frame_count++;
            int64_t now = esp_timer_get_time();
            if (now - last_time >= 1000000) {
                printf("FPS: %d\n", frame_count);
                frame_count = 0;
                last_time = now;
            }
        } else {
            frame_count = 0;
            last_time = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    led_init();
    my_spi_init();
    myiic_init();
    xl9555_init();
    spilcd_init();
    init_camera();
    sd_init();
    ret = lcd_resource_init();
    if (ret != APP_OK) {
        ESP_LOGE(TAG, "Resource init failed [%s]", app_err_to_name(ret));
    }
    ret = lvgl_port_init();
    if (ret != APP_OK) {
        ESP_LOGE(TAG, "LVGL init failed [%s]", app_err_to_name(ret));
    }
    ESP_ERROR_CHECK(mywifi_init_softap());
    ESP_ERROR_CHECK(http_server_start());
    spilcd_show_string(30, 50, 200, 16, 16, "ESP32-S3", RED);
    spilcd_show_string(30, 70, 200, 16, 16, "CAMERA TEST", RED);
    spilcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    vTaskDelay(pdMS_TO_TICKS(1000));
    event_bus_init();
    xTaskCreate(key_task, "key_task", 2048, NULL, 5, NULL);
    xTaskCreate(save_task, "save_task", 3072, NULL, 1, NULL);
    xTaskCreate(camera_task, "camera_task", 8192, NULL, 7, NULL);
    xTaskCreate(lvgl_port_task, "lvgl_task", 8192, NULL, 3, NULL);
    xTaskCreate(fps_task, "fps_task", 2048, NULL, 2, NULL);
    ESP_LOGI(TAG, "All tasks created, system running");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
sdkconfig.defaults
CONFIG_LV_COLOR_DEPTH_16=y
CONFIG_LV_USE_DEMO_WIDGETS=y
CONFIG_LV_FONT_MONTSERRAT_14=y
CONFIG_LV_FONT_MONTSERRAT_16=y
---
4. 错误码打印汇总
所有模块的错误日志统一格式：ESP_LOGE(TAG, "描述 [%s]", app_err_to_name(err_code))
模块	错误场景	错误码	打印示例
RESOURCE	互斥量创建失败	APP_ERR_MUTEX	Failed to create camera mutex [MUTEX]
RESOURCE	帧缓冲分配失败	APP_ERR_FRAME_BUF	Failed to allocate frame buffer 0 [FRAME_BUF]
LVGL	显示对象创建失败	APP_ERR_LVGL_DISP	Failed to create display [LVGL_DISP]
LVGL	缓冲区分配失败	APP_ERR_LVGL_BUF	Failed to allocate LVGL buffer [LVGL_BUF]
LVGL	LCD 刷新失败	APP_ERR_LCD_FLUSH	LCD flush failed [LCD_FLUSH]: ...
CAM	帧采集失败	APP_ERR_CAM_CAPTURE	Frame capture failed [CAM_CAPTURE]
CAM	JPEG 解码失败	APP_ERR_CAM_DECODE	JPEG decode failed [CAM_DECODE]: ...
CAM	照片缓冲分配失败	APP_ERR_FRAME_BUF	Photo buffer alloc failed [FRAME_BUF]
EVT_BUS	事件队列创建失败	APP_ERR_EVENT_BUS	Failed to create event queue [EVENT_BUS]
SCREENS	canvas 缓冲分配失败	APP_ERR_LVGL_BUF	Preview canvas buf alloc failed [LVGL_BUF]
SCREENS	保存失败	传入的 err	Save failed [SD_SAVE]