/* microbit.c - BBC micro:bit specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>

#include <display/mb_display.h>

#include <bluetooth/mesh.h>

#include "board.h"
#include "level_srv.h"
#include "generic_level.h"

static u32_t oob_number;
static int is_prov_completed = 0;

#define BUTTON_DEBOUNCE_DELAY_MS 250
static u32_t last_time = 0, time = 0;

// GPIO for the buttons
static struct gpio_callback gpio_btn_a_cb;
static struct gpio_callback gpio_btn_b_cb;
// for use with k_work_submit which we use to handle button presses in a background thread to avoid holding onto an IRQ for too long
static struct k_work button_a_work;
static struct k_work button_b_work;


void button_a_work_handler(struct k_work *work)
{
	//printk("button_a_work_handler\n");

	//transform_set_level(32767, 0, 100);
	//transform_set_delta(6552, 0, 300);
	transform_set_move(6552, 0, 1000);
}

void button_b_work_handler(struct k_work *work)
{
	//printk("button_b_work_handler\n");

	//transform_set_level(-32768, 0, 100);
	//transform_set_delta(-6552, 0, 300);
	transform_set_move(-6552, 0, 1000);
}

void button_a_pressed(struct device *gpiob, struct gpio_callback *cb,
											u32_t pins)
{
	struct mb_display *disp = mb_display_get();
	time = k_uptime_get_32();

	if(time < last_time + BUTTON_DEBOUNCE_DELAY_MS) {
		last_time = time;
		return;
	}

	last_time = time;

	if(!is_prov_completed) {
		mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, K_MSEC(500),
						"%04u", oob_number);
		return;
	}

	k_work_submit(&button_a_work);
}

void button_b_pressed(struct device *gpiob, struct gpio_callback *cb,
											u32_t pins)
{
	time = k_uptime_get_32();

	if(time < last_time + BUTTON_DEBOUNCE_DELAY_MS) {
		last_time = time;
		return;
	}
	last_time = time;

	if(!is_prov_completed)
		return;

	k_work_submit(&button_b_work);
}


static void configure_button(void)
{
	struct device *gpiob;

	printk("Press button A or button B\n");
	gpiob = device_get_binding(DT_ALIAS_SW0_GPIOS_CONTROLLER);

	gpio_pin_configure(gpiob, DT_ALIAS_SW0_GPIOS_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_INT_ACTIVE_LOW));

	gpio_pin_configure(gpiob, DT_ALIAS_SW1_GPIOS_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_INT_ACTIVE_LOW));

	// Button A
	k_work_init(&button_a_work, button_a_work_handler);
	gpio_init_callback(&gpio_btn_a_cb, button_a_pressed, BIT(DT_ALIAS_SW0_GPIOS_PIN));
	gpio_add_callback(gpiob, &gpio_btn_a_cb);
	gpio_pin_enable_callback(gpiob, DT_ALIAS_SW0_GPIOS_PIN);

	// Button B
	k_work_init(&button_b_work, button_b_work_handler);
	gpio_init_callback(&gpio_btn_b_cb, button_b_pressed, BIT(DT_ALIAS_SW1_GPIOS_PIN));
	gpio_add_callback(gpiob, &gpio_btn_b_cb);
	gpio_pin_enable_callback(gpiob, DT_ALIAS_SW1_GPIOS_PIN);
}

void board_output_number(bt_mesh_output_action_t action, u32_t number)
{
	struct mb_display *disp = mb_display_get();

	oob_number = number;

	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, K_SECONDS(1),
				"%04u", oob_number);
}

void board_prov_complete(void)
{
	struct mb_display *disp = mb_display_get();
	struct mb_image arrow = MB_IMAGE({ 0, 1, 0, 1, 0 },
					 { 0, 1, 0, 1, 0 },
					 { 0, 0, 0, 0, 0 },
					 { 1, 0, 0, 0, 1 },
					 { 0, 1, 1, 1, 0 });

	is_prov_completed = 1;

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT, K_SECONDS(10),
			 &arrow, 1);
}

void board_init(void)
{
	struct mb_display *disp = mb_display_get();
	static struct mb_image blink[] = {
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 })
	};

	mb_display_image(disp, MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
			 K_SECONDS(1), blink, ARRAY_SIZE(blink));

	configure_button();
	transform_init();
}


