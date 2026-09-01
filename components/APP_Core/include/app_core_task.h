#ifndef APP_CORE_TASK_H
#define APP_CORE_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_core_action_engine.h"
#include "app_core_dispatcher.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief App Core 任务控制对象。
     *
     * App Core Task 负责周期性驱动：
     *
     * 1. Dispatcher 处理 Request 和 Event；
     * 2. Action Engine 执行待处理 Action；
     * 3. Action Engine 等待并处理底层 Event。
     *
     * 当前任务不直接执行 Camera、LVGL、Web 或 OTA，
     * 具体动作由 Runtime 完成。
     */
    typedef struct
    {
        /** 要处理 Request 和 Event 的 Dispatcher。 */
        app_core_dispatcher_t *dispatcher;

        /** 要执行 Action 的 Action Engine。 */
        app_core_action_engine_t *action_engine;

        /** FreeRTOS 任务句柄。 */
        TaskHandle_t task_handle;

        /** 任务栈大小。 */
        uint32_t stack_size;

        /** 任务优先级。 */
        UBaseType_t priority;

        /** 两次处理循环之间的最大等待时间，单位为毫秒。 */
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
     * @param action_engine Action Engine 对象。
     * @param stack_size 任务栈大小。
     * @param priority 任务优先级。
     * @param poll_interval_ms 轮询间隔。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_core_task_init(
        app_core_task_t *task,
        app_core_dispatcher_t *dispatcher,
        app_core_action_engine_t *action_engine,
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
     * 该函数只设置退出标志，
     * 任务会自行退出并释放自己的任务资源。
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