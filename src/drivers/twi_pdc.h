/*
 * my_twi.h
 *
 * Created: 15.10.2021 15:33:01
 *  Author: malygosstationar
 */

#ifndef TWI_PDC_H_
#define TWI_PDC_H_
#define DATABUFF_LEN          1500UL
#define TWI_PDC_IRQ_PRIO      3
#define TWI_PDC_MODULE        TWI0
#define TWI_PDC_MODULE_IRQ_N  TWI0_IRQn
#define TWI_WAIT_FREE_TIMEOUT 100000UL

#ifdef __cplusplus
extern "C" {
#endif

static volatile struct {
    uint8_t  data[DATABUFF_LEN];
    uint16_t data_len[DATABUFF_LEN];
    uint8_t  device_addr[DATABUFF_LEN];
    uint16_t bytes_occupied;
    uint16_t packets_occupied;
    bool     is_enabled;
    bool     is_free;
    bool     overflow;
} twi_instance;

void twiPdc_enable(void);
bool twiPdc_disable(void);
void twiPdc_init(void);
void twiPdc_write(uint8_t *data, uint8_t datalen, uint8_t devadr);

#ifdef __cplusplus
}
#endif
#endif /* TWI_PDC_H_ */