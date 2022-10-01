#pragma once
#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif
#define MCP23016_TWI_ADDR		0b00100000	//SLA+W
#define MCP23016_INT_PIN		11
#define MCP23016_INT_PORT		PIOD
#define MCP23016_INT_PORT_ID	ID_PIOD
#define MCP23016_IRQ_ID			PIOD_IRQn
#define MCP23016_INT_PIN_NUM	MCP23016_INT_PIN + 32 * 3 
#define MCP23016_INTERRUPT_PRIO	3
#define MCP23016_TWI_FREQ		200000UL

#ifdef __cplusplus
extern "C" {
#endif

void		mcp23016_init				(void);
uint16_t	mcp23016_read_pindata		(void);

#ifdef __cplusplus
}
#endif
