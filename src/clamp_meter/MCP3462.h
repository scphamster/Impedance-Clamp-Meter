/*
 * MCP3462.h
 *
 * Created: 23.09.2021 21:42:19
 *  Author: malygosstationar
 */ 

#include <asf.h>

#ifndef MCP3462_H_
#define MCP3462_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _refsel_type {
	REF_CH0 = 0,
	REF_CH1,
	REF_CH2,
	REF_CH3,
	REF_CH4,
	REF_CH5,
	REF_CH6,
	REF_CH7,
	REF_AGND,
	REF_AVDD,
	REF_REFIN_POS = 11,
	REF_REFIN_NEG,
	REF_DIODE_P,
	REF_DIODE_M,
	REF_VCM
} refsel_type_t;

typedef enum{
	CLK_SEL_EXT = 0,
	CLK_SEL_INT_OUTDIS =  2,
	CLK_SEL_INT_OUTEN
} clk_sel_type_t;

typedef enum{
	CS_SEL_0 = 0,
	CS_SEL_0P9,
	CS_SEL_3P7,
	CS_SEL_15
} cs_sel_type_t;

typedef enum{
	ADC_SHUTDOWN_MODE = 0,
	ADC_STBY_MODE = 2,
	ADC_CONV_MODE
} adc_conv_mode_type_t;

typedef enum{
	PRE_0 = 0,
	PRE_2,
	PRE_4,
	PRE_8
} prescaller_type_t;

typedef enum{
	OSR_32 = 0,
	OSR_64,
	OSR_128,
	OSR_256,
	OSR_512,
	OSR_1024,
	OSR_2048,
	OSR_4096,
	OSR_8192,
	OSR_16384,
	OSR_20480,
	OSR_24576,
	OSR_40960,
	OSR_49152,
	OSR_81920,
	OSR_98304
} osr_type_t;

typedef enum{
	GAIN_1V3 = 0,
	GAIN_1,
	GAIN_2,
	GAIN_4,
	GAIN_8,
	GAIN_16,
	GAIN_32,
	GAIN_64
} gain_type_t;

typedef enum{
	BOOST_0P5 = 0,
	BOOST_0P66,
	BOOST_1,
	BOOST_2
} boost_type_t;

typedef enum{
	CONV_MODE_ONESHOT_SHUTDOWN = 0,
	CONV_MODE_ONESHOT_STBY = 2,
	CONV_MODE_CONT
} conv_mode_type_t;

void	MCP3462_init			(void);
int32_t MCP3462_read			(uint16_t ch_nb);
void	MCP3462_enable_clock	(void);
void	MCP3462_disable_clock	(void);
void	MCP3462_set_gain		(gain_type_t gain);
void	MCP3462_set_mux			(uint8_t positive_ch, uint8_t negative_ch);

#ifdef __cplusplus
}
#endif

#endif /* MCP3462_H_ */

