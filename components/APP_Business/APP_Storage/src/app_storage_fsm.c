#include "app_storage_fsm.h"

#include <stddef.h>

/**
 * @brief 判断 Storage 是否处于异步处理状态。
 *
 * SCANNING 和 SHOWING 状态下不能再次发起新的 Storage 请求。
 */
static bool app_storage_state_is_busy(
    app_core_storage_state_t state)
{
    return state == APP_CORE_STORAGE_STATE_SCANNING ||
           state == APP_CORE_STORAGE_STATE_SHOWING;
}

/**
 * @brief 分配下一个 Storage Action 编号。
 *
 * 0 保留给无效 Action ID；编号溢出后从 1 重新开始。
 */
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

/**
 * @brief 清除当前正在处理的 Storage 请求信息。
 */
static void app_storage_clear_active(
    app_storage_fsm_t *fsm)
{
    fsm->active_meta = (app_core_message_meta_t){0};
    fsm->active_request_id = APP_CORE_INVALID_REQUEST_ID;
}

/**
 * @brief 将 Storage Effect 封装为 Action 并输出。
 *
 * 该函数只负责把 FSM 决定的 Effect 包装成 Action，
 * 再交给 Controller 的 Action Engine。
 */
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

/**
 * @brief 初始化 Storage 状态机。
 */
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

/**
 * @brief 获取当前 Storage 状态。
 */
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

/**
 * @brief 根据 Request 推进 Storage 状态机。
 *
 * 支持扫描图库和显示指定照片两类请求，
 * 并在请求通过校验后生成对应的 Storage Action。
 */
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
        effect_type = app_core_effect_type_make_storage(
            APP_CORE_STORAGE_EFFECT_SCAN_GALLERY);

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

        effect_type = app_core_effect_type_make_storage(
            APP_CORE_STORAGE_EFFECT_SHOW_GALLERY_PHOTO);
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

/**
 * @brief 根据 Event 完成 Storage 状态转换。
 *
 * 成功事件会更新图库数据并回到 IDLE，
 * FAILED 事件会记录错误并进入 FAILED 状态。
 */
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
