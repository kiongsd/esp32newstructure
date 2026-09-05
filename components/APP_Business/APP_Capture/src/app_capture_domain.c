#include "app_capture_domain.h"

/**
 * @brief 判断 capture Domain 是否负责处理指定 Request。
 */
static bool capture_domain_match_request(
    void *ctx,
    const app_core_request_t *request)
{
    (void)ctx;

    if (request == NULL)
    {
        return false;
    }

    if (request->meta.scope !=
        APP_CORE_SCOPE_CAPTURE)
    {
        return false;
    }

    switch (request->type)
    {
    case APP_CORE_REQUEST_TYPE_CAPTURE_START_PREVIEW:
    case APP_CORE_REQUEST_TYPE_CAPTURE_PREPARE_PHOTO:
    case APP_CORE_REQUEST_TYPE_CAPTURE:
    case APP_CORE_REQUEST_TYPE_CAPTURE_STOP:
    case APP_CORE_REQUEST_TYPE_CAPTURE_FOCUS:
        return true;

    default:
        return false;
    }
}

/**
 * @brief 判断 capture Domain 是否负责处理指定 Event。
 */
static bool capture_domain_match_event(
    void *ctx,
    const app_core_event_t *event)
{
    (void)ctx;

    if (event == NULL)
    {
        return false;
    }

    if (event->meta.scope !=
        APP_CORE_SCOPE_CAPTURE)
    {
        return false;
    }

    switch (event->type)
    {
    case APP_CORE_EVENT_TYPE_CAMERA_PREVIEW_STARTED:
    case APP_CORE_EVENT_TYPE_CAMERA_PREPARED:
    case APP_CORE_EVENT_TYPE_CAMERA_CAPTURED:
    case APP_CORE_EVENT_TYPE_CAMERA_STOPPED:
    case APP_CORE_EVENT_TYPE_CAMERA_FOCUSED:
    case APP_CORE_EVENT_TYPE_PHOTO_SAVED:
    case APP_CORE_EVENT_TYPE_FAILED:
        return true;

    default:
        return false;
    }
}

/**
 * @brief 将 Controller 当前的 capture 状态同步到 State Store。
 */
static esp_err_t capture_domain_sync_state(
    app_capture_domain_t *domain)
{
    app_core_state_snapshot_t snapshot;
    app_core_capture_state_t capture_state;
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
        app_capture_controller_get_state(
            domain->controller,
            &capture_state);

    if (result != ESP_OK)
    {
        return result;
    }

    snapshot.capture_state =
        capture_state;

    snapshot.active_request_id =
        domain->controller->fsm.active_request_id;

    snapshot.active_job_id =
        domain->controller->fsm.active_job_id;

    snapshot.preview_active =
        domain->controller->fsm.preview_active;

    snapshot.web_capture_active =
        domain->controller->fsm.web_capture_active;

    snapshot.last_photo_handle =
        domain->controller->fsm.active_photo_handle;

    snapshot.last_error =
        domain->controller->fsm.last_error;

    return app_core_state_store_publish(
        domain->state_store,
        &snapshot);
}

/**
 * @brief 处理 Dispatcher 分发过来的 capture Request。
 *
 * Request 处理完成后，将 FSM 状态同步到公共状态快照。
 */
static esp_err_t capture_domain_handle_request(
    void *ctx,
    const app_core_request_t *request)
{
    app_capture_domain_t *domain;
    esp_err_t result;
    esp_err_t sync_result;

    if (ctx == NULL ||
        request == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    domain =
        (app_capture_domain_t *)ctx;

    result =
        app_capture_controller_handle_request(
            domain->controller,
            request);

    sync_result =
        capture_domain_sync_state(
            domain);

    if (result != ESP_OK)
    {
        return result;
    }

    return sync_result;
}

/**
 * @brief 处理 Dispatcher 分发过来的 capture Event。
 *
 * Event 处理完成后，将 FSM 状态同步到公共状态快照。
 */
static esp_err_t capture_domain_handle_event(
    void *ctx,
    const app_core_event_t *event)
{
    app_capture_domain_t *domain;
    esp_err_t result;

    if (ctx == NULL ||
        event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    domain =
        (app_capture_domain_t *)ctx;

    result =
        app_capture_controller_handle_event(
            domain->controller,
            event);

    if (result == ESP_ERR_NOT_FOUND ||
        result == ESP_ERR_NOT_SUPPORTED)
    {
        return ESP_OK;
    }

    if (result != ESP_OK)
    {
        return result;
    }

    return capture_domain_sync_state(
        domain);
}

/**
 * @brief 执行一次 capture Domain 周期处理。
 *
 * 该函数由 Dispatcher 周期调用，
 * 用于驱动 capture Controller 执行待处理 Action。
 */
static esp_err_t capture_domain_process_once(
    void *ctx)
{
    app_capture_domain_t *domain;
    esp_err_t result;

    if (ctx == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    domain =
        (app_capture_domain_t *)ctx;

    result =
        app_capture_controller_process_once(
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
 * @brief 初始化 capture Domain。
 *
 * 初始化完成后，可以通过
 * app_capture_domain_get_handler() 获取通用回调表，
 * 并注册到 App Core Dispatcher。
 */
esp_err_t app_capture_domain_init(
    app_capture_domain_t *domain,
    app_capture_controller_t *controller,
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
        (app_capture_domain_t){0};

    domain->controller =
        controller;

    domain->state_store =
        state_store;

    app_core_domain_handler_init(
        &domain->handler,
        domain,
        capture_domain_match_request,
        capture_domain_handle_request,
        capture_domain_match_event,
        capture_domain_handle_event,
        capture_domain_process_once);

    if (!app_core_domain_handler_is_valid(
            &domain->handler))
    {
        *domain =
            (app_capture_domain_t){0};

        return ESP_ERR_INVALID_STATE;
    }

    domain->initialized =
        true;

    return ESP_OK;
}

/**
 * @brief 释放 capture Domain。
 */
void app_capture_domain_deinit(
    app_capture_domain_t *domain)
{
    if (domain == NULL)
    {
        return;
    }

    *domain =
        (app_capture_domain_t){0};
}

/**
 * @brief 获取 capture Domain 的 Dispatcher 回调表。
 */
const app_core_domain_handler_t *
app_capture_domain_get_handler(
    const app_capture_domain_t *domain)
{
    if (domain == NULL ||
        !domain->initialized)
    {
        return NULL;
    }

    return &domain->handler;
}
