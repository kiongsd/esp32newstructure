#include <stdio.h>

#include "app_core_domain.h"
#include "app_core_effect.h"
#include "app_core_event.h"
#include "app_core_request.h"
#include "app_core_state.h"
#include "app_core_action.h"
#include "app_core_state_store.h"
#include "app_core_request_gateway.h"
#include "app_core_dispatcher.h"
#include "app_core_runtime.h"
#include "app_core_action_engine.h"
/**
 * @brief 测试 Domain 是否处理指定 Request。
 */

static bool test_domain_match_request(
    void *ctx,
    const app_core_request_t *request)
{
    (void)ctx;

    return request != NULL &&
           request->type == APP_CORE_REQUEST_TYPE_CAPTURE;
}

/**
 * @brief 测试 Domain 的 Request 处理函数。
 */
static esp_err_t test_domain_handle_request(
    void *ctx,
    const app_core_request_t *request)
{
    (void)ctx;
    (void)request;

    return ESP_OK;
}
/**
 * @brief Runtime 的假页面显示回调。
 *
 * 当前没有开发板，只返回 ESP_OK，
 * 用于验证 Runtime 能够正确调用底层回调。
 */
static esp_err_t test_runtime_show_page(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_lvgl_page_t page)
{
    (void)ctx;
    (void)meta;
    (void)page;

    return ESP_OK;
}
/**
 * @brief 测试 Domain 是否处理指定 Event。
 */
static bool test_domain_match_event(
    void *ctx,
    const app_core_event_t *event)
{
    (void)ctx;

    return event != NULL &&
           event->type == APP_CORE_EVENT_TYPE_CAMERA_CAPTURED;
}

/**
 * @brief 测试 Domain 的 Event 处理函数。
 */
static esp_err_t test_domain_handle_event(
    void *ctx,
    const app_core_event_t *event)
{
    (void)ctx;
    (void)event;

    return ESP_OK;
}

/**
 * @brief 测试 Domain 的周期处理函数。
 */
static esp_err_t test_domain_process_once(
    void *ctx)
{
    (void)ctx;

    return ESP_OK;
}

typedef struct
{
    app_core_request_t last_request;
    app_core_state_snapshot_t snapshot;
    bool request_received;

} test_gateway_context_t;

static esp_err_t test_gateway_submit(
    void *ctx,
    const app_core_request_t *request)
{
    test_gateway_context_t *test_ctx =
        (test_gateway_context_t *)ctx;

    if (test_ctx == NULL ||
        request == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    test_ctx->last_request = *request;
    test_ctx->request_received = true;

    return ESP_OK;
}

static esp_err_t test_gateway_read_state(
    void *ctx,
    app_core_state_snapshot_t *snapshot)
{
    test_gateway_context_t *test_ctx =
        (test_gateway_context_t *)ctx;

    if (test_ctx == NULL ||
        snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *snapshot = test_ctx->snapshot;

    return ESP_OK;
}
typedef struct
{
    app_core_event_t last_event;
    bool event_received;

} test_action_engine_event_context_t;
/**
 * @brief Action Engine 的假 Event 输出回调。
 */
static esp_err_t test_action_engine_emit_event(
    void *ctx,
    const app_core_event_t *event)
{
    test_action_engine_event_context_t *test_ctx =
        (test_action_engine_event_context_t *)ctx;

    if (test_ctx == NULL ||
        event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    test_ctx->last_event =
        *event;

    test_ctx->event_received =
        true;

    return ESP_OK;
}
/**
 * @brief App Core 公共层编译验证入口。
 *
 * 当前没有开发板时，只验证公共层对象是否能够正确编译、
 * 链接以及完成基本的对象初始化。
 */
void app_main(void)
{
    app_core_request_t request;
    app_core_event_t event;
    app_core_effect_t effect;
    app_core_state_snapshot_t snapshot;
    app_core_domain_handler_t domain;

    bool request_valid;
    bool event_valid;
    bool effect_valid;
    bool state_valid;
    bool domain_valid;

    app_core_action_t action;
    bool action_valid;

    app_core_state_store_t state_store = {0};
    app_core_state_snapshot_t stored_snapshot = {0};

    esp_err_t state_store_init_result;
    esp_err_t state_store_publish_result;
    esp_err_t state_store_read_result;

    bool state_store_valid;

    test_gateway_context_t gateway_context = {0};
    app_core_request_t gateway_request;
    app_core_state_snapshot_t gateway_snapshot = {0};

    esp_err_t gateway_init_result;
    esp_err_t gateway_bind_result;
    esp_err_t gateway_submit_result;
    esp_err_t gateway_read_result;

    bool gateway_valid;

    app_core_dispatcher_t dispatcher = {0};

    esp_err_t dispatcher_init_result;
    esp_err_t dispatcher_register_result;
    esp_err_t dispatcher_request_result;
    esp_err_t dispatcher_event_result;
    esp_err_t dispatcher_process_result;

    bool dispatcher_valid;
    app_core_runtime_t runtime = {0};
    app_core_effect_t runtime_effect;
    esp_err_t runtime_execute_result;
    bool runtime_valid;

    QueueHandle_t action_queue = NULL;

    app_core_action_engine_t action_engine = {0};
    app_core_effect_t engine_effect;
    app_core_action_t engine_action;
    app_core_action_t last_engine_action = {0};
    app_core_event_t engine_result_event;

    test_action_engine_event_context_t
        action_event_context = {0};

    esp_err_t action_engine_init_result;
    esp_err_t action_engine_bind_result;
    esp_err_t action_engine_submit_result;
    esp_err_t action_engine_process_result;
    esp_err_t action_engine_event_result;
    esp_err_t action_engine_last_result;

    bool action_engine_valid;

    app_core_request_init(
        &request,
        APP_CORE_REQUEST_TYPE_CAPTURE,
        1U,
        APP_CORE_INVALID_REQUEST_ID,
        APP_CORE_SOURCE_TEST,
        APP_CORE_SCOPE_CAPTURE);

    request.data.job_id = 100U;

    app_core_event_init(
        &event,
        APP_CORE_EVENT_TYPE_CAMERA_CAPTURED,
        request.meta.request_id,
        request.meta.request_id,
        APP_CORE_SOURCE_DRIVER,
        APP_CORE_SCOPE_CAPTURE);

    event.data.job_id = request.data.job_id;

    app_core_effect_init(
        &effect,
        APP_CORE_EFFECT_TYPE_SAVE_PHOTO,
        request.meta.request_id,
        request.meta.request_id,
        APP_CORE_SOURCE_ACTION,
        APP_CORE_SCOPE_STORAGE);

    effect.data.photo_handle = 1U;

    app_core_action_init(
        &action,
        1U,
        &effect);

    app_core_state_snapshot_init(&snapshot);

    app_core_domain_handler_init(
        &domain,
        NULL,
        test_domain_match_request,
        test_domain_handle_request,
        test_domain_match_event,
        test_domain_handle_event,
        test_domain_process_once);
    dispatcher_init_result =
        app_core_dispatcher_init(
            &dispatcher,
            4U,
            4U);

    dispatcher_register_result =
        ESP_ERR_INVALID_STATE;

    dispatcher_request_result =
        ESP_ERR_INVALID_STATE;

    dispatcher_event_result =
        ESP_ERR_INVALID_STATE;

    dispatcher_process_result =
        ESP_ERR_INVALID_STATE;

    dispatcher_valid = false;

    if (dispatcher_init_result == ESP_OK)
    {
        dispatcher_register_result =
            app_core_dispatcher_register_domain(
                &dispatcher,
                &domain);

        if (dispatcher_register_result == ESP_OK)
        {
            dispatcher_request_result =
                app_core_dispatcher_submit_request(
                    &dispatcher,
                    &request);

            dispatcher_event_result =
                app_core_dispatcher_emit_event(
                    &dispatcher,
                    &event);

            dispatcher_process_result =
                app_core_dispatcher_process_once(
                    &dispatcher);

            dispatcher_valid =
                dispatcher_request_result == ESP_OK &&
                dispatcher_event_result == ESP_OK &&
                dispatcher_process_result == ESP_OK;
        }
    }

    printf(
        "dispatcher: valid=%s\n",
        dispatcher_valid ? "true" : "false");

    app_core_dispatcher_deinit(
        &dispatcher);
    state_store_init_result =
        app_core_state_store_init(&state_store);

    app_core_state_snapshot_init(
        &gateway_context.snapshot);

    app_core_request_init(
        &gateway_request,
        APP_CORE_REQUEST_TYPE_CAPTURE,
        APP_CORE_INVALID_REQUEST_ID,
        APP_CORE_INVALID_REQUEST_ID,
        APP_CORE_SOURCE_TEST,
        APP_CORE_SCOPE_CAPTURE);

    state_store_publish_result =
        ESP_ERR_INVALID_STATE;

    state_store_read_result =
        ESP_ERR_INVALID_STATE;

    state_store_valid = false;

    gateway_init_result =
        app_core_request_gateway_init();

    gateway_bind_result =
        ESP_ERR_INVALID_STATE;

    gateway_submit_result =
        ESP_ERR_INVALID_STATE;

    gateway_read_result =
        ESP_ERR_INVALID_STATE;

    gateway_valid = false;

    if (state_store_init_result == ESP_OK)
    {
        snapshot.system_state =
            APP_CORE_SYSTEM_STATE_READY;

        state_store_publish_result =
            app_core_state_store_publish(
                &state_store,
                &snapshot);

        state_store_read_result =
            app_core_state_store_read(
                &state_store,
                &stored_snapshot);

        state_store_valid =
            state_store_publish_result == ESP_OK &&
            state_store_read_result == ESP_OK &&
            app_core_state_snapshot_is_valid(
                &stored_snapshot);
    }

    printf(
        "state store: valid=%s, revision=%u\n",
        state_store_valid ? "true" : "false",
        (unsigned)stored_snapshot.revision);
    app_core_effect_init(
        &runtime_effect,
        APP_CORE_EFFECT_TYPE_SHOW_PAGE,
        request.meta.request_id,
        request.meta.request_id,
        APP_CORE_SOURCE_ACTION,
        APP_CORE_SCOPE_LVGL);

    runtime_effect.data.page =
        APP_CORE_LVGL_PAGE_MENU;

    runtime.show_page =
        test_runtime_show_page;

    runtime_execute_result =
        app_core_runtime_execute(
            &runtime,
            &runtime_effect);

    runtime_valid =
        runtime_execute_result == ESP_OK &&
        app_core_effect_is_valid(
            &runtime_effect);

    printf(
        "runtime: valid=%s\n",
        runtime_valid ? "true" : "false");
    app_core_effect_init(
        &engine_effect,
        APP_CORE_EFFECT_TYPE_SHOW_PAGE,
        request.meta.request_id,
        request.meta.request_id,
        APP_CORE_SOURCE_ACTION,
        APP_CORE_SCOPE_LVGL);

    engine_effect.data.page =
        APP_CORE_LVGL_PAGE_MENU;

    app_core_action_init(
        &engine_action,
        2U,
        &engine_effect);

    action_queue =
        xQueueCreate(
            2U,
            sizeof(app_core_action_t));

    action_engine_init_result =
        ESP_ERR_INVALID_STATE;

    action_engine_bind_result =
        ESP_ERR_INVALID_STATE;

    action_engine_submit_result =
        ESP_ERR_INVALID_STATE;

    action_engine_process_result =
        ESP_ERR_INVALID_STATE;

    action_engine_event_result =
        ESP_ERR_INVALID_STATE;

    action_engine_last_result =
        ESP_ERR_INVALID_STATE;

    action_engine_valid =
        false;

    if (action_queue != NULL)
    {
        action_engine_init_result =
            app_core_action_engine_init(
                &action_engine,
                &runtime,
                action_queue);

        if (action_engine_init_result == ESP_OK)
        {
            action_engine_bind_result =
                app_core_action_engine_bind_event_sink(
                    &action_engine,
                    test_action_engine_emit_event,
                    &action_event_context);
        }

        if (action_engine_bind_result == ESP_OK)
        {
            action_engine_submit_result =
                app_core_action_engine_submit(
                    &action_engine,
                    &engine_action);
        }

        if (action_engine_submit_result == ESP_OK)
        {
            action_engine_process_result =
                app_core_action_engine_process_once(
                    &action_engine);
        }

        app_core_event_init(
            &engine_result_event,
            APP_CORE_EVENT_TYPE_PAGE_SHOWN,
            engine_effect.meta.request_id,
            engine_effect.meta.parent_request_id,
            APP_CORE_SOURCE_DRIVER,
            APP_CORE_SCOPE_LVGL);

        engine_result_event.data.page =
            APP_CORE_LVGL_PAGE_MENU;

        if (action_engine_process_result == ESP_OK)
        {
            action_engine_event_result =
                app_core_action_engine_handle_event(
                    &action_engine,
                    &engine_result_event);
        }

        if (action_engine_event_result == ESP_OK)
        {
            action_engine_last_result =
                app_core_action_engine_get_last_action(
                    &action_engine,
                    &last_engine_action);
        }

        action_engine_valid =
            action_engine_init_result == ESP_OK &&
            action_engine_bind_result == ESP_OK &&
            action_engine_submit_result == ESP_OK &&
            action_engine_process_result == ESP_OK &&
            action_engine_event_result == ESP_OK &&
            action_engine_last_result == ESP_OK &&
            last_engine_action.state ==
                APP_CORE_ACTION_STATE_SUCCEEDED;
    }

    printf(
        "action engine: valid=%s, state=%s\n",
        action_engine_valid ? "true" : "false",
        app_core_action_state_to_string(
            last_engine_action.state));

    if (action_queue != NULL)
    {
        vQueueDelete(action_queue);
        action_queue = NULL;
    }

    action_engine =
        (app_core_action_engine_t){0};

    app_core_state_store_deinit(&state_store);
    request_valid =
        app_core_request_is_valid(&request);

    event_valid =
        app_core_event_is_valid(&event);

    effect_valid =
        app_core_effect_is_valid(&effect);

    state_valid =
        app_core_state_snapshot_is_valid(&snapshot);

    domain_valid =
        app_core_domain_handler_is_valid(&domain);

    action_valid =
        app_core_action_is_valid(&action);

    printf(
        "request: %s, valid=%s\n",
        app_core_request_type_to_string(request.type),
        request_valid ? "true" : "false");

    printf(
        "event: %s, valid=%s\n",
        app_core_event_type_to_string(event.type),
        event_valid ? "true" : "false");

    printf(
        "effect: %s, valid=%s\n",
        app_core_effect_type_to_string(effect.type),
        effect_valid ? "true" : "false");

    printf(
        "state: %s, valid=%s\n",
        app_core_system_state_to_string(snapshot.system_state),
        state_valid ? "true" : "false");

    printf(
        "domain valid: %s\n",
        domain_valid ? "true" : "false");

    printf(
        "action: %s, valid=%s\n",
        app_core_action_state_to_string(action.state),
        action_valid ? "true" : "false");

    if (gateway_init_result == ESP_OK)
    {
        gateway_bind_result =
            app_core_request_gateway_bind(
                test_gateway_submit,
                &gateway_context,
                test_gateway_read_state,
                &gateway_context);

        if (gateway_bind_result == ESP_OK)
        {
            gateway_submit_result =
                app_core_request_gateway_submit(
                    &gateway_request);

            gateway_read_result =
                app_core_request_gateway_read_state(
                    &gateway_snapshot);

            gateway_valid =
                gateway_submit_result == ESP_OK &&
                gateway_read_result == ESP_OK &&
                gateway_context.request_received &&
                gateway_context.last_request.meta.request_id !=
                    APP_CORE_INVALID_REQUEST_ID &&
                app_core_state_snapshot_is_valid(
                    &gateway_snapshot);
        }
    }

    printf(
        "gateway: valid=%s, request_id=%u\n",
        gateway_valid ? "true" : "false",
        (unsigned)gateway_context.last_request.meta.request_id);

    app_core_request_gateway_unbind(
        &gateway_context);

    app_core_request_gateway_deinit(
        &gateway_context);
}