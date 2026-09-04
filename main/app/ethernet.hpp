#pragma once

#include <stdint.h>
#include "esp_event.h"

class Ethernet {
public:
    Ethernet();
    ~Ethernet();
private:
    static void ethEventHandler(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData);
    static void ipEventHandler(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData);

};