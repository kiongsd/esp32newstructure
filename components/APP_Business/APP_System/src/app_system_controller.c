#include "app_system_controller.h"

esp_err_t app_system_controller_init(app_system_controller_t *controller, const app_core_runtime_t *runtime, UBaseType_t action_queue_length, app_core_action_engine_emit_event_fn emit_event, void *event_ctx)
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

    *controller = (app_system_controller_t){0};

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

    app_system_fsm_init(
        &controller->fsm);

    controller->initialized =
        true;

    return ESP_OK;
}

void app_system_controller_deinit(app_system_controller_t *controller)
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
        (app_system_controller_t){0};
}

esp_err_t app_system_controller_handle_request(app_system_controller_t *controller, const app_core_request_t *request)
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

    return app_system_fsm_handle_request(
        &controller->fsm,
        request,
        app_core_action_engine_emit_action,
        &controller->action_engine);
}

esp_err_t app_system_controller_handle_event(app_system_controller_t *controller, const app_core_event_t *event)
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

    return app_system_fsm_handle_event(
        &controller->fsm,
        event);
}

esp_err_t app_system_controller_process_once(app_system_controller_t *controller)
{
    if (controller == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!controller->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return app_core_action_engine_process_once(&controller->action_engine);
}

esp_err_t app_system_controller_get_state(const app_system_controller_t *controller, app_core_system_state_t *state)
{
    if (controller == NULL ||
        state == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!controller->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return app_system_fsm_get_state(
        &controller->fsm,
        state);
}