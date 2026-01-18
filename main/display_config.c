#include <string.h>

#include "display.h"

const DisplayConfig * get_display_config(const char * name)
{
    if (name == NULL) {
        return NULL;
    }

    for (int i = 0; i < (int)(sizeof(display_configs) / sizeof(display_configs[0])); i++) {
        if (strcmp(display_configs[i].name, name) == 0) {
            return &display_configs[i];
        }
    }
    return NULL;
}

