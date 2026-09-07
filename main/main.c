#include "esp_log.h"

#include "app_application.h"
#include "app_runtime.h"

#include "app_system_test.h"
#include "app_capture_test.h"
#include "app_storage_test.h"
#include "app_ui_test.h"

#include "app_core_request.h"
#include "app_core_request_gateway.h"

static const char *TAG = "MAIN";

/**
 * @brief 全局 Application 对象。
 */
static app_application_t application = {0};

/**
 * @brief Runtime 上下文。
 */
static app_runtime_context_t runtime_context = {0};
/**
 * @brief 应用层集成测试入口。
 *
 * 依次执行 System、Capture、Storage 和 UI 业务测试。
 */

/**
 * @brief 提交系统初始化请求。
 */
static void app_submit_system_initialize_request(void)
{
    app_core_request_t request;

    app_core_request_init(
        &request,
        APP_CORE_REQUEST_TYPE_SYSTEM_INITIALIZE,
        APP_CORE_INVALID_REQUEST_ID,
        APP_CORE_INVALID_REQUEST_ID,
        APP_CORE_SOURCE_SYSTEM,
        APP_CORE_SCOPE_SYSTEM);

    ESP_ERROR_CHECK(
        app_core_request_gateway_submit(
            &request));

    ESP_LOGI(
        TAG,
        "system initialize request submitted");
}

/**
 * @brief 提交初始菜单页面请求。
 */
static void app_submit_menu_page_request(void)
{
    app_core_request_t request;

    app_core_request_init(
        &request,
        APP_CORE_REQUEST_TYPE_SHOW_PAGE,
        APP_CORE_INVALID_REQUEST_ID,
        APP_CORE_INVALID_REQUEST_ID,
        APP_CORE_SOURCE_SYSTEM,
        APP_CORE_SCOPE_LVGL);

    request.data.page =
        APP_CORE_LVGL_PAGE_MENU;

    ESP_ERROR_CHECK(
        app_core_request_gateway_submit(
            &request));

    ESP_LOGI(
        TAG,
        "menu page request submitted");
}

/**
 * @brief 应用程序入口。
 */
void app_main(void)
{
    app_core_runtime_t runtime = {0};

    /*
     * 第一步：初始化 Runtime 回调表。
     */
    ESP_ERROR_CHECK(
        app_runtime_init(
            &runtime,
            &runtime_context));

    /*
     * 第二步：初始化 Application。
     */
    ESP_ERROR_CHECK(
        app_application_init(
            &application,
            &runtime));

    ESP_LOGI(
        TAG,
        "start system test");

    app_system_test_run();

    ESP_LOGI(
        TAG,
        "start capture test");

    app_capture_test_run();

    ESP_LOGI(
        TAG,
        "start storage test");

    app_storage_test_run();

    ESP_LOGI(
        TAG,
        "start ui test");

    app_ui_test_run();

    ESP_LOGI(
        TAG,
        "all business tests finished");
    /*
     * 第三步：绑定 Dispatcher。
     *
     * app_application_init() 内部会复制 Runtime，
     * 因此这里必须在启动 App Core Task 之前绑定 Dispatcher。
     */
    runtime_context.dispatcher =
        &application.dispatcher;

    /*
     * 第四步：启动 Application。
     */
    ESP_ERROR_CHECK(
        app_application_start(
            &application));

    /*
     * 第五步：提交初始业务请求。
     */
    app_submit_system_initialize_request();
    app_submit_menu_page_request();

    ESP_LOGI(
        TAG,
        "application started");
}