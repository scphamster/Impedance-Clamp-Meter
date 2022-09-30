/*
 * DSP_functions.h
 *
 * Created: 14.10.2021 19:06:58
 *  Author: malygosstationar
 */

#ifndef DSP_FUNCTIONS_H_
#define DSP_FUNCTIONS_H_
#include "arm_math.h"
#include "MCP3462.h"
#define ADC_TEST_DEF

#define ADC_INTERRUPT_PIN   2
#define ADC_INTERRUPT_PRIO  2
#define DACC_INTERRUPT_PRIO 2
#define DACC_INTERRUPT_MASK 0x04

#define SINTABLE_LEN 22

#define DACC_PACKETLEN                  SINTABLE_LEN
#define DACC_OFFSET_HALFSCALE           2047
#define DACC_MAX_AMPLITUDE              50
#define DACC_VOLTS_TO_DIGITS_CONV_COEFF 40

#define REAL_AMPLITUDE_MAX        37.15
#define SHUNT_SENSOR_GAIN_0_COEFF 1.9406e-08
#define SHUNT_SENSOR_GAIN_1_COEFF 7.7274e-09
#define SHUNT_SENSOR_GAIN_2_COEFF 3.8774e-09
#define SHUNT_SENSOR_GAIN_3_COEFF 1.1933e-10
#define SHUNT_SENSOR_GAIN_4_COEFF 6.1571e-11

#define R_SHUNT 1000

#define FIR1_DEC_BLOCKSIZE 100
#define FIR1_DEC_NCOEFFS   5
#define FIR_DEC_FACTOR     10

#define FIR2_DEC_BLOCKSIZE 10
#define FIR2_DEC_NCOEFFS   5

#define BIQUAD1_NSTAGES  2
#define BIQUAD1_BUFFSIZE FIR1_DEC_BLOCKSIZE

#define BIQUAD2_NSTAGES  2
#define BIQUAD2_BUFFSIZE FIR2_DEC_BLOCKSIZE

#define BIQUAD3_NSTAGES  3
#define BIQUAD3_BUFFSIZE 1

#define INTEGRATOR_LENGTH 300

#ifdef __cplusplus
extern "C" {
#endif

//lastchange: extern added, static cleared
extern volatile float32_t g_costable_f[DACC_PACKETLEN];
extern volatile float32_t g_sintable_f[DACC_PACKETLEN];

extern pdc_packet_t g_dacc_packet;
extern pdc_packet_t g_dacc_next_packet;
extern Pdc         *g_dacc_pdc_base;
extern volatile uint16_t     g_sintable[DACC_PACKETLEN];
extern bool decimator_databuff_is_ready;
extern bool fir2_dataready_flag;
extern uint32_t test_counter_dacc;
extern uint32_t test_counter_adc;

typedef struct {
    bool new_data_is_ready;

    bool      clamp_pos_gain_calibrated;
    float32_t clamp_pos_gain;

    float32_t sin_vect_postfilter;
    float32_t cos_vect_postfilter;

    uint32_t integrator_len;

    float32_t R_ovrl;
    float32_t X_ovrl;
    float32_t Z_ovrl;
    float32_t Z_ovrl_phi;

    float32_t R_clamp;
    float32_t X_clamp;
    float32_t Z_clamp;
    float32_t Z_clamp_phi;

    float32_t V_ovrl_I;
    float32_t V_ovrl_Q;
    float32_t V_ovrl;
    float32_t V_ovrl_phi;
    float32_t V_ovrl_phi_orig;

    float32_t V_applied_I;
    float32_t V_applied_Q;
    float32_t V_applied;
    float32_t V_applied_phi;

    float32_t V_shunt_I;
    float32_t V_shunt_Q;
    float32_t V_shunt;
    float32_t V_shunt_phi;

    float32_t I_ovrl_I_norm;
    float32_t I_ovrl_Q_norm;
    float32_t I_ovrl_phi_norm;

    float32_t I_clamp_I;
    float32_t I_clamp_Q;
    float32_t I_clamp;
    float32_t I_clamp_phi;

    float32_t degree;
} Dsp_t;

extern Dsp_t Dsp;

void      dsp_calculate_sine_table(uint16_t amplitude);
void      adc_interrupt_handler(uint32_t id, uint32_t mask);
void      dsp_init(void);
void      dsp_integrating_filter(void);
void      do_filter(float32_t *sin_out, float32_t *cos_out);
void      reset_filters(void);
float32_t find_angle(float32_t sine, float32_t cosine, float32_t absval);

static volatile float32_t sin_table[] = { 0,
                                 0.28173256f,
                                 0.54064083f,
                                 0.75574958f,
                                 0.90963197f,
                                 0.98982143f,
                                 0.98982143f,
                                 0.90963197f,
                                 0.75574958f,
                                 0.54064083f,
                                 0.28173256f,
                                 1.2246469e-16f,
                                 -0.28173256f,
                                 -0.54064083f,
                                 -0.75574958f,
                                 -0.90963197f,
                                 -0.98982143f,
                                 -0.98982143f,
                                 -0.90963197f,
                                 -0.75574958f,
                                 -0.54064083f,
                                 -0.28173256f };

static volatile float32_t cos_table[] = { 1,
                                 0.95949298f,
                                 0.84125352f,
                                 0.65486073f,
                                 0.41541502f,
                                 0.14231484f,
                                 -0.14231484f,
                                 -0.41541502f,
                                 -0.65486073f,
                                 -0.84125352f,
                                 -0.95949298f,
                                 -1,
                                 -0.95949298f,
                                 -0.84125352f,
                                 -0.65486073f,
                                 -0.41541502f,
                                 -0.14231484f,
                                 0.14231484f,
                                 0.41541502f,
                                 0.65486073f,
                                 0.84125352f,
                                 0.95949298f };

#ifdef __cplusplus
}
#endif
#endif /* DSP_FUNCTIONS_H_ */