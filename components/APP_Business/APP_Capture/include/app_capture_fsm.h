#ifndef APP_CAPTURE_FSM_H
#define APP_CAPTURE_FSM_H

#include <stdbool.h>
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
     * @brief capture 状态机的 Action 输出回调。
     *
     * capture 状态机只负责判断业务状态和生成 Action，
     * 不直接调用 Camera、LVGL、Storage 或其他底层模块。
     *
     * @param ctx Action 输出回调的上下文。
     * @param action 状态机生成的 Action。
     * @return ESP_OK 表示 Action 已经成功交给下一个处理环节。
     */
    typedef esp_err_t (*app_capture_fsm_emit_action_fn)(void *ctx, const app_core_action_t *action);

    /**
     * @brief capture 生命周期状态机。
     *
     * 该结构体保存 capture 业务流程自身的状态，
     * 不负责创建 FreeRTOS 任务，也不直接操作硬件。
     */
    typedef struct
    {
        /** 当前 capture 生命周期状态。 */
        app_core_capture_state_t state;

        /** 当前正在处理的请求元数据。 */
        app_core_message_meta_t active_meta;

        /** 当前正在处理的请求编号，用于匹配后续 Event。 */
        app_core_request_id_t active_request_id;
        app_core_job_id_t active_job_id;
        app_core_photo_handle_t active_photo_handle;

        /** Camera 预览是否已经启动。 */
        bool preview_active;

        /** 当前拍照流程是否由 Web 请求触发。 */
        bool web_capture_active;

        /** 下一个可分配的 capture Action 编号。 */
        app_core_action_id_t next_action_id;
        app_core_job_id_t next_job_id;

        /** 准备完成后是否继续执行完整拍照。 */
        bool capture_after_prepare;
        /** 最近一次 capture 业务错误。 */
        esp_err_t last_error;
    } app_capture_fsm_t;

    /**
     * @brief 初始化 capture 状态机。
     *
     * 初始化后状态为 IDLE，且当前没有正在处理的请求。
     *
     * @param fsm 要初始化的 capture 状态机。
     */
    void app_capture_fsm_init(app_capture_fsm_t *fsm);

    /**
     * @brief 获取当前 capture 状态。
     *
     * @param fsm capture 状态机。
     * @param state 用于接收当前 capture 状态的对象。
     * @return ESP_OK 表示读取成功，否则返回错误码。
     */
    esp_err_t app_capture_fsm_get_state(const app_capture_fsm_t *fsm, app_core_capture_state_t *state);

    /**
     * @brief 处理一个 capture Request。
     *
     * 函数根据当前状态判断请求是否允许执行。
     * 如果请求需要底层动作，则生成 Action 并通过 emit_action 输出。
     *
     * @param fsm capture 状态机。
     * @param request 要处理的 capture Request。
     * @param emit_action Action 输出回调。
     * @param emit_ctx Action 输出回调的上下文。
     * @return ESP_OK 表示请求已处理，否则返回错误码。
     */
    esp_err_t app_capture_fsm_handle_request(app_capture_fsm_t *fsm, const app_core_request_t *request, app_capture_fsm_emit_action_fn emit_action, void *emit_ctx);

    /**
     * @brief 处理一个 capture Event。
     *
     * 函数根据底层动作产生的结果更新 capture 状态，
     * 例如将 INITIALIZING 更新为 READY，
     * 或在收到 FAILED 后进入 FAULT。
     *
     * @param fsm capture 状态机。
     * @param event 要处理的 capture Event。
     * @return ESP_OK 表示事件已处理，否则返回错误码。
     */
    esp_err_t app_capture_fsm_handle_event(
        app_capture_fsm_t *fsm,
        const app_core_event_t *event,
        app_capture_fsm_emit_action_fn emit_action,
        void *emit_ctx);

#ifdef __cplusplus
}
#endif

#endif
