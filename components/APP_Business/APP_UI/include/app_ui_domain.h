#ifndef APP_UI_DOMAIN_H
#define APP_UI_DOMAIN_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "app_core_domain.h"
#include "app_core_state_store.h"

#include "app_ui_controller.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief UI 业务 Domain。
     *
     * Domain 负责将 UI Controller 适配为 APP_Core
     * Dispatcher 所要求的通用 Domain 接口。
     */
    typedef struct
    {
        /** UI 业务控制器。 */
        app_ui_controller_t *controller;

        /** 用于保存 UI 页面状态的公共状态存储对象。 */
        app_core_state_store_t *state_store;

        /** 提供给 Dispatcher 使用的通用 Domain 回调表。 */
        app_core_domain_handler_t handler;

        /** Domain 是否已经完成初始化。 */
        bool initialized;

    } app_ui_domain_t;

    /**
     * @brief 初始化 UI Domain。
     *
     * 初始化时会绑定 Controller 和 State Store，
     * 并生成可注册到 Dispatcher 的回调表。
     *
     * @param domain 要初始化的 UI Domain。
     * @param controller UI Controller。
     * @param state_store 公共状态存储对象。
     * @return ESP_OK 表示初始化成功，否则返回错误码。
     */
    esp_err_t app_ui_domain_init(
        app_ui_domain_t *domain,
        app_ui_controller_t *controller,
        app_core_state_store_t *state_store);

    /**
     * @brief 释放 UI Domain。
     *
     * 该函数只清空 Domain 自身的绑定关系，
     * 不释放 Controller 和 State Store。
     *
     * @param domain 要释放的 UI Domain。
     */
    void app_ui_domain_deinit(
        app_ui_domain_t *domain);

    /**
     * @brief 获取 UI Domain 的通用回调表。
     *
     * 返回值可以直接传给：
     * app_core_dispatcher_register_domain()。
     *
     * @param domain UI Domain。
     * @return 有效的 Domain 回调表；未初始化时返回 NULL。
     */
    const app_core_domain_handler_t *
    app_ui_domain_get_handler(
        const app_ui_domain_t *domain);


#ifdef __cplusplus
}
#endif

#endif
