#ifndef APP_UI_FSM_H
#define APP_UI_FSM_H

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
     * @brief UI 状态机的 Action 输出回调。
     *
     * UI FSM 只负责判断页面切换状态并生成 Action，
     * 不直接调用 LVGL 或其他底层显示模块。
     *
     * @param ctx Action 输出回调的上下文。
     * @param action UI 状态机生成的 Action。
     * @return ESP_OK 表示 Action 已经成功交给下一个处理环节。
     */
    typedef esp_err_t (*app_ui_fsm_emit_action_fn)(
        void *ctx,
        const app_core_action_t *action);

    /**
     * @brief UI 页面切换状态机。
     *
     * 该结构体保存当前页面、目标页面、菜单选择项和正在处理的请求，
     * 不负责创建 FreeRTOS 任务，也不直接执行底层显示操作。
     */
    typedef struct
    {
        /** 当前已经显示完成的 LVGL 页面。 */
        app_core_lvgl_page_t current_page;

        /** 当前正在切换的目标页面。 */
        app_core_lvgl_page_t target_page;

        /** 当前选中的菜单项索引。 */
        uint8_t selected_menu_index;

        /** 是否正在等待页面切换完成事件。 */
        bool transitioning;

        /** 当前正在处理的请求元数据。 */
        app_core_message_meta_t active_meta;

        /** 当前正在处理的请求编号，用于匹配后续 Event。 */
        app_core_request_id_t active_request_id;

        /** 下一个可分配的 UI Action 编号。 */
        app_core_action_id_t next_action_id;

        /** 最近一次 UI 业务错误。 */
        esp_err_t last_error;
    } app_ui_fsm_t;

    /**
     * @brief 初始化 UI 状态机。
     *
     * 初始化后当前页面和目标页面均为 BOOT，
     * 当前没有正在处理的页面切换请求。
     *
     * @param fsm 要初始化的 UI 状态机。
     */
    void app_ui_fsm_init(app_ui_fsm_t *fsm);

    /**
     * @brief 获取当前 UI 页面。
     *
     * @param fsm UI 状态机。
     * @param page 用于接收当前页面的对象。
     * @return ESP_OK 表示读取成功，否则返回错误码。
     */
    esp_err_t app_ui_fsm_get_page(
        const app_ui_fsm_t *fsm,
        app_core_lvgl_page_t *page);

    /**
     * @brief 处理一个 UI Request。
     *
     * 函数根据当前页面切换状态判断请求是否允许执行。
     * 对于有效的 SHOW_PAGE 请求，会更新目标页面并生成显示页面 Action。
     *
     * @param fsm UI 状态机。
     * @param request 要处理的 UI Request。
     * @param emit_action Action 输出回调。
     * @param emit_ctx Action 输出回调的上下文。
     * @return ESP_OK 表示请求已处理，否则返回错误码。
     */
    esp_err_t app_ui_fsm_handle_request(
        app_ui_fsm_t *fsm,
        const app_core_request_t *request,
        app_ui_fsm_emit_action_fn emit_action,
        void *emit_ctx);

    /**
     * @brief 处理一个 UI Event。
     *
     * 收到 PAGE_SHOWN 后提交页面切换结果，
     * 收到 FAILED 后记录错误并结束当前页面切换请求。
     *
     * @param fsm UI 状态机。
     * @param event 要处理的 UI Event。
     * @param emit_action Action 输出回调；当前页面切换完成时不会产生新的 Action。
     * @param emit_ctx Action 输出回调的上下文。
     * @return ESP_OK 表示事件已处理，否则返回错误码。
     */
    esp_err_t app_ui_fsm_handle_event(
        app_ui_fsm_t *fsm,
        const app_core_event_t *event,
        app_ui_fsm_emit_action_fn emit_action,
        void *emit_ctx);

#ifdef __cplusplus
}
#endif

#endif
