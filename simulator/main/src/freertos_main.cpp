#include <cstdio>
#include <cstdlib>

#include "FreeRTOS.h"
#include "task.h"

extern "C" int lvgl_main(void);

extern "C" void vApplicationMallocFailedHook(void)
{
    std::fprintf(stderr, "FreeRTOS allocation failed; %zu heap bytes remain\n", xPortGetFreeHeapSize());
    std::abort();
}

static void preview_task(void* context)
{
    // The desktop process ends after the UI and SDL resources have been released.
    std::exit(lvgl_main());
}

extern "C" int freertos_main(void)
{
    if (xTaskCreate(preview_task, "Nimbus UI", 8192, nullptr, 1, nullptr) != pdPASS) {
        std::fputs("Could not create the FreeRTOS UI task\n", stderr);

        return EXIT_FAILURE;
    }

    vTaskStartScheduler();
    std::fputs("FreeRTOS scheduler returned unexpectedly\n", stderr);

    return EXIT_FAILURE;
}
