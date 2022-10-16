/*
 * DSP_functions.c
 *
 * Created: 14.10.2021 19:06:39
 *  Author: malygosstationar
 */

#include "asf.h"
#include "arm_math.h"
#include "fastmath.h"
#include "DSP_functions.h"
#include "MCP3462.h"
#include "pio.h"
#include "system_init.h"
#include "signal_conditioning.h"
#include "menu_calibration.h"

// #define TEST_DATA_LEN 100
#ifdef __cplusplus
extern "C" {
#endif

void manage_sensed_data(float32_t sin_vect, float32_t cos_vect);

volatile float32_t g_costable_f[DACC_PACKETLEN];
volatile float32_t g_sintable_f[DACC_PACKETLEN];

uint16_t biquad1_counter = 0;

pdc_packet_t      g_dacc_packet;
pdc_packet_t      g_dacc_next_packet;
Pdc              *g_dacc_pdc_base;
volatile uint16_t g_sintable[DACC_PACKETLEN];
bool              decimator_databuff_is_ready;
bool              fir2_dataready_flag;
uint32_t          test_counter_dacc;
uint32_t          test_counter_adc;

Dsp_t Dsp;

#ifdef TEST_DATA_LEN
float32_t my_test_data[TEST_DATA_LEN];
float32_t my_test_data2[TEST_DATA_LEN];
#endif

uint16_t my_counter = 0;

uint32_t  phase_counter;
float32_t adc_sin_prod;
float32_t adc_cos_prod;
bool      use_next_buffer = false;

float32_t fir1_sin[FIR1_DEC_BLOCKSIZE / FIR_DEC_FACTOR];
float32_t fir1_cos[FIR1_DEC_BLOCKSIZE / FIR_DEC_FACTOR];
float32_t fir2_sin[FIR2_DEC_BLOCKSIZE / FIR_DEC_FACTOR];
float32_t fir2_cos[FIR2_DEC_BLOCKSIZE / FIR_DEC_FACTOR];
float32_t biquad1_sin_buff_A[BIQUAD1_BUFFSIZE];
float32_t biquad1_cos_buff_A[BIQUAD1_BUFFSIZE];
float32_t biquad1_sin_buff_B[BIQUAD1_BUFFSIZE];
float32_t biquad1_cos_buff_B[BIQUAD1_BUFFSIZE];
float32_t biquad2_sin[FIR2_DEC_BLOCKSIZE];
float32_t biquad2_cos[FIR2_DEC_BLOCKSIZE];
float32_t biquad3_sin;
float32_t biquad3_cos;

arm_fir_decimate_instance_f32 firdec1_sin_inst;
arm_fir_decimate_instance_f32 firdec1_cos_inst;

arm_fir_decimate_instance_f32 firdec2_sin_inst;
arm_fir_decimate_instance_f32 firdec2_cos_inst;

arm_biquad_cascade_df2T_instance_f32 biquad1_sin_inst;
arm_biquad_cascade_df2T_instance_f32 biquad1_cos_inst;

arm_biquad_cascade_df2T_instance_f32 biquad2_sin_inst;
arm_biquad_cascade_df2T_instance_f32 biquad2_cos_inst;

arm_biquad_cascade_df2T_instance_f32 biquad3_sin_inst;
arm_biquad_cascade_df2T_instance_f32 biquad3_cos_inst;

void dacc_setup(void);
void adc_interrupt_init(void);
void filters_init(void);

static inline void
check_amplitude(int32_t data)
{
    if (Calibrator.is_calibrating)
        return;

    static uint32_t check_counter = 0;

    if (data < 0)
        data = -data;

    if (data > AMPLITUDE_TOO_HIGH_LIMIT)
        Analog.ampl_too_high_counter++;
    else if (data < AMPLITUDE_TOO_LOW_LIMIT)
        Analog.ampl_too_low_counter++;

    if (Analog.ampl_too_high_counter > AMPLITUDE_HIGH_COUNTER_LIMIT) {
        Analog.ampl_is_too_high = true;

        if (Analog.AGC_on)
            decrease_gain();
    }
    else if (Analog.ampl_too_low_counter > AMPLITUDE_LOW_COUNTER_LIMIT) {
        Analog.ampl_is_too_low = true;

        if (Analog.AGC_on)
            increase_gain();
    }

    check_counter++;

    if (check_counter > AMPLITUDE_LOW_COUNTER_LIMIT) {
        check_counter               = 0;
        Analog.ampl_too_low_counter = 0;
    }
}

void
adc_interrupt_handler(uint32_t id, uint32_t mask)
{
    int32_t    adc_data;
    float32_t *biquad1_sin_buffer;
    float32_t *biquad1_cos_buffer;
    float32_t  sinprod_buff;
    float32_t  cosprod_buff;

    test_counter_adc++;

    if (use_next_buffer) {
        biquad1_sin_buffer = &biquad1_sin_buff_A[biquad1_counter];
        biquad1_cos_buffer = &biquad1_cos_buff_A[biquad1_counter];
    }
    else {
        biquad1_sin_buffer = &biquad1_sin_buff_B[biquad1_counter];
        biquad1_cos_buffer = &biquad1_cos_buff_B[biquad1_counter];
    }

    adc_data = MCP3462_read(0);
    check_amplitude(adc_data);

#ifdef TEST_DATA_LEN
    static uint16_t counter2 = 0;

    my_test_data[counter2] = adc_data;
    // my_test_data2[counter2] = biquad1_sin_buffer[0];

    if (counter2 == TEST_DATA_LEN - 1)
        counter2 = 0;
    else
        counter2++;

#endif

    sinprod_buff = sin_table[phase_counter] * adc_data;
    cosprod_buff = cos_table[phase_counter] * adc_data;

    arm_biquad_cascade_df2T_f32(&biquad1_sin_inst, &sinprod_buff, biquad1_sin_buffer, 1);

    arm_biquad_cascade_df2T_f32(&biquad1_cos_inst, &cosprod_buff, biquad1_cos_buffer, 1);

    if (biquad1_counter == (BIQUAD1_BUFFSIZE - 1)) {
        biquad1_counter = 0;

        decimator_databuff_is_ready = true;

        if (use_next_buffer)
            use_next_buffer = false;
        else
            use_next_buffer = true;

        Analog.ampl_too_high_counter = 0;
    }
    else
        biquad1_counter++;

    if (phase_counter == (DACC_PACKETLEN - 1))
        phase_counter = 0;
    else
        phase_counter++;
}

void
DACC_Handler(void)
{
    pdc_tx_init(g_dacc_pdc_base, NULL, &g_dacc_next_packet);
}

void
dsp_calculate_sine_table(uint16_t amplitude)
{
    uint16_t counter = 0;
    uint16_t offs    = DACC_OFFSET_HALFSCALE;

    if (amplitude > DACC_MAX_AMPLITUDE) {
        amplitude            = DACC_MAX_AMPLITUDE;
        Analog.error_occured = true;
    }

    amplitude = amplitude * DACC_VOLTS_TO_DIGITS_CONV_COEFF;

    while (counter < DACC_PACKETLEN) {
        g_sintable[counter] = (uint16_t)((float)amplitude * sin_table[counter] + (float)offs);
        counter++;
    }
}

void
dacc_setup(void)
{
    g_dacc_pdc_base            = dacc_get_pdc_base(DACC);
    g_dacc_packet.ul_addr      = (uint32_t)g_sintable;
    g_dacc_packet.ul_size      = DACC_PACKETLEN;
    g_dacc_next_packet.ul_addr = (uint32_t)g_sintable;
    g_dacc_next_packet.ul_size = DACC_PACKETLEN;

    NVIC_ClearPendingIRQ(DACC_IRQn);
    NVIC_SetPriority(DACC_IRQn, DACC_INTERRUPT_PRIO);
    NVIC_EnableIRQ(DACC_IRQn);
}

void
adc_interrupt_init(void)
{
    pio_handler_set(PIOA, ID_PIOA, (1 << ADC_INTERRUPT_PIN), PIO_IT_FALL_EDGE, adc_interrupt_handler);
    pio_handler_set_priority(PIOA, PIOA_IRQn, ADC_INTERRUPT_PRIO);
}

void
filters_init(void)
{
    static float32_t fir1_statebuff_sin[FIR1_DEC_BLOCKSIZE + FIR1_DEC_NCOEFFS - 1];
    static float32_t fir1_statebuff_cos[FIR1_DEC_BLOCKSIZE + FIR1_DEC_NCOEFFS - 1];
    static float32_t fir2_statebuff_sin[FIR2_DEC_BLOCKSIZE + FIR2_DEC_NCOEFFS - 1];
    static float32_t fir2_statebuff_cos[FIR2_DEC_BLOCKSIZE + FIR2_DEC_NCOEFFS - 1];
    static float32_t biquad1_statebuff_sin[BIQUAD1_NSTAGES * 2];
    static float32_t biquad1_statebuff_cos[BIQUAD1_NSTAGES * 2];
    static float32_t biquad2_statebuff_sin[BIQUAD2_NSTAGES * 2];
    static float32_t biquad2_statebuff_cos[BIQUAD2_NSTAGES * 2];
    static float32_t biquad3_statebuff_sin[BIQUAD3_NSTAGES * 2];
    static float32_t biquad3_statebuff_cos[BIQUAD3_NSTAGES * 2];

    static float32_t fir1_3kHz_coeffs[] = { 0.02868781797587871551513671875f,
                                            0.25f,
                                            0.4426243603229522705078125f,
                                            0.25f,
                                            0.02868781797587871551513671875f };

    static float32_t fir2_300Hz_coeffs[] = { 0.02867834083735942840576171875f,
                                             0.25f,
                                             0.4426433145999908447265625f,
                                             0.25f,
                                             0.02867834083735942840576171875f };

    static float32_t biquad1_coeffs[] = {
        0.00018072660895995795726776123046875f,
        0.0003614532179199159145355224609375f,
        0.00018072660895995795726776123046875f,

        1.9746592044830322265625f,
        -0.975681483745574951171875f,

        0.00035528256557881832122802734375f,
        0.0007105651311576366424560546875f,
        0.00035528256557881832122802734375f,

        1.94127738475799560546875f,
        -0.9422824382781982421875f,
    };

    static float32_t biquad2_coeffs[] = { 0.000180378541699610650539398193359375f,
                                          0.00036075708339922130107879638671875f,
                                          0.000180378541699610650539398193359375f,

                                          1.97468459606170654296875f,
                                          -0.975704848766326904296875f,

                                          0.00035460057551972568035125732421875f,
                                          0.0007092011510394513607025146484375f,
                                          0.00035460057551972568035125732421875f,

                                          1.941333770751953125f,
                                          -0.942336857318878173828125f };

    static float32_t biquad3_coeffs[] = { 0.0000231437370530329644680023193359375f,
                                          0.000046287474106065928936004638671875f,
                                          0.0000231437370530329644680023193359375f,

                                          1.98140633106231689453125f,
                                          -0.981498897075653076171875f,

                                          0.000020184184904792346060276031494140625f,
                                          0.00004036836980958469212055206298828125f,
                                          0.000020184184904792346060276031494140625f,

                                          1.99491560459136962890625f,
                                          -0.99500882625579833984375f,

                                          0.000026784562578541226685047149658203125f,
                                          0.00005356912515708245337009429931640625f,
                                          0.000026784562578541226685047149658203125f,

                                          1.9863297939300537109375f,
                                          -0.986422598361968994140625f };

    arm_fir_decimate_init_f32(&firdec1_sin_inst,
                              FIR1_DEC_NCOEFFS,
                              FIR_DEC_FACTOR,
                              fir1_3kHz_coeffs,
                              fir1_statebuff_sin,
                              FIR1_DEC_BLOCKSIZE);

    arm_fir_decimate_init_f32(&firdec1_cos_inst,
                              FIR1_DEC_NCOEFFS,
                              FIR_DEC_FACTOR,
                              fir1_3kHz_coeffs,
                              fir1_statebuff_cos,
                              FIR1_DEC_BLOCKSIZE);

    arm_fir_decimate_init_f32(&firdec2_sin_inst,
                              FIR2_DEC_NCOEFFS,
                              FIR_DEC_FACTOR,
                              fir2_300Hz_coeffs,
                              fir2_statebuff_sin,
                              FIR2_DEC_BLOCKSIZE);

    arm_fir_decimate_init_f32(&firdec2_cos_inst,
                              FIR2_DEC_NCOEFFS,
                              FIR_DEC_FACTOR,
                              fir2_300Hz_coeffs,
                              fir2_statebuff_cos,
                              FIR2_DEC_BLOCKSIZE);

    arm_biquad_cascade_df2T_init_f32(&biquad1_sin_inst, BIQUAD1_NSTAGES, biquad1_coeffs, biquad1_statebuff_sin);

    arm_biquad_cascade_df2T_init_f32(&biquad1_cos_inst, BIQUAD1_NSTAGES, biquad1_coeffs, biquad1_statebuff_cos);

    arm_biquad_cascade_df2T_init_f32(&biquad2_sin_inst, BIQUAD2_NSTAGES, biquad2_coeffs, biquad2_statebuff_sin);

    arm_biquad_cascade_df2T_init_f32(&biquad2_cos_inst, BIQUAD2_NSTAGES, biquad2_coeffs, biquad2_statebuff_cos);

    arm_biquad_cascade_df2T_init_f32(&biquad3_sin_inst, BIQUAD3_NSTAGES, biquad3_coeffs, biquad3_statebuff_sin);

    arm_biquad_cascade_df2T_init_f32(&biquad3_cos_inst, BIQUAD3_NSTAGES, biquad3_coeffs, biquad3_statebuff_cos);
}

void
dsp_init(void)
{
    recall_coeffs_from_flash_struct();

    Analog.generator_amplitude = DACC_MAX_AMPLITUDE;
    Analog.pos_ch              = REF_CH4;
    Analog.neg_ch              = REF_CH5;
    Analog.adc_gain            = GAIN_1;
    Analog.AGC_on              = true;
    Analog.selected_sensor     = VOLTAGE_SENSOR;
    Analog.clamp_sensor_gain   = 0;
    Analog.shunt_sensor_gain   = 0;
    Analog.overall_gain        = 1;
    Analog.mes_mode            = MEASUREMENT_MODE_MANUAL;

    Dsp.integrator_len = INTEGRATOR_LENGTH;

    filters_init();
    dsp_calculate_sine_table(Analog.generator_amplitude);
    dacc_setup();
#ifdef ADC_TEST_DEF
    adc_interrupt_init();
#endif
}

void
dsp_integrating_filter(void)
{
    static uint32_t  counter = 0;
    static float32_t sin_vect;
    static float32_t cos_vect;
    float32_t        sin_buff;
    float32_t        cos_buff;

    if (decimator_databuff_is_ready == true) {
        do_filter(&sin_buff, &cos_buff);
        counter++;
        sin_vect += sin_buff;
        cos_vect += cos_buff;

        if (Analog.selected_sensor == CLAMP_SENSOR) {
            float32_t abs_val;
            float32_t gain = Cal_data.clamp_gain[Analog.clamp_sensor_gain] * adc_gain_coeffs[Analog.adc_gain];

            arm_sqrt_f32(sin_buff * sin_buff + cos_buff * cos_buff, &abs_val);

            abs_val /= gain;

            buzzer_set_freq(abs_val);
        }
    }
    else
        return;

    if (counter == (Dsp.integrator_len - 1)) {
        counter = 0;

        sin_vect /= Dsp.integrator_len;
        cos_vect /= Dsp.integrator_len;

        manage_sensed_data(sin_vect, cos_vect);

        sin_vect = 0;
        cos_vect = 0;
    }
}

void
do_filter(float32_t *sin_out, float32_t *cos_out)
{
    decimator_databuff_is_ready = false;

    float32_t *biquad1_sin_buffer_ptr;
    float32_t *biquad1_cos_buffer_ptr;

    if (use_next_buffer) {
        biquad1_sin_buffer_ptr = biquad1_sin_buff_B;
        biquad1_cos_buffer_ptr = biquad1_cos_buff_B;
    }
    else {
        biquad1_sin_buffer_ptr = biquad1_sin_buff_A;
        biquad1_cos_buffer_ptr = biquad1_cos_buff_A;
    }

    arm_fir_decimate_f32(&firdec1_sin_inst, biquad1_sin_buffer_ptr, fir1_sin, FIR1_DEC_BLOCKSIZE);

    arm_fir_decimate_f32(&firdec1_cos_inst, biquad1_cos_buffer_ptr, fir1_cos, FIR1_DEC_BLOCKSIZE);

    arm_biquad_cascade_df2T_f32(&biquad2_sin_inst, fir1_sin, biquad2_sin, BIQUAD2_BUFFSIZE);

    arm_biquad_cascade_df2T_f32(&biquad2_cos_inst, fir1_cos, biquad2_cos, BIQUAD2_BUFFSIZE);

    arm_fir_decimate_f32(&firdec2_sin_inst, biquad2_sin, fir2_sin, FIR2_DEC_BLOCKSIZE);

    arm_fir_decimate_f32(&firdec2_cos_inst, biquad2_cos, fir2_cos, FIR2_DEC_BLOCKSIZE);

    arm_biquad_cascade_df2T_f32(&biquad3_sin_inst, fir2_sin, &biquad3_sin, BIQUAD3_BUFFSIZE);

    arm_biquad_cascade_df2T_f32(&biquad3_cos_inst, fir2_cos, &biquad3_cos, BIQUAD3_BUFFSIZE);

    *sin_out = biquad3_sin;
    *cos_out = biquad3_cos;
}

void
reset_filters(void)
{
    decimator_databuff_is_ready = false;
    fir2_dataready_flag         = false;
    use_next_buffer             = false;
    biquad1_counter             = 0;
    phase_counter               = 0;

    test_counter_adc  = 0;
    test_counter_dacc = 0;
}

static inline float32_t
normalize_angle(float32_t degree)
{
    if (degree < 0)
        degree = 360 + degree;
    else if (degree >= 360)
        degree -= 360;

    return degree;
}

float32_t
find_angle(float32_t sine, float32_t cosine, float32_t absval)
{
    float32_t        abs_sin;
    float32_t        abs_cos;
    float32_t        degree;
    static float32_t rad2deg_conv_coeff = 180 / PI;

    if (sine < 0)
        abs_sin = -sine;
    else
        abs_sin = sine;

    if (cosine < 0)
        abs_cos = -cosine;
    else
        abs_cos = cosine;

    if (abs_sin < abs_cos)
        degree = acosf(abs_cos / absval) * rad2deg_conv_coeff;
    else
        degree = asinf(abs_sin / absval) * rad2deg_conv_coeff;

    if (sine < 0) {
        if (cosine < 0)
            degree += 180;
        else
            degree = 360 - degree;
    }
    else {
        if (cosine < 0)
            degree = 180 - degree;
    }

    return degree;
}

void
calculate_clamp_sensor(float32_t I, float32_t Q, float32_t abs_val, float32_t degree)
{
    float32_t sine;
    float32_t cosine;
    float32_t buff;
    float32_t gain = Cal_data.clamp_gain[Analog.clamp_sensor_gain] * adc_gain_coeffs[Analog.adc_gain];

    if (Clamp_calibrator.is_calibrated)
        Dsp.I_clamp *= Clamp_calibrator.position_gain;

    Dsp.I_clamp = abs_val / gain;

    if (Clamp_calibrator.is_calibrated)
        Dsp.I_clamp *= Clamp_calibrator.position_gain;

    Dsp.I_clamp_phi = degree - Dsp.V_ovrl_phi - Dsp.V_applied_phi - Cal_data.clamp_phi[Analog.clamp_sensor_gain];

    Dsp.Z_clamp     = Dsp.V_applied / Dsp.I_clamp;
    Dsp.Z_clamp_phi = normalize_angle(Dsp.I_clamp_phi);

    arm_sin_cos_f32(Dsp.I_clamp_phi, &sine, &cosine);
    Dsp.I_clamp_I = Dsp.I_clamp * cosine;
    Dsp.I_clamp_Q = Dsp.I_clamp * sine;

    Dsp.R_clamp = Dsp.V_applied / Dsp.I_clamp_I;
    Dsp.X_clamp = Dsp.V_applied / Dsp.I_clamp_Q;
}

static inline void
calculate_V_applied(void)
{
    Dsp.V_applied_I = Dsp.V_ovrl_I - Dsp.V_shunt_I;
    Dsp.V_applied_Q = Dsp.V_ovrl_Q - Dsp.V_shunt_Q;

    arm_sqrt_f32(Dsp.V_applied_I * Dsp.V_applied_I + Dsp.V_applied_Q * Dsp.V_applied_Q, &Dsp.V_applied);

    Dsp.V_applied_phi = find_angle(Dsp.V_applied_Q, Dsp.V_applied_I, Dsp.V_applied);
}

void
calculate_shunt_sensor(float32_t I, float32_t Q, float32_t abs_val, float32_t degree)
{
    float32_t A_i, A_q, B_mag, B_phi, magic_mag, magic_phi, magic_i, magic_q;
    float32_t buff;
    float32_t sine;
    float32_t cosine;

    float32_t gain = Cal_data.shunt_gain[Analog.shunt_sensor_gain] * adc_gain_coeffs[Analog.adc_gain];

    Dsp.V_shunt     = abs_val / gain;
    Dsp.V_shunt_phi = normalize_angle(degree - Cal_data.shunt_phi[Analog.shunt_sensor_gain]);

    arm_sin_cos_f32(Dsp.V_shunt_phi, &sine, &cosine);

    Dsp.V_shunt_I = Dsp.V_shunt * cosine;
    Dsp.V_shunt_Q = Dsp.V_shunt * sine;

    calculate_V_applied();

    A_i = Dsp.V_ovrl_I - Dsp.V_shunt_I;
    A_q = Dsp.V_ovrl_Q - Dsp.V_shunt_Q;

    magic_mag = Dsp.V_shunt / (R_SHUNT * Dsp.V_applied);
    magic_phi = Dsp.V_shunt_phi - Dsp.V_applied_phi;

    arm_sin_cos_f32(magic_phi, &sine, &cosine);

    magic_i = magic_mag * cosine;
    magic_q = magic_mag * sine;

    Dsp.R_ovrl     = 1 / magic_i;
    Dsp.X_ovrl     = 1 / magic_q;
    Dsp.Z_ovrl_phi = magic_phi;
    Dsp.Z_ovrl     = 1 / magic_mag;
}

void
calculate_voltage_sensor(float32_t I, float32_t Q, float32_t abs_val, float32_t degree)
{
    float32_t gain = (Cal_data.v_sens_gain * adc_gain_coeffs[Analog.adc_gain]);
    float32_t sine;
    float32_t cosine;

    Dsp.V_ovrl          = abs_val / gain;
    Dsp.V_ovrl_phi_orig = degree;
    Dsp.V_ovrl_phi      = degree - Cal_data.v_sens_phi;

    arm_sin_cos_f32(Dsp.V_ovrl_phi, &sine, &cosine);

    Dsp.V_ovrl_I = Dsp.V_ovrl * cosine;
    Dsp.V_ovrl_Q = Dsp.V_ovrl * sine;
}

void
manage_sensed_data(float32_t sin_vect, float32_t cos_vect)
{
    float32_t abs_val;
    float32_t buff;
    float32_t degree;

    arm_sqrt_f32(sin_vect * sin_vect + cos_vect * cos_vect, &abs_val);
    degree = find_angle(sin_vect, cos_vect, abs_val);

    if (Calibrator.is_calibrating) {
        Calibrator.sensor_mag        = abs_val;
        Calibrator.sensor_phi        = degree;
        Calibrator.new_data_is_ready = true;
    }
    else {
        Dsp.new_data_is_ready = true;
        Dsp.degree            = degree;

        if (Analog.selected_sensor == VOLTAGE_SENSOR)
            calculate_voltage_sensor(cos_vect, sin_vect, abs_val, degree);
        else if (Analog.selected_sensor == SHUNT_SENSOR)
            calculate_shunt_sensor(cos_vect, sin_vect, abs_val, degree);
        else if (Analog.selected_sensor == CLAMP_SENSOR)
            calculate_clamp_sensor(cos_vect, sin_vect, abs_val, degree);
    }
}

#ifdef __cplusplus
}
#endif