#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define TFT_CS_PIO       PIOA
#define TFT_CS_PIN       (1 << 0)
#define TFT_CMD_DATA_PIO PIOD
#define TFT_CMD_DATA_PIN (1 << 29)
#define TFT_WR_PIO       PIOD
#define TFT_WR_PIN       (1 << 9)
#define TFT_RESET_PIO    PIOD
#define TFT_RESET_PIN    (1 << 15)
#ifdef __cplusplus
}
#endif
