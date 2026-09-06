#include "app_ui_fsm.h"

#include <stddef.h>

/**
 * @brief 分配下一个 UI Action 编号。
 *
 * 0 保留给无效 Action ID；编号溢出后从 1 重新开始。
 */
static app_core_action_id_t app_ui_allocate_action_id(
    app_ui_fsm_t *fsm)
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
 * @brief 清除当前正在处理的 UI 请求信息。
 */
static void app_ui_clear_active(
    app_ui_fsm_t *fsm)
{
    fsm->active_meta = (app_core_message_meta_t){0};
    fsm->active_request_id = APP_CORE_INVALID_REQUEST_ID;
}

/**
 * @brief 将 UI Effect 封装为 Action 并输出。
 *
 * 该函数只负责把 FSM 决定的 Effect 包装成 Action，
 * 再交给 Controller 的 Action Engine。
 */
static esp_err_t app_ui_emit_action(
    app_ui_fsm_t *fsm,
    app_core_effect_type_t effect_type,
    const app_core_effect_data_t *effect_data,
    app_ui_fsm_emit_action_fn emit_action,
    void *emit_ctx)
{
    if (fsm == NULL || emit_action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    app_core_action_t action = {0};

    action.action_id = app_ui_allocate_action_id(fsm);
    action.state = APP_CORE_ACTION_STATE_CREATED;

    action.effect.meta = fsm->active_meta;
    action.effect.meta.source = APP_CORE_SOURCE_ACTION;
    action.effect.meta.scope = APP_CORE_SCOPE_LVGL;
    action.effect.type = effect_type;

    if (effect_data != NULL)
    {
        action.effect.data = *effect_data;
    }

    return emit_action(emit_ctx, &action);
}

/**
 * @brief 初始化 UI 状态机。
 */
void app_ui_fsm_init(
    app_ui_fsm_t *fsm)
{
    if (fsm == NULL)
    {
        return;
    }
    *fsm = (app_ui_fsm_t){0};
    fsm->current_page = APP_CORE_LVGL_PAGE_BOOT;
    fsm->target_page = APP_CORE_LVGL_PAGE_BOOT;
    fsm->selected_menu_index = 0U;
    fsm->transitioning = false;
    fsm->active_request_id = APP_CORE_INVALID_REQUEST_ID;
    fsm->next_action_id = 1U;
    fsm->last_error = ESP_OK;
}

/**
 * @brief 获取当前 UI 页面。
 */
esp_err_t app_ui_fsm_get_page(
    const app_ui_fsm_t *fsm,
    app_core_lvgl_page_t *page)
{
    if (fsm == NULL || page == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *page = fsm->current_page;
    return ESP_OK;
}

/**
 * @brief 根据 Request 推进 UI 状态机。
 *
 * 当前支持 SHOW_PAGE 请求，
 * 函数会记录目标页面并生成对应的显示页面 Action。
 */
esp_err_t app_ui_fsm_handle_request(
    app_ui_fsm_t *fsm,
    const app_core_request_t *request,
    app_ui_fsm_emit_action_fn emit_action,
    void *emit_ctx)
{
    app_core_effect_data_t effect_data = {0};
    esp_err_t result;

    if (fsm == NULL ||
        request == NULL ||
        emit_action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (fsm->transitioning)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (request->type != APP_CORE_REQUEST_TYPE_SHOW_PAGE)
    {
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (request->data.page < APP_CORE_LVGL_PAGE_BOOT ||
        request->data.page > APP_CORE_LVGL_PAGE_SETTINGS)
    {
        return ESP_ERR_INVALID_ARG;
    }

    fsm->target_page = request->data.page;
    fsm->transitioning = true;
    fsm->active_meta = request->meta;
    fsm->active_request_id = request->meta.request_id;
    fsm->last_error = ESP_OK;

    effect_data.page = request->data.page;

    result = app_ui_emit_action(
        fsm,
        APP_CORE_EFFECT_TYPE_SHOW_PAGE,
        &effect_data,
        emit_action,
        emit_ctx);

    if (result != ESP_OK)
    {
        fsm->transitioning = false;
        fsm->last_error = result;
        app_ui_clear_active(fsm);
    }

    return result;
}

/**
 * @brief 根据 Event 完成 UI 页面状态转换。
 *
 * PAGE_SHOWN 事件会提交页面切换结果，
 * FAILED 事件会结束当前请求并记录错误。
 */
esp_err_t app_ui_fsm_handle_event(
    app_ui_fsm_t *fsm,
    const app_core_event_t *event,
    app_ui_fsm_emit_action_fn emit_action,
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
    case APP_CORE_EVENT_TYPE_PAGE_SHOWN:
        if (!fsm->transitioning)
        {
            return ESP_ERR_INVALID_STATE;
        }
        if (event->data.page != fsm->target_page)
        {
            return ESP_ERR_INVALID_ARG;
        }
        fsm->current_page = event->data.page;
        fsm->transitioning = false;
        fsm->last_error = ESP_OK;
        app_ui_clear_active(fsm);

        return ESP_OK;

    case APP_CORE_EVENT_TYPE_FAILED:
        if (!fsm->transitioning)
        {
            return ESP_ERR_INVALID_STATE;
        }

        fsm->transitioning = false;
        fsm->last_error =
            event->error == ESP_OK ? ESP_FAIL : event->error;

        app_ui_clear_active(fsm);
        return ESP_OK;

    default:
        return ESP_ERR_NOT_SUPPORTED;
    }
}
