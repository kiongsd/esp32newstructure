#include <stddef.h>

#include "app_core_domain.h"

/**
 * @brief 初始化 Domain 处理器。
 *
 * 将 Domain 的上下文和回调函数统一保存到处理器对象中。
 */
void app_core_domain_handler_init(
    app_core_domain_handler_t *handler,
    void *ctx,
    app_core_domain_match_request_fn match_request,
    app_core_domain_handle_request_fn handle_request,
    app_core_domain_match_event_fn match_event,
    app_core_domain_handle_event_fn handle_event,
    app_core_domain_process_fn process_once)
{
    if (handler == NULL)
    {
        return;
    }

    handler->ctx = ctx;
    handler->match_request = match_request;
    handler->handle_request = handle_request;
    handler->match_event = match_event;
    handler->handle_event = handle_event;
    handler->process_once = process_once;
}

/**
 * @brief 检查 Domain 处理器是否有效。
 *
 * 一个 Domain 至少需要实现以下一种能力：

 * 1. Request 判断和处理；
 * 2. Event 判断和处理；
 * 3. 周期性处理。
 */
bool app_core_domain_handler_is_valid(
    const app_core_domain_handler_t *handler)
{
    bool request_handler_valid;
    bool event_handler_valid;
    bool process_handler_valid;

    if (handler == NULL)
    {
        return false;
    }

    request_handler_valid =
        handler->match_request != NULL &&
        handler->handle_request != NULL;

    event_handler_valid =
        handler->match_event != NULL &&
        handler->handle_event != NULL;

    process_handler_valid =
        handler->process_once != NULL;

    if ((handler->match_request == NULL) !=
        (handler->handle_request == NULL))
    {
        return false;
    }

    if ((handler->match_event == NULL) !=
        (handler->handle_event == NULL))
    {
        return false;
    }

    return request_handler_valid ||
           event_handler_valid ||
           process_handler_valid;
}