#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"

#include "app_core_dispatcher.h"
#include "app_core_event.h"
#include "app_core_request.h"
#include "app_core_runtime.h"
#include "app_core_state_store.h"

#include "app_ui_controller.h"
#include "app_ui_domain.h"

static const char *TAG = "APP_UI_TEST";

/**
 * @brief ui 测试上下文。
 *
 * 当前只保存 Dispatcher，
 * 方便假的 Runtime 回调发送 Event。
 */
typedef struct
{
    /** 用于接收测试 Event 的 Dispatcher。 */
    app_core_dispatcher_t *dispatcher;

} app_ui_test_context_t;

/**
 * @brief 发送一个假的 ui Event。
 *
 * 该函数模拟 Driver 或 Service 完成动作后，
 * 向 Dispatcher 发送执行结果。
 *
 * @param ctx 测试上下文。
 * @param meta 原始 Action 的消息元数据。
 * @param page 页面显示完成事件对应的页面。
 * @return ESP_OK 表示发送成功。
 */
static esp_err_t app_ui_test_emit_event(void *ctx, const app_core_message_meta_t *meta, app_core_lvgl_page_t page)
{
    app_ui_test_context_t *test_context;
    app_core_event_t event;

    if (ctx == NULL || meta == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    test_context = (app_ui_test_context_t *)ctx;

    if (test_context->dispatcher == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    app_core_event_init(&event, APP_CORE_EVENT_TYPE_PAGE_SHOWN, meta->request_id, meta->parent_request_id, APP_CORE_SOURCE_DRIVER, APP_CORE_SCOPE_LVGL);

    event.data.page = page;

    return app_core_dispatcher_emit_event(test_context->dispatcher, &event);
}

static esp_err_t app_ui_test_show_page(void *ctx, const app_core_message_meta_t *meta, app_core_lvgl_page_t page)
{
    return app_ui_test_emit_event(ctx, meta, page);
}

static esp_err_t app_ui_test_process_page(app_core_dispatcher_t *dispatcher, app_core_state_store_t *state_store, app_core_request_id_t request_id, app_core_lvgl_page_t page)
{
    app_core_request_t request = {0};
    app_core_state_snapshot_t snapshot = {0};
    esp_err_t result;
    uint8_t index;
    if (dispatcher == NULL || state_store == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    app_core_request_init(&request,
                          APP_CORE_REQUEST_TYPE_SHOW_PAGE,
                          request_id,
                          APP_CORE_INVALID_REQUEST_ID,
                          APP_CORE_SOURCE_TEST,
                          APP_CORE_SCOPE_LVGL);

    request.data.page = page;
    result = app_core_dispatcher_submit_request(dispatcher, &request);

    if (result != ESP_OK)
    {
        return result;
    }

    for (index = 0U; index < 2U; index++)
    {
        result = app_core_dispatcher_process_once(dispatcher);
        if (result != ESP_OK)
        {
            return result;
        }
    }
    result = app_core_state_store_read(state_store, &snapshot);

    if (result != ESP_OK)
    {
        return result;
    }

    ESP_LOGI(TAG, "page=%s,transitioning=%s", app_core_lvgl_page_to_string(snapshot.lvgl_page), snapshot.lvgl_transitioning ? "true" : "false");
    if (snapshot.lvgl_page != page || snapshot.lvgl_transitioning)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

void app_ui_test_run(void)
{
    app_core_state_store_t state_store = {0};
    app_core_dispatcher_t dispatcher = {0};
    app_core_runtime_t runtime = {0};
    app_ui_controller_t controller = {0};
    app_ui_domain_t domain = {0};
    app_ui_test_context_t test_context = {
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
     * 第四步：配置假的 ui Runtime。
     */
    runtime.display_ctx = &test_context;
    runtime.show_page = app_ui_test_show_page;

    /*
     * 第五步：初始化 ui Controller。
     *
     * Action Engine 产生失败 Event 时，
     * 通过 Dispatcher 重新进入 Event 队列。
     */
    result = app_ui_controller_init(&controller, &runtime, 4U, app_core_dispatcher_emit_event, &dispatcher);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "ui controller init failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第六步：初始化 ui Domain。
     */
    result = app_ui_domain_init(&domain, &controller, &state_store);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "ui domain init failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第七步：将 ui Domain 注册到 Dispatcher。
     */
    result = app_core_dispatcher_register_domain(&dispatcher, app_ui_domain_get_handler(&domain));

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "ui domain register failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第八步：测试页面切换到主菜单。
     *
     * Request → Action → Runtime → Event → State Store
     */

    result = app_ui_test_process_page(
        &dispatcher,
        &state_store,
        1U,
        APP_CORE_LVGL_PAGE_MENU);

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "show menu page failed: %s",
            esp_err_to_name(result));

        goto cleanup;
    }

    result = app_ui_test_process_page(
        &dispatcher,
        &state_store,
        2U,
        APP_CORE_LVGL_PAGE_SETTINGS);

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "show settings page failed: %s",
            esp_err_to_name(result));

        goto cleanup;
    }

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
    app_ui_domain_deinit(&domain);
    app_ui_controller_deinit(&controller);
    app_core_state_store_deinit(&state_store);
}
