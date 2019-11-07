/*
 * embest
 *
 *
 */
#include <stdlib.h>
#include <sys/printk.h>
#include <display/mb_display.h>
#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#include "board.h"
#include "generic_level.h"


#define LED_STEPS    (25)
#define LEVEL_FULL   (65535)
#define LED_STEP_PER_VALUE  (LEVEL_FULL/LED_STEPS)
#define LED_LEVEL_2_STEP(level)  (level/LED_STEP_PER_VALUE)
#define LED_SHIFT_POS (32768)
#define LED_STEP_2_LEVEL(step) (step * LED_STEP_PER_VALUE)

// micro:bit display patterns for OFF and ON
static const struct mb_image led_patterns[] = {
		MB_IMAGE({0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0}),
		MB_IMAGE({0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0},
						 {0, 0, 1, 0, 0},
						 {0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0}),
		MB_IMAGE({0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0},
						 {0, 1, 1, 0, 0},
						 {0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0}),
		MB_IMAGE({0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0},
						 {0, 1, 1, 1, 0},
						 {0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0}),
		MB_IMAGE({0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0},
						 {1, 1, 1, 1, 0},
						 {0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0}),
		MB_IMAGE({0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0},
						 {1, 1, 1, 1, 1},
						 {0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0}),
		MB_IMAGE({0, 0, 0, 0, 0},
						 {0, 0, 1, 0, 0},
						 {1, 1, 1, 1, 1},
						 {0, 0, 0, 0, 0},
						 {0, 0, 0, 0, 0}),
		MB_IMAGE({0, 0, 0, 0, 0},
						 {0, 0, 1, 0, 0},
						 {1, 1, 1, 1, 1},
						 {0, 0, 1, 0, 0},
						 {0, 0, 0, 0, 0}),
		MB_IMAGE({0, 0, 1, 0, 0},
						 {0, 0, 1, 0, 0},
						 {1, 1, 1, 1, 1},
						 {0, 0, 1, 0, 0},
						 {0, 0, 0, 0, 0}),
		MB_IMAGE({0, 0, 1, 0, 0},
						 {0, 0, 1, 0, 0},
						 {1, 1, 1, 1, 1},
						 {0, 0, 1, 0, 0},
						 {0, 0, 1, 0, 0}),
		MB_IMAGE({0, 0, 1, 0, 0},
						 {0, 1, 1, 0, 0},
						 {1, 1, 1, 1, 1},
						 {0, 0, 1, 0, 0},
						 {0, 0, 1, 0, 0}),
		MB_IMAGE({0, 0, 1, 0, 0},
						 {0, 1, 1, 0, 0},
						 {1, 1, 1, 1, 1},
						 {0, 0, 1, 1, 0},
						 {0, 0, 1, 0, 0}),
		MB_IMAGE({0, 0, 1, 0, 0},
						 {0, 1, 1, 1, 0},
						 {1, 1, 1, 1, 1},
						 {0, 0, 1, 1, 0},
						 {0, 0, 1, 0, 0}),
		MB_IMAGE({0, 0, 1, 0, 0},
						 {0, 1, 1, 1, 0},
						 {1, 1, 1, 1, 1},
						 {0, 1, 1, 1, 0},
						 {0, 0, 1, 0, 0}),
		MB_IMAGE({0, 1, 1, 0, 0},
						 {0, 1, 1, 1, 0},
						 {1, 1, 1, 1, 1},
						 {0, 1, 1, 1, 0},
						 {0, 0, 1, 0, 0}),
		MB_IMAGE({0, 1, 1, 0, 0},
						 {0, 1, 1, 1, 0},
						 {1, 1, 1, 1, 1},
						 {0, 1, 1, 1, 0},
						 {0, 0, 1, 1, 0}),
		MB_IMAGE({0, 1, 1, 1, 0},
						 {0, 1, 1, 1, 0},
						 {1, 1, 1, 1, 1},
						 {0, 1, 1, 1, 0},
						 {0, 0, 1, 1, 0}),
		MB_IMAGE({0, 1, 1, 1, 0},
						 {0, 1, 1, 1, 0},
						 {1, 1, 1, 1, 1},
						 {0, 1, 1, 1, 0},
						 {0, 1, 1, 1, 0}),
		MB_IMAGE({0, 1, 1, 1, 0},
						 {1, 1, 1, 1, 0},
						 {1, 1, 1, 1, 1},
						 {0, 1, 1, 1, 0},
						 {0, 1, 1, 1, 0}),
		MB_IMAGE({0, 1, 1, 1, 0},
						 {1, 1, 1, 1, 0},
						 {1, 1, 1, 1, 1},
						 {0, 1, 1, 1, 1},
						 {0, 1, 1, 1, 0}),
		MB_IMAGE({0, 1, 1, 1, 0},
						 {1, 1, 1, 1, 0},
						 {1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {0, 1, 1, 1, 0}),
		MB_IMAGE({0, 1, 1, 1, 0},
						 {1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {0, 1, 1, 1, 0}),
		MB_IMAGE({1, 1, 1, 1, 0},
						 {1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {0, 1, 1, 1, 0}),
		MB_IMAGE({1, 1, 1, 1, 0},
						 {1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {0, 1, 1, 1, 1}),
		MB_IMAGE({1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {0, 1, 1, 1, 1}),
		MB_IMAGE({1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1},
						 {1, 1, 1, 1, 1})};


static void mb_led_set(s16_t idx)
{
	struct mb_display *disp = mb_display_get();
	s16_t current_idx = idx;

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT, K_FOREVER, &led_patterns[current_idx], 1);
	//pub for notice
}

// led has 0-25, totol 26 brightness range
typedef struct {
	s8_t start;
	s8_t end;
	s8_t current_step;
	s32_t period;
	s32_t delay;
}led_step_t;

led_step_t mb_led = {
	.start = 0,
	.end = 0,
	.current_step = 0,
	.period = 0,
	.delay = 0,
};

#if 0
void level_transition_work_handler(struct k_work *work)
{

}

K_WORK_DEFINE(level_transition_work, level_transition_work_handler);
void start_set_transform(u8_t delay, u8_t transform_time, s16_t target_state)
{
	// delay
	k_sleep(K_MSEC(5)*delay);

	// start transition worker
	k_work_submit(&level_transition_work);
}
#endif

s8_t transform_going(void)
{
	if (mb_led.current_step == mb_led.end) {
		return 0;
	} else {
		return 1;
	}
}

static void expire_handler(struct k_timer *dummy)
{
	led_step_t * p_led_timer = dummy -> user_data;
	printk("expire_handler start = %d, end = %d, current_step = %d\n", p_led_timer -> start, p_led_timer -> end, p_led_timer -> current_step);
	mb_led_set(p_led_timer -> current_step);

	// update mb leds
	if (p_led_timer -> start < p_led_timer -> end) {
	//positive
		if (p_led_timer -> current_step != p_led_timer -> end) {
			p_led_timer -> current_step ++;
		} else {
			k_timer_stop(dummy);
		}
	} else  if (p_led_timer -> start > p_led_timer -> end){
	// negtive
		if (p_led_timer -> current_step != p_led_timer -> end) {
			p_led_timer -> current_step --;
		} else {
			k_timer_stop(dummy);
		}
	} else {//

	}
}

static void stoped_handler(struct k_timer *dummy)
{
	printk("stoped_handler\n");
	// pub final status
}

K_TIMER_DEFINE(level_timer, expire_handler, stoped_handler);
static void transform_start(s8_t start, s8_t end, s32_t delay, s32_t period)
{
	if (start == end)
		return;

	if (period ==0) {
		mb_led.start = start;
		mb_led.end = end;
		mb_led.current_step = end;
		mb_led.period = period;
		mb_led.delay = delay;
		mb_led_set(end);// set led immediately
		return;
	} else {
		mb_led.start = start;
		mb_led.end = end;
		mb_led.current_step = start;
		mb_led.period = period;
		mb_led.delay = delay;
	}
	k_timer_start(&level_timer, delay , period);
}

// set absolutely level, period can not be 0
s32_t transform_set_level(s16_t end, s32_t delay, s32_t trans_time)
{
	s8_t start = mb_led.current_step;
	s32_t end_t = end + LED_SHIFT_POS;//shift to positive
	s8_t end_lv = LED_LEVEL_2_STEP(end_t);
	s32_t period;

	if (end_lv < 0){
		end_lv = 0;
	} else if (end_lv >25)
	{
		end_lv = 25;
	}

	period = trans_time / (int)abs(end_lv - start);
	transform_start(start, end_lv, delay, period);

	// return remaining time
	return (int)abs(mb_led.current_step - mb_led.end)*period;
}

// set level continously within a time, period can not be 0
s32_t transform_set_delta(s16_t delta, s32_t delay, s32_t trans_time)
{
	s8_t start = mb_led.current_step;
	s8_t end_lv =  start + LED_LEVEL_2_STEP(delta);
	s32_t period;

	if (end_lv < 0){
		end_lv = 0;
	} else if (end_lv >25)
	{
		end_lv = 25;
	}

	period = trans_time / (int)abs(end_lv - start);
	transform_start(start, end_lv, delay, period);

	// return remaining time
	return (int)abs(mb_led.current_step - mb_led.end)*period;
}

// set level continously move within a time
s32_t transform_set_move(s16_t delta, s32_t delay, s32_t trans_time)
{
	s8_t start = mb_led.current_step;
	s8_t end_lv =  start + LED_LEVEL_2_STEP((s32_t)delta * 1000 / trans_time);
	s32_t period;

	if (end_lv < 0){
		end_lv = 0;
	} else if (end_lv >25)
	{
		end_lv = 25;
	}

	period = trans_time / (int)abs(end_lv - start);
	transform_start(start, end_lv, delay, period);

	// return remaining time
	return (int)abs(mb_led.current_step - mb_led.end)*period;
}

s16_t transform_get_current(void)
{
	return LED_STEP_2_LEVEL(mb_led.current_step) - LED_SHIFT_POS;
}

s16_t transform_get_target(void)
{
	return LED_STEP_2_LEVEL(mb_led.end) - LED_SHIFT_POS;
}

s32_t transform_get_remain(void)
{
	return (int)abs(mb_led.current_step - mb_led.end)*mb_led.period;
}

static void init_level_timer(void * user_data)
{
	k_timer_user_data_set(&level_timer, user_data);
}

void transform_init(void)
{
	init_level_timer((void *) &mb_led);
}
