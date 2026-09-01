#include <stddef.h>

#include "app_core_state.h"

/**
 * @brief 初始化状态快照。
 *
 * 这里设置的是系统刚启动时的默认状态。
 */
void app_core_state_snapshot_init(
    app_core_state_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    *snapshot = (app_core_state_snapshot_t){
        .revision = 0U,

        .system_state =
            APP_CORE_SYSTEM_STATE_UNKNOWN,

        .capture_state =
            APP_CORE_CAPTURE_STATE_IDLE,

        .web_state =
            APP_CORE_WEB_STATE_STOPPED,

        .storage_state =
            APP_CORE_STORAGE_STATE_IDLE,

        .ota_state =
            APP_CORE_OTA_STATE_IDLE,

        .lvgl_page =
            APP_CORE_LVGL_PAGE_BOOT,

        .lvgl_transitioning = false,
        .preview_active = false,
        .web_capture_active = false,
        .ota_active = false,

        .selected_gallery_index = 0U,
        .gallery_count = 0U,
        .selected_menu_index = 0U,

        .active_request_id =
            APP_CORE_INVALID_REQUEST_ID,

        .active_job_id =
            APP_CORE_INVALID_JOB_ID,

        .last_photo_handle =
            APP_CORE_INVALID_PHOTO_HANDLE,

        .last_error = ESP_OK,
    };
}

/**
 * @brief 检查状态快照是否有效。
 *
 * 该函数只检查状态数据本身，
 * 不检查不同状态之间的业务逻辑关系。
 */
bool app_core_state_snapshot_is_valid(
    const app_core_state_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return false;
    }

    if (snapshot->system_state <
            APP_CORE_SYSTEM_STATE_UNKNOWN ||
        snapshot->system_state >
            APP_CORE_SYSTEM_STATE_FAULT)
    {
        return false;
    }

    if (snapshot->capture_state <
            APP_CORE_CAPTURE_STATE_IDLE ||
        snapshot->capture_state >
            APP_CORE_CAPTURE_STATE_CANCELED)
    {
        return false;
    }

    if (snapshot->web_state <
            APP_CORE_WEB_STATE_STOPPED ||
        snapshot->web_state >
            APP_CORE_WEB_STATE_ERROR)
    {
        return false;
    }

    if (snapshot->storage_state <
            APP_CORE_STORAGE_STATE_IDLE ||
        snapshot->storage_state >
            APP_CORE_STORAGE_STATE_FAILED)
    {
        return false;
    }

    if (snapshot->ota_state <
            APP_CORE_OTA_STATE_IDLE ||
        snapshot->ota_state >
            APP_CORE_OTA_STATE_FAILED)
    {
        return false;
    }

    if (snapshot->lvgl_page <
            APP_CORE_LVGL_PAGE_BOOT ||
        snapshot->lvgl_page >
            APP_CORE_LVGL_PAGE_SETTINGS)
    {
        return false;
    }

    return true;
}