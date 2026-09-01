#ifndef APP_CORE_EFFECT_H
#define APP_CORE_EFFECT_H

#include "esp_err.h"

#include "app_core_types.h"

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
        /** 无效 Effect。 */
        APP_CORE_EFFECT_TYPE_NONE = 0,

        /** 显示指定页面。 */
        APP_CORE_EFFECT_TYPE_SHOW_PAGE,

        /** 启动 Camera 预览。 */
        APP_CORE_EFFECT_TYPE_CAMERA_START_PREVIEW,

        /** 准备 Camera 拍照。 */
        APP_CORE_EFFECT_TYPE_CAMERA_PREPARE_PHOTO,

        /** 停止 Camera。 */
        APP_CORE_EFFECT_TYPE_CAMERA_STOP,

        /** 执行 Camera 拍照。 */
        APP_CORE_EFFECT_TYPE_CAMERA_CAPTURE,

        /** 执行 Camera 对焦。 */
        APP_CORE_EFFECT_TYPE_CAMERA_FOCUS,

        /** 保存照片。 */
        APP_CORE_EFFECT_TYPE_SAVE_PHOTO,

        /** 扫描图库。 */
        APP_CORE_EFFECT_TYPE_SCAN_GALLERY,

        /** 显示图库中的指定照片。 */
        APP_CORE_EFFECT_TYPE_SHOW_GALLERY_PHOTO,

        /** 启动 Web 服务。 */
        APP_CORE_EFFECT_TYPE_WEB_START,

        /** 停止 Web 服务。 */
        APP_CORE_EFFECT_TYPE_WEB_STOP,

        /** 开始 OTA。 */
        APP_CORE_EFFECT_TYPE_OTA_BEGIN,

        /** 完成 OTA。 */
        APP_CORE_EFFECT_TYPE_OTA_FINISH,

        /** 中止 OTA。 */
        APP_CORE_EFFECT_TYPE_OTA_ABORT,

        /** 初始化系统。 */
        APP_CORE_EFFECT_TYPE_SYSTEM_INITIALIZE,

        /** 让系统进入挂起状态。 */
        APP_CORE_EFFECT_TYPE_SYSTEM_SUSPEND,

        /** 让系统恢复运行。 */
        APP_CORE_EFFECT_TYPE_SYSTEM_RESUME,

        /** 更新菜单选择项。 */
        APP_CORE_EFFECT_TYPE_UPDATE_MENU_SELECTION,

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

#ifdef __cplusplus
}
#endif

#endif