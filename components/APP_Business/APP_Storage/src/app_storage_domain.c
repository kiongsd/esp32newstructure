#include "app_storage_domain.h"

#include <stddef.h>

static bool storage_domain_match_request(
    void *ctx,
    const app_core_request_t *request)
{
    (void)ctx;

    if (request == NULL ||
        request->meta.scope != APP_CORE_SCOPE_STORAGE)
    {
        return false;
    }

    switch (request->type)
    {
    case APP_CORE_REQUEST_TYPE_SCAN_GALLERY:
    case APP_CORE_REQUEST_TYPE_SHOW_GALLERY_PHOTO:
        return true;

    default:
        return false;
    }
}

static bool storage_domain_match_event(
    void *ctx,
    const app_core_event_t *event)
{
    (void)ctx;

    if (event == NULL ||
        event->meta.scope != APP_CORE_SCOPE_STORAGE)
    {
        return false;
    }

    switch (event->type)
    {
    case APP_CORE_EVENT_TYPE_GALLERY_SCANNED:
    case APP_CORE_EVENT_TYPE_GALLERY_SELECTION_UPDATED:
    case APP_CORE_EVENT_TYPE_FAILED:
        return true;

    default:
        return false;
    }
}

static esp_err_t storage_domain_sync_state(
    app_storage_domain_t *domain)
{
    app_core_state_snapshot_t snapshot;
    app_core_storage_state_t storage_state;
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

    result = app_storage_controller_get_state(
        domain->controller,
        &storage_state);

    if (result != ESP_OK)
    {
        return result;
    }

    snapshot.storage_state = storage_state;
    snapshot.selected_gallery_index =
        domain->controller->fsm.selected_gallery_index;
    snapshot.gallery_count =
        domain->controller->fsm.gallery_count;
    snapshot.active_request_id =
        domain->controller->fsm.active_request_id;
    snapshot.last_error =
        domain->controller->fsm.last_error;

    return app_core_state_store_publish(
        domain->state_store,
        &snapshot);
}

static esp_err_t storage_domain_handle_request(
    void *ctx,
    const app_core_request_t *request)
{
    app_storage_domain_t *domain;
    esp_err_t result;
    esp_err_t sync_result;

    if (ctx == NULL || request == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    domain = (app_storage_domain_t *)ctx;

    result = app_storage_controller_handle_request(
        domain->controller,
        request);

    sync_result = storage_domain_sync_state(domain);

    if (result != ESP_OK)
    {
        return result;
    }

    return sync_result;
}

static esp_err_t storage_domain_handle_event(
    void *ctx,
    const app_core_event_t *event)
{
    app_storage_domain_t *domain;
    esp_err_t result;

    if (ctx == NULL || event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    domain = (app_storage_domain_t *)ctx;

    result = app_storage_controller_handle_event(
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

    return storage_domain_sync_state(domain);
}

static esp_err_t storage_domain_process_once(
    void *ctx)
{
    app_storage_domain_t *domain;
    esp_err_t result;

    if (ctx == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    domain = (app_storage_domain_t *)ctx;

    result = app_storage_controller_process_once(
        domain->controller);

    if (result == ESP_ERR_TIMEOUT)
    {
        return ESP_OK;
    }

    return result;
}

esp_err_t app_storage_domain_init(
    app_storage_domain_t *domain,
    app_storage_controller_t *controller,
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

    *domain = (app_storage_domain_t){0};
    domain->controller = controller;
    domain->state_store = state_store;

    app_core_domain_handler_init(
        &domain->handler,
        domain,
        storage_domain_match_request,
        storage_domain_handle_request,
        storage_domain_match_event,
        storage_domain_handle_event,
        storage_domain_process_once);

    if (!app_core_domain_handler_is_valid(&domain->handler))
    {
        *domain = (app_storage_domain_t){0};
        return ESP_ERR_INVALID_STATE;
    }

    domain->initialized = true;
    return ESP_OK;
}

void app_storage_domain_deinit(
    app_storage_domain_t *domain)
{
    if (domain == NULL)
    {
        return;
    }

    *domain = (app_storage_domain_t){0};
}

const app_core_domain_handler_t *
app_storage_domain_get_handler(
    const app_storage_domain_t *domain)
{
    if (domain == NULL || !domain->initialized)
    {
        return NULL;
    }

    return &domain->handler;
}
