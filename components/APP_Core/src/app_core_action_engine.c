#include "app_core_action_engine.h"

#include "esp_err.h"

/**
 * @brief 检查 Action 是否适合进入执行队列。
 */
static bool action_engine_action_is_valid(
    const app_core_action_t *action)
{
    if (!app_core_action_is_valid(action))
    {
        return false;
    }

    if (action->effect.type ==
        APP_CORE_EFFECT_TYPE_NONE)
    {
        return false;
    }

    return true;
}

/**
 * @brief 判断两个 Request ID 是否匹配。
 *
 * 如果任意一方没有 Request ID，
 * 则认为当前暂时可以匹配。
 */
static bool request_id_matches(
    app_core_request_id_t action_request_id,
    app_core_request_id_t event_request_id)
{
    if (action_request_id ==
            APP_CORE_INVALID_REQUEST_ID ||
        event_request_id ==
            APP_CORE_INVALID_REQUEST_ID)
    {
        return true;
    }

    return action_request_id ==
           event_request_id;
}

/**
 * @brief 获取 Effect 对应的成功 Event 类型。
 */
static app_core_event_type_t
get_expected_event_type(
    app_core_effect_type_t effect_type)
{
    switch (effect_type)
    {
    case APP_CORE_EFFECT_TYPE_SHOW_PAGE:
        return APP_CORE_EVENT_TYPE_PAGE_SHOWN;

    case APP_CORE_EFFECT_TYPE_CAMERA_START_PREVIEW:
        return APP_CORE_EVENT_TYPE_CAMERA_PREVIEW_STARTED;

    case APP_CORE_EFFECT_TYPE_CAMERA_PREPARE_PHOTO:
        return APP_CORE_EVENT_TYPE_CAMERA_PREPARED;

    case APP_CORE_EFFECT_TYPE_CAMERA_STOP:
        return APP_CORE_EVENT_TYPE_CAMERA_STOPPED;

    case APP_CORE_EFFECT_TYPE_CAMERA_CAPTURE:
        return APP_CORE_EVENT_TYPE_CAMERA_CAPTURED;

    case APP_CORE_EFFECT_TYPE_CAMERA_FOCUS:
        return APP_CORE_EVENT_TYPE_CAMERA_FOCUSED;

    case APP_CORE_EFFECT_TYPE_SAVE_PHOTO:
        return APP_CORE_EVENT_TYPE_PHOTO_SAVED;

    case APP_CORE_EFFECT_TYPE_SCAN_GALLERY:
        return APP_CORE_EVENT_TYPE_GALLERY_SCANNED;

    case APP_CORE_EFFECT_TYPE_SHOW_GALLERY_PHOTO:
        return APP_CORE_EVENT_TYPE_GALLERY_SELECTION_UPDATED;

    case APP_CORE_EFFECT_TYPE_WEB_START:
        return APP_CORE_EVENT_TYPE_WEB_STARTED;

    case APP_CORE_EFFECT_TYPE_WEB_STOP:
        return APP_CORE_EVENT_TYPE_WEB_STOPPED;

    case APP_CORE_EFFECT_TYPE_OTA_BEGIN:
        return APP_CORE_EVENT_TYPE_OTA_STARTED;

    case APP_CORE_EFFECT_TYPE_OTA_FINISH:
        return APP_CORE_EVENT_TYPE_OTA_FINISHED;

    case APP_CORE_EFFECT_TYPE_OTA_ABORT:
        return APP_CORE_EVENT_TYPE_OTA_ABORTED;

    case APP_CORE_EFFECT_TYPE_UPDATE_MENU_SELECTION:
        return APP_CORE_EVENT_TYPE_MENU_SELECTION_UPDATED;

    case APP_CORE_EFFECT_TYPE_SYSTEM_INITIALIZE:
        return APP_CORE_EVENT_TYPE_SYSTEM_READY;

    case APP_CORE_EFFECT_TYPE_SYSTEM_SUSPEND:
        return APP_CORE_EVENT_TYPE_SYSTEM_SUSPENDED;

    case APP_CORE_EFFECT_TYPE_SYSTEM_RESUME:
        return APP_CORE_EVENT_TYPE_SYSTEM_RESUMED;

    default:
        return APP_CORE_EVENT_TYPE_NONE;
    }
}

/**
 * @brief 判断 Action 是否匹配指定 Event。
 */
static bool action_matches_event(
    const app_core_action_t *action,
    const app_core_event_t *event)
{
    app_core_event_type_t expected_event_type;

    if (action == NULL ||
        event == NULL)
    {
        return false;
    }

    if (!request_id_matches(
            action->effect.meta.request_id,
            event->meta.request_id))
    {
        return false;
    }

    if (event->type ==
        APP_CORE_EVENT_TYPE_FAILED)
    {
        return true;
    }

    expected_event_type =
        get_expected_event_type(
            action->effect.type);

    return expected_event_type ==
           event->type;
}

/**
 * @brief 保存最近一次 Action。
 */
static void record_action(
    app_core_action_engine_t *engine,
    const app_core_action_t *action)
{
    if (engine == NULL ||
        action == NULL)
    {
        return;
    }

    engine->last_action = *action;
    engine->has_last_action = true;
}

/**
 * @brief 查找空闲的 pending Action 槽位。
 */
static int find_free_pending_slot(
    const app_core_action_engine_t *engine)
{
    uint8_t index;

    if (engine == NULL)
    {
        return -1;
    }

    for (index = 0U;
         index < APP_CORE_ACTION_ENGINE_MAX_PENDING;
         index++)
    {
        if (!engine->pending_used[index])
        {
            return (int)index;
        }
    }

    return -1;
}

/**
 * @brief 发送 Action 执行失败 Event。
 */
static esp_err_t emit_failed_event(
    app_core_action_engine_t *engine,
    const app_core_action_t *action,
    esp_err_t error)
{
    app_core_event_t event;

    if (engine == NULL ||
        action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (engine->emit_event == NULL)
    {
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (error == ESP_OK)
    {
        error = ESP_FAIL;
    }

    event =
        (app_core_event_t){0};

    event.meta =
        action->effect.meta;

    event.meta.source =
        APP_CORE_SOURCE_ACTION;

    event.type =
        APP_CORE_EVENT_TYPE_FAILED;

    event.error =
        error;

    return engine->emit_event(
        engine->event_ctx,
        &event);
}

/**
 * @brief 初始化 Action Engine。
 */
esp_err_t app_core_action_engine_init(
    app_core_action_engine_t *engine,
    const app_core_runtime_t *runtime,
    QueueHandle_t action_queue)
{
    uint8_t index;

    if (engine == NULL ||
        runtime == NULL ||
        action_queue == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (engine->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    *engine =
        (app_core_action_engine_t){0};

    engine->runtime =
        runtime;

    engine->action_queue =
        action_queue;

    for (index = 0U;
         index < APP_CORE_ACTION_ENGINE_MAX_PENDING;
         index++)
    {
        engine->pending_actions[index] =
            (app_core_action_t){0};

        engine->pending_used[index] =
            false;
    }

    engine->initialized =
        true;

    return ESP_OK;
}

/**
 * @brief 绑定 Event 输出回调。
 */
esp_err_t app_core_action_engine_bind_event_sink(
    app_core_action_engine_t *engine,
    app_core_action_engine_emit_event_fn emit_event,
    void *event_ctx)
{
    if (engine == NULL ||
        emit_event == NULL ||
        event_ctx == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!engine->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    engine->emit_event =
        emit_event;

    engine->event_ctx =
        event_ctx;

    return ESP_OK;
}

/**
 * @brief 提交 Action 到执行队列。
 */
esp_err_t app_core_action_engine_submit(
    app_core_action_engine_t *engine,
    const app_core_action_t *action)
{
    app_core_action_t queued_action;

    if (engine == NULL ||
        action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!engine->initialized ||
        engine->action_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!action_engine_action_is_valid(action))
    {
        return ESP_ERR_INVALID_ARG;
    }

    queued_action =
        *action;

    queued_action.state =
        APP_CORE_ACTION_STATE_QUEUED;

    queued_action.last_error =
        ESP_OK;

    if (xQueueSend(
            engine->action_queue,
            &queued_action,
            0U) != pdTRUE)
    {
        queued_action.state =
            APP_CORE_ACTION_STATE_FAILED;

        queued_action.last_error =
            ESP_ERR_TIMEOUT;

        record_action(
            engine,
            &queued_action);

        (void)emit_failed_event(
            engine,
            &queued_action,
            ESP_ERR_TIMEOUT);

        return ESP_ERR_TIMEOUT;
    }

    record_action(
        engine,
        &queued_action);

    return ESP_OK;
}

/**
 * @brief 从队列取出并执行一个 Action。
 */
esp_err_t app_core_action_engine_process_once(
    app_core_action_engine_t *engine)
{
    app_core_action_t action;
    esp_err_t runtime_result;
    esp_err_t event_result;
    int pending_slot;

    if (engine == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!engine->initialized ||
        engine->runtime == NULL ||
        engine->action_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (xQueueReceive(
            engine->action_queue,
            &action,
            0U) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    if (!action_engine_action_is_valid(&action))
    {
        return ESP_ERR_INVALID_ARG;
    }

    pending_slot =
        find_free_pending_slot(engine);

    if (pending_slot < 0)
    {
        action.state =
            APP_CORE_ACTION_STATE_FAILED;

        action.last_error =
            ESP_ERR_NO_MEM;

        record_action(
            engine,
            &action);

        (void)emit_failed_event(
            engine,
            &action,
            ESP_ERR_NO_MEM);

        return ESP_OK;
    }

    action.state =
        APP_CORE_ACTION_STATE_EXECUTING;

    action.last_error =
        ESP_OK;

    record_action(
        engine,
        &action);

    runtime_result =
        app_core_runtime_execute(
            engine->runtime,
            &action.effect);

    if (runtime_result != ESP_OK)
    {
        action.state =
            APP_CORE_ACTION_STATE_FAILED;

        action.last_error =
            runtime_result;

        record_action(
            engine,
            &action);

        event_result =
            emit_failed_event(
                engine,
                &action,
                runtime_result);

        if (event_result == ESP_OK)
        {
            return ESP_OK;
        }

        return runtime_result;
    }

    action.state =
        APP_CORE_ACTION_STATE_WAITING_RESULT;

    action.last_error =
        ESP_OK;

    engine->pending_actions[pending_slot] =
        action;

    engine->pending_used[pending_slot] =
        true;

    record_action(
        engine,
        &action);

    return ESP_OK;
}

/**
 * @brief 处理底层 Event 并更新 Action 状态。
 */
esp_err_t app_core_action_engine_handle_event(
    app_core_action_engine_t *engine,
    const app_core_event_t *event)
{
    app_core_action_t action;
    uint8_t index;

    if (engine == NULL ||
        event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!engine->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!app_core_event_is_valid(event))
    {
        return ESP_ERR_INVALID_ARG;
    }

    for (index = 0U;
         index < APP_CORE_ACTION_ENGINE_MAX_PENDING;
         index++)
    {
        if (!engine->pending_used[index])
        {
            continue;
        }

        action =
            engine->pending_actions[index];

        if (!action_matches_event(
                &action,
                event))
        {
            continue;
        }

        if (event->type ==
            APP_CORE_EVENT_TYPE_FAILED)
        {
            action.state =
                APP_CORE_ACTION_STATE_FAILED;

            action.last_error =
                event->error == ESP_OK
                    ? ESP_FAIL
                    : event->error;
        }
        else
        {
            action.state =
                APP_CORE_ACTION_STATE_SUCCEEDED;

            action.last_error =
                ESP_OK;
        }

        engine->pending_actions[index] =
            (app_core_action_t){0};

        engine->pending_used[index] =
            false;

        record_action(
            engine,
            &action);

        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief 获取最近一次 Action。
 */
esp_err_t app_core_action_engine_get_last_action(
    const app_core_action_engine_t *engine,
    app_core_action_t *action)
{
    if (engine == NULL ||
        action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!engine->initialized ||
        !engine->has_last_action)
    {
        return ESP_ERR_NOT_FOUND;
    }

    *action =
        engine->last_action;

    return ESP_OK;
}

/**
 * @brief 将 Action 提交到 Action Engine。
 */
esp_err_t app_core_action_engine_emit_action(
    void *ctx,
    const app_core_action_t *action)
{
    if (ctx == NULL ||
        action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return app_core_action_engine_submit(
        (app_core_action_engine_t *)ctx,
        action);
}