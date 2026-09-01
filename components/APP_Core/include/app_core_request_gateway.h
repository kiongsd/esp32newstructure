#ifndef APP_CORE_REQUEST_GATEWAY_H
#define APP_CORE_REQUEST_GATEWAY_H

#include "app_core_request.h"
#include "app_core_state.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Request 提交回调。
     *
     * Gateway 不直接依赖 Dispatcher，
     * 而是通过该回调将 Request 转交给实际接收者。
     */
    typedef esp_err_t (*app_core_request_gateway_submit_fn)(
        void *ctx,
        const app_core_request_t *request);

    /**
     * @brief 状态读取回调。
     *
     * Gateway 通过该回调从实际的 State Store 中读取状态。
     */
    typedef esp_err_t (*app_core_request_gateway_read_state_fn)(
        void *ctx,
        app_core_state_snapshot_t *snapshot);

    /**
     * @brief 初始化 Request Gateway。
     *
     * 初始化 Gateway 的内部状态，并清空已有绑定。
     *
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_core_request_gateway_init(void);

    /**
     * @brief 释放 Request Gateway。
     *
     * 如果 submit_ctx 为空，则强制解除当前绑定。
     * 如果 submit_ctx 不为空，则只有绑定对象一致时才解除绑定。
     *
     * @param submit_ctx 当前绑定的提交上下文。
     */
    void app_core_request_gateway_deinit(
        void *submit_ctx);

    /**
     * @brief 绑定 Request 接收者和状态读取者。
     *
     * 通常在 Dispatcher 和 State Store 初始化完成后调用。
     *
     * @param submit Request 提交回调。
     * @param submit_ctx Request 提交上下文。
     * @param read_state 状态读取回调。
     * @param read_state_ctx 状态读取上下文。
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_core_request_gateway_bind(
        app_core_request_gateway_submit_fn submit,
        void *submit_ctx,
        app_core_request_gateway_read_state_fn read_state,
        void *read_state_ctx);

    /**
     * @brief 解除 Request Gateway 绑定。
     *
     * @param submit_ctx 要解除绑定的提交上下文。
     */
    void app_core_request_gateway_unbind(
        void *submit_ctx);

    /**
     * @brief 向 App Core 提交 Request。
     *
     * 如果 Request 没有设置 request_id，
     * Gateway 会自动分配一个新的请求编号。
     *
     * @param request 要提交的 Request。
     * @return ESP_OK 表示提交成功，否则返回错误码。
     */
    esp_err_t app_core_request_gateway_submit(
        const app_core_request_t *request);

    /**
     * @brief 读取当前 App Core 状态。
     *
     * @param snapshot 用于接收状态快照的对象。
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_core_request_gateway_read_state(
        app_core_state_snapshot_t *snapshot);

    /**
     * @brief 获取下一个 Request ID。
     *
     * @return 新的请求编号。
     *         Gateway 未初始化时返回无效请求编号。
     */
    app_core_request_id_t
    app_core_request_gateway_next_request_id(void);

#ifdef __cplusplus
}
#endif

#endif