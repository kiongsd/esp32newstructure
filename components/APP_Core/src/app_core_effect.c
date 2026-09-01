#include <stddef.h>
#include <string.h>

#include "app_core_effect.h"

/**
 * @brief 初始化 Effect 对象。
 *
 * 初始化时先清空整个对象，
 * 防止 union 中保留无效数据。
 */
void app_core_effect_init(
    app_core_effect_t *effect,
    app_core_effect_type_t type,
    app_core_request_id_t request_id,
    app_core_request_id_t parent_request_id,
    app_core_source_t source,
    app_core_scope_t scope)
{
    if (effect == NULL)
    {
        return;
    }

    memset(effect, 0, sizeof(*effect));

    app_core_message_meta_init(
        &effect->meta,
        request_id,
        parent_request_id,
        source,
        scope);

    effect->type = type;
}

/**
 * @brief 检查 Effect 类型是否有效。
 */
bool app_core_effect_type_is_valid(
    app_core_effect_type_t type)
{
    return type >= APP_CORE_EFFECT_TYPE_SHOW_PAGE &&
           type <= APP_CORE_EFFECT_TYPE_UPDATE_MENU_SELECTION;
}

/**
 * @brief 检查 Effect 对象是否有效。
 *
 * 当前检查：

 * 1. Effect 指针不为空；
 * 2. Effect 类型有效；
 * 3. Effect 元数据有效。
 */
bool app_core_effect_is_valid(
    const app_core_effect_t *effect)
{
    if (effect == NULL)
    {
        return false;
    }

    if (!app_core_effect_type_is_valid(effect->type))
    {
        return false;
    }

    return app_core_message_meta_is_valid(&effect->meta);
}

/**
 * @brief 将 Effect 类型转换成字符串。
 *
 * 该函数只负责日志和调试信息转换，
 * 不负责执行具体动作。
 */
const char *app_core_effect_type_to_string(
    app_core_effect_type_t type)
{
    switch (type)
    {
    case APP_CORE_EFFECT_TYPE_NONE:
        return "NONE";

    case APP_CORE_EFFECT_TYPE_SHOW_PAGE:
        return "SHOW_PAGE";

    case APP_CORE_EFFECT_TYPE_CAMERA_START_PREVIEW:
        return "CAMERA_START_PREVIEW";

    case APP_CORE_EFFECT_TYPE_CAMERA_PREPARE_PHOTO:
        return "CAMERA_PREPARE_PHOTO";

    case APP_CORE_EFFECT_TYPE_CAMERA_STOP:
        return "CAMERA_STOP";

    case APP_CORE_EFFECT_TYPE_CAMERA_CAPTURE:
        return "CAMERA_CAPTURE";

    case APP_CORE_EFFECT_TYPE_CAMERA_FOCUS:
        return "CAMERA_FOCUS";

    case APP_CORE_EFFECT_TYPE_SAVE_PHOTO:
        return "SAVE_PHOTO";

    case APP_CORE_EFFECT_TYPE_SCAN_GALLERY:
        return "SCAN_GALLERY";

    case APP_CORE_EFFECT_TYPE_SHOW_GALLERY_PHOTO:
        return "SHOW_GALLERY_PHOTO";

    case APP_CORE_EFFECT_TYPE_WEB_START:
        return "WEB_START";

    case APP_CORE_EFFECT_TYPE_WEB_STOP:
        return "WEB_STOP";

    case APP_CORE_EFFECT_TYPE_OTA_BEGIN:
        return "OTA_BEGIN";

    case APP_CORE_EFFECT_TYPE_OTA_FINISH:
        return "OTA_FINISH";

    case APP_CORE_EFFECT_TYPE_OTA_ABORT:
        return "OTA_ABORT";

    case APP_CORE_EFFECT_TYPE_SYSTEM_INITIALIZE:
        return "SYSTEM_INITIALIZE";

    case APP_CORE_EFFECT_TYPE_SYSTEM_SUSPEND:
        return "SYSTEM_SUSPEND";

    case APP_CORE_EFFECT_TYPE_SYSTEM_RESUME:
        return "SYSTEM_RESUME";

    case APP_CORE_EFFECT_TYPE_UPDATE_MENU_SELECTION:
        return "UPDATE_MENU_SELECTION";

    default:
        return "UNKNOWN";
    }
}