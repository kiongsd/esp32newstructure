#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"

#include "app_core_dispatcher.h"
#include "app_core_event.h"
#include "app_core_request.h"
#include "app_core_runtime.h"
#include "app_core_state_store.h"

#include "app_capture_controller.h"
#include "app_capture_domain.h"

static const char *TAG = "APP_CAPTURE_TEST";

/**
 * @brief capture 测试上下文。
 *
 * 当前只保存 Dispatcher，
 * 方便假的 Runtime 回调发送 Event。
 */
typedef struct
{
    /** 用于接收测试 Event 的 Dispatcher。 */
    app_core_dispatcher_t *dispatcher;

} app_capture_test_context_t;

/**
 * @brief 发送一个假的 capture Event。
 *
 * 该函数模拟 Driver 或 Service 完成动作后，
 * 向 Dispatcher 发送执行结果。
 *
 * @param ctx 测试上下文。
 * @param meta 原始 Action 的消息元数据。
 * @param event_type 要发送的 Event 类型。
 * @return ESP_OK 表示发送成功。
 */
static esp_err_t app_capture_test_emit_event(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_event_type_t event_type,
    app_core_job_id_t job_id,
    app_core_photo_handle_t photo_handle)
{
    app_capture_test_context_t *test_context;
    app_core_event_t event;
    if (ctx == NULL || meta == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    test_context = (app_capture_test_context_t *)ctx;

    if (test_context->dispatcher == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    app_core_event_init(&event, event_type, meta->request_id, meta->parent_request_id, APP_CORE_SOURCE_DRIVER, APP_CORE_SCOPE_CAPTURE);
    if (event_type ==
        APP_CORE_EVENT_TYPE_CAMERA_PREPARED)
    {
        event.data.job_id =
            job_id;
    }
    else if (event_type ==
                 APP_CORE_EVENT_TYPE_CAMERA_CAPTURED ||
             event_type ==
                 APP_CORE_EVENT_TYPE_PHOTO_SAVED)
    {
        event.data.photo_handle =
            photo_handle;
    }

    return app_core_dispatcher_emit_event(
        test_context->dispatcher,
        &event);
}

/**
 * @brief 模拟 Camera 启动预览完成。
 */
static esp_err_t app_capture_test_camera_start_preview(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    return app_capture_test_emit_event(
        ctx,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_PREVIEW_STARTED,
        APP_CORE_INVALID_JOB_ID,
        APP_CORE_INVALID_PHOTO_HANDLE);
}

/**
 * @brief 模拟 Camera 完成拍照准备。
 */
static esp_err_t app_capture_test_camera_prepare_photo(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_job_id_t job_id)
{
    return app_capture_test_emit_event(
        ctx,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_PREPARED,
        job_id,
        APP_CORE_INVALID_PHOTO_HANDLE);
}

/**
 * @brief 模拟 Camera 完成拍照并返回照片句柄。
 */
static esp_err_t app_capture_test_camera_capture(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_job_id_t job_id)
{
    (void)job_id;

    return app_capture_test_emit_event(
        ctx,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_CAPTURED,
        APP_CORE_INVALID_JOB_ID,
        1001U);
}

/**
 * @brief 模拟 Camera 停止预览。
 */
static esp_err_t app_capture_test_camera_stop(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    return app_capture_test_emit_event(
        ctx,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_STOPPED,
        APP_CORE_INVALID_JOB_ID,
        APP_CORE_INVALID_PHOTO_HANDLE);
}

/**
 * @brief 模拟 Camera 完成自动对焦。
 */
static esp_err_t app_capture_test_camera_focus(
    void *ctx,
    const app_core_message_meta_t *meta)
{
    return app_capture_test_emit_event(
        ctx,
        meta,
        APP_CORE_EVENT_TYPE_CAMERA_FOCUSED,
        APP_CORE_INVALID_JOB_ID,
        APP_CORE_INVALID_PHOTO_HANDLE);
}

/**
 * @brief 模拟照片保存完成。
 */
static esp_err_t app_capture_test_save_photo(
    void *ctx,
    const app_core_message_meta_t *meta,
    app_core_photo_handle_t photo_handle)
{
    return app_capture_test_emit_event(
        ctx,
        meta,
        APP_CORE_EVENT_TYPE_PHOTO_SAVED,
        APP_CORE_INVALID_JOB_ID,
        photo_handle);
}

/**
 * @brief 提交并处理一个 capture Request。
 *
 * 根据 Request 类型，处理轮数由 process_rounds 指定：
 *
 * 预览、停止和对焦通常需要两轮 Dispatcher：
 *
 * 1. Dispatcher 处理 Request，FSM 生成 Action；
 * 2. Action Engine 执行假的 Runtime，处理 Runtime 产生的 Event。
 *
 * 完整拍照需要四轮 Dispatcher：
 *
 * 1. 准备照片；
 * 2. 拍照；
 * 3. 保存照片；
 * 4. 完成保存事件并进入 SUCCEEDED。
 *
 * @param dispatcher Dispatcher。
 * @param state_store 状态存储对象。
 * @param request_type 要测试的 Request 类型。
 * @param request_id 测试 Request ID。
 * @param expected_state 预期的 capture 状态。
 * @param process_rounds Dispatcher 处理轮数。
 * @return ESP_OK 表示测试通过。
 */

static esp_err_t app_capture_test_process_request(
    app_core_dispatcher_t *dispatcher,
    app_core_state_store_t *state_store,
    app_core_request_type_t request_type,
    app_core_request_id_t request_id,
    app_core_capture_state_t expected_state,
    uint8_t process_rounds)
{
    app_core_request_t request;
    app_core_state_snapshot_t snapshot;
    esp_err_t result;
    uint8_t index;
    if (dispatcher == NULL ||
        state_store == NULL ||
        process_rounds == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    app_core_request_init(&request, request_type, request_id, APP_CORE_INVALID_REQUEST_ID, APP_CORE_SOURCE_TEST, APP_CORE_SCOPE_CAPTURE);

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
         index < process_rounds;
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
        app_core_capture_state_to_string(
            snapshot.capture_state));

    if (snapshot.capture_state !=
        expected_state)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

/**
 * @brief 执行 APP_Capture 集成测试。
 *
 * 使用假的 Camera 和照片存储 Runtime，
 * 验证预览、准备拍照、完整拍照、对焦和停止流程。
 */
void app_capture_test_run(void)
{
    app_core_state_store_t state_store = {0};
    app_core_dispatcher_t dispatcher = {0};
    app_core_runtime_t runtime = {0};
    app_capture_controller_t controller = {0};
    app_capture_domain_t domain = {0};
    app_capture_test_context_t test_context = {
        .dispatcher = &dispatcher,
    };
    app_core_state_snapshot_t snapshot;
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
     * 第四步：配置假的 capture Runtime。
     */
    runtime.camera_ctx =
        &test_context;

    runtime.camera_start_preview =
        app_capture_test_camera_start_preview;

    runtime.camera_prepare_photo =
        app_capture_test_camera_prepare_photo;

    runtime.camera_capture =
        app_capture_test_camera_capture;

    runtime.camera_stop =
        app_capture_test_camera_stop;

    runtime.camera_focus =
        app_capture_test_camera_focus;

    runtime.photo_storage_ctx =
        &test_context;

    runtime.save_photo =
        app_capture_test_save_photo;

    /*
     * 第五步：初始化 capture Controller。
     *
     * Action Engine 产生失败 Event 时，
     * 通过 Dispatcher 重新进入 Event 队列。
     */
    result = app_capture_controller_init(&controller, &runtime, 4U, app_core_dispatcher_emit_event, &dispatcher);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "capture controller init failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第六步：初始化 capture Domain。
     */
    result = app_capture_domain_init(&domain, &controller, &state_store);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "capture domain init failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第七步：将 capture Domain 注册到 Dispatcher。
     */
    result = app_core_dispatcher_register_domain(&dispatcher, app_capture_domain_get_handler(&domain));

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "capture domain register failed : %s", esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第八步：启动 Camera 预览。
     *
     * REQUESTED → IDLE
     */
    result =
        app_capture_test_process_request(
            &dispatcher,
            &state_store,
            APP_CORE_REQUEST_TYPE_CAPTURE_START_PREVIEW,
            1U,
            APP_CORE_CAPTURE_STATE_IDLE,
            2U);

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "capture start preview failed: %s",
            esp_err_to_name(result));

        goto cleanup;
    }

    result =
        app_core_state_store_read(
            &state_store,
            &snapshot);

    if (result != ESP_OK ||
        !snapshot.preview_active)
    {
        ESP_LOGE(
            TAG,
            "preview state invalid");

        goto cleanup;
    }
    /*
     * 第九步：准备照片。
     *
     * REQUESTED → PREPARING → IDLE
     */
    result =
        app_capture_test_process_request(
            &dispatcher,
            &state_store,
            APP_CORE_REQUEST_TYPE_CAPTURE_PREPARE_PHOTO,
            2U,
            APP_CORE_CAPTURE_STATE_IDLE,
            2U);

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "capture prepare photo failed: %s",
            esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第十步：执行完整拍照流程。
     *
     * IDLE → PREPARING → ACQUIRING
     *      → PROCESSING → SAVING → SUCCEEDED
     */
    result =
        app_capture_test_process_request(
            &dispatcher,
            &state_store,
            APP_CORE_REQUEST_TYPE_CAPTURE,
            3U,
            APP_CORE_CAPTURE_STATE_SUCCEEDED,
            4U);

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "capture photo failed: %s",
            esp_err_to_name(result));

        goto cleanup;
    }

    result =
        app_core_state_store_read(
            &state_store,
            &snapshot);

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "read capture state failed: %s",
            esp_err_to_name(result));

        goto cleanup;
    }

    if (snapshot.last_photo_handle !=
        1001U)
    {
        ESP_LOGE(
            TAG,
            "unexpected photo handle: %lu",
            (unsigned long)snapshot.last_photo_handle);

        goto cleanup;
    }

    ESP_LOGI(
        TAG,
        "capture photo handle: %lu",
        (unsigned long)snapshot.last_photo_handle);

    /*
     * 第十一步：重新启动 Camera 预览，
     * 为自动对焦准备有效前置条件。
     */
    result =
        app_capture_test_process_request(
            &dispatcher,
            &state_store,
            APP_CORE_REQUEST_TYPE_CAPTURE_START_PREVIEW,
            4U,
            APP_CORE_CAPTURE_STATE_IDLE,
            2U);

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "restart preview failed: %s",
            esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第十二步：测试自动对焦。
     *
     * REQUESTED → IDLE
     */
    result =
        app_capture_test_process_request(
            &dispatcher,
            &state_store,
            APP_CORE_REQUEST_TYPE_CAPTURE_FOCUS,
            4U,
            APP_CORE_CAPTURE_STATE_IDLE,
            2U);

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "capture focus failed: %s",
            esp_err_to_name(result));

        goto cleanup;
    }

    /*
     * 第十三步：停止 Camera 预览。
     *
     * REQUESTED → CANCELED
     */
    result =
        app_capture_test_process_request(
            &dispatcher,
            &state_store,
            APP_CORE_REQUEST_TYPE_CAPTURE_STOP,
            5U,
            APP_CORE_CAPTURE_STATE_CANCELED,
            2U);

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "capture stop failed: %s",
            esp_err_to_name(result));

        goto cleanup;
    }

    result =
        app_core_state_store_read(
            &state_store,
            &snapshot);

    if (result != ESP_OK ||
        snapshot.preview_active)
    {
        ESP_LOGE(
            TAG,
            "preview did not stop");

        goto cleanup;
    }

    ESP_LOGI(
        TAG,
        "capture test success");

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
    app_capture_domain_deinit(&domain);
    app_capture_controller_deinit(&controller);
    app_core_state_store_deinit(&state_store);
}
