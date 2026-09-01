#include <stddef.h>

#include "app_core_dispatcher.h"

/**
 * @brief 将 Request 分发给对应 Domain。
 */
static esp_err_t dispatch_request(
    app_core_dispatcher_t *dispatcher,
    const app_core_request_t *request)
{
    uint8_t index;
    bool handled = false;

    for (index = 0U;
         index < dispatcher->domain_count;
         index++)
    {
        app_core_domain_handler_t *domain =
            &dispatcher->domains[index];

        if (domain->match_request != NULL &&
            domain->match_request(
                domain->ctx,
                request))
        {
            handled = true;

            if (domain->handle_request == NULL)
            {
                return ESP_ERR_INVALID_STATE;
            }

            return domain->handle_request(
                domain->ctx,
                request);
        }
    }

    if (!handled)
    {
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}

/**
 * @brief 将 Event 分发给对应 Domain。
 */
static esp_err_t dispatch_event(
    app_core_dispatcher_t *dispatcher,
    const app_core_event_t *event)
{
    uint8_t index;
    bool handled = false;

    for (index = 0U;
         index < dispatcher->domain_count;
         index++)
    {
        app_core_domain_handler_t *domain =
            &dispatcher->domains[index];

        if (domain->match_event != NULL &&
            domain->match_event(
                domain->ctx,
                event))
        {
            handled = true;

            if (domain->handle_event == NULL)
            {
                return ESP_ERR_INVALID_STATE;
            }

            return domain->handle_event(
                domain->ctx,
                event);
        }
    }

    if (!handled)
    {
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}

/**
 * @brief 初始化 Dispatcher。
 */
esp_err_t app_core_dispatcher_init(
    app_core_dispatcher_t *dispatcher,
    UBaseType_t request_queue_length,
    UBaseType_t event_queue_length)
{
    if (dispatcher == NULL ||
        request_queue_length == 0U ||
        event_queue_length == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (dispatcher->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    dispatcher->request_queue =
        xQueueCreate(
            request_queue_length,
            sizeof(app_core_request_t));

    if (dispatcher->request_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    dispatcher->event_queue =
        xQueueCreate(
            event_queue_length,
            sizeof(app_core_event_t));

    if (dispatcher->event_queue == NULL)
    {
        vQueueDelete(dispatcher->request_queue);
        dispatcher->request_queue = NULL;

        return ESP_ERR_NO_MEM;
    }

    dispatcher->domain_count = 0U;
    dispatcher->state_store = NULL;
    dispatcher->initialized = true;

    return ESP_OK;
}

/**
 * @brief 释放 Dispatcher。
 */
void app_core_dispatcher_deinit(
    app_core_dispatcher_t *dispatcher)
{
    if (dispatcher == NULL)
    {
        return;
    }

    if (dispatcher->request_queue != NULL)
    {
        vQueueDelete(dispatcher->request_queue);
        dispatcher->request_queue = NULL;
    }

    if (dispatcher->event_queue != NULL)
    {
        vQueueDelete(dispatcher->event_queue);
        dispatcher->event_queue = NULL;
    }

    dispatcher->domain_count = 0U;
    dispatcher->state_store = NULL;
    dispatcher->initialized = false;
}

/**
 * @brief 注册一个 Domain。
 */
esp_err_t app_core_dispatcher_register_domain(
    app_core_dispatcher_t *dispatcher,
    const app_core_domain_handler_t *domain)
{
    if (dispatcher == NULL ||
        domain == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!dispatcher->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!app_core_domain_handler_is_valid(domain))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (dispatcher->domain_count >=
        APP_CORE_DISPATCHER_MAX_DOMAINS)
    {
        return ESP_ERR_NO_MEM;
    }

    dispatcher->domains[dispatcher->domain_count] = *domain;

    dispatcher->domain_count++;

    return ESP_OK;
}

/**
 * @brief 提交 Request。
 */
esp_err_t app_core_dispatcher_submit_request(
    app_core_dispatcher_t *dispatcher,
    const app_core_request_t *request)
{
    if (dispatcher == NULL ||
        request == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!dispatcher->initialized ||
        dispatcher->request_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!app_core_request_is_valid(request))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (xQueueSend(
            dispatcher->request_queue,
            request,
            0U) != pdPASS)
    {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

/**
 * @brief 向 Dispatcher 发送 Event。
 */
esp_err_t app_core_dispatcher_emit_event(
    void *ctx,
    const app_core_event_t *event)
{
    app_core_dispatcher_t *dispatcher =
        (app_core_dispatcher_t *)ctx;

    if (dispatcher == NULL ||
        event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!dispatcher->initialized ||
        dispatcher->event_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!app_core_event_is_valid(event))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (xQueueSend(
            dispatcher->event_queue,
            event,
            0U) != pdPASS)
    {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

/**
 * @brief 绑定 State Store。
 */
esp_err_t app_core_dispatcher_bind_state_store(
    app_core_dispatcher_t *dispatcher,
    app_core_state_store_t *state_store)
{
    if (dispatcher == NULL ||
        state_store == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!dispatcher->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    dispatcher->state_store = state_store;

    return ESP_OK;
}

/**
 * @brief 读取当前系统状态。
 */
esp_err_t app_core_dispatcher_read_state(
    const app_core_dispatcher_t *dispatcher,
    app_core_state_snapshot_t *snapshot)
{
    if (dispatcher == NULL ||
        snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!dispatcher->initialized ||
        dispatcher->state_store == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return app_core_state_store_read(
        dispatcher->state_store,
        snapshot);
}

/**
 * @brief 执行一次 Dispatcher 处理。
 */
esp_err_t app_core_dispatcher_process_once(
    app_core_dispatcher_t *dispatcher)
{
    app_core_request_t request;
    app_core_event_t event;
    esp_err_t result = ESP_OK;
    esp_err_t current_result;
    uint8_t index;

    if (dispatcher == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!dispatcher->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (xQueueReceive(
            dispatcher->request_queue,
            &request,
            0U) == pdTRUE)
    {
        current_result =
            dispatch_request(
                dispatcher,
                &request);

        if (current_result != ESP_OK &&
            result == ESP_OK)
        {
            result = current_result;
        }
    }

    if (xQueueReceive(
            dispatcher->event_queue,
            &event,
            0U) == pdTRUE)
    {
        current_result =
            dispatch_event(
                dispatcher,
                &event);

        if (current_result != ESP_OK &&
            result == ESP_OK)
        {
            result = current_result;
        }
    }

    for (index = 0U;
         index < dispatcher->domain_count;
         index++)
    {
        app_core_domain_handler_t *domain =
            &dispatcher->domains[index];

        if (domain->process_once != NULL)
        {
            current_result =
                domain->process_once(
                    domain->ctx);

            if (current_result != ESP_OK &&
                result == ESP_OK)
            {
                result = current_result;
            }
        }
    }

    return result;
}