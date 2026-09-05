#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"

#include "app_core_dispatcher.h"
#include "app_core_event.h"
#include "app_core_request.h"
#include "app_core_runtime.h"
#include "app_core_state_store.h"

#include "app_storage_controller.h"
#include "app_storage_domain.h"

static const char *TAG = "APP_STORAGE_TEST";

typedef struct
{
    app_core_dispatcher_t *dispatcher;
} app_storage_test_context_t;

static esp_err_t app_storage_test_emit_event(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_event_type_t event_type,
    uint16_t index,
    uint16_t total)
{
    app_storage_test_context_t *test_context;
    app_core_event_t event;

    if (ctx == NULL || meta == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    test_context = (app_storage_test_context_t *)ctx;

    if (test_context->dispatcher == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    app_core_event_init(
        &event,
        event_type,
        meta->request_id,
        meta->parent_request_id,
        APP_CORE_SOURCE_DRIVER,
        APP_CORE_SCOPE_STORAGE);

    event.data.gallery.index = index;
    event.data.gallery.total = total;

    return app_core_dispatcher_emit_event(
        test_context->dispatcher,
        &event);
}

static esp_err_t app_storage_test_scan_gallery(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    return app_storage_test_emit_event(
        ctx,
        meta,
        APP_CORE_EVENT_TYPE_GALLERY_SCANNED,
        0U,
        3U);
}

static esp_err_t app_storage_test_show_gallery_photo(
    void *ctx,
    const app_core_message_meta_t *meta,
    uint16_t index,
    uint16_t total)
{
    return app_storage_test_emit_event(
        ctx,
        meta,
        APP_CORE_EVENT_TYPE_GALLERY_SELECTION_UPDATED,
        index,
        total);
}

static esp_err_t app_storage_test_process_request(
    app_core_dispatcher_t *dispatcher,
    app_core_state_store_t *state_store,
    app_core_request_type_t request_type,
    app_core_request_id_t request_id,
    uint16_t gallery_index,
    app_core_storage_state_t expected_state,
    uint8_t process_rounds)
{
    app_core_request_t request;
    app_core_state_snapshot_t snapshot;
    esp_err_t result;
    uint8_t index;

    if (dispatcher == NULL ||
        state_store == NULL ||
        process_rounds == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    app_core_request_init(
        &request,
        request_type,
        request_id,
        APP_CORE_INVALID_REQUEST_ID,
        APP_CORE_SOURCE_TEST,
        APP_CORE_SCOPE_STORAGE);

    if (request_type == APP_CORE_REQUEST_TYPE_SHOW_GALLERY_PHOTO)
    {
        request.data.gallery.index = gallery_index;
    }

    result = app_core_dispatcher_submit_request(
        dispatcher,
        &request);

    if (result != ESP_OK)
    {
        return result;
    }

    for (index = 0U; index < process_rounds; index++)
    {
        result = app_core_dispatcher_process_once(dispatcher);

        if (result != ESP_OK)
        {
            return result;
        }
    }

    result = app_core_state_store_read(
        state_store,
        &snapshot);

    if (result != ESP_OK)
    {
        return result;
    }

    ESP_LOGI(
        TAG,
        "request=%s, state=%s, total=%u, selected=%u",
        app_core_request_type_to_string(request_type),
        app_core_storage_state_to_string(snapshot.storage_state),
        (unsigned int)snapshot.gallery_count,
        (unsigned int)snapshot.selected_gallery_index);

    if (snapshot.storage_state != expected_state)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

void app_storage_test_run(void)
{
    app_core_state_store_t state_store = {0};
    app_core_dispatcher_t dispatcher = {0};
    app_core_runtime_t runtime = {0};
    app_storage_controller_t controller = {0};
    app_storage_domain_t domain = {0};
    app_storage_test_context_t test_context = {
        .dispatcher = &dispatcher,
    };
    app_core_state_snapshot_t snapshot;
    esp_err_t result = ESP_OK;
    bool success = false;

    result = app_core_state_store_init(&state_store);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "state store init failed: %s", esp_err_to_name(result));
        goto cleanup;
    }

    result = app_core_dispatcher_init(&dispatcher, 8U, 8U);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "dispatcher init failed: %s", esp_err_to_name(result));
        goto cleanup;
    }

    result = app_core_dispatcher_bind_state_store(
        &dispatcher,
        &state_store);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "bind state store failed: %s", esp_err_to_name(result));
        goto cleanup;
    }

    runtime.storage_ctx = &test_context;
    runtime.scan_gallery = app_storage_test_scan_gallery;
    runtime.show_gallery_photo = app_storage_test_show_gallery_photo;

    result = app_storage_controller_init(
        &controller,
        &runtime,
        4U,
        app_core_dispatcher_emit_event,
        &dispatcher);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "storage controller init failed: %s", esp_err_to_name(result));
        goto cleanup;
    }

    result = app_storage_domain_init(
        &domain,
        &controller,
        &state_store);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "storage domain init failed: %s", esp_err_to_name(result));
        goto cleanup;
    }

    result = app_core_dispatcher_register_domain(
        &dispatcher,
        app_storage_domain_get_handler(&domain));

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "storage domain register failed: %s", esp_err_to_name(result));
        goto cleanup;
    }

    result = app_storage_test_process_request(
        &dispatcher,
        &state_store,
        APP_CORE_REQUEST_TYPE_SCAN_GALLERY,
        1U,
        0U,
        APP_CORE_STORAGE_STATE_IDLE,
        2U);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "scan gallery failed: %s", esp_err_to_name(result));
        goto cleanup;
    }

    result = app_core_state_store_read(&state_store, &snapshot);
    if (result != ESP_OK || snapshot.gallery_count != 3U)
    {
        ESP_LOGE(TAG, "gallery count invalid");
        result = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }

    result = app_storage_test_process_request(
        &dispatcher,
        &state_store,
        APP_CORE_REQUEST_TYPE_SHOW_GALLERY_PHOTO,
        2U,
        1U,
        APP_CORE_STORAGE_STATE_IDLE,
        2U);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "show gallery photo failed: %s", esp_err_to_name(result));
        goto cleanup;
    }

    result = app_core_state_store_read(&state_store, &snapshot);
    if (result != ESP_OK || snapshot.selected_gallery_index != 1U)
    {
        ESP_LOGE(TAG, "selected gallery index invalid");
        result = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }

    success = true;

cleanup:
    app_core_dispatcher_deinit(&dispatcher);
    app_storage_domain_deinit(&domain);
    app_storage_controller_deinit(&controller);
    app_core_state_store_deinit(&state_store);

    if (success)
    {
        ESP_LOGI(TAG, "storage test success");
    }
    else
    {
        ESP_LOGE(TAG, "storage test failed: %s", esp_err_to_name(result));
    }
}
