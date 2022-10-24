/*
 * signal_conditioning.c
 *
 * Created: 01.11.2021 7:50:17
 *  Author: malygosstationar
 */
#include "asf.h"
#include "signal_conditioning.h"
#include "MCP3462.h"
#include "DSP_functions.h"
#include "system_init.h"
#include "system.h"
#include "LCD1608.h"
#include "menu.h"
#include "menu_calibration.h"
#include "arm_math.h"
#include "external_periph_ctrl.h"

#define TEST_PAGE_ADDRESS COEFFS_FLASH_START_ADDR

#ifdef __cplusplus
extern "C" {
#endif

Analog_t           Analog;
Calibrator_t       Calibrator;
calibration_data_t Cal_data;
bool               flash_test_runing;

void
shunt_sensor_set_gain(uint8_t gain)
{
    switch (gain) {
    case 0: {
        pio_pull_down(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, 0);

        pio_pull_down(PIOA, SH_SENSOR_GAIN_A_PIN, 0);

        pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, true);

        pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, true);
    } break;

    case 1: {
        pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN, false);
        pio_pull_down(PIOD, SH_SENSOR_GAIN_B_PIN, true);

        pio_pull_down(PIOD, SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, false);
        pio_pull_down(PIOA, SH_SENSOR_GAIN_A_PIN, false);
        pio_pull_up(PIOD, SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, true);
        pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, true);
    } break;

    case 2: {
        pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, false);
        pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN, false);
        pio_pull_down(PIOA, SH_SENSOR_GAIN_A_PIN, true);
        pio_pull_down(PIOD, SH_SENSOR_GAIN_B_PIN, true);

        pio_pull_down(PIOD, SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, false);
        pio_pull_up(PIOD, SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, true);
    } break;

    case 3: {
        pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, false);
        pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_D_PIN, false);
        pio_pull_down(PIOA, SH_SENSOR_GAIN_A_PIN, true);
        pio_pull_down(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_D_PIN, true);

        pio_pull_down(PIOD, SH_SENSOR_GAIN_C_PIN, false);
        pio_pull_up(PIOD, SH_SENSOR_GAIN_C_PIN, true);
    } break;

    case 4: {
        pio_pull_up(PIOA, SH_SENSOR_GAIN_A_PIN, false);
        pio_pull_up(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, false);
        pio_pull_down(PIOA, SH_SENSOR_GAIN_A_PIN, true);
        pio_pull_down(PIOD, SH_SENSOR_GAIN_B_PIN | SH_SENSOR_GAIN_C_PIN | SH_SENSOR_GAIN_D_PIN, true);
    } break;
    }
}

void
clamp_sensor_set_gain(uint8_t gain)
{
    switch (gain) {
    case 0: {
        /*turn off all pull_up*/
        pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, false);

        pio_pull_up(PIOD, CLAMP_SENSOR_GAIN_A_PIN, false);

        /* turn on all pull_down */
        pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, true);

        pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);
    } break;

    case 1: {
        /*turn off pull_down at A, turn off pull_up at B,C,D*/
        pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, false);
        pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, false);

        /* turn on pull_up at A, turn on pull_down at B,C,D */
        pio_pull_up(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);
        pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, true);
    } break;

    case 2: {
        /*turn off pull_down at B, turn off pull_up at A,C,D*/
        pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN, false);
        pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, false);
        pio_pull_up(PIOD, CLAMP_SENSOR_GAIN_A_PIN, false);

        /* turn on pull_up at B, turn on pull_down at A,C,D */
        pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_B_PIN, true);
        pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);
        pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_C_PIN | CLAMP_SENSOR_GAIN_D_PIN, true);
    } break;

        // 	case 3: {
        // 		/*turn off pull_down at C, turn off pull_up at A,B,D*/
        // 		pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_C_PIN, false);
        // 		pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_D_PIN, false);
        // 		pio_pull_up(PIOD, CLAMP_SENSOR_GAIN_A_PIN, false);
        //
        // 		/* turn on pull_up at C, turn on pull_down at A,B,D */
        // 		pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_C_PIN, true);
        // 		pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);
        // 		pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_D_PIN, true);
        // 	}
        // 	break;

    case 3:
    case 4: {
        /*turn off pull_down at D, turn off pull_up at A,B,C*/
        pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_D_PIN, false);
        pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN, false);
        pio_pull_up(PIOD, CLAMP_SENSOR_GAIN_A_PIN, false);

        /* turn on pull_up at D, turn on pull_down at A,B,C*/
        pio_pull_up(PIOA, CLAMP_SENSOR_GAIN_D_PIN, true);
        pio_pull_down(PIOD, CLAMP_SENSOR_GAIN_A_PIN, true);
        pio_pull_down(PIOA, CLAMP_SENSOR_GAIN_B_PIN | CLAMP_SENSOR_GAIN_C_PIN, true);
    } break;
    }
}

void
adc_increase_gain(void)
{
    if (Analog.adc_gain == GAIN_64)
        return;

    Analog.adc_gain++;
    Analog.ampl_is_too_low = false;
    MCP3462_set_gain(Analog.adc_gain);
}

void
adc_decrease_gain(void)
{
    if (Analog.adc_gain == GAIN_1V3)
        return;

    Analog.adc_gain--;
    Analog.ampl_is_too_high = false;
    MCP3462_set_gain(Analog.adc_gain);
}

void
composite_gain_controll(sensor_type_t sensor)
{
    if (sensor == SHUNT_SENSOR) {
        if (Analog.adc_gain != shunt_sensor_adc_gain_preset[Analog.overall_gain]) {
            Analog.adc_gain = shunt_sensor_adc_gain_preset[Analog.overall_gain];
            MCP3462_set_gain(Analog.adc_gain);
        }

        if (Analog.shunt_sensor_gain != shunt_sensor_gain_preset[Analog.overall_gain]) {
            Analog.shunt_sensor_gain = shunt_sensor_gain_preset[Analog.overall_gain];
            shunt_sensor_set_gain(Analog.shunt_sensor_gain);
        }
    }
    else if (sensor == CLAMP_SENSOR) {
        if (Analog.adc_gain != clamp_sensor_adc_gain_preset[Analog.overall_gain]) {
            Analog.adc_gain = clamp_sensor_adc_gain_preset[Analog.overall_gain];
            MCP3462_set_gain(Analog.adc_gain);
        }

        if (Analog.clamp_sensor_gain != clamp_sensor_gain_preset[Analog.overall_gain]) {
            Analog.clamp_sensor_gain = clamp_sensor_gain_preset[Analog.overall_gain];
            clamp_sensor_set_gain(Analog.clamp_sensor_gain);
        }
    }
    else
        Analog.error_occured = true;
}

void
increase_gain(void)
{
    if (Calibrator.is_calibrating)
        return;

    if (Analog.selected_sensor == SHUNT_SENSOR) {
        if (Analog.overall_gain < SHUNT_SENSOR_MAX_OVERALL_GAIN)
            Analog.overall_gain++;
    }
    else if (Analog.selected_sensor == CLAMP_SENSOR) {
        if (Analog.overall_gain < CLAMP_SENSOR_MAX_OVERALL_GAIN)
            Analog.overall_gain++;
    }
    else {
        Analog.error_occured = true;
        return;
    }

    composite_gain_controll(Analog.selected_sensor);
}

void
decrease_gain(void)
{
    if (Calibrator.is_calibrating)
        return;

    if (Analog.overall_gain > 0)
        Analog.overall_gain--;
    else
        return;

    if ((Analog.selected_sensor != SHUNT_SENSOR) && (Analog.selected_sensor != CLAMP_SENSOR)) {
        Analog.error_occured = true;
        return;
    }

    composite_gain_controll(Analog.selected_sensor);
}

void
measurement_start(void)
{
    const uint32_t adc_int_pin = (1 << ADC_INTERRUPT_PIN);

    hi_voltage_enable();
    reset_filters();
    dacc_init();
    dacc_set_trigger(DACC, DACC_TRGSEL);
    dsp_calculate_sine_table(Analog.generator_amplitude);
    pdc_tx_init(g_dacc_pdc_base, &g_dacc_packet, &g_dacc_next_packet);
    pdc_enable_transfer(g_dacc_pdc_base, PERIPH_PTCR_TXTEN);

    NVIC_ClearPendingIRQ(DACC_IRQn);
    NVIC_ClearPendingIRQ(PIOA_IRQn);

    dacc_enable_interrupt(DACC, DACC_INTERRUPT_MASK);
    // todo test
    //    pio_enable_interrupt(PIOA, adc_int_pin);
    //    MCP3462_enable_clock();
    //    delay_ms(100);

    output_enable();
    Analog.generator_is_active = true;
}

void
measurement_stop(void)
{
    const uint32_t adc_int_pin = (1 << ADC_INTERRUPT_PIN);
    output_disable();
    hi_voltage_disable();
    //todo test
//    MCP3462_disable_clock();
    dacc_disable_interrupt(DACC, DACC_INTERRUPT_MASK);
    pdc_disable_transfer(g_dacc_pdc_base, PERIPH_PTCR_TXTDIS);
    dacc_disable_trigger(DACC);
//    pio_disable_interrupt(PIOA, adc_int_pin);
    Analog.generator_is_active = false;
}

void
switch_sensing_chanel(sensor_type_t switch_to_sensor)
{
    switch (switch_to_sensor) {
    case VOLTAGE_SENSOR: {
        MCP3462_set_mux(REF_CH4, REF_CH5);
        Analog.AGC_on = false;

        if (Analog.adc_gain != GAIN_1) {
            MCP3462_set_gain(GAIN_1);
            Analog.adc_gain = GAIN_1;
        }

        shunt_sensor_set_gain(0);
        clamp_sensor_set_gain(0);

        Analog.clamp_sensor_gain = 0;
        Analog.shunt_sensor_gain = 0;
    } break;

    case SHUNT_SENSOR:
        MCP3462_set_mux(REF_CH2, REF_CH3);
        {
            Analog.AGC_on = true;
            clamp_sensor_set_gain(0);
            Analog.clamp_sensor_gain = 0;
        }
        break;

    case CLAMP_SENSOR:
        MCP3462_set_mux(REF_CH0, REF_CH1);
        {
            Analog.AGC_on = true;
            shunt_sensor_set_gain(0);
            Analog.shunt_sensor_gain = 0;
        }
    }

    Analog.selected_sensor = switch_to_sensor;
}

bool
store_coeffs_to_flash_struct(void)
{
    uint32_t            start_addr  = COEFFS_FLASH_START_ADDR;
    calibration_data_t *end_storage = (calibration_data_t *)start_addr;
    uint32_t            data_size   = sizeof(calibration_data_t);
    uint32_t            rc;
    uint32_t            idx;

    rc = flash_unlock(start_addr, start_addr + data_size - 1, 0, 0);

    if (rc != FLASH_RC_OK) {
        system_fault_set(FAULT_SW_FLASH, 1, rc);
        return true;
    }

    rc = flash_erase_page(start_addr, COEFFS_FLASH_PAGES_OCCUPIED);

    if (rc != FLASH_RC_OK) {
        system_fault_set(FAULT_SW_FLASH, 2, rc);
        return true;
    }

    rc = flash_write(start_addr, &Cal_data, data_size, 0);

    if (rc != FLASH_RC_OK) {
        system_fault_set(FAULT_SW_FLASH, 3, rc);
        return true;
    }

    rc = flash_lock(start_addr, start_addr + data_size - 1, 0, 0);

    if (rc != FLASH_RC_OK) {
        system_fault_set(FAULT_SW_FLASH, 4, rc);
        return true;
    }

    /*for (idx = 0; idx < n_coeffs; idx++) {
        if (buffer[idx] != end_storage[idx]) {
            system_fault_set(FAULT_SW_FLASH, 5, rc);
            return true;
        }
    }*/

    return false;
}

void
recall_coeffs_from_flash_struct(void)
{
    uint32_t            start_addr  = COEFFS_FLASH_START_ADDR;
    calibration_data_t *end_storage = (calibration_data_t *)start_addr;
    uint32_t            data_size   = sizeof(calibration_data_t);
    uint32_t            rc;
    uint32_t            idx;

    if (end_storage->data_password == CALIBRATION_DATA_NOT_ERASED_MARKER) {
        Analog.is_calibrated = true;

        for (idx = 0; idx < SHUNT_SENSOR_GAIN_COEFFS_NUM; idx++) {
            Cal_data.shunt_gain[idx] = end_storage->shunt_gain[idx];

            Cal_data.shunt_phi[idx] = end_storage->shunt_phi[idx];
        }

        for (idx = 0; idx < CLAMP_SENSOR_GAIN_COEFFS_NUM; idx++) {
            Cal_data.clamp_gain[idx] = end_storage->clamp_gain[idx];

            Cal_data.clamp_phi[idx] = end_storage->clamp_phi[idx];
        }

        Cal_data.v_sens_gain = end_storage->v_sens_gain;
        Cal_data.v_sens_phi  = end_storage->v_sens_phi;
    }
    else {
        Analog.is_calibrated = false;

        for (idx = 0; idx < CLAMP_SENSOR_GAIN_COEFFS_NUM; idx++) {
            Cal_data.clamp_gain[idx] = Reserve_cal_data.clamp_gain[idx];

            Cal_data.clamp_phi[idx] = Reserve_cal_data.clamp_phi[idx];
        }

        for (idx = 0; idx < SHUNT_SENSOR_GAIN_COEFFS_NUM; idx++) {
            Cal_data.shunt_gain[idx] = Reserve_cal_data.shunt_gain[idx];

            Cal_data.shunt_phi[idx] = Reserve_cal_data.shunt_phi[idx];
        }
        Cal_data.v_sens_gain = Reserve_cal_data.v_sens_gain;
        Cal_data.v_sens_phi  = Reserve_cal_data.v_sens_phi;
    }
}

#ifdef __cplusplus
}
#endif