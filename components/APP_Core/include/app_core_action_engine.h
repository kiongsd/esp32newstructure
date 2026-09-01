#ifndef APP_CORE_ACTION_ENGINE_H
#define APP_CORE_ACTION_ENGINE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "app_core_action.h"
#include "app_core_event.h"
#include "app_core_runtime.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Action Engine 最大支持的等待结果数量。
 */
#define APP_CORE_ACTION_ENGINE_MAX_PENDING 8U

    /**
     * @brief Action Engine Event 输出回调。
     *
     * Action 执行失败或需要向系统发送结果时，
     * 通过该回调将 Event 交给 Dispatcher。
     */
    typedef esp_err_t (*app_core_action_engine_emit_event_fn)(
        void *ctx,
        const app_core_event_t *event);

    /**
     * @brief App Core Action Engine。
     *
     * Action Engine 负责：
     *
     * 1. 接收待执行的 Action；
     * 2. 调用 Runtime 执行 Action 中的 Effect；
     * 3. 保存正在等待结果的 Action；
     * 4. 根据 Event 更新 Action 的最终状态；
     * 5. 在执行失败时发送 FAILED Event。
     *
     * Action Engine 本身不直接操作 Camera、LVGL、Web 或 OTA，
     * 所有底层动作都通过 Runtime 完成。
     */
    typedef struct
    {
        /** Runtime 执行接口。 */
        const app_core_runtime_t *runtime;

        /** 待执行 Action 队列。 */
        QueueHandle_t action_queue;

        /** Action 结果 Event 输出回调。 */
        app_core_action_engine_emit_event_fn emit_event;

        /** Event 输出回调使用的上下文。 */
        void *event_ctx;

        /** 最近一次进入执行流程的 Action。 */
        app_core_action_t last_action;

        /** 是否存在最近一次 Action。 */
        bool has_last_action;

        /** 已经执行但仍等待底层结果的 Action。 */
        app_core_action_t pending_actions[APP_CORE_ACTION_ENGINE_MAX_PENDING];

        /** pending_actions 对应槽位是否正在使用。 */
        bool pending_used[APP_CORE_ACTION_ENGINE_MAX_PENDING];

        /** Action Engine 是否已经初始化。 */
        bool initialized;

    } app_core_action_engine_t;

    /**
     * @brief 初始化 Action Engine。
     *
     * 注意：Action Engine 不创建 Action 队列，
     * 队列由调用者创建并传入。
     *
     * @param engine 要初始化的 Action Engine。
     * @param runtime Runtime 回调表。
     * @param action_queue Action 队列。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_core_action_engine_init(
        app_core_action_engine_t *engine,
        const app_core_runtime_t *runtime,
        QueueHandle_t action_queue);

    /**
     * @brief 绑定 Event 输出接口。
     *
     * Action 执行失败时，会通过该接口向外发送 FAILED Event。
     *
     * @param engine Action Engine。
     * @param emit_event Event 输出回调。
     * @param event_ctx Event 输出上下文。
     * @return ESP_OK 表示成功。
     */
    esp_err_t app_core_action_engine_bind_event_sink(
        app_core_action_engine_t *engine,
        app_core_action_engine_emit_event_fn emit_event,
        void *event_ctx);

    /**
     * @brief 提交一个 Action。
     *
     * Action 会被复制到队列中，调用者可以安全地释放或复用原对象。
     *
     * @param engine Action Engine。
     * @param action 要提交的 Action。
     * @return ESP_OK 表示提交成功。
     */
    esp_err_t app_core_action_engine_submit(
        app_core_action_engine_t *engine,
        const app_core_action_t *action);

    /**
     * @brief 执行一次 Action。
     *
     * 该函数最多从队列中取出一个 Action，
     * 并调用 Runtime 执行对应的 Effect。
     *
     * @param engine Action Engine。
     * @return ESP_OK 表示执行成功或已经处理；
     *         ESP_ERR_TIMEOUT 表示当前没有待执行 Action。
     */
    esp_err_t app_core_action_engine_process_once(
        app_core_action_engine_t *engine);

    /**
     * @brief 处理一个底层 Event。
     *
     * 根据 Event 的 Request ID 和 Event 类型，
     * 找到对应的 pending Action 并更新其状态。
     *
     * @param engine Action Engine。
     * @param event 底层产生的 Event。
     * @return ESP_OK 表示找到并处理；
     *         ESP_ERR_NOT_FOUND 表示没有匹配的 Action。
     */
    esp_err_t app_core_action_engine_handle_event(
        app_core_action_engine_t *engine,
        const app_core_event_t *event);

    /**
     * @brief 获取最近一次 Action。
     *
     * @param engine Action Engine。
     * @param action 用于接收 Action 的对象。
     * @return ESP_OK 表示成功；
     *         ESP_ERR_NOT_FOUND 表示当前没有历史 Action。
     */
    esp_err_t app_core_action_engine_get_last_action(
        const app_core_action_engine_t *engine,
        app_core_action_t *action);

    /**
     * @brief Action 提交适配回调。
     *
     * 可以把该函数作为 Domain 的 Action 输出回调。
     *
     * @param ctx Action Engine 上下文。
     * @param action 要提交的 Action。
     * @return ESP_OK 表示提交成功。
     */
    esp_err_t app_core_action_engine_emit_action(
        void *ctx,
        const app_core_action_t *action);

#ifdef __cplusplus
}
#endif

#endif