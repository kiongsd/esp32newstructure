#ifndef APP_CORE_REQUEST_H
#define APP_CORE_REQUEST_H

#include "app_core_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief App Core 请求类型。
     *
     * Request 表示外部模块希望系统执行的操作。
     * 按键、LVGL、HTTP 和系统启动流程都通过 Request
     * 向 App Core 提交业务请求。
     */
    typedef enum
    {
        /** 无效请求，不应该被提交。 */
        APP_CORE_REQUEST_TYPE_NONE = 0,

        /** 请求系统初始化。 */
        APP_CORE_REQUEST_TYPE_SYSTEM_INITIALIZE,

        /** 请求系统进入挂起状态。 */
        APP_CORE_REQUEST_TYPE_SYSTEM_SUSPEND,

        /** 请求系统恢复运行。 */
        APP_CORE_REQUEST_TYPE_SYSTEM_RESUME,

        /** 请求启动 Camera 预览。 */
        APP_CORE_REQUEST_TYPE_CAPTURE_START_PREVIEW,

        /** 请求 Camera 准备拍照。 */
        APP_CORE_REQUEST_TYPE_CAPTURE_PREPARE_PHOTO,

        /** 请求执行拍照。 */
        APP_CORE_REQUEST_TYPE_CAPTURE,

        /** 请求停止 Camera。 */
        APP_CORE_REQUEST_TYPE_CAPTURE_STOP,

        /** 请求 Camera 对焦。 */
        APP_CORE_REQUEST_TYPE_CAPTURE_FOCUS,

        /** 请求扫描图库。 */
        APP_CORE_REQUEST_TYPE_SCAN_GALLERY,

        /** 请求显示图库中的某一张照片。 */
        APP_CORE_REQUEST_TYPE_SHOW_GALLERY_PHOTO,

        /** 请求切换 LVGL 页面。 */
        APP_CORE_REQUEST_TYPE_SHOW_PAGE,

        /** 请求启动 Web 服务。 */
        APP_CORE_REQUEST_TYPE_WEB_START,

        /** 请求停止 Web 服务。 */
        APP_CORE_REQUEST_TYPE_WEB_STOP,

        /** 请求开始 OTA 升级。 */
        APP_CORE_REQUEST_TYPE_OTA_BEGIN,

        /** 请求完成 OTA 升级。 */
        APP_CORE_REQUEST_TYPE_OTA_FINISH,

        /** 请求中止 OTA 升级。 */
        APP_CORE_REQUEST_TYPE_OTA_ABORT,

        /** UI 请求选择当前菜单项。 */
        APP_CORE_REQUEST_TYPE_UI_SELECT,

        /** UI 请求向上移动。 */
        APP_CORE_REQUEST_TYPE_UI_UP,

        /** UI 请求向下移动。 */
        APP_CORE_REQUEST_TYPE_UI_DOWN,

        /** UI 请求返回上一级页面。 */
        APP_CORE_REQUEST_TYPE_UI_BACK,

        /** UI 请求执行拍照操作。 */
        APP_CORE_REQUEST_TYPE_UI_SHUTTER,

        /** UI 请求执行对焦操作。 */
        APP_CORE_REQUEST_TYPE_UI_FOCUS,

    } app_core_request_type_t;

    /**
     * @brief Request 携带的数据。
     *
     * 不同类型的 Request 使用不同的 union 成员。
     * 例如 SHOW_PAGE 使用 page，CAPTURE 使用 job_id。
     */
    typedef union
    {
        /** 页面切换请求使用的目标页面。 */
        app_core_lvgl_page_t page;

        /** 拍照或其他异步任务使用的任务编号。 */
        app_core_job_id_t job_id;

        /** OTA 请求使用的固件描述信息。 */
        app_core_ota_manifest_t ota_manifest;

        /** 图库操作使用的照片索引。 */
        struct
        {
            /** 图库照片的零基索引。 */
            uint16_t index;

        } gallery;

    } app_core_request_data_t;

    /**
     * @brief App Core 请求对象。
     *
     * Request 是业务入口消息。
     * 外部模块不应该直接调用 Camera、SD 或 LVGL 的底层函数，
     * 而应该构造 Request 后提交给 App Core。
     */
    typedef struct
    {
        /** 请求的来源、范围和追踪信息。 */
        app_core_message_meta_t meta;

        /** 请求的具体操作类型。 */
        app_core_request_type_t type;

        /** 请求附带的数据。 */
        app_core_request_data_t data;

    } app_core_request_t;

    /**
     * @brief 初始化 Request 对象。
     *
     * 该函数会清空 data，并统一初始化 meta 和 type。
     *
     * @param request 要初始化的 Request 对象。
     * @param type 请求类型。
     * @param request_id 当前请求编号。
     * @param parent_request_id 父请求编号。
     * @param source 请求来源。
     * @param scope 请求所属业务范围。
     */
    void app_core_request_init(
        app_core_request_t *request,
        app_core_request_type_t type,
        app_core_request_id_t request_id,
        app_core_request_id_t parent_request_id,
        app_core_source_t source,
        app_core_scope_t scope);

    /**
     * @brief 检查请求类型是否有效。
     *
     * NONE 不属于有效业务请求。
     *
     * @param type 要检查的请求类型。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_request_type_is_valid(
        app_core_request_type_t type);

    /**
     * @brief 检查 Request 对象是否有效。
     *
     * @param request 要检查的 Request 对象。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_request_is_valid(
        const app_core_request_t *request);

    /**
     * @brief 将请求类型转换为字符串。
     *
     * 主要用于日志和调试输出。
     *
     * @param type 请求类型。
     * @return 请求类型对应的字符串。
     */
    const char *app_core_request_type_to_string(
        app_core_request_type_t type);

#ifdef __cplusplus
}
#endif

#endif