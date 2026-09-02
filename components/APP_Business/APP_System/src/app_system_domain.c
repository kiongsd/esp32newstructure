#include "app_system_domain.h"

/**
 * @brief 判断 System Domain 是否负责处理指定 Request。
 */
static bool system_domain_match_request(
    void *ctx,
    const app_core_request_t *request)
{
    (void)ctx;

    if (request == NULL)
    {
        return false;
    }

    if (request->meta.scope !=
        APP_CORE_SCOPE_SYSTEM)
    {
        return false;
    }

    switch (request->type)
    {
    case APP_CORE_REQUEST_TYPE_SYSTEM_INITIALIZE:
    case APP_CORE_REQUEST_TYPE_SYSTEM_SUSPEND:
    case APP_CORE_REQUEST_TYPE_SYSTEM_RESUME:
        return true;

    default:
        return false;
    }
}

/**
 * @brief 判断 System Domain 是否负责处理指定 Event。
 */
static bool system_domain_match_event(
    void *ctx,
    const app_core_event_t *event)
{
    (void)ctx;

    if (event == NULL)
    {
        return false;
    }

    if (event->meta.scope !=
        APP_CORE_SCOPE_SYSTEM)
    {
        return false;
    }

    switch (event->type)
    {
    case APP_CORE_EVENT_TYPE_SYSTEM_READY:
    case APP_CORE_EVENT_TYPE_SYSTEM_SUSPENDED:
    case APP_CORE_EVENT_TYPE_SYSTEM_RESUMED:
    case APP_CORE_EVENT_TYPE_FAILED:
        return true;

    default:
        return false;
    }
}

/**
 * @brief 将 Controller 当前的 System 状态同步到 State Store。
 */
static esp_err_t system_domain_sync_state(
    app_system_domain_t *domain)
{
    app_core_state_snapshot_t snapshot;
    app_core_system_state_t system_state;
    esp_err_t result;

    if (domain == NULL ||
        domain->controller == NULL ||
        domain->state_store == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    result =
        app_core_state_store_read(
            domain->state_store,
            &snapshot);

    if (result != ESP_OK)
    {
        return result;
    }

    result =
        app_system_controller_get_state(
            domain->controller,
            &system_state);

    if (result != ESP_OK)
    {
        return result;
    }

    snapshot.system_state =
        system_state;

    return app_core_state_store_publish(
        domain->state_store,
        &snapshot);
}

/**
 * @brief 处理 Dispatcher 分发过来的 System Request。
 *
 * Request 处理完成后，将 FSM 状态同步到公共状态快照。
 */
static esp_err_t system_domain_handle_request(
    void *ctx,
    const app_core_request_t *request)
{
    app_system_domain_t *domain;
    esp_err_t result;
    esp_err_t sync_result;

    if (ctx == NULL ||
        request == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    domain =
        (app_system_domain_t *)ctx;

    result =
        app_system_controller_handle_request(
            domain->controller,
            request);

    sync_result =
        system_domain_sync_state(
            domain);

    if (result != ESP_OK)
    {
        return result;
    }

    return sync_result;
}

/**
 * @brief 处理 Dispatcher 分发过来的 System Event。
 *
 * Event 处理完成后，将 FSM 状态同步到公共状态快照。
 */
static esp_err_t system_domain_handle_event(
    void *ctx,
    const app_core_event_t *event)
{
    app_system_domain_t *domain;
    esp_err_t result;
    esp_err_t sync_result;

    if (ctx == NULL ||
        event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    domain =
        (app_system_domain_t *)ctx;

    result =
        app_system_controller_handle_event(
            domain->controller,
            event);

    sync_result =
        system_domain_sync_state(
            domain);

    if (result != ESP_OK)
    {
        return result;
    }

    return sync_result;
}

/**
 * @brief 执行一次 System Domain 周期处理。
 *
 * 该函数由 Dispatcher 周期调用，
 * 用于驱动 System Controller 执行待处理 Action。
 */
static esp_err_t system_domain_process_once(
    void *ctx)
{
    app_system_domain_t *domain;
    esp_err_t result;

    if (ctx == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    domain =
        (app_system_domain_t *)ctx;

    result =
        app_system_controller_process_once(
            domain->controller);

    /*
     * 当前没有待执行 Action
     * 属于正常情况，不应该被视为业务错误。
     */
    if (result == ESP_ERR_TIMEOUT)
    {
        return ESP_OK;
    }

    return result;
}

/**
 * @brief 初始化 System Domain。
 *
 * 初始化完成后，可以通过
 * app_system_domain_get_handler() 获取通用回调表，
 * 并注册到 App Core Dispatcher。
 */
esp_err_t app_system_domain_init(
    app_system_domain_t *domain,
    app_system_controller_t *controller,
    app_core_state_store_t *state_store)
{
    if (domain == NULL ||
        controller == NULL ||
        state_store == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!controller->initialized ||
        !state_store->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (domain->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    *domain =
        (app_system_domain_t){0};

    domain->controller =
        controller;

    domain->state_store =
        state_store;

    app_core_domain_handler_init(
        &domain->handler,
        domain,
        system_domain_match_request,
        system_domain_handle_request,
        system_domain_match_event,
        system_domain_handle_event,
        system_domain_process_once);

    if (!app_core_domain_handler_is_valid(
            &domain->handler))
    {
        *domain =
            (app_system_domain_t){0};

        return ESP_ERR_INVALID_STATE;
    }

    domain->initialized =
        true;

    return ESP_OK;
}

/**
 * @brief 释放 System Domain。
 */
void app_system_domain_deinit(
    app_system_domain_t *domain)
{
    if (domain == NULL)
    {
        return;
    }

    *domain =
        (app_system_domain_t){0};
}

/**
 * @brief 获取 System Domain 的 Dispatcher 回调表。
 */
const app_core_domain_handler_t *
app_system_domain_get_handler(
    const app_system_domain_t *domain)
{
    if (domain == NULL ||
        !domain->initialized)
    {
        return NULL;
    }

    return &domain->handler;
}
