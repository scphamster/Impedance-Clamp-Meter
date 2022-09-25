/*
 * MCP23016.c
 *
 * Created: 27.10.2021 21:29:33
 *  Author: malygosstationar
 */

#include "asf.h"
#include "MCP23016.h"
#include "twi_pdc.h"
#include "keyboard.h"
#include "LCD1608.h"

#define MCP23016_GP0		0X00
#define MCP23016_GP1		0X01
#define MCP23016_OLAT0		0X02
#define MCP23016_OLAT1		0X03
#define MCP23016_IPOL0		0X04
#define MCP23016_IPOL1		0X05
#define MCP23016_IODIR0		0X06
#define MCP23016_IODIR1		0X07
#define MCP23016_INTCAP0	0X08
#define MCP23016_INTCAP1	0X09
#define MCP23016_IOCON0		0X0A
#define MCP23016_IOCON1		0X0B

#ifdef __cplusplus
extern "C" {
#endif

void mcp23016_irq_handler (uint32_t id, uint32_t mask);

void mcp23016_init (void)
{
	pio_set_input(MCP23016_INT_PORT, (1 << MCP23016_INT_PIN), PIO_PULLUP);
	
	uint8_t data1[] = {MCP23016_IODIR0, 0xff};

	twiPdc_write(data1, 2, MCP23016_TWI_ADDR);

	uint8_t data2[] = {MCP23016_IODIR1, 0xff};

	twiPdc_write(data2, 2, MCP23016_TWI_ADDR);

	uint8_t data3[] = {MCP23016_IPOL0, 0xff};

	twiPdc_write(data3, 2, MCP23016_TWI_ADDR);

	uint8_t data4[] = {MCP23016_IPOL1, 0xff};

	twiPdc_write(data4, 2, MCP23016_TWI_ADDR);

	uint8_t data5[] = {MCP23016_IOCON0, 0x01};

	twiPdc_write(data5, 2, MCP23016_TWI_ADDR);
	
	pio_handler_set(MCP23016_INT_PORT, MCP23016_INT_PORT_ID, (1 << MCP23016_INT_PIN),
	PIO_IT_FALL_EDGE, mcp23016_irq_handler);
	
	pio_handler_set_priority(MCP23016_INT_PORT, MCP23016_IRQ_ID, MCP23016_INTERRUPT_PRIO);
	pio_enable_interrupt(MCP23016_INT_PORT, (1 << MCP23016_INT_PIN));
}

uint16_t mcp23016_read_pindata (void)
{
	twiPdc_disable();
	
	uint32_t cpu_freq = sysclk_get_cpu_hz();
	
	twi_set_speed(TWI0, MCP23016_TWI_FREQ, cpu_freq);
	
	uint8_t buffer[2];
	twi_packet_t packet;
		
	packet.buffer = buffer;
	packet.length = 3;
	packet.chip = MCP23016_TWI_ADDR;
	packet.addr[0] = MCP23016_GP0;
	packet.addr_length = 1;
	
	twi_master_read(TWI0, &packet);
	
	twi_set_speed(TWI0, LCD_TWI_FREQ, cpu_freq);
	twiPdc_enable();
	
	return (buffer[0] | (buffer[1] << 8));
}

void mcp23016_irq_handler (uint32_t id, uint32_t mask)
{
	if (pio_get_pin_value(MCP23016_INT_PIN_NUM))
		return;
		
	uint16_t keys;
	
	keys = mcp23016_read_pindata();
	
	keyboard_handler(keys);
}

#ifdef __cplusplus
}
#endif