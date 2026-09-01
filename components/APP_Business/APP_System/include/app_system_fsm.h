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

    typedef esp_err_t (*app_system_fsm_emit_action_fn)(void *ctx, const app_core_action_t *action);

    typedef struct
    {
        app_core_system_state_t state;

        app_core_message_meta_t active_meta;
        app_core_request_id_t active_request_id;

        app_core_action_id_t next_action_id;

        esp_err_t last_error;
    } app_system_fsm_t;

    void app_system_fsm_init(app_system_fsm_t *fsm);

    esp_err_t app_system_fsm_get_state(const app_system_fsm_t *fsm, app_core_system_state_t *state);

    esp_err_t app_system_fsm_handle_request(app_system_fsm_t *fsm, const app_core_request_t *request, app_system_fsm_emit_action_fn emit_action, void *emit_ctx);

    esp_err_t app_system_fsm_handle_event(app_system_fsm_t *fsm, const app_core_event_t *event);

#ifdef __cplusplus
}
#endif

#endif