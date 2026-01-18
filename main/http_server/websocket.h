#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#include "esp_err.h"

#if defined(CONFIG_ESP_MINER_LIGHTWEIGHT_RAM) && CONFIG_ESP_MINER_LIGHTWEIGHT_RAM
#define MESSAGE_QUEUE_SIZE (32)
#define WEBSOCKET_MAX_RX_FRAME_BYTES (256)
#define WEBSOCKET_MAX_LOG_LINE_BYTES (256)
#else
#define MESSAGE_QUEUE_SIZE (128)
#define WEBSOCKET_MAX_RX_FRAME_BYTES (1024)
#define WEBSOCKET_MAX_LOG_LINE_BYTES (1024)
#endif

#if defined(CONFIG_ESP_MINER_LIGHTWEIGHT_RAM) && CONFIG_ESP_MINER_LIGHTWEIGHT_RAM
#define MAX_WEBSOCKET_CLIENTS (4)
#else
#define MAX_WEBSOCKET_CLIENTS (10)
#endif

esp_err_t websocket_handler(httpd_req_t * req);
void websocket_task(void * pvParameters);
void websocket_close_fn(httpd_handle_t hd, int sockfd);

#endif /* WEBSOCKET_H_ */
