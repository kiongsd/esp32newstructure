#ifndef APP_CORE_EVENT_H
#define APP_CORE_EVENT_H

#include "esp_err.h"

#include "app_core_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief App Core 事件类型。
     *
     * Event 表示系统已经发生的结果或状态变化。
     * Event 通常由 Driver、Service 或 App Core 内部产生，
     * 然后交给对应的业务域处理。
     */
    typedef enum
    {
        /** 无效事件。 */
        APP_CORE_EVENT_TYPE_NONE = 0,

        /** 系统初始化完成。 */
        APP_CORE_EVENT_TYPE_SYSTEM_READY,

        /** 系统已经进入挂起状态。 */
        APP_CORE_EVENT_TYPE_SYSTEM_SUSPENDED,

        /** 系统已经恢复运行。 */
        APP_CORE_EVENT_TYPE_SYSTEM_RESUMED,

        /** Camera 预览已经启动。 */
        APP_CORE_EVENT_TYPE_CAMERA_PREVIEW_STARTED,

        /** Camera 已经准备好拍照。 */
        APP_CORE_EVENT_TYPE_CAMERA_PREPARED,

        /** Camera 已经完成拍照。 */
        APP_CORE_EVENT_TYPE_CAMERA_CAPTURED,

        /** Camera 已经停止。 */
        APP_CORE_EVENT_TYPE_CAMERA_STOPPED,

        /** Camera 已经完成对焦。 */
        APP_CORE_EVENT_TYPE_CAMERA_FOCUSED,

        /** 照片已经保存完成。 */
        APP_CORE_EVENT_TYPE_PHOTO_SAVED,

        /** 图库扫描完成。 */
        APP_CORE_EVENT_TYPE_GALLERY_SCANNED,

        /** 页面已经显示完成。 */
        APP_CORE_EVENT_TYPE_PAGE_SHOWN,

        /** Web 服务已经启动。 */
        APP_CORE_EVENT_TYPE_WEB_STARTED,

        /** Web 服务已经停止。 */
        APP_CORE_EVENT_TYPE_WEB_STOPPED,

        /** OTA 已经开始。 */
        APP_CORE_EVENT_TYPE_OTA_STARTED,

        /** OTA 已经完成。 */
        APP_CORE_EVENT_TYPE_OTA_FINISHED,

        /** OTA 已经中止。 */
        APP_CORE_EVENT_TYPE_OTA_ABORTED,

        /** 菜单选择项已经更新。 */
        APP_CORE_EVENT_TYPE_MENU_SELECTION_UPDATED,

        /** 图库选择项已经更新。 */
        APP_CORE_EVENT_TYPE_GALLERY_SELECTION_UPDATED,

        /** 某个业务流程执行失败。 */
        APP_CORE_EVENT_TYPE_FAILED,

    } app_core_event_type_t;

    /**
     * @brief Event 携带的数据。
     *
     * 不同事件根据事件类型使用不同的数据成员。
     */
    typedef union
    {
        /** 拍照、保存照片等异步任务的编号。 */
        app_core_job_id_t job_id;

        /** 已经保存完成的照片句柄。 */
        app_core_photo_handle_t photo_handle;

        /** 页面显示完成事件对应的页面。 */
        app_core_lvgl_page_t page;

        /** 菜单选择项的零基索引。 */
        uint8_t menu_index;

        /** 图库选择项和图库总数。 */
        struct
        {
            /** 当前照片的零基索引。 */
            uint16_t index;

            /** 当前图库中的照片总数。 */
            uint16_t total;

        } gallery;

    } app_core_event_data_t;

    /**
     * @brief App Core 事件对象。
     *
     * Event 用于通知系统中的其他模块：
     * 某个动作已经完成，或者某个状态已经发生变化。
     */
    typedef struct
    {
        /** 事件的来源、业务范围和追踪信息。 */
        app_core_message_meta_t meta;

        /** 事件的具体类型。 */
        app_core_event_type_t type;

        /** 事件对应的错误码。
         *
         * 对于成功事件，该值通常为 ESP_OK。
         * 对于失败事件，该值保存具体错误原因。
         */
        esp_err_t error;

        /** 事件附带的数据。 */
        app_core_event_data_t data;

    } app_core_event_t;

    /**
     * @brief 初始化 Event 对象。
     *
     * 该函数会清空 data，并初始化事件元数据、事件类型和错误码。
     *
     * @param event 要初始化的 Event 对象。
     * @param type 事件类型。
     * @param request_id 当前事件对应的请求编号。
     * @param parent_request_id 父请求编号。
     * @param source 事件来源。
     * @param scope 事件所属业务范围。
     */
    void app_core_event_init(
        app_core_event_t *event,
        app_core_event_type_t type,
        app_core_request_id_t request_id,
        app_core_request_id_t parent_request_id,
        app_core_source_t source,
        app_core_scope_t scope);

    /**
     * @brief 检查事件类型是否有效。
     *
     * NONE 不属于有效事件。
     *
     * @param type 要检查的事件类型。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_event_type_is_valid(
        app_core_event_type_t type);

    /**
     * @brief 检查 Event 对象是否有效。
     *
     * @param event 要检查的 Event 对象。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_event_is_valid(
        const app_core_event_t *event);

    /**
     * @brief 将事件类型转换成字符串。
     *
     * 主要用于日志输出和调试。
     *
     * @param type 事件类型。
     * @return 事件类型对应的字符串。
     */
    const char *app_core_event_type_to_string(
        app_core_event_type_t type);

#ifdef __cplusplus
}
#endif

#endif