#include <stddef.h>

#include "app_core_types.h"
/**
 * @brief 初始化消息元数据。
 *
 * 该函数统一填写消息的请求编号、父请求编号、
 * 消息来源和业务范围，避免各个模块重复初始化。
 */
void app_core_message_meta_init(
    app_core_message_meta_t *meta,
    app_core_request_id_t request_id,
    app_core_request_id_t parent_request_id,
    app_core_source_t source,
    app_core_scope_t scope)
{
    if (meta == NULL)
    {
        return;
    }

    meta->request_id = request_id;
    meta->parent_request_id = parent_request_id;
    meta->source = source;
    meta->scope = scope;
}

/**
 * @brief 检查消息来源是否处于合法范围。
 */
bool app_core_source_is_valid(
    app_core_source_t source)
{
    return source >= APP_CORE_SOURCE_SYSTEM &&
           source <= APP_CORE_SOURCE_TEST;
}

/**
 * @brief 检查消息业务范围是否处于合法范围。
 */
bool app_core_scope_is_valid(
    app_core_scope_t scope)
{
    return scope >= APP_CORE_SCOPE_SYSTEM &&
           scope <= APP_CORE_SCOPE_OTA;
}

/**
 * @brief 检查消息元数据是否有效。
 *
 * 当前主要检查 source 和 scope。
 * request_id 可以为 0，因为某些系统事件不一定对应具体请求。
 */
bool app_core_message_meta_is_valid(
    const app_core_message_meta_t *meta)
{
    if (meta == NULL)
    {
        return false;
    }

    return app_core_source_is_valid(meta->source) &&
           app_core_scope_is_valid(meta->scope);
}

/**
 * @brief 将消息来源转换为字符串。
 */
const char *app_core_source_to_string(
    app_core_source_t source)
{
    switch (source)
    {
    case APP_CORE_SOURCE_SYSTEM:
        return "SYSTEM";

    case APP_CORE_SOURCE_KEY:
        return "KEY";

    case APP_CORE_SOURCE_LVGL:
        return "LVGL";

    case APP_CORE_SOURCE_WEB:
        return "WEB";

    case APP_CORE_SOURCE_ACTION:
        return "ACTION";

    case APP_CORE_SOURCE_SERVICE:
        return "SERVICE";

    case APP_CORE_SOURCE_DRIVER:
        return "DRIVER";

    case APP_CORE_SOURCE_POWER:
        return "POWER";

    case APP_CORE_SOURCE_TEST:
        return "TEST";

    default:
        return "UNKNOWN";
    }
}

/**
 * @brief 将消息业务范围转换为字符串。
 */
const char *app_core_scope_to_string(
    app_core_scope_t scope)
{
    switch (scope)
    {
    case APP_CORE_SCOPE_SYSTEM:
        return "SYSTEM";

    case APP_CORE_SCOPE_CAPTURE:
        return "CAPTURE";

    case APP_CORE_SCOPE_WEB:
        return "WEB";

    case APP_CORE_SCOPE_LVGL:
        return "LVGL";

    case APP_CORE_SCOPE_STORAGE:
        return "STORAGE";

    case APP_CORE_SCOPE_OTA:
        return "OTA";

    default:
        return "UNKNOWN";
    }
}

/**
 * @brief 将系统状态转换为字符串。
 */
const char *app_core_system_state_to_string(
    app_core_system_state_t state)
{
    switch (state)
    {
    case APP_CORE_SYSTEM_STATE_UNKNOWN:
        return "UNKNOWN";

    case APP_CORE_SYSTEM_STATE_BOOTING:
        return "BOOTING";

    case APP_CORE_SYSTEM_STATE_INITIALIZING:
        return "INITIALIZING";

    case APP_CORE_SYSTEM_STATE_READY:
        return "READY";

    case APP_CORE_SYSTEM_STATE_DEGRADED:
        return "DEGRADED";

    case APP_CORE_SYSTEM_STATE_SUSPENDING:
        return "SUSPENDING";

    case APP_CORE_SYSTEM_STATE_SUSPENDED:
        return "SUSPENDED";

    case APP_CORE_SYSTEM_STATE_RESUMING:
        return "RESUMING";

    case APP_CORE_SYSTEM_STATE_FAULT:
        return "FAULT";

    default:
        return "UNKNOWN";
    }
}

/**
 * @brief 将拍照状态转换为字符串。
 */
const char *app_core_capture_state_to_string(
    app_core_capture_state_t state)
{
    switch (state)
    {
    case APP_CORE_CAPTURE_STATE_IDLE:
        return "IDLE";

    case APP_CORE_CAPTURE_STATE_REQUESTED:
        return "REQUESTED";

    case APP_CORE_CAPTURE_STATE_PREPARING:
        return "PREPARING";

    case APP_CORE_CAPTURE_STATE_ACQUIRING:
        return "ACQUIRING";

    case APP_CORE_CAPTURE_STATE_PROCESSING:
        return "PROCESSING";

    case APP_CORE_CAPTURE_STATE_SAVING:
        return "SAVING";

    case APP_CORE_CAPTURE_STATE_SUCCEEDED:
        return "SUCCEEDED";

    case APP_CORE_CAPTURE_STATE_FAILED:
        return "FAILED";

    case APP_CORE_CAPTURE_STATE_CANCELED:
        return "CANCELED";

    default:
        return "UNKNOWN";
    }
}

/**
 * @brief 将 Web 状态转换为字符串。
 */
const char *app_core_web_state_to_string(
    app_core_web_state_t state)
{
    switch (state)
    {
    case APP_CORE_WEB_STATE_STOPPED:
        return "STOPPED";

    case APP_CORE_WEB_STATE_STARTING:
        return "STARTING";

    case APP_CORE_WEB_STATE_RUNNING:
        return "RUNNING";

    case APP_CORE_WEB_STATE_STOPPING:
        return "STOPPING";

    case APP_CORE_WEB_STATE_ERROR:
        return "ERROR";

    default:
        return "UNKNOWN";
    }
}

/**
 * @brief 将存储状态转换为字符串。
 */
const char *app_core_storage_state_to_string(
    app_core_storage_state_t state)
{
    switch (state)
    {
    case APP_CORE_STORAGE_STATE_IDLE:
        return "IDLE";

    case APP_CORE_STORAGE_STATE_SCANNING:
        return "SCANNING";

    case APP_CORE_STORAGE_STATE_SHOWING:
        return "SHOWING";

    case APP_CORE_STORAGE_STATE_FAILED:
        return "FAILED";

    default:
        return "UNKNOWN";
    }
}

/**
 * @brief 将 OTA 状态转换为字符串。
 */
const char *app_core_ota_state_to_string(
    app_core_ota_state_t state)
{
    switch (state)
    {
    case APP_CORE_OTA_STATE_IDLE:
        return "IDLE";

    case APP_CORE_OTA_STATE_PREPARING:
        return "PREPARING";

    case APP_CORE_OTA_STATE_RECEIVING:
        return "RECEIVING";

    case APP_CORE_OTA_STATE_VERIFYING:
        return "VERIFYING";

    case APP_CORE_OTA_STATE_ABORTING:
        return "ABORTING";

    case APP_CORE_OTA_STATE_READY_TO_REBOOT:
        return "READY_TO_REBOOT";

    case APP_CORE_OTA_STATE_FAILED:
        return "FAILED";

    default:
        return "UNKNOWN";
    }
}

/**
 * @brief 将 LVGL 页面转换为字符串。
 */
const char *app_core_lvgl_page_to_string(
    app_core_lvgl_page_t page)
{
    switch (page)
    {
    case APP_CORE_LVGL_PAGE_BOOT:
        return "BOOT";

    case APP_CORE_LVGL_PAGE_MENU:
        return "MENU";

    case APP_CORE_LVGL_PAGE_PREVIEW:
        return "PREVIEW";

    case APP_CORE_LVGL_PAGE_PHOTO:
        return "PHOTO";

    case APP_CORE_LVGL_PAGE_GALLERY:
        return "GALLERY";

    case APP_CORE_LVGL_PAGE_GALLERY_VIEW:
        return "GALLERY_VIEW";

    case APP_CORE_LVGL_PAGE_SETTINGS:
        return "SETTINGS";

    default:
        return "UNKNOWN";
    }
}