#include "app_ui_domain.h"

#include <stddef.h>

/**
 * @brief 判断 UI Domain 是否负责处理指定 Request。
 *
 * UI Domain 只接收 LVGL 作用域下的 SHOW_PAGE 请求。
 */
static bool ui_domain_match_request(
    void *ctx,
    const app_core_request_t *request)
{
    (void)ctx;

    if (request == NULL ||
        request->meta.scope != APP_CORE_SCOPE_LVGL)
    {
        return false;
    }

    switch (request->type)
    {
    case APP_CORE_REQUEST_TYPE_SHOW_PAGE:
        return true;

    default:
        return false;
    }
}

/**
 * @brief 判断 UI Domain 是否负责处理指定 Event。
 *
 * UI Domain 只接收 LVGL 作用域下的页面完成和失败事件。
 */
static bool ui_domain_match_event(
    void *ctx,
    const app_core_event_t *event)
{
    (void)ctx;

    if (event == NULL ||
        event->meta.scope != APP_CORE_SCOPE_LVGL)
    {
        return false;
    }

    switch (event->type)
    {
    case APP_CORE_EVENT_TYPE_PAGE_SHOWN:
    case APP_CORE_EVENT_TYPE_FAILED:
        return true;

    default:
        return false;
    }
}

/**
 * @brief 将 UI FSM 状态同步到公共 State Store。
 *
 * Request 或 Event 处理完成后，
 * 将当前页面、过渡状态和请求信息写入公共状态快照。
 */
static esp_err_t ui_domain_sync_state(
    app_ui_domain_t *domain)
{
    app_core_state_snapshot_t snapshot;
    app_core_lvgl_page_t current_page;
    esp_err_t result;

    if (domain == NULL ||
        domain->controller == NULL ||
        domain->state_store == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    result = app_core_state_store_read(
        domain->state_store,
        &snapshot);

    if (result != ESP_OK)
    {
        return result;
    }

    result = app_ui_controller_get_page(
        domain->controller,
        &current_page);

    if (result != ESP_OK)
    {
        return result;
    }

    snapshot.lvgl_page = current_page;
    snapshot.lvgl_transitioning =
        domain->controller->fsm.transitioning;
    snapshot.selected_menu_index =
        domain->controller->fsm.selected_menu_index;
    snapshot.active_request_id =
        domain->controller->fsm.active_request_id;
    snapshot.last_error =
        domain->controller->fsm.last_error;
        
    return app_core_state_store_publish(
        domain->state_store,
        &snapshot);
}

/**
 * @brief 处理 Dispatcher 分发过来的 UI Request。
 *
 * Request 处理完成后，将 UI FSM 状态同步到公共状态快照。
 */
static esp_err_t ui_domain_handle_request(
    void *ctx,
    const app_core_request_t *request)
{
    app_ui_domain_t *domain;
    esp_err_t result;
    esp_err_t sync_result;

    if (ctx == NULL || request == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    domain = (app_ui_domain_t *)ctx;

    result = app_ui_controller_handle_request(
        domain->controller,
        request);

    sync_result = ui_domain_sync_state(domain);

    if (result != ESP_OK)
    {
        return result;
    }

    return sync_result;
}

/**
 * @brief 处理 Dispatcher 分发过来的 UI Event。
 *
 * Event 处理完成后，将 UI FSM 状态同步到公共状态快照。
 */
static esp_err_t ui_domain_handle_event(
    void *ctx,
    const app_core_event_t *event)
{
    app_ui_domain_t *domain;
    esp_err_t result;

    if (ctx == NULL || event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    domain = (app_ui_domain_t *)ctx;

    result = app_ui_controller_handle_event(
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

    return ui_domain_sync_state(domain);
}

/**
 * @brief 执行一次 UI Domain 周期处理。
 *
 * 该函数由 Dispatcher 周期调用，
 * 用于驱动 UI Controller 执行待处理 Action。
 */
static esp_err_t ui_domain_process_once(
    void *ctx)
{
    app_ui_domain_t *domain;
    esp_err_t result;

    if (ctx == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    domain = (app_ui_domain_t *)ctx;

    result = app_ui_controller_process_once(
        domain->controller);

    if (result == ESP_ERR_TIMEOUT)
    {
        return ESP_OK;
    }

    return result;
}

/**
 * @brief 初始化 UI Domain。
 *
 * 初始化时绑定 Controller 和 State Store，
 * 并创建可注册到 Dispatcher 的通用 Domain Handler。
 */
esp_err_t app_ui_domain_init(
    app_ui_domain_t *domain,
    app_ui_controller_t *controller,
    app_core_state_store_t *state_store)
{
    if (domain == NULL ||
        controller == NULL ||
        state_store == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (domain->initialized ||
        !controller->initialized ||
        !state_store->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    *domain = (app_ui_domain_t){0};
    domain->controller = controller;
    domain->state_store = state_store;

    app_core_domain_handler_init(
        &domain->handler,
        domain,
        ui_domain_match_request,
        ui_domain_handle_request,
        ui_domain_match_event,
        ui_domain_handle_event,
        ui_domain_process_once);

    if (!app_core_domain_handler_is_valid(&domain->handler))
    {
        *domain = (app_ui_domain_t){0};
        return ESP_ERR_INVALID_STATE;
    }

    domain->initialized = true;
    return ESP_OK;
}

/**
 * @brief 释放 UI Domain 自身的绑定关系。
 */
void app_ui_domain_deinit(
    app_ui_domain_t *domain)
{
    if (domain == NULL)
    {
        return;
    }

    *domain = (app_ui_domain_t){0};
}

/**
 * @brief 获取 UI Domain 的 Dispatcher Handler。
 */
const app_core_domain_handler_t *
app_ui_domain_get_handler(
    const app_ui_domain_t *domain)
{
    if (domain == NULL || !domain->initialized)
    {
        return NULL;
    }

    return &domain->handler;
}
