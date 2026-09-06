#ifndef APP_STORAGE_FSM_H
#define APP_STORAGE_FSM_H

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
     * @brief Storage 状态机的 Action 输出回调。
     *
     * Storage FSM 只负责状态判断和 Action 生成，
     * 不直接访问 SD 卡、文件系统或其他底层模块。
     */
    typedef esp_err_t (*app_storage_fsm_emit_action_fn)(
        void *ctx,
        const app_core_action_t *action);

    /**
     * @brief Storage 图库业务状态机。
     *
     * 该结构体保存图库数量、当前选择项和正在处理的请求，
     * 不负责创建 FreeRTOS 任务，也不直接执行 SD 卡或文件系统 IO。
     */
    typedef struct
    {
        /** 当前 Storage 状态。 */
        app_core_storage_state_t state;

        /** 当前扫描得到的照片数量。 */
        uint16_t gallery_count;

        /** 当前选中的照片索引。 */
        uint16_t selected_gallery_index;

        /** 当前请求的消息元数据。 */
        app_core_message_meta_t active_meta;

        /** 当前请求编号，用于匹配后续 Event。 */
        app_core_request_id_t active_request_id;

        /** 下一个可分配的 Action 编号。 */
        app_core_action_id_t next_action_id;

        /** 最近一次 Storage 业务错误。 */
        esp_err_t last_error;
    } app_storage_fsm_t;

    /**
     * @brief 初始化 Storage 状态机。
     *
     * 初始化后状态为 IDLE，图库数量和选择项均为 0，
     * 当前没有正在处理的请求。
     *
     * @param fsm 要初始化的 Storage 状态机。
     */
    void app_storage_fsm_init(app_storage_fsm_t *fsm);

    /**
     * @brief 获取当前 Storage 状态。
     *
     * @param fsm Storage 状态机。
     * @param state 用于接收当前 Storage 状态的对象。
     * @return ESP_OK 表示读取成功，否则返回错误码。
     */
    esp_err_t app_storage_fsm_get_state(
        const app_storage_fsm_t *fsm,
        app_core_storage_state_t *state);

    /**
     * @brief 处理一个 Storage Request。
     *
     * 支持扫描图库和显示指定照片，
     * 请求通过状态校验后会生成对应 Action。
     *
     * @param fsm Storage 状态机。
     * @param request 要处理的 Storage Request。
     * @param emit_action Action 输出回调。
     * @param emit_ctx Action 输出回调的上下文。
     * @return ESP_OK 表示请求已处理，否则返回错误码。
     */
    esp_err_t app_storage_fsm_handle_request(
        app_storage_fsm_t *fsm,
        const app_core_request_t *request,
        app_storage_fsm_emit_action_fn emit_action,
        void *emit_ctx);

    /**
     * @brief 处理一个 Storage Event。
     *
     * 根据底层扫描或显示结果更新 Storage 状态，
     * 收到 FAILED 事件时记录错误并进入 FAILED 状态。
     *
     * @param fsm Storage 状态机。
     * @param event 要处理的 Storage Event。
     * @param emit_action Action 输出回调；当前 Storage Event 处理不会产生新的 Action。
     * @param emit_ctx Action 输出回调的上下文。
     * @return ESP_OK 表示事件已处理，否则返回错误码。
     */
    esp_err_t app_storage_fsm_handle_event(
        app_storage_fsm_t *fsm,
        const app_core_event_t *event,
        app_storage_fsm_emit_action_fn emit_action,
        void *emit_ctx);

#ifdef __cplusplus
}
#endif

#endif
