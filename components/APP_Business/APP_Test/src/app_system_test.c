#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"

#include "app_core_dispatcher.h"
#include "app_core_event.h"
#include "app_core_request.h"
#include "app_core_runtime.h"
#include "app_core_state_store.h"

#include "app_system_controller.h"
#include "app_system_domain.h"

static const char *TAG = "APP_SYSTEM_TEST";

/**
 * @brief System 测试上下文。
 *
 * 当前只保存 Dispatcher，
 * 方便假的 Runtime 回调发送 Event。
 */
typedef struct
{
    /** 用于接收测试 Event 的 Dispatcher。 */
    app_core_dispatcher_t *dispatcher;

} app_system_test_context_t;

/**
 * @brief 发送一个假的 System Event。
 *
 * 该函数模拟 Driver 或 Service 完成动作后，
 * 向 Dispatcher 发送执行结果。
 *
 * @param ctx 测试上下文。
 * @param meta 原始 Action 的消息元数据。
 * @param event_type 要发送的 Event 类型。
 * @return ESP_OK 表示发送成功。
 */
static esp_err_t app_system_test_emit_event(void *ctx, const app_core_message_meta_t *meta, app_core_event_type_t event_type)
{
    app_system_test_context_t *test_context;
    app_core_event_t event;
    if (ctx == NULL || meta == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    test_context = (app_system_test_context_t *)ctx;

    if (test_context->dispatcher == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    app_core_event_init(&event, event_type, meta->request_id, meta->parent_request_id, APP_CORE_SOURCE_DRIVER, APP_CORE_SCOPE_SYSTEM);

    return app_core_dispatcher_emit_event(test_context->dispatcher, &event);
}

/**
 * @brief System 初始化测试回调。
 *
 * 模拟真实系统初始化完成，
 * 直接发送 SYSTEM_READY Event。
 */
static esp_err_t app_system_test_initialize(void *ctx, const app_core_message_meta_t *meta)
{
    return app_system_test_emit_event(ctx, meta, APP_CORE_EVENT_TYPE_SYSTEM_READY);
}

/**
 * @brief System 挂起测试回调。
 *
 * 模拟系统挂起完成，
 * 直接发送 SYSTEM_SUSPENDED Event。
 */
static esp_err_t app_system_test_suspend(void *ctx, const app_core_message_meta_t *meta)
{
    return app_system_test_emit_event(ctx, meta, APP_CORE_EVENT_TYPE_SYSTEM_SUSPENDED);
}

/**
 * @brief System 恢复测试回调。
 *
 * 模拟系统恢复完成，
 * 直接发送 SYSTEM_RESUMED Event。
 */
static esp_err_t app_system_test_resume(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    return app_system_test_emit_event(
        ctx,
        meta,
        APP_CORE_EVENT_TYPE_SYSTEM_RESUMED);
}

/**
 * @brief 提交并处理一个 System Request。
 *
 * 一次完整处理需要两轮 Dispatcher：
 *
 * 第一轮：
 *
 * 1. Dispatcher 取出 Request；
 * 2. System Domain 处理 Request；
 * 3. FSM 生成 Action；
 * 4. Action Engine 执行假的 Runtime；
 * 5. 假 Runtime 发送 Event。
 *
 * 第二轮：
 *
 * 1. Dispatcher 取出 Event；
 * 2. Action Engine 完成 Action；
 * 3. FSM 更新 System 状态；
 * 4. Domain 将状态同步到 State Store。
 *
 * @param dispatcher Dispatcher。
 * @param state_store 状态存储对象。
 * @param request_type 要测试的 Request 类型。
 * @param request_id 测试 Request ID。
 * @param expected_state 预期的 System 状态。
 * @return ESP_OK 表示测试通过。
 */

static esp_err_t app_system_test_process_request(
    app_core_dispatcher_t *dispatcher,
    app_core_state_store_t *state_store,
    app_core_request_type_t request_type,
    app_core_request_id_t request_id,
    app_core_system_state_t expected_state)
{
    app_core_request_t request;
    app_core_state_snapshot_t snapshot;
    esp_err_t result;
    uint8_t index;
    if (dispatcher == NULL ||
        state_store == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    app_core_request_init(&request, request_type, request_id, APP_CORE_INVALID_REQUEST_ID, APP_CORE_SOURCE_TEST, APP_CORE_SCOPE_SYSTEM);

    result = app_core_dispatcher_submit_request(dispatcher, &request);
    if (result != ESP_OK)
    {
        return result;
    }
    /**
     * 第一轮处理 Request 和 Action，
     * 第二轮处理 Runtime 产生的 Event。
     */
    for (index = 0U;
         index < 2U;
         index++)
    {
        result =
            app_core_dispatcher_process_once(
                dispatcher);

        if (result != ESP_OK)
        {
            return result;
        }
    }

    result =
        app_core_state_store_read(
            state_store,
            &snapshot);

    if (result != ESP_OK)
    {
        return result;
    }

    ESP_LOGI(
        TAG,
        "request=%s, state=%s",
        app_core_request_type_to_string(
            request_type),
        app_core_system_state_to_string(
            snapshot.system_state));

    if (snapshot.system_state !=
        expected_state)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

void app_system_test_run(void)
{
    app_core_state_store_t state_store = {0};
    app_core_dispatcher_t dispatcher = {0};
    app_core_runtime_t runtime = {0};
    app_system_controller_t controller = {0};
    app_system_domain_t domain = {0};
    app_system_test_context_t test_context = {
        .dispatcher = &dispatcher,
    };
    esp_err_t result;
    /*
     * 第一步：初始化 State Store。
     */

    result = app_core_state_store_init(&state_store);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "state store init failed : %s", esp_err_to_name(result));

        goto cleanup;
    }
    /*
     * 第二步：初始化 Dispatcher。
     */

    result = app_core_dispatcher_init(&dispatcher, 8U, 8U);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "dispatcher  store init failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第三步：将 State Store 绑定到 Dispatcher。
     */
    result = app_core_dispatcher_bind_state_store(&dispatcher, &state_store);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "dispatcher bind state store failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第四步：配置假的 System Runtime。
     */
    runtime.system_ctx = &test_context;
    runtime.system_initialize = app_system_test_initialize;
    runtime.system_suspend = app_system_test_suspend;
    runtime.system_resume = app_system_test_resume;

    /*
     * 第五步：初始化 System Controller。
     *
     * Action Engine 产生失败 Event 时，
     * 通过 Dispatcher 重新进入 Event 队列。
     */
    result = app_system_controller_init(&controller, &runtime, 4U, app_core_dispatcher_emit_event, &dispatcher);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "system controller init failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第六步：初始化 System Domain。
     */
    result = app_system_domain_init(&domain, &controller, &state_store);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "system domain init failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第七步：将 System Domain 注册到 Dispatcher。
     */
    result = app_core_dispatcher_register_domain(&dispatcher, app_system_domain_get_handler(&domain));

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "system domain register failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第八步：测试系统初始化。
     *
     * UNKNOWN → INITIALIZING → READY
     */
    result = app_system_test_process_request(&dispatcher, &state_store, APP_CORE_REQUEST_TYPE_SYSTEM_INITIALIZE, 1U, APP_CORE_SYSTEM_STATE_READY);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "system initialize failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第九步：测试系统挂起。
     *
     * READY → SUSPENDING → SUSPENDED
     */
    result = app_system_test_process_request(&dispatcher, &state_store, APP_CORE_REQUEST_TYPE_SYSTEM_SUSPEND, 2U, APP_CORE_SYSTEM_STATE_SUSPENDED);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "system suspend failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第十步：测试系统恢复。
     *
     * SUSPENDED → RESUMING → READY
     */
    result = app_system_test_process_request(&dispatcher, &state_store, APP_CORE_REQUEST_TYPE_SYSTEM_RESUME, 3U, APP_CORE_SYSTEM_STATE_READY);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "system resume failed : %s", esp_err_to_name(result));

        goto cleanup;
    }
    ESP_LOGI(TAG, "system test success");

cleanup:
    /*
     * Dispatcher 中保存的是 Domain Handler 的副本，
     * 因此释放顺序应当是：
     *
     * 1. Dispatcher；
     * 2. Domain；
     * 3. Controller；
     * 4. State Store。
     */
    app_core_dispatcher_deinit(&dispatcher);
    app_system_domain_deinit(&domain);
    app_system_controller_deinit(&controller);
    app_core_state_store_deinit(&state_store);
}