#include "app_core_runtime.h"

#include "esp_err.h"

/**
 * @brief 返回“当前 Runtime 不支持该动作”。
 */
static esp_err_t app_core_runtime_not_supported(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

/**
 * @brief 根据 Effect 类型执行对应 Runtime 回调。
 */
esp_err_t app_core_runtime_execute(
    const app_core_runtime_t *runtime,
    const app_core_effect_t *effect)
{
    if (runtime == NULL ||
        effect == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    switch (effect->type)
    {
    case APP_CORE_EFFECT_TYPE_NONE:
        return ESP_OK;

    case APP_CORE_EFFECT_TYPE_SHOW_PAGE:
        if (runtime->show_page == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->show_page(
            runtime->display_ctx,
            &effect->meta,
            effect->data.page);

    case APP_CORE_EFFECT_TYPE_CAMERA_START_PREVIEW:
        if (runtime->camera_start_preview == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->camera_start_preview(
            runtime->camera_ctx,
            &effect->meta);

    case APP_CORE_EFFECT_TYPE_CAMERA_PREPARE_PHOTO:
        if (runtime->camera_prepare_photo == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->camera_prepare_photo(
            runtime->camera_ctx,
            &effect->meta,
            effect->data.job_id);

    case APP_CORE_EFFECT_TYPE_CAMERA_STOP:
        if (runtime->camera_stop == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->camera_stop(
            runtime->camera_ctx,
            &effect->meta);

    case APP_CORE_EFFECT_TYPE_CAMERA_CAPTURE:
        if (runtime->camera_capture == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->camera_capture(
            runtime->camera_ctx,
            &effect->meta,
            effect->data.job_id);

    case APP_CORE_EFFECT_TYPE_CAMERA_FOCUS:
        if (runtime->camera_focus == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->camera_focus(
            runtime->camera_ctx,
            &effect->meta);

    case APP_CORE_EFFECT_TYPE_SAVE_PHOTO:
        if (runtime->save_photo == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->save_photo(
            runtime->photo_storage_ctx,
            &effect->meta,
            effect->data.photo_handle);

    case APP_CORE_EFFECT_TYPE_SCAN_GALLERY:
        if (runtime->scan_gallery == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->scan_gallery(
            runtime->storage_ctx,
            &effect->meta);

    case APP_CORE_EFFECT_TYPE_SHOW_GALLERY_PHOTO:
        if (runtime->show_gallery_photo == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->show_gallery_photo(
            runtime->display_ctx,
            &effect->meta,
            effect->data.gallery.index,
            effect->data.gallery.total);

    case APP_CORE_EFFECT_TYPE_WEB_START:
        if (runtime->web_start == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->web_start(
            runtime->web_ctx,
            &effect->meta);

    case APP_CORE_EFFECT_TYPE_WEB_STOP:
        if (runtime->web_stop == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->web_stop(
            runtime->web_ctx,
            &effect->meta);

    case APP_CORE_EFFECT_TYPE_OTA_BEGIN:
        if (runtime->ota_begin == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->ota_begin(
            runtime->ota_ctx,
            &effect->meta,
            &effect->data.ota_manifest);

    case APP_CORE_EFFECT_TYPE_OTA_FINISH:
        if (runtime->ota_finish == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->ota_finish(
            runtime->ota_ctx,
            &effect->meta);

    case APP_CORE_EFFECT_TYPE_OTA_ABORT:
        if (runtime->ota_abort == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->ota_abort(
            runtime->ota_ctx,
            &effect->meta);

    case APP_CORE_EFFECT_TYPE_UPDATE_MENU_SELECTION:
        if (runtime->update_menu_selection == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->update_menu_selection(
            runtime->display_ctx,
            &effect->meta,
            effect->data.menu_index);

    case APP_CORE_EFFECT_TYPE_SYSTEM_INITIALIZE:
        if (runtime->system_initialize == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->system_initialize(
            runtime->system_ctx,
            &effect->meta);

    case APP_CORE_EFFECT_TYPE_SYSTEM_SUSPEND:
        if (runtime->system_suspend == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->system_suspend(
            runtime->system_ctx,
            &effect->meta);

    case APP_CORE_EFFECT_TYPE_SYSTEM_RESUME:
        if (runtime->system_resume == NULL)
        {
            return app_core_runtime_not_supported();
        }

        return runtime->system_resume(
            runtime->system_ctx,
            &effect->meta);

    default:
        return ESP_ERR_NOT_SUPPORTED;
    }
}