
#include "motor.h"
#include "basework/errno.h"
#include "string.h"
#include "gpio_if.h"
#include "device_board_config.h"
#include "basework/os/osapi_timer.h"

static motor_vibration_mode_e m_motor_vibration_mode = MOTOR_VIBRATION_MODE_DEFAULT;
static uint8_t motor_vibration_times = 0;
static bool motor_vibration_ss_falg = false;

static os_timer_t motor_vibration_timer;


extern uint32_t motor_vibration_start(uint8_t times);
extern uint32_t motor_vibration_stop(void);


uint32_t motor_vibration_mode_set(motor_vibration_mode_e mode)
{
	m_motor_vibration_mode = mode;
	
	return ERR_SUCCESS;
}

motor_vibration_mode_e motor_vibration_mode_get(void)
{
	return m_motor_vibration_mode;
}

uint32_t motor_vibration_start(uint8_t times)
{
	gpio_if_state_set(MOTOR_POWER_EN,1);
	gpio_if_state_set(MOTOR_GPIO_EN,1);
	motor_vibration_ss_falg = true;
	motor_vibration_times = times;
	
    os_timer_add(&motor_vibration_timer, m_motor_vibration_mode);

	return ERR_SUCCESS;
}

uint32_t motor_vibration_stop(void)
{
	os_timer_del(&motor_vibration_timer);

	gpio_if_state_set(MOTOR_POWER_EN,0);
	gpio_if_state_set(MOTOR_GPIO_EN,0);
	
	motor_vibration_times = 0;
	return ERR_SUCCESS;
}	

static void motor_vibration_timer_handler(os_timer_t timer, void *arg)
{
	if(	motor_vibration_ss_falg == true)
	{
		gpio_if_state_set(MOTOR_GPIO_EN,0);
		motor_vibration_times--;
		if(motor_vibration_times > 0)
		{
			os_timer_add(&motor_vibration_timer, m_motor_vibration_mode);
		}
		else
		{
			gpio_if_state_set(MOTOR_POWER_EN,0);
		}
	}
	else
	{
		gpio_if_state_set(MOTOR_GPIO_EN,1);
		os_timer_add(&motor_vibration_timer, m_motor_vibration_mode);
	}
	motor_vibration_ss_falg = !motor_vibration_ss_falg; 
	
}
	

uint32_t motor_init(void)
{
	uint32_t ret = ERR_SUCCESS;
	
	gpio_if_config(MOTOR_POWER_EN,PIN_OUTPUT_NOPULL);
	gpio_if_state_set(MOTOR_POWER_EN,0);
	
	gpio_if_config(MOTOR_GPIO_EN,PIN_OUTPUT_NOPULL);
	gpio_if_state_set(MOTOR_GPIO_EN,0);
	
	//重启后,flash加载
//	motor_vibration_mode_set(motor_vibration_mode_e mode); 

    os_timer_create(&motor_vibration_timer, motor_vibration_timer_handler, NULL, true);
	
	return ret;
}	


