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

    typedef esp_err_t (*app_storage_fsm_emit_action_fn)(
        void *ctx,
        const app_core_action_t *action);

    typedef struct
    {
        app_core_storage_state_t state;

        uint16_t gallery_count;
        uint16_t selected_gallery_index;

        app_core_message_meta_t active_meta;
        app_core_request_id_t active_request_id;
        app_core_action_id_t next_action_id;

        esp_err_t last_error;
    } app_storage_fsm_t;

    void app_storage_fsm_init(app_storage_fsm_t *fsm);

    esp_err_t app_storage_fsm_get_state(
        const app_storage_fsm_t *fsm,
        app_core_storage_state_t *state);

    esp_err_t app_storage_fsm_handle_request(
        app_storage_fsm_t *fsm,
        const app_core_request_t *request,
        app_storage_fsm_emit_action_fn emit_action,
        void *emit_ctx);

    esp_err_t app_storage_fsm_handle_event(
        app_storage_fsm_t *fsm,
        const app_core_event_t *event,
        app_storage_fsm_emit_action_fn emit_action,
        void *emit_ctx);

#ifdef __cplusplus
}
#endif

#endif