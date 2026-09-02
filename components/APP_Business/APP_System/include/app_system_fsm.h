#ifndef APP_SYSTEM_FSM_H
#define APP_SYSTEM_FSM_H

#include <stdint.h>
#include "esp_err.h"
#include "app_core_action.h"
#include "app_core_event.h"
#include "app_core_request.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief System 状态机的 Action 输出回调。
     *
     * System 状态机只负责判断业务状态和生成 Action，
     * 不直接调用 Camera、LVGL、Storage 或其他底层模块。
     *
     * @param ctx Action 输出回调的上下文。
     * @param action 状态机生成的 Action。
     * @return ESP_OK 表示 Action 已经成功交给下一个处理环节。
     */
    typedef esp_err_t (*app_system_fsm_emit_action_fn)(void *ctx, const app_core_action_t *action);

    /**
     * @brief System 生命周期状态机。
     *
     * 该结构体保存 System 业务流程自身的状态，
     * 不负责创建 FreeRTOS 任务，也不直接操作硬件。
     */
    typedef struct
    {
        /** 当前 System 生命周期状态。 */
        app_core_system_state_t state;

        /** 当前正在处理的请求元数据。 */
        app_core_message_meta_t active_meta;

        /** 当前正在处理的请求编号，用于匹配后续 Event。 */
        app_core_request_id_t active_request_id;

        /** 下一个可分配的 System Action 编号。 */
        app_core_action_id_t next_action_id;

        /** 最近一次 System 业务错误。 */
        esp_err_t last_error;
    } app_system_fsm_t;

    /**
     * @brief 初始化 System 状态机。
     *
     * 初始化后状态为 UNKNOWN，且当前没有正在处理的请求。
     *
     * @param fsm 要初始化的 System 状态机。
     */
    void app_system_fsm_init(app_system_fsm_t *fsm);

    /**
     * @brief 获取当前 System 状态。
     *
     * @param fsm System 状态机。
     * @param state 用于接收当前 System 状态的对象。
     * @return ESP_OK 表示读取成功，否则返回错误码。
     */
    esp_err_t app_system_fsm_get_state(const app_system_fsm_t *fsm, app_core_system_state_t *state);

    /**
     * @brief 处理一个 System Request。
     *
     * 函数根据当前状态判断请求是否允许执行。
     * 如果请求需要底层动作，则生成 Action 并通过 emit_action 输出。
     *
     * @param fsm System 状态机。
     * @param request 要处理的 System Request。
     * @param emit_action Action 输出回调。
     * @param emit_ctx Action 输出回调的上下文。
     * @return ESP_OK 表示请求已处理，否则返回错误码。
     */
    esp_err_t app_system_fsm_handle_request(app_system_fsm_t *fsm, const app_core_request_t *request, app_system_fsm_emit_action_fn emit_action, void *emit_ctx);

    /**
     * @brief 处理一个 System Event。
     *
     * 函数根据底层动作产生的结果更新 System 状态，
     * 例如将 INITIALIZING 更新为 READY，
     * 或在收到 FAILED 后进入 FAULT。
     *
     * @param fsm System 状态机。
     * @param event 要处理的 System Event。
     * @return ESP_OK 表示事件已处理，否则返回错误码。
     */
    esp_err_t app_system_fsm_handle_event(app_system_fsm_t *fsm, const app_core_event_t *event);

#ifdef __cplusplus
}
#endif

#endif
