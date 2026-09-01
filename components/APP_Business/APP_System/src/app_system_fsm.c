#include "app_system_fsm.h"

static bool system_state_is_busy(app_core_system_state_t state)
{
    switch (state)
    {
    case APP_CORE_SYSTEM_STATE_BOOTING:
    case APP_CORE_SYSTEM_STATE_INITIALIZING:
    case APP_CORE_SYSTEM_STATE_SUSPENDING:
    case APP_CORE_SYSTEM_STATE_RESUMING:
        return true;
    default:
        return false;
    }
}

static app_core_action_id_t system_allocate_action_id(
    app_system_fsm_t *fsm)
{
    app_core_action_id_t action_id;
    if (fsm->next_action_id == APP_CORE_INVALID_ACTION_ID)
    {
        fsm->next_action_id = 1U;
    }
    action_id = fsm->next_action_id;
    fsm->next_action_id++;
    return action_id;
}

static void system_clear_active_request(app_system_fsm_t *fsm)
{
    fsm->active_meta = (app_core_message_meta_t){0};
    fsm->active_request_id = APP_CORE_INVALID_REQUEST_ID;
}

static void system_set_fault(app_system_fsm_t *fsm, esp_err_t error)
{
    if (error == ESP_OK)
    {
        error = ESP_FAIL;
    }

    fsm->state = APP_CORE_SYSTEM_STATE_FAULT;
    fsm->last_error = error;
}

static esp_err_t system_emit_action(app_system_fsm_t *fsm, const app_core_message_meta_t *source_meta, app_core_effect_type_t effect_type, app_system_fsm_emit_action_fn emit_action, void *emit_ctx)
{
    app_core_effect_t effect;
    app_core_action_t action;

    if (fsm == NULL || source_meta == NULL || emit_action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    app_core_effect_init(&effect, effect_type, source_meta->request_id, source_meta->parent_request_id, APP_CORE_SOURCE_ACTION, APP_CORE_SCOPE_SYSTEM);
    app_core_action_init(&action, system_allocate_action_id(fsm), &effect);

    return emit_action(emit_ctx, &action);
}

static esp_err_t system_begin_action(app_system_fsm_t *fsm,    const app_core_message_meta_t *meta,
    app_core_system_state_t pending_state,
    app_core_effect_type_t effect_type,
    app_system_fsm_emit_action_fn emit_action,
    void *emit_ctx){
    esp_err_t result;
    if (fsm == NULL || meta == NULL || emit_action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (system_state_is_busy(fsm->state))
    {
        return ESP_ERR_INVALID_STATE;
    }
    fsm->active_meta = *meta;

    fsm->active_meta.scope = APP_CORE_SCOPE_SYSTEM;

    fsm->active_request_id = meta->request_id;
    fsm->last_error = ESP_OK;
    fsm->state = pending_state;
    result = system_emit_action(fsm, &fsm->active_meta, effect_type, emit_action, emit_ctx);

    if (result != ESP_OK)
    {
        system_set_fault(fsm, result);
        system_clear_active_request(fsm);
        return result;
    }
    return ESP_OK;
}

void app_system_fsm_init(app_system_fsm_t *fsm)
{
    if (fsm == NULL)
    {
        return;
    }
    *fsm = (app_system_fsm_t){0};
    fsm->state = APP_CORE_SYSTEM_STATE_UNKNOWN;
    fsm->active_meta = (app_core_message_meta_t){0};
    fsm->active_request_id = APP_CORE_INVALID_REQUEST_ID;
    fsm->next_action_id = 1U;
    fsm->last_error = ESP_OK;
}

esp_err_t app_system_fsm_get_state(const app_system_fsm_t *fsm, app_core_system_state_t *state)
{
    if (fsm == NULL || state == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *state = fsm->state;
    return ESP_OK;
}

esp_err_t app_system_fsm_handle_request(app_system_fsm_t *fsm, const app_core_request_t *request, app_system_fsm_emit_action_fn emit_action, void *emit_ctx)
{
    app_core_message_meta_t request_meta;

    if (fsm == NULL || request == NULL || emit_action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    request_meta = request->meta;
    request_meta.scope = APP_CORE_SCOPE_SYSTEM;

    switch (request->type)
    {
    case APP_CORE_REQUEST_TYPE_SYSTEM_INITIALIZE:
        if (fsm->state ==
                APP_CORE_SYSTEM_STATE_READY ||
            fsm->state ==
                APP_CORE_SYSTEM_STATE_SUSPENDED ||
            system_state_is_busy(fsm->state))
        {
            return ESP_ERR_INVALID_STATE;
        }

        return system_begin_action(
            fsm,
            &request_meta,
            APP_CORE_SYSTEM_STATE_INITIALIZING,
            APP_CORE_EFFECT_TYPE_SYSTEM_INITIALIZE,
            emit_action,
            emit_ctx);

    case APP_CORE_REQUEST_TYPE_SYSTEM_SUSPEND:
        if (fsm->state !=
                APP_CORE_SYSTEM_STATE_READY &&
            fsm->state !=
                APP_CORE_SYSTEM_STATE_DEGRADED)
        {
            return ESP_ERR_INVALID_STATE;
        }

        return system_begin_action(
            fsm,
            &request_meta,
            APP_CORE_SYSTEM_STATE_SUSPENDING,
            APP_CORE_EFFECT_TYPE_SYSTEM_SUSPEND,
            emit_action,
            emit_ctx);

    case APP_CORE_REQUEST_TYPE_SYSTEM_RESUME:
        if (fsm->state !=
            APP_CORE_SYSTEM_STATE_SUSPENDED)
        {
            return ESP_ERR_INVALID_STATE;
        }

        return system_begin_action(
            fsm,
            &request_meta,
            APP_CORE_SYSTEM_STATE_RESUMING,
            APP_CORE_EFFECT_TYPE_SYSTEM_RESUME,
            emit_action,
            emit_ctx);

    default:
        return ESP_ERR_NOT_SUPPORTED;
    }
}

esp_err_t app_system_fsm_handle_event(app_system_fsm_t *fsm,const app_core_event_t *event)
{
    if (fsm == NULL || event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (fsm->active_request_id !=
            APP_CORE_INVALID_REQUEST_ID &&
        event->meta.request_id !=
            APP_CORE_INVALID_REQUEST_ID &&
        event->meta.request_id !=
            fsm->active_request_id)
    {
        return ESP_ERR_INVALID_STATE;
    }

    switch (event->type)
    {
    case APP_CORE_EVENT_TYPE_SYSTEM_READY:
        if (fsm->state !=
                APP_CORE_SYSTEM_STATE_INITIALIZING &&
            fsm->state !=
                APP_CORE_SYSTEM_STATE_BOOTING &&
            fsm->state !=
                APP_CORE_SYSTEM_STATE_DEGRADED)
        {
            return ESP_ERR_INVALID_STATE;
        }

        fsm->state =
            APP_CORE_SYSTEM_STATE_READY;

        fsm->last_error =
            ESP_OK;

        system_clear_active_request(
            fsm);

        return ESP_OK;

    case APP_CORE_EVENT_TYPE_SYSTEM_SUSPENDED:
        if (fsm->state !=
            APP_CORE_SYSTEM_STATE_SUSPENDING)
        {
            return ESP_ERR_INVALID_STATE;
        }

        fsm->state =
            APP_CORE_SYSTEM_STATE_SUSPENDED;

        fsm->last_error =
            ESP_OK;

        system_clear_active_request(
            fsm);

        return ESP_OK;

    case APP_CORE_EVENT_TYPE_SYSTEM_RESUMED:
        if (fsm->state !=
            APP_CORE_SYSTEM_STATE_RESUMING)
        {
            return ESP_ERR_INVALID_STATE;
        }

        fsm->state =
            APP_CORE_SYSTEM_STATE_READY;

        fsm->last_error =
            ESP_OK;

        system_clear_active_request(
            fsm);

        return ESP_OK;

    case APP_CORE_EVENT_TYPE_FAILED:
        system_set_fault(
            fsm,
            event->error);

        system_clear_active_request(
            fsm);

        return ESP_OK;

    default:
        return ESP_ERR_NOT_SUPPORTED;
    }
}