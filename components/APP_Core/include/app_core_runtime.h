#ifndef APP_CORE_RUNTIME_H
#define APP_CORE_RUNTIME_H

#include <stdint.h>

#include "app_core_effect.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 显示页面回调。
     */
    typedef esp_err_t (*app_core_runtime_show_page_fn)(
        void *ctx,
        const app_core_message_meta_t *meta,
        app_core_lvgl_page_t page);

    /**
     * @brief 启动 Camera 预览回调。
     */
    typedef esp_err_t (*app_core_runtime_camera_start_preview_fn)(
        void *ctx,
        const app_core_message_meta_t *meta);

    /**
     * @brief 准备 Camera 拍照回调。
     */
    typedef esp_err_t (*app_core_runtime_camera_prepare_photo_fn)(
        void *ctx,
        const app_core_message_meta_t *meta,
        app_core_job_id_t job_id);

    /**
     * @brief 停止 Camera 回调。
     */
    typedef esp_err_t (*app_core_runtime_camera_stop_fn)(
        void *ctx,
        const app_core_message_meta_t *meta);

    /**
     * @brief 执行 Camera 拍照回调。
     */
    typedef esp_err_t (*app_core_runtime_camera_capture_fn)(
        void *ctx,
        const app_core_message_meta_t *meta,
        app_core_job_id_t job_id);

    /**
     * @brief Camera 自动对焦回调。
     */
    typedef esp_err_t (*app_core_runtime_camera_focus_fn)(
        void *ctx,
        const app_core_message_meta_t *meta);

    /**
     * @brief 保存照片回调。
     */
    typedef esp_err_t (*app_core_runtime_save_photo_fn)(
        void *ctx,
        const app_core_message_meta_t *meta,
        app_core_photo_handle_t photo_handle);

    /**
     * @brief 扫描图库回调。
     */
    typedef esp_err_t (*app_core_runtime_scan_gallery_fn)(
        void *ctx,
        const app_core_message_meta_t *meta);

    /**
     * @brief 显示图库照片回调。
     */
    typedef esp_err_t (*app_core_runtime_show_gallery_photo_fn)(
        void *ctx,
        const app_core_message_meta_t *meta,
        uint16_t index,
        uint16_t total);

    /**
     * @brief 启动 Web 服务回调。
     */
    typedef esp_err_t (*app_core_runtime_web_start_fn)(
        void *ctx,
        const app_core_message_meta_t *meta);

    /**
     * @brief 停止 Web 服务回调。
     */
    typedef esp_err_t (*app_core_runtime_web_stop_fn)(
        void *ctx,
        const app_core_message_meta_t *meta);

    /**
     * @brief 开始 OTA 回调。
     */
    typedef esp_err_t (*app_core_runtime_ota_begin_fn)(
        void *ctx,
        const app_core_message_meta_t *meta,
        const app_core_ota_manifest_t *manifest);

    /**
     * @brief OTA 操作回调。
     *
     * 可以用于完成 OTA 或中止 OTA。
     */
    typedef esp_err_t (*app_core_runtime_ota_operation_fn)(
        void *ctx,
        const app_core_message_meta_t *meta);

    /**
     * @brief 更新菜单选择项回调。
     */
    typedef esp_err_t (*app_core_runtime_update_menu_selection_fn)(
        void *ctx,
        const app_core_message_meta_t *meta,
        uint8_t index);

    /**
     * @brief 系统初始化回调。
     */
    typedef esp_err_t (*app_core_runtime_system_initialize_fn)(
        void *ctx,
        const app_core_message_meta_t *meta);

    /**
     * @brief 系统挂起回调。
     */
    typedef esp_err_t (*app_core_runtime_system_suspend_fn)(
        void *ctx,
        const app_core_message_meta_t *meta);

    /**
     * @brief 系统恢复回调。
     */
    typedef esp_err_t (*app_core_runtime_system_resume_fn)(
        void *ctx,
        const app_core_message_meta_t *meta);

    /**
     * @brief Runtime 回调集合。
     *
     * Runtime 不直接实现 Camera、LVGL、Web 或 OTA，
     * 而是保存底层模块提供的回调函数。
     */
    typedef struct
    {
        /** LVGL 或显示模块使用的上下文。 */
        void *display_ctx;

        /** Camera 模块使用的上下文。 */
        void *camera_ctx;

        /** 照片存储模块使用的上下文。 */
        void *photo_storage_ctx;

        /** 图库存储模块使用的上下文。 */
        void *storage_ctx;

        /** 系统模块使用的上下文。 */
        void *system_ctx;

        /** Web 模块使用的上下文。 */
        void *web_ctx;

        /** OTA 模块使用的上下文。 */
        void *ota_ctx;

        /** 显示页面。 */
        app_core_runtime_show_page_fn show_page;

        /** 启动 Camera 预览。 */
        app_core_runtime_camera_start_preview_fn camera_start_preview;

        /** 准备 Camera 拍照。 */
        app_core_runtime_camera_prepare_photo_fn camera_prepare_photo;

        /** 停止 Camera。 */
        app_core_runtime_camera_stop_fn camera_stop;

        /** 执行 Camera 拍照。 */
        app_core_runtime_camera_capture_fn camera_capture;

        /** Camera 自动对焦。 */
        app_core_runtime_camera_focus_fn camera_focus;

        /** 保存照片。 */
        app_core_runtime_save_photo_fn save_photo;

        /** 扫描图库。 */
        app_core_runtime_scan_gallery_fn scan_gallery;

        /** 显示图库照片。 */
        app_core_runtime_show_gallery_photo_fn show_gallery_photo;

        /** 启动 Web 服务。 */
        app_core_runtime_web_start_fn web_start;

        /** 停止 Web 服务。 */
        app_core_runtime_web_stop_fn web_stop;

        /** 开始 OTA。 */
        app_core_runtime_ota_begin_fn ota_begin;

        /** 完成 OTA。 */
        app_core_runtime_ota_operation_fn ota_finish;

        /** 中止 OTA。 */
        app_core_runtime_ota_operation_fn ota_abort;

        /** 更新菜单选择项。 */
        app_core_runtime_update_menu_selection_fn update_menu_selection;

        /** 初始化系统。 */
        app_core_runtime_system_initialize_fn system_initialize;

        /** 挂起系统。 */
        app_core_runtime_system_suspend_fn system_suspend;

        /** 恢复系统。 */
        app_core_runtime_system_resume_fn system_resume;

    } app_core_runtime_t;

    /**
     * @brief 执行一个 Effect。
     *
     * Runtime 根据 Effect 类型选择对应的底层回调执行。
     *
     * @param runtime Runtime 回调集合。
     * @param effect 要执行的 Effect。
     * @return ESP_OK 表示执行成功；
     *         ESP_ERR_INVALID_ARG 表示参数无效；
     *         ESP_ERR_NOT_SUPPORTED 表示没有对应回调。
     */
    esp_err_t app_core_runtime_execute(
        const app_core_runtime_t *runtime,
        const app_core_effect_t *effect);

#ifdef __cplusplus
}
#endif

#endif