#include "app_ui_controller.h"

/**
 * @brief 初始化 UI Controller。
 *
 * Controller 负责创建 UI Action 队列，
 * 初始化 Action Engine 和 UI FSM，
 * 并绑定 Action Engine 的 Event 输出接口。
 */
esp_err_t app_ui_controller_init(app_ui_controller_t *controller, const app_core_runtime_t *runtime, UBaseType_t action_queue_length, app_core_action_engine_emit_event_fn emit_event, void *event_ctx)
{
    esp_err_t result;

    if (controller == NULL ||
        runtime == NULL ||
        action_queue_length == 0U ||
        emit_event == NULL ||
        event_ctx == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (controller->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    *controller = (app_ui_controller_t){0};

    controller->action_queue = xQueueCreate(action_queue_length, sizeof(app_core_action_t));

    if (controller->action_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    result = app_core_action_engine_init(&controller->action_engine, runtime, controller->action_queue);

    if (result != ESP_OK)
    {
        vQueueDelete(controller->action_queue);
        controller->action_queue = NULL;
        return result;
    }

    result = app_core_action_engine_bind_event_sink(&controller->action_engine, emit_event, event_ctx);

    if (result != ESP_OK)
    {
        vQueueDelete(
            controller->action_queue);

        controller->action_queue =
            NULL;

        return result;
    }

    app_ui_fsm_init(
        &controller->fsm);

    controller->initialized =
        true;

    return ESP_OK;
}

/**
 * @brief 释放 UI Controller 及其 Action 队列。
 */
void app_ui_controller_deinit(app_ui_controller_t *controller)
{
    if (controller == NULL)
    {
        return;
    }

    if (controller->action_queue != NULL)
    {
        vQueueDelete(
            controller->action_queue);
    }

    *controller =
        (app_ui_controller_t){0};
}

/**
 * @brief 将 Request 交给 UI FSM 处理。
 */
esp_err_t app_ui_controller_handle_request(app_ui_controller_t *controller, const app_core_request_t *request)
{
    if (controller == NULL ||
        request == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!controller->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return app_ui_fsm_handle_request(
        &controller->fsm,
        request,
        app_core_action_engine_emit_action,
        &controller->action_engine);
}

/**
 * @brief 处理 UI Event。
 *
 * 先由 Action Engine 根据 Request ID 和 Event 类型完成 Action，
 * 再由 FSM 更新 UI 页面状态。
 */
esp_err_t app_ui_controller_handle_event(app_ui_controller_t *controller, const app_core_event_t *event)
{
    esp_err_t result;
    if (controller == NULL ||
        event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!controller->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    result = app_core_action_engine_handle_event(&controller->action_engine, event);
    if (result != ESP_OK &&
        result != ESP_ERR_NOT_FOUND)
    {
        return result;
    }

    return app_ui_fsm_handle_event(
        &controller->fsm,
        event,
        app_core_action_engine_emit_action,
        &controller->action_engine);
}

/**
 * @brief 执行一次 UI Action。
 */
esp_err_t app_ui_controller_process_once(app_ui_controller_t *controller)
{
    if (controller == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!controller->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return app_core_action_engine_process_once(&controller->action_engine);
}

/**
 * @brief 获取 UI FSM 当前页面。
 */
esp_err_t app_ui_controller_get_page(const app_ui_controller_t *controller, app_core_lvgl_page_t *page)
{
    if (controller == NULL ||
        page == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!controller->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return app_ui_fsm_get_page(
        &controller->fsm,
        page);
}
