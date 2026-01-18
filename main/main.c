#include "esp_event.h"
#include "esp_log.h"
#if defined(CONFIG_SPIRAM) && CONFIG_SPIRAM
#include "esp_psram.h"
#endif

#include "asic_result_task.h"
#include "asic_task.h"
#include "create_jobs_task.h"
#include "hashrate_monitor_task.h"
#include "statistics_task.h"
#include "system.h"
#include "http_server.h"
#include "serial.h"
#include "stratum_task.h"
#include "i2c_bitaxe.h"
#include "adc.h"
#include "nvs_config.h"
#include "self_test.h"
#include "asic.h"
#if !defined(CONFIG_ESP_MINER_DISABLE_BAP) || !CONFIG_ESP_MINER_DISABLE_BAP
#include "bap/bap.h"
#endif
#include "device_config.h"
#include "connect.h"
#include "asic_reset.h"
#include "asic_init.h"

#include "esp_miner_caps.h"

static GlobalState GLOBAL_STATE;

static const char * TAG = "bitaxe";

void app_main(void)
{
    ESP_LOGI(TAG, "Welcome to the bitaxe - FOSS || GTFO!");

#if defined(CONFIG_SPIRAM) && CONFIG_SPIRAM
    GLOBAL_STATE.psram_is_available = esp_psram_is_initialized();
    if (!GLOBAL_STATE.psram_is_available) {
        ESP_LOGE(TAG, "No PSRAM available on ESP32 device!");
    }
#else
    GLOBAL_STATE.psram_is_available = false;
#endif

    // Init I2C
    ESP_ERROR_CHECK(i2c_bitaxe_init());
    ESP_LOGI(TAG, "I2C initialized successfully");
    
    // Initialize RST pin to low early to minimize ASIC power consumption
    ESP_ERROR_CHECK(asic_hold_reset_low());
    ESP_LOGI(TAG, "RST pin initialized to low");

    //wait for I2C to init
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //Init ADC
    ADC_init();

    //initialize the ESP32 NVS
    if (nvs_config_init() != ESP_OK){
        ESP_LOGE(TAG, "Failed to init NVS");
        return;
    }

    // Ensure SSID is initialized before any screen/self-test uses it.
    GLOBAL_STATE.SYSTEM_MODULE.ssid = nvs_config_get_string(NVS_CONFIG_WIFI_SSID);
    if (GLOBAL_STATE.SYSTEM_MODULE.ssid == NULL) {
        GLOBAL_STATE.SYSTEM_MODULE.ssid = strdup("");
    }

    if (device_config_init(&GLOBAL_STATE) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init device config");
        return;
    }

    if (self_test(&GLOBAL_STATE)) return;

    SYSTEM_init_system(&GLOBAL_STATE);

    // init AP and connect to wifi
    wifi_init(&GLOBAL_STATE);

    // Create the memory-critical tasks early (before display/LVGL init) to reduce heap fragmentation
    // on no-PSRAM targets. These tasks self-gate until the ASIC and peripherals are ready.
    if (xTaskCreateWithCaps(ASIC_result_task, "asic result", ESP_MINER_TASK_STACK_SIZE_SMALL, (void *) &GLOBAL_STATE, 15, NULL, ESP_MINER_TASK_STACK_CAPS) != pdPASS) {
        ESP_LOGE(TAG, "Error creating asic result task");
    }
    if (xTaskCreateWithCaps(hashrate_monitor_task, "hashrate monitor", ESP_MINER_TASK_STACK_SIZE_SMALL, (void *) &GLOBAL_STATE, 5, NULL, ESP_MINER_TASK_STACK_CAPS) != pdPASS) {
        ESP_LOGE(TAG, "Error creating hashrate monitor task");
    }
    if (xTaskCreateWithCaps(statistics_task, "statistics", ESP_MINER_TASK_STACK_SIZE_SMALL, (void *) &GLOBAL_STATE, 3, NULL, ESP_MINER_TASK_STACK_CAPS) != pdPASS) {
        ESP_LOGE(TAG, "Error creating statistics task");
    }

    SYSTEM_init_peripherals(&GLOBAL_STATE);

    if (xTaskCreateWithCaps(POWER_MANAGEMENT_task, "power management", ESP_MINER_TASK_STACK_SIZE_DEFAULT, (void *) &GLOBAL_STATE, 10, NULL, ESP_MINER_TASK_STACK_CAPS) != pdPASS) {
        ESP_LOGE(TAG, "Error creating power management task");
    }

    //start the API for AxeOS
    start_rest_server((void *) &GLOBAL_STATE);

#if !defined(CONFIG_ESP_MINER_DISABLE_BAP) || !CONFIG_ESP_MINER_DISABLE_BAP
    // Initialize BAP interface
    esp_err_t bap_ret = BAP_init(&GLOBAL_STATE);
    if (bap_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BAP interface: %d", bap_ret);
        // Continue anyway, as BAP is not critical for core functionality
    }
#endif

    while (!GLOBAL_STATE.SYSTEM_MODULE.is_connected) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    queue_init(&GLOBAL_STATE.stratum_queue);
    queue_init(&GLOBAL_STATE.ASIC_jobs_queue);

    if (asic_initialize(&GLOBAL_STATE, ASIC_INIT_COLD_BOOT, 0) == 0) {
        return;
    }

    if (xTaskCreateWithCaps(stratum_task, "stratum admin", ESP_MINER_TASK_STACK_SIZE_DEFAULT, (void *) &GLOBAL_STATE, 5, NULL, ESP_MINER_TASK_STACK_CAPS) != pdPASS) {
        ESP_LOGE(TAG, "Error creating stratum admin task");
    }
    if (xTaskCreateWithCaps(create_jobs_task, "stratum miner", ESP_MINER_TASK_STACK_SIZE_DEFAULT, (void *) &GLOBAL_STATE, 10, NULL, ESP_MINER_TASK_STACK_CAPS) != pdPASS) {
        ESP_LOGE(TAG, "Error creating stratum miner task");
    }
    if (xTaskCreateWithCaps(ASIC_task, "asic", ESP_MINER_TASK_STACK_SIZE_DEFAULT, (void *) &GLOBAL_STATE, 10, NULL, ESP_MINER_TASK_STACK_CAPS) != pdPASS) {
        ESP_LOGE(TAG, "Error creating asic task");
    }
}
