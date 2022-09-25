/*
 * my_twi.c
 *
 * Created: 15.10.2021 15:32:46
 *  Author: malygosstationar
 */
#include <asf.h>
#include "twi.h"
#include "twi_pdc.h"

#define ENDTX	(uint32_t)(1 << 13)
#define TXCOMP	(uint32_t)(1 << 0)

#ifdef __cplusplus
extern "C" {
#endif

void start_transfer					(void);
void delete_last_data_from_buffer	(void);

Pdc *twi_pdc;
Twi *twi_module;
pdc_packet_t pdc_twi_packet;

void twiPdc_enable (void)
{
	twi_instance.is_enabled = true;
	twi_instance.bytes_occupied = 0;
	twi_instance.packets_occupied = 0;
}

bool twiPdc_disable (void)
{
	uint32_t counter = TWI_WAIT_FREE_TIMEOUT;

	twi_instance.is_enabled = false;
	twi_instance.packets_occupied = 0;
	
	while (1) {
		if (twi_module->TWI_SR & TWI_SR_TXCOMP) {
			twi_disable_interrupt(twi_module, TXCOMP);
		pdc_disable_transfer(twi_pdc, PERIPH_PTCR_TXTDIS);
			return false;
		}
	}

	return true;
}

void delete_last_data_from_buffer (void)
{
	uint16_t bytes_left;
	uint16_t data_counter = 0;
	uint16_t shift_size = twi_instance.data_len[0];

	if (twi_instance.packets_occupied)
		twi_instance.packets_occupied--;
	else {
		twi_instance.bytes_occupied = 0;
		return;
	}

	if (twi_instance.bytes_occupied < shift_size) {
		twi_instance.bytes_occupied = 0;
		twi_instance.packets_occupied = 0;
		return;
	} else
		twi_instance.bytes_occupied -= shift_size;

	bytes_left = twi_instance.bytes_occupied;

	while (bytes_left) {
		twi_instance.data[data_counter] = twi_instance.data[data_counter +
		                                  shift_size];

		bytes_left--;
		data_counter++;
	}

	data_counter = 0;

	do {
		twi_instance.data_len[data_counter] = twi_instance.data_len[data_counter + 1];
		twi_instance.device_addr[data_counter] = twi_instance.device_addr[data_counter
		    + 1];
		data_counter++;
	} while ((twi_instance.data_len[data_counter - 1] != 0) &&
	         ((data_counter + 1) < DATABUFF_LEN));
}

void twiPdc_write (uint8_t *write_data, uint8_t datalen, uint8_t devadr)
{
	if (!twi_instance.is_enabled)
		return;

	uint16_t data_counter = 0;

	if ((twi_instance.bytes_occupied + datalen) >= DATABUFF_LEN) {
		twi_instance.overflow = true;
		
		return;
	}
		

	while (data_counter < datalen) {
		twi_instance.data[twi_instance.bytes_occupied + data_counter] =
		  write_data[data_counter];

		data_counter++;
	}

	twi_instance.data_len[twi_instance.packets_occupied] = datalen;
	twi_instance.device_addr[twi_instance.packets_occupied] = devadr;
	twi_instance.packets_occupied++;
	twi_instance.bytes_occupied += datalen;

	if ((twi_get_interrupt_mask(twi_module) == 0) &&
	    ((twi_instance.packets_occupied - 1) == 0))
		start_transfer();
}

void start_transfer (void)
{
	if (twi_instance.packets_occupied == 0)
		return;

	pdc_twi_packet.ul_addr = (uint32_t)twi_instance.data;
	pdc_twi_packet.ul_size = (uint32_t)twi_instance.data_len[0];

	if (twi_instance.device_addr[0] != ((twi_module->TWI_MMR >> 16) && 0x7F)) {
		uint32_t MMR_buffer;

		MMR_buffer = twi_module->TWI_MMR;
		MMR_buffer = MMR_buffer & TWI_MMR_DADR(0);
		MMR_buffer |= TWI_MMR_DADR(twi_instance.device_addr[0]);
		twi_module->TWI_MMR = MMR_buffer;
	}

	pdc_tx_init(twi_pdc, &pdc_twi_packet, NULL);
	pdc_enable_transfer(twi_pdc, PERIPH_PTCR_TXTEN);
	twi_module->TWI_CR = TWI_CR_STOP;

	twi_enable_interrupt(TWI0, TXCOMP);
	twi_instance.is_free = false;
}

void TWI0_Handler (void)
{
	if (!(twi_instance.is_enabled)) {
		twi_disable_interrupt(twi_module, TXCOMP);
		pdc_disable_transfer(twi_pdc, PERIPH_PTCR_TXTDIS);
		twi_instance.is_free = true;
		return;
	}

	delete_last_data_from_buffer();

	if (twi_instance.packets_occupied == 0) {
		twi_instance.bytes_occupied = 0;
		twi_disable_interrupt(twi_module, TXCOMP);
		pdc_disable_transfer(twi_pdc, PERIPH_PTCR_TXTDIS);
		twi_instance.is_free = true;
		return;
	}

	pdc_twi_packet.ul_addr = (uint32_t)twi_instance.data;
	pdc_twi_packet.ul_size = (uint32_t)twi_instance.data_len[0];

	if (twi_instance.device_addr[0] != ((twi_module->TWI_MMR >> 16) && 0x7F)) {
		uint32_t MMR_buffer;

		MMR_buffer = twi_module->TWI_MMR;
		MMR_buffer = MMR_buffer & TWI_MMR_DADR(0);
		MMR_buffer |= TWI_MMR_DADR(twi_instance.device_addr[0]);
		twi_module->TWI_MMR = MMR_buffer;
	}

	pdc_tx_init(twi_pdc, &pdc_twi_packet, NULL);
	pdc_enable_transfer(twi_pdc, PERIPH_PTCR_TXTEN);
	twi_module->TWI_CR = TWI_CR_STOP;
}

void twiPdc_init (void)
{
	twi_module = TWI_PDC_MODULE;
	twi_pdc = twi_get_pdc_base(twi_module);
	NVIC_SetPriority(TWI_PDC_MODULE_IRQ_N, TWI_PDC_IRQ_PRIO);
	NVIC_EnableIRQ(TWI_PDC_MODULE_IRQ_N);
	twi_instance.is_free = true;
	twiPdc_enable();
}
#ifdef __cplusplus
}
#endif