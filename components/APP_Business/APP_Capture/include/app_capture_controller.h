#ifndef APP_CAPTURE_CONTROLLER_H
#define APP_CAPTURE_CONTROLLER_H

#include <stdbool.h>

#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "app_core_action_engine.h"
#include "app_core_event.h"
#include "app_core_request.h"
#include "app_core_runtime.h"

#include "app_capture_fsm.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief capture 业务控制器。
     *
     * Controller 负责组合 capture 状态机和 Action Engine，
     * 对外提供统一的 Request、Event 和周期处理接口。
     */
    typedef struct
    {
        /** capture 生命周期状态机。 */
        app_capture_fsm_t fsm;

        /** 负责执行 capture Action 的 Action Engine。 */
        app_core_action_engine_t action_engine;

        /** capture Action 待执行队列。 */
        QueueHandle_t action_queue;

        /** Controller 是否已经完成初始化。 */
        bool initialized;

    } app_capture_controller_t;

    /**
     * @brief 初始化 capture Controller。
     *
     * 函数会创建 Action 队列，初始化 Action Engine，
     * 并初始化 capture 状态机。
     *
     * @param controller 要初始化的 Controller。
     * @param runtime 底层 Effect 执行回调集合。
     * @param action_queue_length Action 队列长度。
     * @param emit_event Action Engine 的 Event 输出回调。
     * @param event_ctx Event 输出回调的上下文。
     * @return ESP_OK 表示初始化成功，否则返回错误码。
     */
    esp_err_t app_capture_controller_init(app_capture_controller_t *controller, const app_core_runtime_t *runtime, UBaseType_t action_queue_length, app_core_action_engine_emit_event_fn emit_event, void *event_ctx);

    /**
     * @brief 释放 capture Controller。
     *
     * 函数会删除 Controller 创建的 Action 队列，
     * 并清空 Controller 内部状态。
     *
     * @param controller 要释放的 Controller。
     */
    void app_capture_controller_deinit(app_capture_controller_t *controller);

    /**
     * @brief 向 capture Controller 提交 Request。
     *
     * Controller 会将 Request 交给 capture 状态机判断和处理。
     *
     * @param controller capture Controller。
     * @param request 要处理的 Request。
     * @return ESP_OK 表示处理成功，否则返回错误码。
     */
    esp_err_t app_capture_controller_handle_request(app_capture_controller_t *controller, const app_core_request_t *request);

    /**
     * @brief 向 capture Controller 提交 Event。
     *
     * Controller 会先让 Action Engine 匹配 Action 的执行结果，
     * 再让 capture 状态机根据 Event 更新业务状态。
     *
     * @param controller capture Controller。
     * @param event 要处理的 Event。
     * @return ESP_OK 表示处理成功，否则返回错误码。
     */
    esp_err_t app_capture_controller_handle_event(app_capture_controller_t *controller, const app_core_event_t *event);

    /**
     * @brief 执行一次 capture Controller 周期处理。
     *
     * 函数会尝试从 Action 队列中取出一个 Action，
     * 并交给 Action Engine 执行。
     *
     * @param controller capture Controller。
     * @return ESP_OK 表示处理成功；
     *         ESP_ERR_TIMEOUT 表示当前没有待执行 Action。
     */
    esp_err_t app_capture_controller_process_once(app_capture_controller_t *controller);

    /**
     * @brief 获取当前 capture 状态。
     *
     * @param controller capture Controller。
     * @param state 用于接收 capture 状态的对象。
     * @return ESP_OK 表示读取成功，否则返回错误码。
     */
    esp_err_t app_capture_controller_get_state(const app_capture_controller_t *controller, app_core_capture_state_t *state);

#ifdef __cplusplus
}
#endif

#endif
