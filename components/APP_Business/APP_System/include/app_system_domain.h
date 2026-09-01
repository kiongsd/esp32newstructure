#ifndef APP_SYSTEM_DOMAIN_H
#define APP_SYSTEM_DOMAIN_H

#include <stdbool.h>

#include "esp_err.h"

#include "app_core_domain.h"
#include "app_core_state_store.h"

#include "app_system_controller.h"

#ifdef __cplusplus
extern "C"
{
#endif
typedef struct 
{
    app_system_controller_t *controller;
    app_core_state_store_t *state_store;
    app_core_domain_handler_t handler;
    bool initialized;
    /* data */
}app_system_domain_t;

esp_err_t app_system_domain_init(app_system_domain_t *domain, app_system_controller_t *controller, app_core_state_store_t *state_store);

void app_system_domain_deinit(app_system_domain_t *domain);

const app_core_domain_handler_t * app_system_domain_get_handler(const app_system_domain_t *domain);


#ifdef __cplusplus
}
#endif

#endif