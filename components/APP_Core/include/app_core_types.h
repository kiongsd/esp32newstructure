#ifndef APP_CORE_TYPES_H
#define APP_CORE_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief App Core 请求编号。
     *
     * 用于标识一次来自按键、LVGL、HTTP 或系统自身的请求。
     * 同一个异步业务流程可以通过 request_id 进行跟踪。
     */
    typedef uint32_t app_core_request_id_t;

    /**
     * @brief App Core 任务编号。
     *
     * 用于标识一次具体的拍照、存储、OTA 等异步任务。
     */
    typedef uint32_t app_core_job_id_t;

    /**
     * @brief 照片句柄。
     *
     * Camera 拍照完成后，不直接把大块图像数据复制到消息中，
     * 而是通过 photo_handle 引用照片资源。
     */
    typedef uint32_t app_core_photo_handle_t;

/**
 * @brief 无效请求编号。
 */
#define APP_CORE_INVALID_REQUEST_ID ((app_core_request_id_t)0U)

/**
 * @brief 无效任务编号。
 */
#define APP_CORE_INVALID_JOB_ID ((app_core_job_id_t)0U)

/**
 * @brief 无效照片句柄。
 */
#define APP_CORE_INVALID_PHOTO_HANDLE ((app_core_photo_handle_t)0U)

/**
 * @brief OTA 产品名最大长度。
 */
#define APP_CORE_OTA_PRODUCT_MAX_LEN 32U

/**
 * @brief OTA 版本号最大长度。
 */
#define APP_CORE_OTA_VERSION_MAX_LEN 32U

/**
 * @brief OTA SHA-256 十六进制字符串长度。
 */
#define APP_CORE_OTA_SHA256_HEX_LEN 64U

    /**
     * @brief 消息来源。
     *
     * 用于标识一条 Request 或 Event 是由哪个模块产生的。
     */
    typedef enum
    {
        /** 系统启动流程产生的消息。 */
        APP_CORE_SOURCE_SYSTEM = 0,

        /** 物理按键产生的消息。 */
        APP_CORE_SOURCE_KEY,

        /** LVGL 界面产生的消息。 */
        APP_CORE_SOURCE_LVGL,

        /** HTTP/Web 接口产生的消息。 */
        APP_CORE_SOURCE_WEB,

        /** App Core 内部 Action 产生的消息。 */
        APP_CORE_SOURCE_ACTION,

        /** Service 层产生的消息。 */
        APP_CORE_SOURCE_SERVICE,

        /** Driver 或 BSP 层产生的消息。 */
        APP_CORE_SOURCE_DRIVER,

        /** 电源管理模块产生的消息。 */
        APP_CORE_SOURCE_POWER,

        /** 测试代码产生的消息。 */
        APP_CORE_SOURCE_TEST,

    } app_core_source_t;

    /**
     * @brief 消息所属业务范围。
     *
     * Dispatcher 根据 scope 将消息分发给对应的业务域。
     */
    typedef enum
    {
        /** 系统业务域。 */
        APP_CORE_SCOPE_SYSTEM = 0,

        /** Camera 和拍照业务域。 */
        APP_CORE_SCOPE_CAPTURE,

        /** Web/HTTP 业务域。 */
        APP_CORE_SCOPE_WEB,

        /** LVGL/UI 业务域。 */
        APP_CORE_SCOPE_LVGL,

        /** SD 卡和照片存储业务域。 */
        APP_CORE_SCOPE_STORAGE,

        /** OTA 升级业务域。 */
        APP_CORE_SCOPE_OTA,

    } app_core_scope_t;

    /**
     * @brief 系统状态。
     */
    typedef enum
    {
        /** 系统状态未知。 */
        APP_CORE_SYSTEM_STATE_UNKNOWN = 0,

        /** 系统正在启动。 */
        APP_CORE_SYSTEM_STATE_BOOTING,

        /** 系统正在初始化各个模块。 */
        APP_CORE_SYSTEM_STATE_INITIALIZING,

        /** 系统已经准备完成。 */
        APP_CORE_SYSTEM_STATE_READY,

        /** 系统部分功能不可用，但仍可运行。 */
        APP_CORE_SYSTEM_STATE_DEGRADED,

        /** 系统正在进入挂起状态。 */
        APP_CORE_SYSTEM_STATE_SUSPENDING,

        /** 系统已经挂起。 */
        APP_CORE_SYSTEM_STATE_SUSPENDED,

        /** 系统正在恢复运行。 */
        APP_CORE_SYSTEM_STATE_RESUMING,

        /** 系统发生严重故障。 */
        APP_CORE_SYSTEM_STATE_FAULT,

    } app_core_system_state_t;

    /**
     * @brief Camera 拍照业务状态。
     */
    typedef enum
    {
        /** Camera 当前空闲。 */
        APP_CORE_CAPTURE_STATE_IDLE = 0,

        /** 已经收到拍照请求。 */
        APP_CORE_CAPTURE_STATE_REQUESTED,

        /** 正在准备拍照。 */
        APP_CORE_CAPTURE_STATE_PREPARING,

        /** 正在获取图像。 */
        APP_CORE_CAPTURE_STATE_ACQUIRING,

        /** 正在处理图像。 */
        APP_CORE_CAPTURE_STATE_PROCESSING,

        /** 正在保存照片。 */
        APP_CORE_CAPTURE_STATE_SAVING,

        /** 拍照流程成功完成。 */
        APP_CORE_CAPTURE_STATE_SUCCEEDED,

        /** 拍照流程失败。 */
        APP_CORE_CAPTURE_STATE_FAILED,

        /** 拍照流程被取消。 */
        APP_CORE_CAPTURE_STATE_CANCELED,

    } app_core_capture_state_t;

    /**
     * @brief Web 服务状态。
     */
    typedef enum
    {
        /** Web 服务未启动。 */
        APP_CORE_WEB_STATE_STOPPED = 0,

        /** Web 服务正在启动。 */
        APP_CORE_WEB_STATE_STARTING,

        /** Web 服务正在运行。 */
        APP_CORE_WEB_STATE_RUNNING,

        /** Web 服务正在停止。 */
        APP_CORE_WEB_STATE_STOPPING,

        /** Web 服务发生错误。 */
        APP_CORE_WEB_STATE_ERROR,

    } app_core_web_state_t;

    /**
     * @brief 照片存储状态。
     */
    typedef enum
    {
        /** 存储模块空闲。 */
        APP_CORE_STORAGE_STATE_IDLE = 0,

        /** 正在扫描照片。 */
        APP_CORE_STORAGE_STATE_SCANNING,

        /** 正在显示照片。 */
        APP_CORE_STORAGE_STATE_SHOWING,

        /** 存储模块发生错误。 */
        APP_CORE_STORAGE_STATE_FAILED,

    } app_core_storage_state_t;

    /**
     * @brief OTA 升级状态。
     */
    typedef enum
    {
        /** OTA 模块空闲。 */
        APP_CORE_OTA_STATE_IDLE = 0,

        /** 正在准备 OTA。 */
        APP_CORE_OTA_STATE_PREPARING,

        /** 正在接收固件数据。 */
        APP_CORE_OTA_STATE_RECEIVING,

        /** 正在校验固件。 */
        APP_CORE_OTA_STATE_VERIFYING,

        /** 正在中止 OTA。 */
        APP_CORE_OTA_STATE_ABORTING,

        /** 固件已经准备好重启。 */
        APP_CORE_OTA_STATE_READY_TO_REBOOT,

        /** OTA 流程失败。 */
        APP_CORE_OTA_STATE_FAILED,

    } app_core_ota_state_t;

    /**
     * @brief OTA 固件描述信息。
     *
     * 该结构体用于描述即将升级的固件，
     * 后续可以用于版本校验、产品校验和完整性校验。
     */
    typedef struct
    {
        /** OTA 目标产品名称。 */
        char product[APP_CORE_OTA_PRODUCT_MAX_LEN + 1U];

        /** OTA 目标版本号。 */
        char version[APP_CORE_OTA_VERSION_MAX_LEN + 1U];

        /** 固件 SHA-256 校验值，以十六进制字符串保存。 */
        char sha256[APP_CORE_OTA_SHA256_HEX_LEN + 1U];

        /** 固件完整大小，单位为字节。 */
        uint32_t image_size;

    } app_core_ota_manifest_t;

    /**
     * @brief LVGL 页面类型。
     */
    typedef enum
    {
        /** 启动页面。 */
        APP_CORE_LVGL_PAGE_BOOT = 0,

        /** 主菜单页面。 */
        APP_CORE_LVGL_PAGE_MENU,

        /** Camera 预览页面。 */
        APP_CORE_LVGL_PAGE_PREVIEW,

        /** 单张照片查看页面。 */
        APP_CORE_LVGL_PAGE_PHOTO,

        /** 图库页面。 */
        APP_CORE_LVGL_PAGE_GALLERY,

        /** 图库照片详细查看页面。 */
        APP_CORE_LVGL_PAGE_GALLERY_VIEW,

        /** 设置页面。 */
        APP_CORE_LVGL_PAGE_SETTINGS,

    } app_core_lvgl_page_t;

    /**
     * @brief 消息的公共元数据。
     *
     * Request、Event 和 Effect 都会携带该结构体。
     * 它用于追踪消息来源、父子关系和业务范围。
     */
    typedef struct
    {
        /** 当前消息的请求编号。 */
        app_core_request_id_t request_id;

        /** 当前消息对应的父请求编号。 */
        app_core_request_id_t parent_request_id;

        /** 产生当前消息的模块。 */
        app_core_source_t source;

        /** 当前消息所属的业务范围。 */
        app_core_scope_t scope;

    } app_core_message_meta_t;

    /**
     * @brief 初始化消息元数据。
     *
     * @param meta 要初始化的消息元数据对象。
     * @param request_id 当前请求编号。
     * @param parent_request_id 父请求编号。
     * @param source 消息来源。
     * @param scope 消息业务范围。
     */
    void app_core_message_meta_init(
        app_core_message_meta_t *meta,
        app_core_request_id_t request_id,
        app_core_request_id_t parent_request_id,
        app_core_source_t source,
        app_core_scope_t scope);

    /**
     * @brief 检查消息来源是否有效。
     *
     * @param source 要检查的消息来源。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_source_is_valid(
        app_core_source_t source);

    /**
     * @brief 检查消息业务范围是否有效。
     *
     * @param scope 要检查的业务范围。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_scope_is_valid(
        app_core_scope_t scope);

    /**
     * @brief 检查消息元数据是否有效。
     *
     * @param meta 要检查的消息元数据。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_message_meta_is_valid(
        const app_core_message_meta_t *meta);

    /**
     * @brief 将消息来源转换为字符串。
     *
     * 主要用于日志输出和调试。
     *
     * @param source 消息来源。
     * @return 对应的字符串。
     */
    const char *app_core_source_to_string(
        app_core_source_t source);

    /**
     * @brief 将消息业务范围转换为字符串。
     *
     * 主要用于日志输出和调试。
     *
     * @param scope 消息业务范围。
     * @return 对应的字符串。
     */
    const char *app_core_scope_to_string(
        app_core_scope_t scope);

    /**
     * @brief 将系统状态转换为字符串。
     *
     * @param state 系统状态。
     * @return 对应的字符串。
     */
    const char *app_core_system_state_to_string(
        app_core_system_state_t state);

    /**
     * @brief 将拍照状态转换为字符串。
     *
     * @param state 拍照状态。
     * @return 对应的字符串。
     */
    const char *app_core_capture_state_to_string(
        app_core_capture_state_t state);

    /**
     * @brief 将 Web 状态转换为字符串。
     *
     * @param state Web 状态。
     * @return 对应的字符串。
     */
    const char *app_core_web_state_to_string(
        app_core_web_state_t state);

    /**
     * @brief 将存储状态转换为字符串。
     *
     * @param state 存储状态。
     * @return 对应的字符串。
     */
    const char *app_core_storage_state_to_string(
        app_core_storage_state_t state);

    /**
     * @brief 将 OTA 状态转换为字符串。
     *
     * @param state OTA 状态。
     * @return 对应的字符串。
     */
    const char *app_core_ota_state_to_string(
        app_core_ota_state_t state);

    /**
     * @brief 将 LVGL 页面转换为字符串。
     *
     * @param page LVGL 页面。
     * @return 对应的字符串。
     */
    const char *app_core_lvgl_page_to_string(
        app_core_lvgl_page_t page);

#ifdef __cplusplus
}
#endif

#endif