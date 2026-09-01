#include <stddef.h>

#include "app_core_state_store.h"

/**
 * @brief 比较两个状态快照的业务内容。
 *
 * revision 不参与比较，
 * 因为 revision 是由 State Store 自动维护的。
 */
static bool state_snapshot_equal(
    const app_core_state_snapshot_t *left,
    const app_core_state_snapshot_t *right)
{
    if (left == NULL || right == NULL)
    {
        return false;
    }

    return left->system_state == right->system_state &&
           left->capture_state == right->capture_state &&
           left->web_state == right->web_state &&
           left->storage_state == right->storage_state &&
           left->ota_state == right->ota_state &&
           left->lvgl_page == right->lvgl_page &&
           left->lvgl_transitioning == right->lvgl_transitioning &&
           left->preview_active == right->preview_active &&
           left->web_capture_active == right->web_capture_active &&
           left->ota_active == right->ota_active &&
           left->selected_gallery_index == right->selected_gallery_index &&
           left->gallery_count == right->gallery_count &&
           left->selected_menu_index == right->selected_menu_index &&
           left->active_request_id == right->active_request_id &&
           left->active_job_id == right->active_job_id &&
           left->last_photo_handle == right->last_photo_handle &&
           left->last_error == right->last_error;
}

/**
 * @brief 初始化 State Store。
 */
esp_err_t app_core_state_store_init(
    app_core_state_store_t *store)
{
    if (store == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (store->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    store->mutex = xSemaphoreCreateMutex();

    if (store->mutex == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    app_core_state_snapshot_init(&store->snapshot);

    store->initialized = true;

    return ESP_OK;
}

/**
 * @brief 释放 State Store 资源。
 */
void app_core_state_store_deinit(
    app_core_state_store_t *store)
{
    if (store == NULL)
    {
        return;
    }

    if (store->mutex != NULL)
    {
        vSemaphoreDelete(store->mutex);
        store->mutex = NULL;
    }

    store->snapshot =
        (app_core_state_snapshot_t){0};

    store->initialized = false;
}

/**
 * @brief 读取当前状态快照。
 */
esp_err_t app_core_state_store_read(
    const app_core_state_store_t *store,
    app_core_state_snapshot_t *snapshot)
{
    if (store == NULL || snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!store->initialized || store->mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(
            store->mutex,
            portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    *snapshot = store->snapshot;

    xSemaphoreGive(store->mutex);

    return ESP_OK;
}

/**
 * @brief 发布新的状态快照。
 */
esp_err_t app_core_state_store_publish(
    app_core_state_store_t *store,
    const app_core_state_snapshot_t *snapshot)
{
    app_core_state_snapshot_t next_snapshot;

    if (store == NULL || snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!app_core_state_snapshot_is_valid(snapshot))
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!store->initialized || store->mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(
            store->mutex,
            portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    if (state_snapshot_equal(
            &store->snapshot,
            snapshot))
    {
        xSemaphoreGive(store->mutex);
        return ESP_OK;
    }

    next_snapshot = *snapshot;

    next_snapshot.revision =
        store->snapshot.revision + 1U;

    store->snapshot = next_snapshot;

    xSemaphoreGive(store->mutex);

    return ESP_OK;
}