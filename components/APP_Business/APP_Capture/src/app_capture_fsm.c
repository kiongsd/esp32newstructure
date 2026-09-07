#include "app_capture_fsm.h"

/**
 * @brief 判断 capture 是否处于忙状态。
 *
 * 忙状态下不能再次发起新的生命周期转换请求。
 */
static bool capture_state_is_busy(app_core_capture_state_t state)
{
    switch (state)
    {
    case APP_CORE_CAPTURE_STATE_REQUESTED:
    case APP_CORE_CAPTURE_STATE_PREPARING:
    case APP_CORE_CAPTURE_STATE_ACQUIRING:
    case APP_CORE_CAPTURE_STATE_PROCESSING:
    case APP_CORE_CAPTURE_STATE_SAVING:
        return true;
    default:
        return false;
    }
}

/**
 * @brief 分配下一个 capture Action 编号。
 *
 * 0 保留给无效 Action ID；编号溢出后从 1 重新开始。
 */
static app_core_action_id_t capture_allocate_action_id(
    app_capture_fsm_t *fsm)
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
static app_core_job_id_t capture_allocate_job_id(
    app_capture_fsm_t *fsm)
{
    app_core_job_id_t job_id;

    if (fsm->next_job_id ==
        APP_CORE_INVALID_JOB_ID)
    {
        fsm->next_job_id =
            1U;
    }

    job_id =
        fsm->next_job_id;

    fsm->next_job_id++;

    return job_id;
}
/**
 * @brief 清除当前正在处理的 capture 请求信息。
 */
static void capture_clear_active(app_capture_fsm_t *fsm)
{
    fsm->active_meta =
        (app_core_message_meta_t){0};

    fsm->active_request_id =
        APP_CORE_INVALID_REQUEST_ID;

    fsm->active_job_id =
        APP_CORE_INVALID_JOB_ID;

    fsm->capture_after_prepare =
        false;

    fsm->web_capture_active =
        false;
}

/**
 * @brief 将 capture 状态机置为故障状态。
 */
static void capture_set_failed(app_capture_fsm_t *fsm, esp_err_t error)
{
    if (error == ESP_OK)
    {
        error = ESP_FAIL;
    }

    fsm->state = APP_CORE_CAPTURE_STATE_FAILED;

    fsm->last_error = error;
}

/**
 * @brief 创建并输出一个 capture Action。
 *
 * 该函数只负责把状态机决定的 Effect 包装成 Action，
 * 再交给 Controller 的 Action Engine。
 */
static esp_err_t capture_emit_action(
    app_capture_fsm_t *fsm,
    const app_core_message_meta_t *source_meta,
    app_core_effect_type_t effect_type,
    const app_core_effect_data_t *effect_data,
    app_capture_fsm_emit_action_fn emit_action,
    void *emit_ctx)
{
    app_core_effect_t effect;
    app_core_action_t action;

    if (fsm == NULL || source_meta == NULL || emit_action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    app_core_effect_init(&effect, effect_type, source_meta->request_id, source_meta->parent_request_id, APP_CORE_SOURCE_ACTION, APP_CORE_SCOPE_CAPTURE);
    if (effect_data != NULL)
    {
        effect.data =
            *effect_data;
    }

    app_core_action_init(&action, capture_allocate_action_id(fsm), &effect);

    return emit_action(emit_ctx, &action);
}

static esp_err_t capture_begin_prepare(
    app_capture_fsm_t *fsm,
    const app_core_request_t *request,
    bool capture_after_prepare,
    app_capture_fsm_emit_action_fn emit_action,
    void *emit_ctx)
{
    app_core_effect_data_t effect_data;
    app_core_message_meta_t request_meta;
    esp_err_t result;

    if (fsm == NULL ||
        request == NULL ||
        emit_action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (capture_state_is_busy(fsm->state))
    {
        return ESP_ERR_INVALID_STATE;
    }

    request_meta =
        request->meta;

    request_meta.scope =
        APP_CORE_SCOPE_CAPTURE;

    fsm->active_meta =
        request_meta;

    fsm->active_request_id =
        request_meta.request_id;

    if (request->data.job_id !=
        APP_CORE_INVALID_JOB_ID)
    {
        fsm->active_job_id =
            request->data.job_id;
    }
    else
    {
        fsm->active_job_id =
            capture_allocate_job_id(fsm);
    }

    fsm->active_photo_handle =
        APP_CORE_INVALID_PHOTO_HANDLE;

    fsm->preview_active =
        false;

    fsm->web_capture_active =
        request_meta.source == APP_CORE_SOURCE_WEB;

    fsm->capture_after_prepare =
        capture_after_prepare;

    fsm->state =
        APP_CORE_CAPTURE_STATE_PREPARING;

    fsm->last_error =
        ESP_OK;

    effect_data =
        (app_core_effect_data_t){0};

    effect_data.job_id =
        fsm->active_job_id;

    result =
        capture_emit_action(
            fsm,
            &fsm->active_meta,
            APP_CORE_EFFECT_TYPE_CAMERA_PREPARE_PHOTO,
            &effect_data,
            emit_action,
            emit_ctx);

    if (result != ESP_OK)
    {
        capture_set_failed(
            fsm,
            result);

        capture_clear_active(
            fsm);

        return result;
    }

    return ESP_OK;
}

/**
 * @brief 初始化 capture 状态机。
 */
void app_capture_fsm_init(app_capture_fsm_t *fsm)
{
    if (fsm == NULL)
    {
        return;
    }
    *fsm = (app_capture_fsm_t){0};
    fsm->state = APP_CORE_CAPTURE_STATE_IDLE;
    fsm->active_meta = (app_core_message_meta_t){0};
    fsm->active_request_id = APP_CORE_INVALID_REQUEST_ID;
    fsm->active_job_id = APP_CORE_INVALID_JOB_ID;
    fsm->active_photo_handle = APP_CORE_INVALID_PHOTO_HANDLE;
    fsm->next_job_id = 1U;
    fsm->next_action_id = 1U;
    fsm->last_error = ESP_OK;
}

/**
 * @brief 获取当前 capture 状态。
 */
esp_err_t app_capture_fsm_get_state(const app_capture_fsm_t *fsm, app_core_capture_state_t *state)
{
    if (fsm == NULL || state == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *state = fsm->state;
    return ESP_OK;
}

/**
 * @brief 根据 Request 推进 capture 状态机。
 *
 * 支持初始化、挂起和恢复三类 capture 生命周期请求。
 */
esp_err_t app_capture_fsm_handle_request(app_capture_fsm_t *fsm, const app_core_request_t *request, app_capture_fsm_emit_action_fn emit_action, void *emit_ctx)
{
    app_core_message_meta_t request_meta;
    esp_err_t result;
    if (fsm == NULL || request == NULL || emit_action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    request_meta = request->meta;
    request_meta.scope = APP_CORE_SCOPE_CAPTURE;

    switch (request->type)
    {
    case APP_CORE_REQUEST_TYPE_CAPTURE_START_PREVIEW:
        if (capture_state_is_busy(fsm->state))
        {
            return ESP_ERR_INVALID_STATE;
        }

        fsm->active_meta =
            request_meta;

        fsm->active_request_id =
            request_meta.request_id;

        fsm->last_error =
            ESP_OK;

        result =
            capture_emit_action(
                fsm,
                &request_meta,
                APP_CORE_EFFECT_TYPE_CAMERA_START_PREVIEW,
                NULL,
                emit_action,
                emit_ctx);

        if (result != ESP_OK)
        {
            capture_set_failed(
                fsm,
                result);

            capture_clear_active(
                fsm);

            return result;
        }

        fsm->state =
            APP_CORE_CAPTURE_STATE_REQUESTED;

        return ESP_OK;

    case APP_CORE_REQUEST_TYPE_CAPTURE_PREPARE_PHOTO:
        return capture_begin_prepare(
            fsm,
            request,
            false,
            emit_action,
            emit_ctx);

    case APP_CORE_REQUEST_TYPE_CAPTURE:
        return capture_begin_prepare(
            fsm,
            request,
            true,
            emit_action,
            emit_ctx);

    case APP_CORE_REQUEST_TYPE_CAPTURE_STOP:
        result =
            capture_emit_action(
                fsm,
                &request_meta,
                APP_CORE_EFFECT_TYPE_CAMERA_STOP,
                NULL,
                emit_action,
                emit_ctx);

        if (result != ESP_OK)
        {
            capture_set_failed(
                fsm,
                result);

            capture_clear_active(
                fsm);

            return result;
        }

        capture_clear_active(
            fsm);

        fsm->active_meta =
            request_meta;

        fsm->active_request_id =
            request_meta.request_id;

        fsm->state =
            APP_CORE_CAPTURE_STATE_REQUESTED;

        fsm->last_error =
            ESP_OK;

        return ESP_OK;

    case APP_CORE_REQUEST_TYPE_CAPTURE_FOCUS:
        if (capture_state_is_busy(fsm->state) ||
            !fsm->preview_active)
        {
            return ESP_ERR_INVALID_STATE;
        }

        fsm->active_meta =
            request_meta;

        fsm->active_request_id =
            request_meta.request_id;

        fsm->last_error =
            ESP_OK;

        result =
            capture_emit_action(
                fsm,
                &request_meta,
                APP_CORE_EFFECT_TYPE_CAMERA_FOCUS,
                NULL,
                emit_action,
                emit_ctx);

        if (result != ESP_OK)
        {
            capture_set_failed(
                fsm,
                result);

            capture_clear_active(
                fsm);

            return result;
        }

        fsm->state =
            APP_CORE_CAPTURE_STATE_REQUESTED;

        return ESP_OK;

    default:
        return ESP_ERR_NOT_SUPPORTED;
    }
}

/**
 * @brief 根据 Event 完成或终止 capture 状态转换。
 *
 * 成功事件会进入对应的稳定状态，
 * FAILED 事件会让 capture 进入 FAULT 状态。
 */
esp_err_t app_capture_fsm_handle_event(
    app_capture_fsm_t *fsm,
    const app_core_event_t *event,
    app_capture_fsm_emit_action_fn emit_action,
    void *emit_ctx)
{
    app_core_effect_data_t effect_data;
    esp_err_t result;

    if (fsm == NULL ||
        event == NULL ||
        emit_action == NULL)
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
        return ESP_ERR_NOT_FOUND;
    }

    switch (event->type)
    {
    case APP_CORE_EVENT_TYPE_CAMERA_PREVIEW_STARTED:
        fsm->preview_active =
            true;

        fsm->state =
            APP_CORE_CAPTURE_STATE_IDLE;

        fsm->last_error =
            ESP_OK;

        capture_clear_active(
            fsm);

        return ESP_OK;

    case APP_CORE_EVENT_TYPE_CAMERA_PREPARED:
        if (fsm->state !=
            APP_CORE_CAPTURE_STATE_PREPARING)
        {
            return ESP_ERR_INVALID_STATE;
        }

        if (!fsm->capture_after_prepare)
        {
            fsm->state =
                APP_CORE_CAPTURE_STATE_IDLE;

            fsm->last_error =
                ESP_OK;

            capture_clear_active(
                fsm);

            return ESP_OK;
        }

        fsm->state =
            APP_CORE_CAPTURE_STATE_ACQUIRING;

        effect_data =
            (app_core_effect_data_t){0};

        effect_data.job_id =
            fsm->active_job_id;

        result =
            capture_emit_action(
                fsm,
                &fsm->active_meta,
                APP_CORE_EFFECT_TYPE_CAMERA_CAPTURE,
                &effect_data,
                emit_action,
                emit_ctx);

        if (result != ESP_OK)
        {
            capture_set_failed(
                fsm,
                result);

            capture_clear_active(
                fsm);

            return result;
        }

        return ESP_OK;

    case APP_CORE_EVENT_TYPE_CAMERA_CAPTURED:
        if (fsm->state !=
            APP_CORE_CAPTURE_STATE_ACQUIRING)
        {
            return ESP_ERR_INVALID_STATE;
        }

        if (event->data.photo_handle ==
            APP_CORE_INVALID_PHOTO_HANDLE)
        {
            capture_set_failed(
                fsm,
                ESP_ERR_INVALID_RESPONSE);

            capture_clear_active(
                fsm);

            return ESP_ERR_INVALID_RESPONSE;
        }

        fsm->active_photo_handle =
            event->data.photo_handle;

        fsm->state =
            APP_CORE_CAPTURE_STATE_PROCESSING;

        effect_data =
            (app_core_effect_data_t){0};

        effect_data.photo_handle =
            fsm->active_photo_handle;

        result =
            capture_emit_action(
                fsm,
                &fsm->active_meta,
                APP_CORE_EFFECT_TYPE_SAVE_PHOTO,
                &effect_data,
                emit_action,
                emit_ctx);

        if (result != ESP_OK)
        {
            capture_set_failed(
                fsm,
                result);

            capture_clear_active(
                fsm);

            return result;
        }

        fsm->state =
            APP_CORE_CAPTURE_STATE_SAVING;

        return ESP_OK;

    case APP_CORE_EVENT_TYPE_PHOTO_SAVED:
        if (fsm->state !=
            APP_CORE_CAPTURE_STATE_SAVING)
        {
            return ESP_ERR_INVALID_STATE;
        }

        if (event->data.photo_handle !=
            APP_CORE_INVALID_PHOTO_HANDLE)
        {
            fsm->active_photo_handle =
                event->data.photo_handle;
        }

        fsm->state =
            APP_CORE_CAPTURE_STATE_SUCCEEDED;

        fsm->last_error =
            ESP_OK;

        capture_clear_active(
            fsm);

        return ESP_OK;

    case APP_CORE_EVENT_TYPE_CAMERA_STOPPED:
        if (fsm->state !=
                APP_CORE_CAPTURE_STATE_IDLE ||
            fsm->preview_active)
        {
            fsm->state =
                APP_CORE_CAPTURE_STATE_CANCELED;
        }

        fsm->preview_active =
            false;

        fsm->last_error =
            ESP_OK;

        capture_clear_active(
            fsm);

        return ESP_OK;

    case APP_CORE_EVENT_TYPE_CAMERA_FOCUSED:
        fsm->state =
            APP_CORE_CAPTURE_STATE_IDLE;

        fsm->last_error =
            ESP_OK;

        capture_clear_active(
            fsm);

        return ESP_OK;

    case APP_CORE_EVENT_TYPE_FAILED:
        capture_set_failed(
            fsm,
            event->error);

        capture_clear_active(
            fsm);

        return ESP_OK;

    default:
        return ESP_ERR_NOT_SUPPORTED;
    }
}
