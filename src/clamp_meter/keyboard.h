#prama once

#define KEYS_DEBOUNCE	1500000UL
#define ENCODER_FAST_MODE_IN_THR	10
#define ENCODER_FAST_MODE_OUT_THR	20
#define ENCODER_HSM_ACTIONS_THR		5
#define ENCODER_SHSM_ACTIONS_THR	5
#define	KEYS_USED_MAP	0X7ff

#define KEY_BACK	(1 << 0)
#define KEY_DOWN	(1 << 2)
#define KEY_UP		(1 << 4)
#define KEY_LEFT	(1 << 1)
#define KEY_RIGHT	(1 << 5)
#define KEY_F1		(1 << 6)
#define KEY_F2		(1 << 7)
#define KEY_MENU	(1 << 8)
#define KEY_F3		(1 << 9)
#define KEY_ENCSW	(1 << 10)
#define KEY_ENCL	(1 << 14)
#define KEY_ENCR	(1 << 15)

#define ENCODER_A_PIN	(1 << 1)
#define ENCODER_B_PIN	(1 << 13)
#define ENCODER_FILTER_DEBOUNCE_HZ	300
#define ENCODER_A_PIN_NUM	1
#define ENCODER_B_PIN_NUM	32*3 + 13

#ifdef __cplusplus
extern "C" {
#endif

extern uint16_t kbrddd;
extern uint16_t encoderA_testval;
extern uint16_t encoderB_testval;
extern uint16_t encoderL_testval;
extern uint16_t encoderR_testval;

typedef struct {
	bool		enc_fastmode;
	bool		enc_superHSM;
	int8_t		enc_hsm_counter;
	int8_t		enc_superHSM_counter;
	uint32_t	enc_last_action;
	uint16_t	keys;
}Keyboard_t;

extern Keyboard_t Keyboard;

void keyboard_handler		(uint16_t keys);
void keyboard_encoder_init	(void);
void keyboard_init			(void);

#ifdef __cplusplus
}
#endif