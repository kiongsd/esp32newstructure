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

    if (!app_core_effect_type_is_valid(
            effect->type))
    {
        return ESP_ERR_INVALID_ARG;
    }

    switch (effect->type.target)
    {
    case APP_CORE_EFFECT_TARGET_SYSTEM:
        switch (effect->type.code.system)
        {
        case APP_CORE_SYSTEM_EFFECT_INITIALIZE:
            if (runtime->system_initialize == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->system_initialize(
                runtime->system_ctx,
                &effect->meta);

        case APP_CORE_SYSTEM_EFFECT_SUSPEND:
            if (runtime->system_suspend == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->system_suspend(
                runtime->system_ctx,
                &effect->meta);

        case APP_CORE_SYSTEM_EFFECT_RESUME:
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

    case APP_CORE_EFFECT_TARGET_CAMERA:
        switch (effect->type.code.camera)
        {
        case APP_CORE_CAMERA_EFFECT_START_PREVIEW:
            if (runtime->camera_start_preview == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->camera_start_preview(
                runtime->camera_ctx,
                &effect->meta);

        case APP_CORE_CAMERA_EFFECT_PREPARE_PHOTO:
            if (runtime->camera_prepare_photo == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->camera_prepare_photo(
                runtime->camera_ctx,
                &effect->meta,
                effect->data.job_id);

        case APP_CORE_CAMERA_EFFECT_STOP:
            if (runtime->camera_stop == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->camera_stop(
                runtime->camera_ctx,
                &effect->meta);

        case APP_CORE_CAMERA_EFFECT_CAPTURE:
            if (runtime->camera_capture == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->camera_capture(
                runtime->camera_ctx,
                &effect->meta,
                effect->data.job_id);

        case APP_CORE_CAMERA_EFFECT_FOCUS:
            if (runtime->camera_focus == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->camera_focus(
                runtime->camera_ctx,
                &effect->meta);

        default:
            return ESP_ERR_NOT_SUPPORTED;
        }

    case APP_CORE_EFFECT_TARGET_STORAGE:
        switch (effect->type.code.storage)
        {
        case APP_CORE_STORAGE_EFFECT_SAVE_PHOTO:
            if (runtime->save_photo == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->save_photo(
                runtime->photo_storage_ctx,
                &effect->meta,
                effect->data.photo_handle);

        case APP_CORE_STORAGE_EFFECT_SCAN_GALLERY:
            if (runtime->scan_gallery == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->scan_gallery(
                runtime->storage_ctx,
                &effect->meta);

        case APP_CORE_STORAGE_EFFECT_SHOW_GALLERY_PHOTO:
            if (runtime->show_gallery_photo == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->show_gallery_photo(
                runtime->display_ctx,
                &effect->meta,
                effect->data.gallery.index,
                effect->data.gallery.total);

        default:
            return ESP_ERR_NOT_SUPPORTED;
        }

    case APP_CORE_EFFECT_TARGET_UI:
        switch (effect->type.code.ui)
        {
        case APP_CORE_UI_EFFECT_SHOW_PAGE:
            if (runtime->show_page == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->show_page(
                runtime->display_ctx,
                &effect->meta,
                effect->data.page);

        case APP_CORE_UI_EFFECT_UPDATE_MENU_SELECTION:
            if (runtime->update_menu_selection == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->update_menu_selection(
                runtime->display_ctx,
                &effect->meta,
                effect->data.menu_index);

        default:
            return ESP_ERR_NOT_SUPPORTED;
        }

    case APP_CORE_EFFECT_TARGET_WEB:
        switch (effect->type.code.web)
        {
        case APP_CORE_WEB_EFFECT_START:
            if (runtime->web_start == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->web_start(
                runtime->web_ctx,
                &effect->meta);

        case APP_CORE_WEB_EFFECT_STOP:
            if (runtime->web_stop == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->web_stop(
                runtime->web_ctx,
                &effect->meta);

        default:
            return ESP_ERR_NOT_SUPPORTED;
        }

    case APP_CORE_EFFECT_TARGET_OTA:
        switch (effect->type.code.ota)
        {
        case APP_CORE_OTA_EFFECT_BEGIN:
            if (runtime->ota_begin == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->ota_begin(
                runtime->ota_ctx,
                &effect->meta,
                &effect->data.ota_manifest);

        case APP_CORE_OTA_EFFECT_FINISH:
            if (runtime->ota_finish == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->ota_finish(
                runtime->ota_ctx,
                &effect->meta);

        case APP_CORE_OTA_EFFECT_ABORT:
            if (runtime->ota_abort == NULL)
            {
                return app_core_runtime_not_supported();
            }

            return runtime->ota_abort(
                runtime->ota_ctx,
                &effect->meta);

        default:
            return ESP_ERR_NOT_SUPPORTED;
        }

    case APP_CORE_EFFECT_TARGET_NONE:
    default:
        return ESP_ERR_NOT_SUPPORTED;
    }
}