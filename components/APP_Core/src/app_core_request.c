#include <stddef.h>
#include <string.h>

#include "app_core_request.h"

/**
 * @brief 初始化 Request 对象。
 *
 * 初始化时先清空整个 Request，
 * 防止 union 中残留上一次使用的数据。
 */
void app_core_request_init(
    app_core_request_t *request,
    app_core_request_type_t type,
    app_core_request_id_t request_id,
    app_core_request_id_t parent_request_id,
    app_core_source_t source,
    app_core_scope_t scope)
{
    if (request == NULL)
    {
        return;
    }

    memset(request, 0, sizeof(*request));

    app_core_message_meta_init(
        &request->meta,
        request_id,
        parent_request_id,
        source,
        scope);

    request->type = type;
}

/**
 * @brief 检查请求类型是否有效。
 */
bool app_core_request_type_is_valid(
    app_core_request_type_t type)
{
    return type >= APP_CORE_REQUEST_TYPE_SYSTEM_INITIALIZE &&
           type <= APP_CORE_REQUEST_TYPE_UI_FOCUS;
}

/**
 * @brief 检查 Request 对象是否有效。
 *
 * 当前只检查两个基础条件：

 * 1. Request 指针不为空；
 * 2. 请求类型有效；
 * 3. 消息元数据有效。
 *
 * 不同请求携带的数据是否合法，
 * 后续可以由对应的业务状态机进一步检查。
 */
bool app_core_request_is_valid(
    const app_core_request_t *request)
{
    if (request == NULL)
    {
        return false;
    }

    if (!app_core_request_type_is_valid(request->type))
    {
        return false;
    }

    return app_core_message_meta_is_valid(&request->meta);
}

/**
 * @brief 将请求类型转换为字符串。
 *
 * 该函数不执行任何业务逻辑，
 * 只用于日志打印和调试。
 */
const char *app_core_request_type_to_string(
    app_core_request_type_t type)
{
    switch (type)
    {
    case APP_CORE_REQUEST_TYPE_NONE:
        return "NONE";

    case APP_CORE_REQUEST_TYPE_SYSTEM_INITIALIZE:
        return "SYSTEM_INITIALIZE";

    case APP_CORE_REQUEST_TYPE_SYSTEM_SUSPEND:
        return "SYSTEM_SUSPEND";

    case APP_CORE_REQUEST_TYPE_SYSTEM_RESUME:
        return "SYSTEM_RESUME";

    case APP_CORE_REQUEST_TYPE_CAPTURE_START_PREVIEW:
        return "CAPTURE_START_PREVIEW";

    case APP_CORE_REQUEST_TYPE_CAPTURE_PREPARE_PHOTO:
        return "CAPTURE_PREPARE_PHOTO";

    case APP_CORE_REQUEST_TYPE_CAPTURE:
        return "CAPTURE";

    case APP_CORE_REQUEST_TYPE_CAPTURE_STOP:
        return "CAPTURE_STOP";

    case APP_CORE_REQUEST_TYPE_CAPTURE_FOCUS:
        return "CAPTURE_FOCUS";

    case APP_CORE_REQUEST_TYPE_SCAN_GALLERY:
        return "SCAN_GALLERY";

    case APP_CORE_REQUEST_TYPE_SHOW_GALLERY_PHOTO:
        return "SHOW_GALLERY_PHOTO";

    case APP_CORE_REQUEST_TYPE_SHOW_PAGE:
        return "SHOW_PAGE";

    case APP_CORE_REQUEST_TYPE_WEB_START:
        return "WEB_START";

    case APP_CORE_REQUEST_TYPE_WEB_STOP:
        return "WEB_STOP";

    case APP_CORE_REQUEST_TYPE_OTA_BEGIN:
        return "OTA_BEGIN";

    case APP_CORE_REQUEST_TYPE_OTA_FINISH:
        return "OTA_FINISH";

    case APP_CORE_REQUEST_TYPE_OTA_ABORT:
        return "OTA_ABORT";

    case APP_CORE_REQUEST_TYPE_UI_SELECT:
        return "UI_SELECT";

    case APP_CORE_REQUEST_TYPE_UI_UP:
        return "UI_UP";

    case APP_CORE_REQUEST_TYPE_UI_DOWN:
        return "UI_DOWN";

    case APP_CORE_REQUEST_TYPE_UI_BACK:
        return "UI_BACK";

    case APP_CORE_REQUEST_TYPE_UI_SHUTTER:
        return "UI_SHUTTER";

    case APP_CORE_REQUEST_TYPE_UI_FOCUS:
        return "UI_FOCUS";

    default:
        return "UNKNOWN";
    }
}