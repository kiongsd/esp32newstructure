#ifndef __APP_APPLICATION_H
#define __APP_APPLICATION_H
#include <stdbool.h>

#include "esp_err.h"

#include "app_core_dispatcher.h"
#include "app_core_request_gateway.h"
#include "app_core_runtime.h"
#include "app_core_state_store.h"
#include "app_core_task.h"

#include "app_system_controller.h"
#include "app_system_domain.h"

#include "app_capture_controller.h"
#include "app_capture_domain.h"

#include "app_storage_controller.h"
#include "app_storage_domain.h"

#include "app_ui_controller.h"
#include "app_ui_domain.h"

#ifdef __cplusplus
extern "C"
{
#endif
    // Your code here
#define APP_APPLICATION_REQUEST_QUEUE_LENGTH 16U
#define APP_APPLICATION_EVENT_QUEUE_LENGTH 16U
#define APP_APPLICATION_ACTION_QUEUE_LENGTH 4U
#define APP_APPLICATION_TASK_STACK_SIZE 4096U
#define APP_APPLICATION_TASK_PRIORITY 5U
#define APP_APPLICATION_POLL_INTERVAL_MS 10U

    /**
     * @brief 应用层统一运行对象。
     *
     * Application 负责组合：
     *
     * 1. App Core 公共基础设施；
     * 2. System、Capture、Storage 和 UI Controller；
     * 3. System、Capture、Storage 和 UI Domain；
     * 4. Request Gateway；
     * 5. App Core 后台任务。
     *
     * Application 不负责实现具体硬件功能，
     * 具体硬件操作通过 app_core_runtime_t 注入。
     */
    typedef struct
    {
        /** 公共状态存储对象。 */
        app_core_state_store_t state_store;

        /** 统一的 Request/Event 分发器。 */
        app_core_dispatcher_t dispatcher;
        /** 驱动 Dispatcher 的 App Core 任务。 */
        app_core_task_t core_task;
        /** 所有业务 Domain 共用的 Runtime 回调表。 */
        app_core_runtime_t runtime;
        /** System Controller。 */
        app_system_controller_t system_controller;
        /** System Domain。 */
        app_system_domain_t system_domain;
        /** Capture Controller。 */
        app_capture_controller_t capture_controller;

        /** Capture Domain。 */
        app_capture_domain_t capture_domain;
        /** Storage Controller。 */
        app_storage_controller_t storage_controller;
        /** Storage Domain。 */
        app_storage_domain_t storage_domain;
        /** UI Controller。 */
        app_ui_controller_t ui_controller;
        /** UI Domain。 */
        app_ui_domain_t ui_domain;
        /** Request Gateway 是否已经初始化。 */
        bool gateway_initialized;
        /** Application 是否已经初始化。 */
        bool initialized;
        /* data */
    } app_application_t;
    /**
     * @brief 初始化统一 Application。
     *
     * 函数会依次初始化 State Store、Dispatcher、
     * Controller、Domain、Request Gateway 和 App Core Task。
     *
     * 该函数只完成对象初始化，不启动后台任务。
     *
     * @param application 要初始化的 Application 对象。
     * @param runtime 底层 Runtime 回调集合。
     * @return ESP_OK 表示初始化成功，否则返回错误码。
     */
    esp_err_t app_application_init(
        app_application_t *application,
        const app_core_runtime_t *runtime);

    /**
     * @brief 启动统一 Application。
     *
     * 函数会绑定 Request Gateway，
     * 并启动 App Core 后台任务。
     *
     * @param application 已初始化的 Application 对象。
     * @return ESP_OK 表示启动成功，否则返回错误码。
     */
    esp_err_t app_application_start(
        app_application_t *application);

    /**
     * @brief 请求停止统一 Application。
     *
     * 该函数只发送停止请求，不会等待任务立即退出。
     *
     * @param application 已启动的 Application 对象。
     * @return ESP_OK 表示停止请求发送成功，否则返回错误码。
     */
    esp_err_t app_application_stop(
        app_application_t *application);

    /**
     * @brief 释放统一 Application。
     *
     * 只有在 App Core Task 已经停止后，
     * 才允许调用该函数。
     *
     * @param application 要释放的 Application 对象。
     * @return ESP_OK 表示释放成功，否则返回错误码。
     */
    esp_err_t app_application_deinit(
        app_application_t *application);
#ifdef __cplusplus
}
#endif
#endif // __APP_APPLICATION_H