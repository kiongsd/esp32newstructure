#ifndef APP_CORE_EFFECT_H
#define APP_CORE_EFFECT_H

#include "esp_err.h"

#include "app_core_types.h"

#include "effect/app_core_camera_effect.h"
#include "effect/app_core_storage_effect.h"
#include "effect/app_core_ui_effect.h"
#include "effect/app_core_system_effect.h"
#include "effect/app_core_web_effect.h"
#include "effect/app_core_ota_effect.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief App Core Effect 类型。
     *
     * Effect 表示 App Core 要求底层模块执行的具体动作。
     * Request 是“想做什么”，Effect 是状态机决定“应该执行什么动作”。
     */
    typedef enum
    {
        APP_CORE_EFFECT_TARGET_NONE = 0,

        /** 系统服务执行目标。 */
        APP_CORE_EFFECT_TARGET_SYSTEM,

        /** Camera 执行目标。 */
        APP_CORE_EFFECT_TARGET_CAMERA,

        /** 存储执行目标。 */
        APP_CORE_EFFECT_TARGET_STORAGE,

        /** UI 执行目标。 */
        APP_CORE_EFFECT_TARGET_UI,

        /** Web 执行目标。 */
        APP_CORE_EFFECT_TARGET_WEB,

        /** OTA 执行目标。 */
        APP_CORE_EFFECT_TARGET_OTA,

    } app_core_effect_target_t;

    typedef union
    {
        app_core_system_effect_type_t system;
        app_core_camera_effect_type_t camera;
        app_core_storage_effect_type_t storage;
        app_core_ui_effect_type_t ui;
        app_core_web_effect_type_t web;
        app_core_ota_effect_type_t ota;

    } app_core_effect_code_t;

    typedef struct
    {
        /** Effect 实际由哪个能力模块执行。 */
        app_core_effect_target_t target;

        /** 对应目标模块内部的 Effect 类型。 */
        app_core_effect_code_t code;

    } app_core_effect_type_t;

    /**
     * @brief Effect 携带的数据。
     *
     * 不同 Effect 根据自身类型使用不同的 data 成员。
     */
    typedef union
    {
        /** 页面显示动作使用的页面。 */
        app_core_lvgl_page_t page;

        /** Camera 或其他异步操作使用的任务编号。 */
        app_core_job_id_t job_id;

        /** 保存照片动作使用的照片句柄。 */
        app_core_photo_handle_t photo_handle;

        /** 动作执行失败时使用的错误码。 */
        esp_err_t error;

        /** 更新菜单选择项时使用的菜单索引。 */
        uint8_t menu_index;

        /** OTA 动作使用的固件描述信息。 */
        app_core_ota_manifest_t ota_manifest;

        /** 图库显示动作使用的数据。 */
        struct
        {
            /** 当前照片的零基索引。 */
            uint16_t index;

            /** 图库中的照片总数。 */
            uint16_t total;

        } gallery;

    } app_core_effect_data_t;

    /**
     * @brief App Core Effect 对象。
     *
     * Effect 是状态机输出给 Runtime 或底层适配层的动作消息。
     */
    typedef struct
    {
        /** Effect 的来源、业务范围和请求追踪信息。 */
        app_core_message_meta_t meta;

        /** Effect 的具体动作类型。 */
        app_core_effect_type_t type;

        /** Effect 携带的数据。 */
        app_core_effect_data_t data;

    } app_core_effect_t;

    /**
     * @brief 初始化 Effect 对象。
     *
     * 函数会清空 data，并初始化消息元数据和 Effect 类型。
     *
     * @param effect 要初始化的 Effect 对象。
     * @param type Effect 类型。
     * @param request_id 当前请求编号。
     * @param parent_request_id 父请求编号。
     * @param source Effect 来源。
     * @param scope Effect 所属业务范围。
     */
    void app_core_effect_init(
        app_core_effect_t *effect,
        app_core_effect_type_t type,
        app_core_request_id_t request_id,
        app_core_request_id_t parent_request_id,
        app_core_source_t source,
        app_core_scope_t scope);

    /**
     * @brief 检查 Effect 类型是否有效。
     *
     * @param type 要检查的 Effect 类型。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_effect_type_is_valid(
        app_core_effect_type_t type);

    /**
     * @brief 检查 Effect 对象是否有效。
     *
     * @param effect 要检查的 Effect 对象。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_effect_is_valid(
        const app_core_effect_t *effect);

    /**
     * @brief 将 Effect 类型转换成字符串。
     *
     * 主要用于日志输出和调试。
     *
     * @param type Effect 类型。
     * @return Effect 类型对应的字符串。
     */
    const char *app_core_effect_type_to_string(
        app_core_effect_type_t type);

    app_core_effect_type_t app_core_effect_type_make_system(
        app_core_system_effect_type_t type);

    app_core_effect_type_t app_core_effect_type_make_camera(
        app_core_camera_effect_type_t type);

    app_core_effect_type_t app_core_effect_type_make_storage(
        app_core_storage_effect_type_t type);

    app_core_effect_type_t app_core_effect_type_make_ui(
        app_core_ui_effect_type_t type);

    app_core_effect_type_t app_core_effect_type_make_web(
        app_core_web_effect_type_t type);

    app_core_effect_type_t app_core_effect_type_make_ota(
        app_core_ota_effect_type_t type);
#ifdef __cplusplus
}
#endif

#endif