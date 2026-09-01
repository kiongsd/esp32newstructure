#ifndef APP_SYSTEM_CONTROLLER_H
#define APP_SYSTEM_CONTROLLER_H

#include <stdbool.h>

#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "app_core_action_engine.h"
#include "app_core_event.h"
#include "app_core_request.h"
#include "app_core_runtime.h"

#include "app_system_fsm.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        app_system_fsm_t fsm;

        app_core_action_engine_t action_engine;

        QueueHandle_t action_queue;

        bool initialized;

    } app_system_controller_t;

    esp_err_t app_system_controller_init(app_system_controller_t *controller, const app_core_runtime_t *runtime, UBaseType_t action_queue_length, app_core_action_engine_emit_event_fn emit_event, void *event_ctx);

    void app_system_controller_deinit(app_system_controller_t *controller);

    esp_err_t app_system_controller_handle_request(app_system_controller_t *controller, const app_core_request_t *request);

    esp_err_t app_system_controller_handle_event(app_system_controller_t *controller, const app_core_event_t *event);

    esp_err_t app_system_controller_process_once(app_system_controller_t *controller);

    esp_err_t app_system_controller_get_state(const app_system_controller_t *controller, app_core_system_state_t *state);

#ifdef __cplusplus
}
#endif

#endif