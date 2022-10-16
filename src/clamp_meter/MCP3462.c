#pragma once

#include "MCP3462.h"
#include "pmc.h"
#include "pio.h"
#include "spi.h"
#include "system_init.h"

#define DEVICE_ADDR				0x40
#define DEVICE_ADDR_ACK			0x10
#define DEVICE_ADDR_ACK_MSK		0x38

#define CMD_ADDR_POS	0x2
#define CMD_ADDR_MSK	0x3C
#define CMD_ADDR(val)	(CMD_ADDR_MSK & (val << CMD_ADDR_POS))

#define CMD_TYPE_POS	0x0
#define CMD_TYPE_MSK	0x3
#define CMD_TYPE(val)	(CMD_TYPE_MSK & (val << CMD_TYPE_POS))

#define CMD_STATIC_READ			0x01
#define CMD_INCREMENTAL_WRITE	0x02
#define CMD_INCREMENTAL_READ	0x03

#define FAST_ADC_START_CONVERSION	0x28
#define FAST_ADC_STBY				0x2C
#define FAST_ADC_SHUTDOWN			0x48
#define FAST_FULL_SHUTDOWN			0x34
#define FAST_DEVICE_RESET			0x38

#define ADCDATA_ADDR			0x0

#define CONFIG0_REG_ADDR		0x1
#define CONFIG0_POS				0x6
#define CONFIG0_MSK				0xC0
#define CONFIG0(val)			(CONFIG0_MSK & (val << CONFIG0_POS))
#define CLK_SEL_POS				0x4
#define CLK_SEL_MSK				0x30
#define CLK_SEL(val)			(CLK_SEL_MSK & (val << CLK_SEL_POS))
#define CS_SEL_POS				0x2
#define CS_SEL_MSK				0xC
#define CS_SEL(val)				(CS_SEL_MSK & (val << CS_SEL_POS))
#define ADC_MODE_POS			0x0
#define ADC_MODE_MSK			0x3
#define ADC_MODE(val)			(ADC_MODE_MSK & (val << ADC_MODE_POS))

#define CONFIG1_REG_ADDR		0x2
#define PRE_POS					0x6
#define PRE_MSK					0xC0
#define PRE(val)				(PRE_MSK & (val << PRE_POS))
#define OSR_POS					0x2
#define OSR_MSK					0x3C
#define OSR(val)				(OSR_MSK & (val << OSR_POS))

#define CONFIG2_REG_ADDR		0x3
#define BOOST_POS				0x6
#define BOOST_MSK				0xC0
#define BOOST(val)				(BOOST_MSK & (val << BOOST_POS))
#define GAIN_POS				0x3
#define GAIN_MSK				0x38
#define GAIN(val)				(GAIN_MSK & (val << GAIN_POS))
#define AZ_MUX_POS				0x2
#define AZ_MUX_MSK				0x4
#define AZ_MUX(val)				(AZ_MUX_MSK & (val << AZ_MUX_POS))

#define CONFIG3_REG_ADDR		0x4
#define CONV_MODE_POS			0x6
#define CONV_MODE_MSK			0xC0
#define CONV_MODE(val)			(CONV_MODE_MSK & (val << CONV_MODE_POS))
#define DATA_FORMAT_POS			0x4
#define DATA_FORMAT_MSK			0x30
#define DATA_FORMAT(val)		(DATA_FORMAT_MSK & (val << DATA_FORMAT_POS))
#define CRC_FORMAT_POS			0x3
#define CRC_FORMAT_MSK			0x8
#define CRC_FORMAT(val)			(CRC_FORMAT_MSK & (val << CRC_FORMAT_POS))
#define EN_CRCCOM_POS			0x2
#define EN_CRCCOM_MSK			0x4
#define EN_CRCCOM(val)			(EN_CRCCOM_MSK & (val << EN_CRCCOM_POS))
#define EN_OFFCAL_POS			0x1
#define EN_OFFCAL_MSK			0x2
#define EN_OFFCAL(val)			(EN_OFFCAL_MSK & (val << EN_OFFCAL_POS))
#define EN_GAINCAL_POS			0x0
#define EN_GAINCAL_MSK			0x1
#define EN_GAINCAL(val)			(EN_GAINCAL_MSK & (val << EN_GAINCAL_POS))

#define IRQ_REG_ADDR			0x5
#define MUX_REG_ADDR			0x6
#define MUX_SET_VPOS(val)		(0xf0 & (val << 4))
#define SCAN_REG_ADDR			0x7
#define TIMER_REG_ADDR			0x8
#define OFFSETCAL_REG_ADDR		0x9
#define GAINCAL_REG_ADDR		0xA
#define LOCK_REG_ADDR			0xD
#define CRCCFG_REG_ADDR			0xF

#define CONFIG0_SHUTDOWN	0
#define CONFIG0_ACTIVE		1

#define LOCK_PASS				0xA5

#define CLOCK_PCK_ID		2
#define CLOCK_PCK_PRES		0
#define CLOCK_PIO			PIOA
#define CLOCK_PIO_PIN_MSK	0x40000UL
#define CLOCK_PIO_PERIPH	PIO_PERIPH_B

#ifdef __cplusplus
extern "C" {
#endif

void clock_init(void);
void test_com(void);
void send_configuration(void);

void clock_init(void)
{
	pmc_enable_pck(CLOCK_PCK_ID);
	
	while(!(pmc_is_pck_enabled(CLOCK_PCK_ID))){
		;
	}
	
	if (pmc_switch_pck_to_mainck(CLOCK_PCK_ID, CLOCK_PCK_PRES)){
		while(1){
			;
		}
	}
	
	pmc_disable_pck(CLOCK_PCK_ID);
	pio_set_peripheral(CLOCK_PIO, CLOCK_PIO_PERIPH, CLOCK_PIO_PIN_MSK);
}

void send_configuration(void)
{
	uint16_t config0_regval = 0;
	uint16_t config1_regval = 0;
	uint16_t config2_regval = 0;
	uint16_t config3_regval = 0;
	uint16_t irq_regval = 0;
	uint16_t mux_regval = 0;
	uint16_t firstbyte = 0;
	uint16_t retval1 = 0;
	uint16_t retval2 = 0;
	
	config0_regval = ADC_MODE(ADC_CONV_MODE) | CS_SEL(CS_SEL_0)
	| CLK_SEL(CLK_SEL_EXT) | CONFIG0(CONFIG0_ACTIVE);
	config1_regval = PRE(PRE_0) | OSR(OSR_256);
	config2_regval = BOOST(BOOST_2) | GAIN(GAIN_1) | AZ_MUX(0) | 1;
	config3_regval = CONV_MODE(CONV_MODE_CONT) | DATA_FORMAT(0)
	| CRC_FORMAT(0) | EN_CRCCOM(0) | EN_OFFCAL(0) | EN_GAINCAL(0);
	irq_regval = 0x4;
	mux_regval = MUX_SET_VPOS(REF_CH4) | REF_CH5;
	firstbyte = DEVICE_ADDR | CMD_ADDR(CONFIG0_REG_ADDR) | CMD_INCREMENTAL_WRITE;
	
	spi_write(SPI, firstbyte, 0, 0);
	spi_read(SPI, &retval1, NULL);
	spi_write(SPI, config0_regval, 0, 0);
	spi_write(SPI, config1_regval, 0, 0);
	spi_write(SPI, config2_regval, 0, 0);
	spi_write(SPI, config3_regval, 0, 0);
	spi_write(SPI, irq_regval, 0, 0);
	spi_write(SPI, mux_regval, 0, 0);
	spi_set_lastxfer(SPI);
}

void MCP3462_init(void)
{
	clock_init();
	send_configuration();
}

void MCP3462_set_gain(gain_type_t gain)
{
	uint32_t timeout_counter = SPI_TIMEOUT;
	uint16_t firstbyte;
	uint16_t config2_byte;
	uint16_t retval;
	firstbyte = DEVICE_ADDR | CMD_ADDR(CONFIG2_REG_ADDR) | CMD_INCREMENTAL_WRITE;
	config2_byte = BOOST(BOOST_2) | GAIN(gain) | AZ_MUX(0) | 1;
	
	
	while((!spi_is_tx_ready(SPI)) && (timeout_counter--));
	
	if (!timeout_counter)
		return;
		
	spi_configure_cs_behavior(SPI, 0, SPI_CS_RISE_FORCED);
	spi_set_bits_per_transfer(SPI, 0, SPI_CSR_BITS_16_BIT);
		
	spi_write(SPI, (firstbyte << 8) | config2_byte, 0, 0);
	spi_set_lastxfer(SPI);
}

void MCP3462_set_mux(uint8_t positive_ch, uint8_t negative_ch)
{
	uint32_t timeout_counter = SPI_TIMEOUT;
	uint16_t firstbyte;
	uint16_t mux_byte;
	uint16_t retval;
	
	if ((positive_ch > REF_CH5) || (negative_ch > REF_CH5))
		return;
	
	firstbyte = DEVICE_ADDR | CMD_ADDR(MUX_REG_ADDR) | CMD_INCREMENTAL_WRITE;
	mux_byte = MUX_SET_VPOS(positive_ch) | negative_ch;
	
	while((!spi_is_tx_ready(SPI)) && (timeout_counter--));
	
	if (!timeout_counter)
		return;
		
	spi_configure_cs_behavior(SPI, 0, SPI_CS_RISE_FORCED);
	spi_set_bits_per_transfer(SPI, 0, SPI_CSR_BITS_16_BIT);
		
	spi_write(SPI, (firstbyte << 8) | mux_byte, 0, 0);
	spi_set_lastxfer(SPI);
}

int32_t MCP3462_read(uint16_t data)
{
	uint32_t timeout_counter = SPI_TIMEOUT;
	uint8_t firstbyte = 0;
	uint16_t readout = 0;
	int32_t retval = 0;
	
	uint16_t data0 = 0;
	uint16_t data1 = 0;
	uint16_t data2 = 0;
	
	firstbyte = DEVICE_ADDR | CMD_ADDR(ADCDATA_ADDR) | CMD_STATIC_READ;
	
	while((!spi_is_tx_ready(SPI)) && (timeout_counter--));
	
	if (!timeout_counter)
		return;
	
	spi_set_bits_per_transfer(SPI, 0, SPI_CSR_BITS_8_BIT);
	spi_configure_cs_behavior(SPI, 0, SPI_CS_KEEP_LOW);
	
	spi_write(SPI, firstbyte, 0, 0);
	spi_read(SPI, &readout, NULL);
	spi_write(SPI, 0, 0, 0);
	spi_read(SPI, &data0, NULL);
	spi_write(SPI, 0, 0, 0);
	spi_read(SPI, &data1, NULL);
	spi_configure_cs_behavior(SPI, 0, SPI_CS_RISE_FORCED);
	spi_write(SPI, 0, 0, 0);
	spi_read(SPI, &data2, NULL);
	spi_set_lastxfer(SPI);
	
	retval =  data2| (data1 << 8) | (data0 << 16);
	
	if (retval & (1 << 23)) {
		retval--;
		retval = -(~retval & (0xffffff));
	}
	
	return retval;
}

void MCP3462_enable_clock(void)
{
	pmc_enable_pck(CLOCK_PCK_ID);
	//TODO add feedback about current status
}

void MCP3462_disable_clock(void)
{
	pmc_disable_pck(CLOCK_PCK_ID);
	//TODO add feedback about current status
}

#ifdef __cplusplus
}
#endif