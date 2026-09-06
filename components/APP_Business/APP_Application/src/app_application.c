#include "app_application.h"

#include <stddef.h>
#include "app_core_action_engine.h"

static esp_err_t app_application_submit_request(
    void *ctx,
    const app_core_request_t *request)
{
    app_application_t *application;
    if (ctx == NULL || request == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    application = (app_application_t *)ctx;
    return app_core_dispatcher_submit_request(&application->dispatcher, request);
}

static esp_err_t app_application_read_state(void *ctx, app_core_state_snapshot_t *snapshot)
{
    app_application_t *application;
    if (ctx == NULL || snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    application = (app_application_t *)ctx;
    return app_core_dispatcher_read_state(&application->dispatcher, snapshot);
}

static esp_err_t app_application_register_domains(app_application_t *application)
{
    esp_err_t result;
    result = app_core_dispatcher_register_domain(&application->dispatcher, app_system_domain_get_handler(&application->system_domain));

    if (result != ESP_OK)
    {
        return result;
    }
    result = app_core_dispatcher_register_domain(&application->dispatcher, app_capture_domain_get_handler(&application->capture_domain));

    if (result != ESP_OK)
    {
        return result;
    }

    result = app_core_dispatcher_register_domain(&application->dispatcher, app_storage_domain_get_handler(&application->storage_domain));

    if (result != ESP_OK)
    {
        return result;
    }

    result =
        app_core_dispatcher_register_domain(
            &application->dispatcher,
            app_ui_domain_get_handler(
                &application->ui_domain));

    return result;
}

/**
 * @brief 初始化统一 Application。
 */

esp_err_t app_application_init(app_application_t *application, const app_core_runtime_t *runtime)
{
    esp_err_t result;
    if (application == NULL || runtime == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (application->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    *application = (app_application_t){0};

    /*
     * 保存 Runtime 回调表。
     *
     * Controller 内部保存的是 Runtime 指针，
     * 因此后续统一使用 application->runtime。
     */
    application->runtime = *runtime;

    /*
     * 第一步：初始化公共 State Store。
     */
    result = app_core_state_store_init(&application->state_store);

    if (result != ESP_OK)
    {
        goto cleanup;
    }

    /*
     * 第二步：初始化统一 Dispatcher。
     */
    result = app_core_dispatcher_init(&application->dispatcher, APP_APPLICATION_REQUEST_QUEUE_LENGTH, APP_APPLICATION_EVENT_QUEUE_LENGTH);
    if (result != ESP_OK)
    {
        goto cleanup;
    }

    /*
     * 第三步：将 State Store 绑定到 Dispatcher。
     */
    result = app_core_dispatcher_bind_state_store(&application->dispatcher, &application->state_store);

    if (result != ESP_OK)
    {
        goto cleanup;
    }

    /*
     * 第四步：初始化 System Controller。
     */

    result = app_system_controller_init(&application->system_controller, &application->runtime, APP_APPLICATION_ACTION_QUEUE_LENGTH, app_core_dispatcher_emit_event, &application->dispatcher);
    if (result != ESP_OK)
    {
        goto cleanup;
    }

    /*
     * 第五步：初始化 Capture Controller。
     */
    result =
        app_capture_controller_init(
            &application->capture_controller,
            &application->runtime,
            APP_APPLICATION_ACTION_QUEUE_LENGTH,
            app_core_dispatcher_emit_event,
            &application->dispatcher);

    if (result != ESP_OK)
    {
        goto cleanup;
    }
    /*
     * 第六步：初始化 Storage Controller。
     */
    result =
        app_storage_controller_init(
            &application->storage_controller,
            &application->runtime,
            APP_APPLICATION_ACTION_QUEUE_LENGTH,
            app_core_dispatcher_emit_event,
            &application->dispatcher);

    if (result != ESP_OK)
    {
        goto cleanup;
    }

    /*
     * 第七步：初始化 UI Controller。
     */
    result =
        app_ui_controller_init(
            &application->ui_controller,
            &application->runtime,
            APP_APPLICATION_ACTION_QUEUE_LENGTH,
            app_core_dispatcher_emit_event,
            &application->dispatcher);

    if (result != ESP_OK)
    {
        goto cleanup;
    }

    /*
     * 第八步：初始化 System Domain。
     */
    result =
        app_system_domain_init(
            &application->system_domain,
            &application->system_controller,
            &application->state_store);

    if (result != ESP_OK)
    {
        goto cleanup;
    }

    /*
     * 第九步：初始化 Capture Domain。
     */
    result =
        app_capture_domain_init(
            &application->capture_domain,
            &application->capture_controller,
            &application->state_store);

    if (result != ESP_OK)
    {
        goto cleanup;
    }

    /*
     * 第十步：初始化 Storage Domain。
     */
    result =
        app_storage_domain_init(
            &application->storage_domain,
            &application->storage_controller,
            &application->state_store);

    if (result != ESP_OK)
    {
        goto cleanup;
    }

    /*
     * 第十一步：初始化 UI Domain。
     */
    result =
        app_ui_domain_init(
            &application->ui_domain,
            &application->ui_controller,
            &application->state_store);

    if (result != ESP_OK)
    {
        goto cleanup;
    }

    /*
     * 第十二步：将四个 Domain 注册到统一 Dispatcher。
     */
    result =
        app_application_register_domains(
            application);

    if (result != ESP_OK)
    {
        goto cleanup;
    }

    /*
     * 第十三步：初始化 App Core Task 对象。
     *
     * 此时只初始化控制对象，
     * 还没有真正创建 FreeRTOS 任务。
     */
    result =
        app_core_task_init(
            &application->core_task,
            &application->dispatcher,
            APP_APPLICATION_TASK_STACK_SIZE,
            APP_APPLICATION_TASK_PRIORITY,
            APP_APPLICATION_POLL_INTERVAL_MS);

    if (result != ESP_OK)
    {
        goto cleanup;
    }

    application->initialized =
        true;

    return ESP_OK;

cleanup:
    app_core_task_deinit(
        &application->core_task);

    app_core_dispatcher_deinit(
        &application->dispatcher);

    app_ui_domain_deinit(
        &application->ui_domain);

    app_storage_domain_deinit(
        &application->storage_domain);

    app_capture_domain_deinit(
        &application->capture_domain);

    app_system_domain_deinit(
        &application->system_domain);

    app_ui_controller_deinit(
        &application->ui_controller);

    app_storage_controller_deinit(
        &application->storage_controller);

    app_capture_controller_deinit(
        &application->capture_controller);

    app_system_controller_deinit(
        &application->system_controller);

    app_core_state_store_deinit(
        &application->state_store);

    *application =
        (app_application_t){0};

    return result;
}

/**
 * @brief 启动统一 Application。
 */
esp_err_t app_application_start(app_application_t *application)
{
    esp_err_t result;
    if (application == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!application->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (application->gateway_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /*第一步：初始化 Request Gateway。*/
    result =
        app_core_request_gateway_init();

    if (result != ESP_OK)
    {
        return result;
    }

    application->gateway_initialized =
        true;

    /*
     * 第二步：绑定 Request 提交和状态读取接口。
     */
    result =
        app_core_request_gateway_bind(
            app_application_submit_request,
            application,
            app_application_read_state,
            application);

    if (result != ESP_OK)
    {
        app_core_request_gateway_deinit(
            application);

        application->gateway_initialized =
            false;

        return result;
    }

    /*
     * 第三步：启动 App Core 后台任务。
     */
    result =
        app_core_task_start(
            &application->core_task);

    if (result != ESP_OK)
    {
        app_core_request_gateway_deinit(
            application);

        application->gateway_initialized =
            false;

        return result;
    }

    return ESP_OK;
}

/**
 * @brief 请求停止统一 Application。
 */
esp_err_t app_application_stop(
    app_application_t *application)
{
    if (application == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!application->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return app_core_task_request_stop(
        &application->core_task);
}

/**
 * @brief 释放统一 Application。
 */
esp_err_t app_application_deinit(
    app_application_t *application)
{

    if (application == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!application->initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * App Core Task 仍在运行时，
     * 不能释放 Dispatcher 和 Domain。
     */

    if (application->core_task.running || application->core_task.task_handle != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (application->gateway_initialized)
    {
        app_core_request_gateway_deinit(application);
        application->gateway_initialized = false;
    }
    /*
     * App Core Task 已经停止后，
     * 释放 Task 控制对象。
     */
    app_core_task_deinit(
        &application->core_task);
    /*
     * 释放顺序：
     *
     * 1. Dispatcher；
     * 2. Domain；
     * 3. Controller；
     * 4. State Store。
     */

    app_core_dispatcher_deinit(&application->dispatcher);

    app_ui_domain_deinit(&application->ui_domain);

    app_storage_domain_deinit(&application->storage_domain);

    app_capture_domain_deinit(&application->capture_domain);

    app_system_domain_deinit(&application->system_domain);

    app_ui_controller_deinit(&application->ui_controller);

    app_storage_controller_deinit(&application->storage_controller);

    app_capture_controller_deinit(&application->capture_controller);

    app_system_controller_deinit(&application->system_controller);

    app_core_state_store_deinit(&application->state_store);

    *application = (app_application_t){0};

    return ESP_OK;
}