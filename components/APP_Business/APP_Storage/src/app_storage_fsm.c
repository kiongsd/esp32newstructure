#include "app_storage_fsm.h"

#include <stddef.h>

static bool app_storage_state_is_busy(
    app_core_storage_state_t state)
{
    return state == APP_CORE_STORAGE_STATE_SCANNING ||
           state == APP_CORE_STORAGE_STATE_SHOWING;
}

static app_core_action_id_t app_storage_allocate_action_id(
    app_storage_fsm_t *fsm)
{
    app_core_action_id_t action_id = fsm->next_action_id;

    fsm->next_action_id++;

    if (fsm->next_action_id == APP_CORE_INVALID_ACTION_ID)
    {
        fsm->next_action_id = 1;
    }

    return action_id;
}

static void app_storage_clear_active(
    app_storage_fsm_t *fsm)
{
    fsm->active_meta = (app_core_message_meta_t){0};
    fsm->active_request_id = APP_CORE_INVALID_REQUEST_ID;
}

static esp_err_t app_storage_emit_action(
    app_storage_fsm_t *fsm,
    app_core_effect_type_t effect_type,
    const app_core_effect_data_t *effect_data,
    app_storage_fsm_emit_action_fn emit_action,
    void *emit_ctx)
{
    if (fsm == NULL || emit_action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    app_core_action_t action = {0};

    action.action_id = app_storage_allocate_action_id(fsm);
    action.state = APP_CORE_ACTION_STATE_CREATED;

    action.effect.meta = fsm->active_meta;
    action.effect.meta.source = APP_CORE_SOURCE_ACTION;
    action.effect.meta.scope = APP_CORE_SCOPE_STORAGE;
    action.effect.type = effect_type;

    if (effect_data != NULL)
    {
        action.effect.data = *effect_data;
    }

    return emit_action(emit_ctx, &action);
}

void app_storage_fsm_init(
    app_storage_fsm_t *fsm)
{
    if (fsm == NULL)
    {
        return;
    }

    *fsm = (app_storage_fsm_t){0};

    fsm->state = APP_CORE_STORAGE_STATE_IDLE;
    fsm->active_request_id = APP_CORE_INVALID_REQUEST_ID;
    fsm->next_action_id = 1;
    fsm->last_error = ESP_OK;
}

esp_err_t app_storage_fsm_get_state(
    const app_storage_fsm_t *fsm,
    app_core_storage_state_t *state)
{
    if (fsm == NULL || state == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *state = fsm->state;
    return ESP_OK;
}

esp_err_t app_storage_fsm_handle_request(
    app_storage_fsm_t *fsm,
    const app_core_request_t *request,
    app_storage_fsm_emit_action_fn emit_action,
    void *emit_ctx)
{
    if (fsm == NULL || request == NULL || emit_action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (app_storage_state_is_busy(fsm->state))
    {
        return ESP_ERR_INVALID_STATE;
    }

    fsm->active_meta = request->meta;
    fsm->active_request_id = request->meta.request_id;
    fsm->last_error = ESP_OK;

    app_core_effect_data_t effect_data = {0};
    app_core_effect_type_t effect_type;

    switch (request->type)
    {
    case APP_CORE_REQUEST_TYPE_SCAN_GALLERY:
        fsm->state = APP_CORE_STORAGE_STATE_SCANNING;
        effect_type = APP_CORE_EFFECT_TYPE_SCAN_GALLERY;
        break;

    case APP_CORE_REQUEST_TYPE_SHOW_GALLERY_PHOTO:
        if (fsm->gallery_count == 0 ||
            request->data.gallery.index >= fsm->gallery_count)
        {
            app_storage_clear_active(fsm);
            return ESP_ERR_INVALID_ARG;
        }

        fsm->selected_gallery_index = request->data.gallery.index;
        fsm->state = APP_CORE_STORAGE_STATE_SHOWING;

        effect_type = APP_CORE_EFFECT_TYPE_SHOW_GALLERY_PHOTO;
        effect_data.gallery.index =
            request->data.gallery.index;
        effect_data.gallery.total =
            fsm->gallery_count;
        break;

    default:
        app_storage_clear_active(fsm);
        return ESP_ERR_NOT_SUPPORTED;
    }

    esp_err_t result = app_storage_emit_action(
        fsm,
        effect_type,
        &effect_data,
        emit_action,
        emit_ctx);

    if (result != ESP_OK)
    {
        fsm->state = APP_CORE_STORAGE_STATE_FAILED;
        fsm->last_error = result;
        app_storage_clear_active(fsm);
    }

    return result;
}

esp_err_t app_storage_fsm_handle_event(
    app_storage_fsm_t *fsm,
    const app_core_event_t *event,
    app_storage_fsm_emit_action_fn emit_action,
    void *emit_ctx)
{
    (void)emit_action;
    (void)emit_ctx;

    if (fsm == NULL || event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (fsm->active_request_id != APP_CORE_INVALID_REQUEST_ID &&
        event->meta.request_id != APP_CORE_INVALID_REQUEST_ID &&
        event->meta.request_id != fsm->active_request_id)
    {
        return ESP_ERR_NOT_FOUND;
    }

    switch (event->type)
    {
    case APP_CORE_EVENT_TYPE_GALLERY_SCANNED:
        if (fsm->state != APP_CORE_STORAGE_STATE_SCANNING)
        {
            return ESP_ERR_INVALID_STATE;
        }

        fsm->gallery_count = event->data.gallery.total;
        fsm->selected_gallery_index = 0;
        fsm->state = APP_CORE_STORAGE_STATE_IDLE;
        fsm->last_error = ESP_OK;
        app_storage_clear_active(fsm);
        return ESP_OK;

    case APP_CORE_EVENT_TYPE_GALLERY_SELECTION_UPDATED:
        if (fsm->state != APP_CORE_STORAGE_STATE_SHOWING)
        {
            return ESP_ERR_INVALID_STATE;
        }

        fsm->selected_gallery_index =
            event->data.gallery.index;

        if (event->data.gallery.total > 0)
        {
            fsm->gallery_count =
                event->data.gallery.total;
        }

        fsm->state = APP_CORE_STORAGE_STATE_IDLE;
        fsm->last_error = ESP_OK;
        app_storage_clear_active(fsm);
        return ESP_OK;

    case APP_CORE_EVENT_TYPE_FAILED:
        if (!app_storage_state_is_busy(fsm->state))
        {
            return ESP_ERR_INVALID_STATE;
        }

        fsm->state = APP_CORE_STORAGE_STATE_FAILED;
        fsm->last_error =
            event->error == ESP_OK ? ESP_FAIL : event->error;

        app_storage_clear_active(fsm);
        return ESP_OK;

    default:
        return ESP_ERR_NOT_SUPPORTED;
    }
}