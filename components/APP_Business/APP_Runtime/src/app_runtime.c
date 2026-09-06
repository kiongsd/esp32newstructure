#include "app_runtime.h"

#include "esp_log.h"

#include "app_core_event.h"
#include "app_core_types.h"

static const char *TAG = "APP_RUNTIME";

/**
 * @brief 创建一个 Runtime Event。
 */

static esp_err_t app_runtime_prepare_event(
    app_runtime_context_t *context,
    const app_core_message_meta_t *meta,
    app_core_event_type_t event_type,
    app_core_event_t *event)
{
    if (context == NULL || meta == NULL || event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!context->initialized || context->dispatcher == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    app_core_event_init(event, event_type, meta->request_id, meta->parent_request_id, APP_CORE_SOURCE_DRIVER, meta->scope);

    return ESP_OK;
}

/**
 * @brief 将 Event 发送给 App Core Dispatcher。
 */

static esp_err_t app_runtime_emit_event(app_runtime_context_t *context,
                                        const app_core_event_t *event)
{
    if (context == NULL || event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (context->dispatcher == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return app_core_dispatcher_emit_event(context->dispatcher, (const app_core_event_t *)event);
}

/**
 * @brief 发送不携带额外数据的 Event。
 */
static esp_err_t app_runtime_emit_simple_event(
    app_runtime_context_t *context,
    const app_core_message_meta_t *meta,
    app_core_event_type_t event_type)
{
    app_core_event_t event;
    esp_err_t result;

    result = app_runtime_prepare_event(
        context,
        meta,
        event_type,
        &event);

    if (result != ESP_OK)
    {
        return result;
    }

    return app_runtime_emit_event(
        context,
        &event);
}

/**
 * @brief 显示页面。
 */
static esp_err_t app_runtime_show_page(void *ctx, const app_core_message_meta_t *meta, app_core_lvgl_page_t page)
{
    app_runtime_context_t *context;
    app_core_event_t event;
    esp_err_t result;

    context = (app_runtime_context_t *)ctx;

    ESP_LOGI(TAG, "show page:%d", (int)page);
    result = app_runtime_prepare_event(context, meta, APP_CORE_EVENT_TYPE_PAGE_SHOWN, &event);
    if (result != ESP_OK)
    {
        return result;
    }
    event.data.page = page;
    return app_runtime_emit_event(context, &event);
}

/**
 * @brief 启动 Camera 预览。
 */
static esp_err_t app_runtime_camera_start_preview(void *ctx, const app_core_message_meta_t *meta)
{
    app_runtime_context_t *context = (app_runtime_context_t *)ctx;
    ESP_LOGI(TAG, "start camera preview");

    return app_runtime_emit_simple_event(context, meta, APP_CORE_EVENT_TYPE_CAMERA_PREVIEW_STARTED);
}

/**
 * @brief 准备 Camera 拍照。
 */
static esp_err_t app_runtime_camera_prepare_photo(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_job_id_t job_id)
{
    app_runtime_context_t *context =
        (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "camera prepare photo, job_id=%lu",
        (unsigned long)job_id);

    return app_runtime_emit_simple_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_PREPARED);
}

/**
 * @brief 停止 Camera。
 */
static esp_err_t app_runtime_camera_stop(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    app_runtime_context_t *context =
        (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "camera stop");

    return app_runtime_emit_simple_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_STOPPED);
}

/**
 * @brief 执行 Camera 拍照。
 */
static esp_err_t app_runtime_camera_capture(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_job_id_t job_id)
{
    app_runtime_context_t *context;
    app_core_event_t event;
    esp_err_t result;

    context = (app_runtime_context_t *)ctx;

    if (context == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (context->next_photo_handle ==
        APP_CORE_INVALID_PHOTO_HANDLE)
    {
        context->next_photo_handle = 1U;
    }

    ESP_LOGI(
        TAG,
        "camera capture, job_id=%lu, photo_handle=%lu",
        (unsigned long)job_id,
        (unsigned long)context->next_photo_handle);

    result = app_runtime_prepare_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_CAPTURED,
        &event);

    if (result != ESP_OK)
    {
        return result;
    }

    event.data.photo_handle =
        context->next_photo_handle;

    context->next_photo_handle++;

    return app_runtime_emit_event(
        context,
        &event);
}

/**
 * @brief Camera 自动对焦。
 */
static esp_err_t app_runtime_camera_focus(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    app_runtime_context_t *context =
        (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "camera focus");

    return app_runtime_emit_simple_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_FOCUSED);
}

/**
 * @brief 保存照片。
 */
static esp_err_t app_runtime_save_photo(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_photo_handle_t photo_handle)
{
    app_runtime_context_t *context;
    app_core_event_t event;
    esp_err_t result;

    context = (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "save photo, photo_handle=%lu",
        (unsigned long)photo_handle);

    result = app_runtime_prepare_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_PHOTO_SAVED,
        &event);

    if (result != ESP_OK)
    {
        return result;
    }

    event.data.photo_handle =
        photo_handle;

    return app_runtime_emit_event(
        context,
        &event);
}

/**
 * @brief 扫描图库。
 */
static esp_err_t app_runtime_scan_gallery(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    app_runtime_context_t *context;
    app_core_event_t event;
    esp_err_t result;

    context = (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "scan gallery, total=%u",
        (unsigned)context->gallery_total);

    result = app_runtime_prepare_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_GALLERY_SCANNED,
        &event);

    if (result != ESP_OK)
    {
        return result;
    }

    event.data.gallery.index = 0U;
    event.data.gallery.total =
        context->gallery_total;

    return app_runtime_emit_event(
        context,
        &event);
}

/**
 * @brief 显示图库照片。
 */
static esp_err_t app_runtime_show_gallery_photo(
    void *ctx,
    const app_core_message_meta_t *meta,
    uint16_t index,
    uint16_t total)
{
    app_runtime_context_t *context;
    app_core_event_t event;
    esp_err_t result;

    context = (app_runtime_context_t *)ctx;

    if (context == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (index >= total)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(
        TAG,
        "show gallery photo, index=%u, total=%u",
        (unsigned)index,
        (unsigned)total);

    result = app_runtime_prepare_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_GALLERY_SELECTION_UPDATED,
        &event);

    if (result != ESP_OK)
    {
        return result;
    }

    event.data.gallery.index = index;
    event.data.gallery.total = total;

    return app_runtime_emit_event(
        context,
        &event);
}

/**
 * @brief 启动 Web 服务。
 */
static esp_err_t app_runtime_web_start(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    app_runtime_context_t *context =
        (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "web start");

    return app_runtime_emit_simple_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_WEB_STARTED);
}

/**
 * @brief 停止 Web 服务。
 */
static esp_err_t app_runtime_web_stop(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    app_runtime_context_t *context =
        (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "web stop");

    return app_runtime_emit_simple_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_WEB_STOPPED);
}

/**
 * @brief 开始 OTA。
 */
static esp_err_t app_runtime_ota_begin(
    void *ctx,
    const app_core_message_meta_t *meta,
    const app_core_ota_manifest_t *manifest)
{
    app_runtime_context_t *context =
        (app_runtime_context_t *)ctx;

    (void)manifest;

    ESP_LOGI(
        TAG,
        "ota begin");

    return app_runtime_emit_simple_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_OTA_STARTED);
}

/**
 * @brief 完成 OTA。
 */
static esp_err_t app_runtime_ota_finish(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    app_runtime_context_t *context =
        (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "ota finish");

    return app_runtime_emit_simple_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_OTA_FINISHED);
}

/**
 * @brief 中止 OTA。
 */
static esp_err_t app_runtime_ota_abort(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    app_runtime_context_t *context =
        (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "ota abort");

    return app_runtime_emit_simple_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_OTA_ABORTED);
}

/**
 * @brief 更新菜单选择项。
 */
static esp_err_t app_runtime_update_menu_selection(
    void *ctx,
    const app_core_message_meta_t *meta,
    uint8_t index)
{
    app_runtime_context_t *context;
    app_core_event_t event;
    esp_err_t result;

    context = (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "update menu selection, index=%u",
        (unsigned)index);

    result = app_runtime_prepare_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_MENU_SELECTION_UPDATED,
        &event);

    if (result != ESP_OK)
    {
        return result;
    }

    event.data.menu_index = index;

    return app_runtime_emit_event(
        context,
        &event);
}

/**
 * @brief 系统初始化。
 */
static esp_err_t app_runtime_system_initialize(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    app_runtime_context_t *context =
        (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "system initialize");

    return app_runtime_emit_simple_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_SYSTEM_READY);
}

/**
 * @brief 系统挂起。
 */
static esp_err_t app_runtime_system_suspend(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    app_runtime_context_t *context =
        (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "system suspend");

    return app_runtime_emit_simple_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_SYSTEM_SUSPENDED);
}

/**
 * @brief 系统恢复。
 */
static esp_err_t app_runtime_system_resume(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    app_runtime_context_t *context =
        (app_runtime_context_t *)ctx;

    ESP_LOGI(
        TAG,
        "system resume");

    return app_runtime_emit_simple_event(
        context,
        meta,
        APP_CORE_EVENT_TYPE_SYSTEM_RESUMED);
}
/**
 * @brief 初始化 App Runtime。
 */

esp_err_t app_runtime_init(app_core_runtime_t *runtime, app_runtime_context_t *context)
{
    if (runtime == NULL || context == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (context->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    *runtime = (app_core_runtime_t){
        0};
    *context = (app_runtime_context_t){
        0};
    context->gallery_total = 3U;
    context->next_photo_handle = 1U;
    context->initialized = true;

    /*
     * 当前所有模块暂时共用同一个 Runtime 上下文。
     * 将来接入真实硬件时，可以拆分成不同的上下文对象。
     */
    runtime->display_ctx = context;
    runtime->camera_ctx = context;
    runtime->photo_storage_ctx = context;
    runtime->storage_ctx = context;
    runtime->system_ctx = context;
    runtime->web_ctx = context;
    runtime->ota_ctx = context;

    runtime->show_page =
        app_runtime_show_page;

    runtime->camera_start_preview = app_runtime_camera_start_preview;

    runtime->camera_prepare_photo =
        app_runtime_camera_prepare_photo;

    runtime->camera_stop =
        app_runtime_camera_stop;

    runtime->camera_capture =
        app_runtime_camera_capture;

    runtime->camera_focus =
        app_runtime_camera_focus;

    runtime->save_photo =
        app_runtime_save_photo;

    runtime->scan_gallery =
        app_runtime_scan_gallery;

    runtime->show_gallery_photo =
        app_runtime_show_gallery_photo;

    runtime->web_start =
        app_runtime_web_start;

    runtime->web_stop =
        app_runtime_web_stop;

    runtime->ota_begin =
        app_runtime_ota_begin;

    runtime->ota_finish =
        app_runtime_ota_finish;

    runtime->ota_abort =
        app_runtime_ota_abort;

    runtime->update_menu_selection =
        app_runtime_update_menu_selection;

    runtime->system_initialize =
        app_runtime_system_initialize;

    runtime->system_suspend =
        app_runtime_system_suspend;

    runtime->system_resume =
        app_runtime_system_resume;

    return ESP_OK;
}
