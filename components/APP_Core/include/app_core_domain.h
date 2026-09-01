#ifndef APP_CORE_DOMAIN_H
#define APP_CORE_DOMAIN_H

#include <stdbool.h>

#include "app_core_event.h"
#include "app_core_request.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 判断 Domain 是否需要处理某个 Request。
     *
     * @param ctx Domain 私有上下文。
     * @param request 要判断的 Request。
     * @return true 表示该 Domain 负责处理，false 表示不处理。
     */
    typedef bool (*app_core_domain_match_request_fn)(
        void *ctx,
        const app_core_request_t *request);

    /**
     * @brief 处理一个 Request。
     *
     * @param ctx Domain 私有上下文。
     * @param request 要处理的 Request。
     * @return ESP_OK 表示处理成功，否则返回错误码。
     */
    typedef esp_err_t (*app_core_domain_handle_request_fn)(
        void *ctx,
        const app_core_request_t *request);

    /**
     * @brief 判断 Domain 是否需要处理某个 Event。
     *
     * @param ctx Domain 私有上下文。
     * @param event 要判断的 Event。
     * @return true 表示该 Domain 负责处理，false 表示不处理。
     */
    typedef bool (*app_core_domain_match_event_fn)(
        void *ctx,
        const app_core_event_t *event);

    /**
     * @brief 处理一个 Event。
     *
     * @param ctx Domain 私有上下文。
     * @param event 要处理的 Event。
     * @return ESP_OK 表示处理成功，否则返回错误码。
     */
    typedef esp_err_t (*app_core_domain_handle_event_fn)(
        void *ctx,
        const app_core_event_t *event);

    /**
     * @brief 执行 Domain 的周期性处理。
     *
     * 主要用于处理异步动作结果、超时和后台流程。
     *
     * @param ctx Domain 私有上下文。
     * @return ESP_OK 表示处理成功，否则返回错误码。
     */
    typedef esp_err_t (*app_core_domain_process_fn)(
        void *ctx);

    /**
     * @brief Domain 统一处理器。
     *
     * Dispatcher 通过该结构体与不同业务域交互。
     * 每个业务域可以根据需要实现 Request、Event 或周期处理回调。
     */
    typedef struct
    {
        /** Domain 的私有上下文。 */
        void *ctx;

        /** 判断是否处理 Request 的回调。 */
        app_core_domain_match_request_fn match_request;

        /** 处理 Request 的回调。 */
        app_core_domain_handle_request_fn handle_request;

        /** 判断是否处理 Event 的回调。 */
        app_core_domain_match_event_fn match_event;

        /** 处理 Event 的回调。 */
        app_core_domain_handle_event_fn handle_event;

        /** 执行周期性处理的回调。 */
        app_core_domain_process_fn process_once;

    } app_core_domain_handler_t;

    /**
     * @brief 初始化 Domain 处理器。
     *
     * 该函数只负责填写上下文和回调函数，
     * 不执行任何具体业务逻辑。
     *
     * @param handler 要初始化的 Domain 处理器。
     * @param ctx Domain 私有上下文。
     * @param match_request Request 判断回调。
     * @param handle_request Request 处理回调。
     * @param match_event Event 判断回调。
     * @param handle_event Event 处理回调。
     * @param process_once 周期处理回调。
     */
    void app_core_domain_handler_init(
        app_core_domain_handler_t *handler,
        void *ctx,
        app_core_domain_match_request_fn match_request,
        app_core_domain_handle_request_fn handle_request,
        app_core_domain_match_event_fn match_event,
        app_core_domain_handle_event_fn handle_event,
        app_core_domain_process_fn process_once);

    /**
     * @brief 检查 Domain 处理器是否有效。
     *
     * Request 的判断回调和处理回调必须成对存在；
     * Event 的判断回调和处理回调也必须成对存在。
     *
     * @param handler 要检查的 Domain 处理器。
     * @return true 表示有效，false 表示无效。
     */
    bool app_core_domain_handler_is_valid(
        const app_core_domain_handler_t *handler);

#ifdef __cplusplus
}
#endif

#endif