#include <stddef.h>
#include <string.h>

#include "app_core_effect.h"
/**
 * @brief 创建 System Effect 类型。
 */
app_core_effect_type_t app_core_effect_type_make_system(
    app_core_system_effect_type_t type)
{
    app_core_effect_type_t result;

    result = (app_core_effect_type_t){0};

    result.target =
        APP_CORE_EFFECT_TARGET_SYSTEM;

    result.code.system =
        type;

    return result;
}

/**
 * @brief 创建 Camera Effect 类型。
 */
app_core_effect_type_t app_core_effect_type_make_camera(
    app_core_camera_effect_type_t type)
{
    app_core_effect_type_t result;

    result = (app_core_effect_type_t){0};

    result.target =
        APP_CORE_EFFECT_TARGET_CAMERA;

    result.code.camera =
        type;

    return result;
}

/**
 * @brief 创建 Storage Effect 类型。
 */
app_core_effect_type_t app_core_effect_type_make_storage(
    app_core_storage_effect_type_t type)
{
    app_core_effect_type_t result;

    result = (app_core_effect_type_t){0};

    result.target =
        APP_CORE_EFFECT_TARGET_STORAGE;

    result.code.storage =
        type;

    return result;
}

/**
 * @brief 创建 UI Effect 类型。
 */
app_core_effect_type_t app_core_effect_type_make_ui(
    app_core_ui_effect_type_t type)
{
    app_core_effect_type_t result;

    result = (app_core_effect_type_t){0};

    result.target =
        APP_CORE_EFFECT_TARGET_UI;

    result.code.ui =
        type;

    return result;
}

/**
 * @brief 创建 Web Effect 类型。
 */
app_core_effect_type_t app_core_effect_type_make_web(
    app_core_web_effect_type_t type)
{
    app_core_effect_type_t result;

    result = (app_core_effect_type_t){0};

    result.target =
        APP_CORE_EFFECT_TARGET_WEB;

    result.code.web =
        type;

    return result;
}

/**
 * @brief 创建 OTA Effect 类型。
 */
app_core_effect_type_t app_core_effect_type_make_ota(
    app_core_ota_effect_type_t type)
{
    app_core_effect_type_t result;

    result = (app_core_effect_type_t){0};

    result.target =
        APP_CORE_EFFECT_TARGET_OTA;

    result.code.ota =
        type;

    return result;
}

/**
 * @brief 初始化 Effect 对象。
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

    memset(
        effect,
        0,
        sizeof(*effect));

    app_core_message_meta_init(
        &effect->meta,
        request_id,
        parent_request_id,
        source,
        scope);

    effect->type =
        type;
}

/**
 * @brief 检查 Effect 类型是否有效。
 */
bool app_core_effect_type_is_valid(
    app_core_effect_type_t type)
{
    switch (type.target)
    {
    case APP_CORE_EFFECT_TARGET_SYSTEM:
        return app_core_system_effect_type_is_valid(
            type.code.system);

    case APP_CORE_EFFECT_TARGET_CAMERA:
        return app_core_camera_effect_type_is_valid(
            type.code.camera);

    case APP_CORE_EFFECT_TARGET_STORAGE:
        return app_core_storage_effect_type_is_valid(
            type.code.storage);

    case APP_CORE_EFFECT_TARGET_UI:
        return app_core_ui_effect_type_is_valid(
            type.code.ui);

    case APP_CORE_EFFECT_TARGET_WEB:
        return app_core_web_effect_type_is_valid(
            type.code.web);

    case APP_CORE_EFFECT_TARGET_OTA:
        return app_core_ota_effect_type_is_valid(
            type.code.ota);

    case APP_CORE_EFFECT_TARGET_NONE:
    default:
        return false;
    }
}

/**
 * @brief 检查 Effect 对象是否有效。
 */
bool app_core_effect_is_valid(
    const app_core_effect_t *effect)
{
    if (effect == NULL)
    {
        return false;
    }

    if (!app_core_effect_type_is_valid(
            effect->type))
    {
        return false;
    }

    return app_core_message_meta_is_valid(
        &effect->meta);
}

/**
 * @brief 将 Effect 类型转换成字符串。
 */
const char *app_core_effect_type_to_string(
    app_core_effect_type_t type)
{
    switch (type.target)
    {
    case APP_CORE_EFFECT_TARGET_SYSTEM:
        return app_core_system_effect_type_to_string(
            type.code.system);

    case APP_CORE_EFFECT_TARGET_CAMERA:
        return app_core_camera_effect_type_to_string(
            type.code.camera);

    case APP_CORE_EFFECT_TARGET_STORAGE:
        return app_core_storage_effect_type_to_string(
            type.code.storage);

    case APP_CORE_EFFECT_TARGET_UI:
        return app_core_ui_effect_type_to_string(
            type.code.ui);

    case APP_CORE_EFFECT_TARGET_WEB:
        return app_core_web_effect_type_to_string(
            type.code.web);

    case APP_CORE_EFFECT_TARGET_OTA:
        return app_core_ota_effect_type_to_string(
            type.code.ota);

    case APP_CORE_EFFECT_TARGET_NONE:
    default:
        return "NONE";
    }
}