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
         * Dispatcher 负责：
         *
         * 1. 处理 Request；
         * 2. 处理 Event；
         * 3. 驱动所有 Domain；
         * 4. 驱动每个 Domain 对应的 Action Engine。
         */
        (void)app_core_dispatcher_process_once(
            task->dispatcher);

        /**
         * 当前 Dispatcher 在没有消息时可能返回 ESP_OK，
         * 因此这里始终休眠，避免任务空转占满 CPU。
         *
         * 请求停止时，app_core_task_request_stop()
         * 会通过任务通知提前唤醒这里。
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
    uint32_t stack_size,
    UBaseType_t priority,
    uint32_t poll_interval_ms)
{
    if (task == NULL ||
        dispatcher == NULL ||
        stack_size == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!dispatcher->initialized)
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
        task->dispatcher == NULL)
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