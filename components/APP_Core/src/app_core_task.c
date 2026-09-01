#include "app_core_task.h"

#include "esp_err.h"

/**
 * @brief 将毫秒转换为 FreeRTOS Tick。
 */
static TickType_t app_core_task_get_poll_ticks(
    uint32_t poll_interval_ms)
{
    TickType_t ticks;

    ticks =
        pdMS_TO_TICKS(
            poll_interval_ms);

    if (ticks == 0U)
    {
        ticks = 1U;
    }

    return ticks;
}

/**
 * @brief App Core FreeRTOS 任务入口。
 */
static void app_core_task_entry(
    void *arg)
{
    app_core_task_t *task;
    TickType_t poll_ticks;

    task =
        (app_core_task_t *)arg;

    if (task == NULL)
    {
        vTaskDelete(NULL);
        return;
    }

    task->task_handle =
        xTaskGetCurrentTaskHandle();

    poll_ticks =
        app_core_task_get_poll_ticks(
            task->poll_interval_ms);

    while (!task->stop_requested)
    {
        /**
         * 先处理外部 Request、底层 Event
         * 和各个 Domain 的周期处理。
         */
        (void)app_core_dispatcher_process_once(
            task->dispatcher);

        /**
         * 再处理待执行的 Action。
         */
        (void)app_core_action_engine_process_once(
            task->action_engine);

        /**
         * 没有新的消息时休眠。
         *
         * Request Stop 时会通过任务通知唤醒，
         * 避免任务一直等待到完整轮询周期结束。
         */
        ulTaskNotifyTake(
            pdTRUE,
            poll_ticks);
    }

    task->running =
        false;

    task->task_handle =
        NULL;

    vTaskDelete(NULL);
}

/**
 * @brief 初始化 App Core Task。
 */
esp_err_t app_core_task_init(
    app_core_task_t *task,
    app_core_dispatcher_t *dispatcher,
    app_core_action_engine_t *action_engine,
    uint32_t stack_size,
    UBaseType_t priority,
    uint32_t poll_interval_ms)
{
    if (task == NULL ||
        dispatcher == NULL ||
        action_engine == NULL ||
        stack_size == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!dispatcher->initialized ||
        !action_engine->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * 调用者应该使用以下方式定义对象：
     *
     * app_core_task_t task = {0};
     */
    if (task->initialized ||
        task->running ||
        task->task_handle != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    *task =
        (app_core_task_t){0};

    task->dispatcher =
        dispatcher;

    task->action_engine =
        action_engine;

    task->stack_size =
        stack_size;

    task->priority =
        priority;

    task->poll_interval_ms =
        poll_interval_ms;

    task->stop_requested =
        false;

    task->running =
        false;

    task->initialized =
        true;

    return ESP_OK;
}

/**
 * @brief 启动 App Core Task。
 */
esp_err_t app_core_task_start(
    app_core_task_t *task)
{
    BaseType_t result;

    if (task == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!task->initialized ||
        task->dispatcher == NULL ||
        task->action_engine == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (task->running ||
        task->task_handle != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    task->stop_requested =
        false;

    task->running =
        true;

    result =
        xTaskCreate(
            app_core_task_entry,
            "app_core_task",
            task->stack_size,
            task,
            task->priority,
            &task->task_handle);

    if (result != pdPASS)
    {
        task->running =
            false;

        task->task_handle =
            NULL;

        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

/**
 * @brief 请求 App Core Task 停止。
 */
esp_err_t app_core_task_request_stop(
    app_core_task_t *task)
{
    if (task == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!task->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    task->stop_requested =
        true;

    if (task->task_handle != NULL)
    {
        xTaskNotifyGive(
            task->task_handle);
    }

    return ESP_OK;
}

/**
 * @brief 释放 App Core Task。
 */
esp_err_t app_core_task_deinit(
    app_core_task_t *task)
{
    if (task == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!task->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (task->running ||
        task->task_handle != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    *task =
        (app_core_task_t){0};

    return ESP_OK;
}

/**
 * @brief 判断 App Core Task 是否运行。
 */
bool app_core_task_is_running(
    const app_core_task_t *task)
{
    if (task == NULL ||
        !task->initialized)
    {
        return false;
    }

    return task->running;
}