#include <stddef.h>
#include <string.h>

#include "app_core_event.h"

/**
 * @brief 初始化 Event 对象。
 *
 * 初始化时清空整个 Event，
 * 避免 union 中残留之前使用的数据。
 */
void app_core_event_init(
    app_core_event_t *event,
    app_core_event_type_t type,
    app_core_request_id_t request_id,
    app_core_request_id_t parent_request_id,
    app_core_source_t source,
    app_core_scope_t scope)
{
    if (event == NULL)
    {
        return;
    }

    memset(event, 0, sizeof(*event));

    app_core_message_meta_init(
        &event->meta,
        request_id,
        parent_request_id,
        source,
        scope);

    event->type = type;
    event->error = ESP_OK;
}

/**
 * @brief 检查事件类型是否有效。
 */
bool app_core_event_type_is_valid(
    app_core_event_type_t type)
{
    return type >= APP_CORE_EVENT_TYPE_SYSTEM_READY &&
           type <= APP_CORE_EVENT_TYPE_FAILED;
}

/**
 * @brief 检查 Event 对象是否有效。
 *
 * 当前检查：

 * 1. Event 指针不为空；
 * 2. Event 类型有效；
 * 3. Event 元数据有效。
 */
bool app_core_event_is_valid(
    const app_core_event_t *event)
{
    if (event == NULL)
    {
        return false;
    }

    if (!app_core_event_type_is_valid(event->type))
    {
        return false;
    }

    return app_core_message_meta_is_valid(&event->meta);
}

/**
 * @brief 将事件类型转换成字符串。
 *
 * 该函数只负责调试信息转换，
 * 不执行任何事件处理逻辑。
 */
const char *app_core_event_type_to_string(
    app_core_event_type_t type)
{
    switch (type)
    {
    case APP_CORE_EVENT_TYPE_NONE:
        return "NONE";

    case APP_CORE_EVENT_TYPE_SYSTEM_READY:
        return "SYSTEM_READY";

    case APP_CORE_EVENT_TYPE_SYSTEM_SUSPENDED:
        return "SYSTEM_SUSPENDED";

    case APP_CORE_EVENT_TYPE_SYSTEM_RESUMED:
        return "SYSTEM_RESUMED";

    case APP_CORE_EVENT_TYPE_CAMERA_PREVIEW_STARTED:
        return "CAMERA_PREVIEW_STARTED";

    case APP_CORE_EVENT_TYPE_CAMERA_PREPARED:
        return "CAMERA_PREPARED";

    case APP_CORE_EVENT_TYPE_CAMERA_CAPTURED:
        return "CAMERA_CAPTURED";

    case APP_CORE_EVENT_TYPE_CAMERA_STOPPED:
        return "CAMERA_STOPPED";

    case APP_CORE_EVENT_TYPE_CAMERA_FOCUSED:
        return "CAMERA_FOCUSED";

    case APP_CORE_EVENT_TYPE_PHOTO_SAVED:
        return "PHOTO_SAVED";

    case APP_CORE_EVENT_TYPE_GALLERY_SCANNED:
        return "GALLERY_SCANNED";

    case APP_CORE_EVENT_TYPE_PAGE_SHOWN:
        return "PAGE_SHOWN";

    case APP_CORE_EVENT_TYPE_WEB_STARTED:
        return "WEB_STARTED";

    case APP_CORE_EVENT_TYPE_WEB_STOPPED:
        return "WEB_STOPPED";

    case APP_CORE_EVENT_TYPE_OTA_STARTED:
        return "OTA_STARTED";

    case APP_CORE_EVENT_TYPE_OTA_FINISHED:
        return "OTA_FINISHED";

    case APP_CORE_EVENT_TYPE_OTA_ABORTED:
        return "OTA_ABORTED";

    case APP_CORE_EVENT_TYPE_MENU_SELECTION_UPDATED:
        return "MENU_SELECTION_UPDATED";

    case APP_CORE_EVENT_TYPE_GALLERY_SELECTION_UPDATED:
        return "GALLERY_SELECTION_UPDATED";

    case APP_CORE_EVENT_TYPE_FAILED:
        return "FAILED";

    default:
        return "UNKNOWN";
    }
}