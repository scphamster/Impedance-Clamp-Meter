/*
 * peripherals_ctrl.h
 *
 * Created: 12.11.2021 8:48:29
 *  Author: malygosstationar
 */ 


#ifndef EXTERNAL_PERIPH_CTRL_H
#define EXTERNAL_PERIPH_CTRL_H

#define HV_EN_PIN				31
#define HV_EN_PIN_PORT			PIOD
#define HV_EN_PIN_PORT_ID		ID_PIOD
#define VOUT_CTRL_PIN			21
#define VOUT_CTRL_PIN_PORT		PIOA
#define VOUT_CTRL_PIN_PORT_ID	ID_PIOA

#define SH_SENSOR_GAIN_A_PIN	(1 << 5)
#define SH_SENSOR_GAIN_B_PIN	(1 << 17)
#define SH_SENSOR_GAIN_C_PIN	(1 << 16)
#define SH_SENSOR_GAIN_D_PIN	(1 << 14)

#define CLAMP_SENSOR_GAIN_A_PIN	(1 << 30)
#define CLAMP_SENSOR_GAIN_B_PIN	(1 << 7)
#define CLAMP_SENSOR_GAIN_C_PIN	(1 << 8)
#define CLAMP_SENSOR_GAIN_D_PIN	(1 << 22)

#ifdef __cplusplus
extern "C" {
#endif

//typedef enum {
//	EXTERNAL_PERIPH_ERROR_1,
//	EXTERNAL_PERIPH_ERROR_2,
//	EXTERNAL_PERIPH_ERROR_3
//}periph_error_code_type_t;
//
//typedef struct {
//	bool output_is_enabled :1;
//	bool hi_voltage_is_enabled :1;
//
//	bool error_occured :1;
//	periph_error_code_type_t error_code;
//}External_periph_status_t;
//
//extern External_periph_status_t External_periph_status;

//void external_periph_ctrl_init(void);
//void hi_voltage_enable (void);
//void hi_voltage_disable (void);
//void output_enable (void);
//void output_disable (void);

#ifdef __cplusplus
}
#endif

#endif /* PERIPHERALS_CTRL_H_ */