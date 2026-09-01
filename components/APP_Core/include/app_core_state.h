#ifndef APP_CORE_STATE_H
#define APP_CORE_STATE_H

#include "esp_err.h"

#include "app_core_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief App Core 状态快照。
     *
     * 该结构体保存系统当前的完整业务状态。
     * UI、HTTP 和其他模块后续只能通过状态快照了解系统状态，
     * 不应该直接访问 Camera 或 SD 的内部变量。
     */
    typedef struct
    {
        /** 状态版本号，每次状态发生有效变化后递增。 */
        uint32_t revision;

        /** 系统当前生命周期状态。 */
        app_core_system_state_t system_state;

        /** Camera 拍照流程状态。 */
        app_core_capture_state_t capture_state;

        /** Web 服务状态。 */
        app_core_web_state_t web_state;

        /** SD 卡和照片存储状态。 */
        app_core_storage_state_t storage_state;

        /** OTA 升级状态。 */
        app_core_ota_state_t ota_state;

        /** 当前 LVGL 页面。 */
        app_core_lvgl_page_t lvgl_page;

        /** 是否正在等待页面切换完成。 */
        bool lvgl_transitioning;

        /** Camera 是否正在预览。 */
        bool preview_active;

        /** 当前拍照是否由 Web 请求触发。 */
        bool web_capture_active;

        /** OTA 是否正在占用系统关键资源。 */
        bool ota_active;

        /** 当前选中的图库照片索引。 */
        uint16_t selected_gallery_index;

        /** 当前图库中的照片总数。 */
        uint16_t gallery_count;

        /** 当前选中的菜单项索引。 */
        uint8_t selected_menu_index;

        /** 当前主要异步操作对应的请求编号。 */
        app_core_request_id_t active_request_id;

        /** 当前主要拍照任务编号。 */
        app_core_job_id_t active_job_id;

        /** 最近一次拍照产生的照片句柄。 */
        app_core_photo_handle_t last_photo_handle;

        /** 最近一次业务错误，正常状态下为 ESP_OK。 */
        esp_err_t last_error;

    } app_core_state_snapshot_t;

    /**
     * @brief 初始化状态快照。
     *
     * 将状态设置为系统初始状态。
     *
     * @param snapshot 要初始化的状态快照。
     */
    void app_core_state_snapshot_init(
        app_core_state_snapshot_t *snapshot);

    /**
     * @brief 检查状态快照是否有效。
     *
     * 主要检查各个枚举值是否处于合法范围。
     *
     * @param snapshot 要检查的状态快照。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_state_snapshot_is_valid(
        const app_core_state_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif