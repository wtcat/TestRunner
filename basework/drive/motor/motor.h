
#ifndef _MOTOR_H_
#define _MOTOR_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

typedef enum
{
	MOTOR_VIBRATION_MODE_DEFAULT = 100,
	MOTOR_VIBRATION_MODE_WEAK 	 = 200,
	MOTOR_VIBRATION_MODE_STRONG  = 300,
}motor_vibration_mode_e;


extern uint32_t motor_init(void);                        

extern uint32_t motor_vibration_mode_set(motor_vibration_mode_e mode);  

extern motor_vibration_mode_e motor_vibration_mode_get(void);  	

extern uint32_t motor_vibration_start(uint8_t times);
extern uint32_t motor_vibration_stop(void);

#endif

