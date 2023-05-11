/*
 * periherals_ctrl.c
 *
 * Created: 12.11.2021 8:48:08
 *  Author: malygosstationar
 */

#include "asf.h"
#include "external_periph_ctrl.h"
#ifdef __cplusplus
extern "C" {
#endif
void hv_ctrl_io_init(void);
void vout_ctrl_io_init(void);
void gain_ctrl_io_init(void);

//External_periph_status_t External_periph_status;

//void hv_ctrl_io_init(void)
//{
//	if (!(pmc_is_periph_clk_enabled(HV_EN_PIN_PORT_ID)))
//		pmc_enable_periph_clk(HV_EN_PIN_PORT_ID);
//
//	pio_set_output(HV_EN_PIN_PORT, (1 << HV_EN_PIN), 0, 0, 0);
//	External_periph_status.hi_voltage_is_enabled = false;
//}
//
//void vout_ctrl_io_init(void)
//{
//	if (!(pmc_is_periph_clk_enabled(VOUT_CTRL_PIN_PORT_ID)))
//		pmc_enable_periph_clk(VOUT_CTRL_PIN_PORT_ID);
//
//	pio_set_output(VOUT_CTRL_PIN_PORT, (1 << VOUT_CTRL_PIN), (1 << VOUT_CTRL_PIN),
//	               0, 0);
//	External_periph_status.output_is_enabled = false;
//}

void gain_ctrl_io_init(void)
{
	pio_set_input(PIOA, SH_SENSOR_GAIN_A_PIN | CLAMP_SENSOR_GAIN_B_PIN |
	              CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, 0);

	pio_set_input(PIOD, CLAMP_SENSOR_GAIN_A_PIN | SH_SENSOR_GAIN_B_PIN |
	              SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, 0);

	pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN |
	              CLAMP_SENSOR_GAIN_D_PIN, true);

	pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);

	pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, true);
	pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN |
	            SH_SENSOR_GAIN_D_PIN, true);
}

void external_periph_ctrl_init(void)
{
	hv_ctrl_io_init();
	vout_ctrl_io_init();
	gain_ctrl_io_init();
}

void hi_voltage_enable(void)
{
	pio_set(PIOD, (1 << HV_EN_PIN));
}

void hi_voltage_disable(void)
{
	pio_clear(PIOD, (1 << HV_EN_PIN));
}

void output_enable(void)
{
	pio_clear(VOUT_CTRL_PIN_PORT, (1 << VOUT_CTRL_PIN));
}

void output_disable(void)
{
	pio_set(VOUT_CTRL_PIN_PORT, (1 << VOUT_CTRL_PIN));
}
#ifdef __cplusplus
}
#endif