#ifndef APP_CORE_TASK_H
#define APP_CORE_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_core_dispatcher.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief App Core 任务控制对象。
     *
     * App Core Task 只负责周期性驱动 Dispatcher。
     * Dispatcher 再负责：
     *
     * 1. 处理 Request；
     * 2. 处理 Event；
     * 3. 驱动所有 Domain 的 process_once；
     * 4. 间接驱动各个 Domain 自己的 Action Engine。
     */
    typedef struct
    {
        /** 要处理 Request、Event 和 Domain 的 Dispatcher。 */
        app_core_dispatcher_t *dispatcher;

        /** FreeRTOS 任务句柄。 */
        TaskHandle_t task_handle;

        /** 任务栈大小。 */
        uint32_t stack_size;

        /** 任务优先级。 */
        UBaseType_t priority;

        /** 两次处理循环之间的等待时间，单位为毫秒。 */
        uint32_t poll_interval_ms;

        /** 请求任务退出。 */
        volatile bool stop_requested;

        /** 任务是否正在运行。 */
        volatile bool running;

        /** Task 对象是否已经初始化。 */
        bool initialized;

    } app_core_task_t;

    /**
     * @brief 初始化 App Core Task。
     *
     * 该函数只初始化任务控制对象，
     * 不会立即创建 FreeRTOS 任务。
     *
     * @param task 要初始化的任务对象。
     * @param dispatcher Dispatcher 对象。
     * @param stack_size 任务栈大小。
     * @param priority 任务优先级。
     * @param poll_interval_ms 轮询间隔。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_core_task_init(
        app_core_task_t *task,
        app_core_dispatcher_t *dispatcher,
        uint32_t stack_size,
        UBaseType_t priority,
        uint32_t poll_interval_ms);

    /**
     * @brief 启动 App Core Task。
     *
     * @param task 已初始化的任务对象。
     * @return ESP_OK 表示任务创建成功。
     */
    esp_err_t app_core_task_start(
        app_core_task_t *task);

    /**
     * @brief 请求 App Core Task 停止。
     *
     * @param task App Core Task。
     * @return ESP_OK 表示请求成功。
     */
    esp_err_t app_core_task_request_stop(
        app_core_task_t *task);

    /**
     * @brief 释放 App Core Task 控制对象。
     *
     * 任务仍在运行时不能调用该函数。
     *
     * @param task App Core Task。
     * @return ESP_OK 表示释放成功。
     */
    esp_err_t app_core_task_deinit(
        app_core_task_t *task);

    /**
     * @brief 判断 App Core Task 是否正在运行。
     *
     * @param task App Core Task。
     * @return true 表示正在运行。
     */
    bool app_core_task_is_running(
        const app_core_task_t *task);

#ifdef __cplusplus
}
#endif

#endif