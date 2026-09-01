#ifndef APP_CORE_DISPATCHER_H
#define APP_CORE_DISPATCHER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "app_core_domain.h"
#include "app_core_state_store.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Dispatcher 最大支持的 Domain 数量。
 */
#define APP_CORE_DISPATCHER_MAX_DOMAINS 8U

    /**
     * @brief App Core Dispatcher。
     *
     * Dispatcher 是 Request 和 Event 的中央分发器。
     */
    typedef struct
    {
        /** Request 队列。 */
        QueueHandle_t request_queue;

        /** Event 队列。 */
        QueueHandle_t event_queue;

        /** 已注册的业务 Domain。 */
        app_core_domain_handler_t domains[APP_CORE_DISPATCHER_MAX_DOMAINS];

        /** 当前已经注册的 Domain 数量。 */
        uint8_t domain_count;

        /** Dispatcher 关联的状态存储对象。 */
        app_core_state_store_t *state_store;

        /** Dispatcher 是否已经初始化。 */
        bool initialized;

    } app_core_dispatcher_t;

    /**
     * @brief 初始化 Dispatcher。
     *
     * 创建 Request 队列和 Event 队列。
     *
     * @param dispatcher 要初始化的 Dispatcher。
     * @param request_queue_length Request 队列长度。
     * @param event_queue_length Event 队列长度。
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_core_dispatcher_init(
        app_core_dispatcher_t *dispatcher,
        UBaseType_t request_queue_length,
        UBaseType_t event_queue_length);

    /**
     * @brief 释放 Dispatcher。
     *
     * 删除 Request 队列和 Event 队列。
     *
     * @param dispatcher 要释放的 Dispatcher。
     */
    void app_core_dispatcher_deinit(
        app_core_dispatcher_t *dispatcher);

    /**
     * @brief 注册一个 Domain。
     *
     * @param dispatcher Dispatcher 对象。
     * @param domain 要注册的 Domain 处理器。
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_core_dispatcher_register_domain(
        app_core_dispatcher_t *dispatcher,
        const app_core_domain_handler_t *domain);

    /**
     * @brief 提交 Request。
     *
     * Request 会被复制到 Dispatcher 队列中。
     *
     * @param dispatcher Dispatcher 对象。
     * @param request 要提交的 Request。
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_core_dispatcher_submit_request(
        app_core_dispatcher_t *dispatcher,
        const app_core_request_t *request);

    /**
     * @brief 向 Dispatcher 发送 Event。
     *
     * 该函数可以作为 Event 接收回调，
     * 供 Camera、Storage 或其他模块调用。
     *
     * @param ctx Dispatcher 对象。
     * @param event 要发送的 Event。
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_core_dispatcher_emit_event(
        void *ctx,
        const app_core_event_t *event);

    /**
     * @brief 绑定 State Store。
     *
     * 绑定后，Dispatcher 可以提供状态读取接口。
     *
     * @param dispatcher Dispatcher 对象。
     * @param state_store State Store 对象。
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_core_dispatcher_bind_state_store(
        app_core_dispatcher_t *dispatcher,
        app_core_state_store_t *state_store);

    /**
     * @brief 读取当前系统状态。
     *
     * @param dispatcher Dispatcher 对象。
     * @param snapshot 用于接收状态快照的对象。
     * @return ESP_OK 表示成功，否则返回错误码。
     */
    esp_err_t app_core_dispatcher_read_state(
        const app_core_dispatcher_t *dispatcher,
        app_core_state_snapshot_t *snapshot);

    /**
     * @brief 执行一次 Dispatcher 处理。
     *
     * 该函数会尝试：

     * 1. 处理一个 Request；
     * 2. 处理一个 Event；
     * 3. 执行所有 Domain 的周期处理。
     *
     * @param dispatcher Dispatcher 对象。
     * @return ESP_OK 表示处理成功，否则返回错误码。
     */
    esp_err_t app_core_dispatcher_process_once(
        app_core_dispatcher_t *dispatcher);

#ifdef __cplusplus
}
#endif

#endif