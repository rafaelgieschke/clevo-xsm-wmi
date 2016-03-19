/*
 * clevo-xsm-wmi.c
 *
 * Copyright (C) 2014-2015 Arnoud Willemsen <mail@lynthium.com>
 *
 * Based on tuxedo-wmi by TUXEDO Computers GmbH
 * Copyright (C) 2013-2015 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 * Custom build Linux Notebooks and Computers: www.tuxedocomputers.com
 *
 * This program is free software;  you can redistribute it and/or modify
 * it under the terms of the  GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is  distributed in the hope that it  will be useful, but
 * WITHOUT  ANY   WARRANTY;  without   even  the  implied   warranty  of
 * MERCHANTABILITY  or FITNESS FOR  A PARTICULAR  PURPOSE.  See  the GNU
 * General Public License for more details.
 *
 * You should  have received  a copy of  the GNU General  Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define CLEVO_XSM_DRIVER_NAME KBUILD_MODNAME
#define pr_fmt(fmt) CLEVO_XSM_DRIVER_NAME ": " fmt

#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/stringify.h>
#include <linux/version.h>
#include <linux/workqueue.h>

#define __CLEVO_XSM_PR(lvl, fmt, ...) do { pr_##lvl(fmt, ##__VA_ARGS__); } \
		while (0)
#define CLEVO_XSM_INFO(fmt, ...) __CLEVO_XSM_PR(info, fmt, ##__VA_ARGS__)
#define CLEVO_XSM_ERROR(fmt, ...) __CLEVO_XSM_PR(err, fmt, ##__VA_ARGS__)
#define CLEVO_XSM_DEBUG(fmt, ...) __CLEVO_XSM_PR(debug, "[%s:%u] " fmt, \
		__func__, __LINE__, ##__VA_ARGS__)

#define CLEVO_EVENT_GUID  "ABBC0F6B-8EA1-11D1-00A0-C90629100000"
#define CLEVO_EMAIL_GUID  "ABBC0F6C-8EA1-11D1-00A0-C90629100000"
#define CLEVO_GET_GUID    "ABBC0F6D-8EA1-11D1-00A0-C90629100000"

/* method IDs for CLEVO_GET */
#define GET_EVENT               0x01  /*   1 */
#define GET_POWER_STATE_FOR_3G  0x0A  /*  10 */
#define GET_AP                  0x46  /*  70 */
#define SET_3G                  0x4C  /*  76 */
#define SET_KB_LED              0x67  /* 103 */
#define AIRPLANE_BUTTON         0x6D  /* 109 */    /* or 0x6C (?) */
#define TALK_BIOS_3G            0x78  /* 120 */

#define COLORS { C(black,  0x000000), C(blue,    0x0000FF), \
				 C(red,    0xFF0000), C(magenta, 0xFF00FF), \
				 C(green,  0x00FF00), C(cyan,    0x00FFFF), \
				 C(yellow, 0xFFFF00), C(white,   0xFFFFFF), }
#undef C

#define C(n, v) KB_COLOR_##n
enum kb_color COLORS;
#undef C

union kb_rgb_color {
	u32 rgb;
	struct { u32 b:8, g:8, r:8, : 8; };
};

#define C(n, v) { .name = #n, .value = { .rgb = v, }, }
struct {
	const char *const name;
	union kb_rgb_color value;
} kb_colors[] = COLORS;
#undef C

#define KB_COLOR_DEFAULT      KB_COLOR_blue
#define KB_BRIGHTNESS_MAX     10
#define KB_BRIGHTNESS_DEFAULT KB_BRIGHTNESS_MAX

static int param_set_kb_color(const char *val, const struct kernel_param *kp)
{
	size_t i;

	if (!val)
		return -EINVAL;

	if (!val[0]) {
		*((enum kb_color *) kp->arg) = KB_COLOR_black;
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(kb_colors); i++) {
		if (!strcmp(val, kb_colors[i].name)) {
			*((enum kb_color *) kp->arg) = i;
			return 0;
		}
	}

	return -EINVAL;
}

static int param_get_kb_color(char *buffer, const struct kernel_param *kp)
{
	return sprintf(buffer, "%s",
		kb_colors[*((enum kb_color *) kp->arg)].name);
}

static const struct kernel_param_ops param_ops_kb_color = {
	.set = param_set_kb_color,
	.get = param_get_kb_color,
};

static enum kb_color param_kb_color[] = { [0 ... 3] = KB_COLOR_DEFAULT };
static int param_kb_color_num;
#define param_check_kb_color(name, p) __param_check(name, p, enum kb_color)
module_param_array_named(kb_color, param_kb_color, kb_color,
						 &param_kb_color_num, S_IRUSR);
MODULE_PARM_DESC(kb_color, "Set the color(s) of the keyboard (sections)");


static int param_set_kb_brightness(const char *val,
	const struct kernel_param *kp)
{
	int ret;

	ret = param_set_byte(val, kp);

	if (!ret && *((unsigned char *) kp->arg) > KB_BRIGHTNESS_MAX)
		return -EINVAL;

	return ret;
}

static const struct kernel_param_ops param_ops_kb_brightness = {
	.set = param_set_kb_brightness,
	.get = param_get_byte,
};

static unsigned char param_kb_brightness = KB_BRIGHTNESS_DEFAULT;
#define param_check_kb_brightness param_check_byte
module_param_named(kb_brightness, param_kb_brightness, kb_brightness, S_IRUSR);
MODULE_PARM_DESC(kb_brightness, "Set the brightness of the keyboard backlight");


static bool param_kb_off;
module_param_named(kb_off, param_kb_off, bool, S_IRUSR);
MODULE_PARM_DESC(kb_off, "Switch keyboard backlight off");

static bool param_kb_cycle_colors;
module_param_named(kb_cycle_colors, param_kb_cycle_colors, bool, S_IRUSR);
MODULE_PARM_DESC(kb_cycle_colors, "Cycle colors rather than modes");


#define POLL_FREQ_MIN     1
#define POLL_FREQ_MAX     20
#define POLL_FREQ_DEFAULT 5

static int param_set_poll_freq(const char *val, const struct kernel_param *kp)
{
	int ret;

	ret = param_set_byte(val, kp);

	if (!ret)
		*((unsigned char *) kp->arg) = clamp_t(unsigned char,
			*((unsigned char *) kp->arg),
			POLL_FREQ_MIN, POLL_FREQ_MAX);

	return ret;
}


static const struct kernel_param_ops param_ops_poll_freq = {
	.set = param_set_poll_freq,
	.get = param_get_byte,
};

static unsigned char param_poll_freq = POLL_FREQ_DEFAULT;
#define param_check_poll_freq param_check_byte
module_param_named(poll_freq, param_poll_freq, poll_freq, S_IRUSR);
MODULE_PARM_DESC(poll_freq, "Set polling frequency");


struct platform_device *clevo_xsm_platform_device;


/* LED sub-driver */

static bool param_led_invert;
module_param_named(led_invert, param_led_invert, bool, 0);
MODULE_PARM_DESC(led_invert, "Invert airplane mode LED state.");

static struct workqueue_struct *led_workqueue;

static struct _led_work {
	struct work_struct work;
	int wk;
} led_work;

static void airplane_led_update(struct work_struct *work)
{
	u8 byte;
	struct _led_work *w;

	w = container_of(work, struct _led_work, work);

	ec_read(0xD9, &byte);

	if (param_led_invert)
		ec_write(0xD9, w->wk ? byte & ~0x40 : byte | 0x40);
	else
		ec_write(0xD9, w->wk ? byte | 0x40 : byte & ~0x40);

	/* wmbb 0x6C 1 (?) */
}

static enum led_brightness airplane_led_get(struct led_classdev *led_cdev)
{
	u8 byte;

	ec_read(0xD9, &byte);

	if (param_led_invert)
		return byte & 0x40 ? LED_OFF : LED_FULL;
	else
		return byte & 0x40 ? LED_FULL : LED_OFF;
}

/* must not sleep */
static void airplane_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	led_work.wk = value;
	queue_work(led_workqueue, &led_work.work);
}

static struct led_classdev airplane_led = {
	.name = "clevo_xsm::airplane",
	.brightness_get = airplane_led_get,
	.brightness_set = airplane_led_set,
	.max_brightness = 1,
};

static int __init clevo_xsm_led_init(void)
{
	int err;

	led_workqueue = create_singlethread_workqueue("led_workqueue");
	if (unlikely(!led_workqueue))
		return -ENOMEM;

	INIT_WORK(&led_work.work, airplane_led_update);

	err = led_classdev_register(&clevo_xsm_platform_device->dev,
		&airplane_led);
	if (unlikely(err))
		goto err_destroy_workqueue;

	return 0;

err_destroy_workqueue:
	destroy_workqueue(led_workqueue);
	led_workqueue = NULL;

	return err;
}

static void __exit clevo_xsm_led_exit(void)
{
	if (!IS_ERR_OR_NULL(airplane_led.dev))
		led_classdev_unregister(&airplane_led);
	if (led_workqueue)
		destroy_workqueue(led_workqueue);
}

/* input sub-driver */

static struct input_dev *clevo_xsm_input_device;
static DEFINE_MUTEX(clevo_xsm_input_report_mutex);

static unsigned int global_report_cnt;

/* call with clevo_xsm_input_report_mutex held */
static void clevo_xsm_input_report_key(unsigned int code)
{
	input_report_key(clevo_xsm_input_device, code, 1);
	input_report_key(clevo_xsm_input_device, code, 0);
	input_sync(clevo_xsm_input_device);

	global_report_cnt++;
}

static struct task_struct *clevo_xsm_input_polling_task;

static int clevo_xsm_input_polling_thread(void *data)
{
	unsigned int report_cnt = 0;

	CLEVO_XSM_INFO("Polling thread started (PID: %i), polling at %i Hz\n",
				current->pid, param_poll_freq);

	while (!kthread_should_stop()) {

		u8 byte;

		ec_read(0xDB, &byte);
		if (byte & 0x40) {
			ec_write(0xDB, byte & ~0x40);

			CLEVO_XSM_DEBUG("Airplane-Mode Hotkey pressed\n");

			mutex_lock(&clevo_xsm_input_report_mutex);

			if (global_report_cnt > report_cnt) {
				mutex_unlock(&clevo_xsm_input_report_mutex);
				break;
			}

			clevo_xsm_input_report_key(KEY_RFKILL);
			report_cnt++;

			CLEVO_XSM_DEBUG("Led status: %d",
				airplane_led_get(&airplane_led));

			airplane_led_set(&airplane_led,
				(airplane_led_get(&airplane_led) ? 0 : 1));

			mutex_unlock(&clevo_xsm_input_report_mutex);
		}
		msleep_interruptible(1000 / param_poll_freq);
	}

	CLEVO_XSM_INFO("Polling thread exiting\n");

	return 0;
}

static int clevo_xsm_input_open(struct input_dev *dev)
{
	clevo_xsm_input_polling_task = kthread_run(
		clevo_xsm_input_polling_thread,
		NULL, "clevo_xsm-polld");

	if (unlikely(IS_ERR(clevo_xsm_input_polling_task))) {
		clevo_xsm_input_polling_task = NULL;
		CLEVO_XSM_ERROR("Could not create polling thread\n");
				return PTR_ERR(clevo_xsm_input_polling_task);
	}

		return 0;
}

static void clevo_xsm_input_close(struct input_dev *dev)
{
	if (unlikely(IS_ERR_OR_NULL(clevo_xsm_input_polling_task)))
		return;

	kthread_stop(clevo_xsm_input_polling_task);
	clevo_xsm_input_polling_task = NULL;
}

static int __init clevo_xsm_input_init(void)
{
	int err;
	u8 byte;

	clevo_xsm_input_device = input_allocate_device();
	if (unlikely(!clevo_xsm_input_device)) {
		CLEVO_XSM_ERROR("Error allocating input device\n");
		return -ENOMEM;
	}

	clevo_xsm_input_device->name = "Clevo Airplane-Mode Hotkey";
	clevo_xsm_input_device->phys = CLEVO_XSM_DRIVER_NAME "/input0";
	clevo_xsm_input_device->id.bustype = BUS_HOST;
	clevo_xsm_input_device->dev.parent = &clevo_xsm_platform_device->dev;

	clevo_xsm_input_device->open  = clevo_xsm_input_open;
	clevo_xsm_input_device->close = clevo_xsm_input_close;

	set_bit(EV_KEY, clevo_xsm_input_device->evbit);
	set_bit(KEY_RFKILL, clevo_xsm_input_device->keybit);

	ec_read(0xDB, &byte);
	ec_write(0xDB, byte & ~0x40);

	err = input_register_device(clevo_xsm_input_device);
	if (unlikely(err)) {
		CLEVO_XSM_ERROR("Error registering input device\n");
		goto err_free_input_device;
	}

	return 0;

err_free_input_device:
		input_free_device(clevo_xsm_input_device);

		return err;
}

static void __exit clevo_xsm_input_exit(void)
{
	if (unlikely(!clevo_xsm_input_device))
		return;

	input_unregister_device(clevo_xsm_input_device);
		clevo_xsm_input_device = NULL;
}


static int clevo_xsm_wmi_evaluate_wmbb_method(u32 method_id, u32 arg,
	u32 *retval)
{
	struct acpi_buffer in  = { (acpi_size) sizeof(arg), &arg };
		struct acpi_buffer out = { ACPI_ALLOCATE_BUFFER, NULL };
		union acpi_object *obj;
		acpi_status status;
	u32 tmp;

	CLEVO_XSM_DEBUG("%0#4x  IN : %0#6x\n", method_id, arg);

	status = wmi_evaluate_method(CLEVO_GET_GUID, 0x01,
		method_id, &in, &out);

	if (unlikely(ACPI_FAILURE(status)))
		goto exit;

	obj = (union acpi_object *) out.pointer;
	if (obj && obj->type == ACPI_TYPE_INTEGER)
			tmp = (u32) obj->integer.value;
	else
			tmp = 0;

	CLEVO_XSM_DEBUG("%0#4x  OUT: %0#6x (IN: %0#6x)\n", method_id, tmp, arg);

	if (likely(retval))
			*retval = tmp;

	kfree(obj);

exit:
	if (unlikely(ACPI_FAILURE(status)))
		return -EIO;

	return 0;
}


static struct {
	enum kb_lower {
		KB_HAS_LOWER_TRUE,
		KB_HAS_LOWER_FALSE,
	} lower;

	enum kb_state {
		KB_STATE_OFF,
		KB_STATE_ON,
	} state;

	struct {
		unsigned left;
		unsigned center;
		unsigned right;
		unsigned lower;
	} color;

	unsigned brightness;

	enum kb_mode {
		KB_MODE_RANDOM_COLOR,
		KB_MODE_CUSTOM,
		KB_MODE_BREATHE,
		KB_MODE_CYCLE,
		KB_MODE_WAVE,
		KB_MODE_DANCE,
		KB_MODE_TEMPO,
		KB_MODE_FLASH,
	} mode;

	struct kb_backlight_ops {
		void (*set_state)(enum kb_state state);
		void (*set_color)(unsigned left, unsigned center,
			unsigned right, unsigned lower);
		void (*set_brightness)(unsigned brightness);
		void (*set_mode)(enum kb_mode);
		void (*init)(void);
	} *ops;

} kb_backlight = { .ops = NULL, };


static void kb_dec_brightness(void)
{
	if (kb_backlight.state == KB_STATE_OFF ||
		kb_backlight.mode != KB_MODE_CUSTOM)
		return;
	if (kb_backlight.brightness == 0)
		return;

	CLEVO_XSM_DEBUG();

	kb_backlight.ops->set_brightness(kb_backlight.brightness - 1);
}

static void kb_inc_brightness(void)
{
	if (kb_backlight.state == KB_STATE_OFF ||
		kb_backlight.mode != KB_MODE_CUSTOM)
		return;

	CLEVO_XSM_DEBUG();

	kb_backlight.ops->set_brightness(kb_backlight.brightness + 1);
}

static void kb_toggle_state(void)
{
	switch (kb_backlight.state) {
	case KB_STATE_OFF:
		kb_backlight.ops->set_state(KB_STATE_ON);
		break;
	case KB_STATE_ON:
		kb_backlight.ops->set_state(KB_STATE_OFF);
		break;
	default:
		BUG();
	}
}

static void kb_next_mode(void)
{
	static enum kb_mode modes[] = {
		KB_MODE_RANDOM_COLOR,
		KB_MODE_DANCE,
		KB_MODE_TEMPO,
		KB_MODE_FLASH,
		KB_MODE_WAVE,
		KB_MODE_BREATHE,
		KB_MODE_CYCLE,
		KB_MODE_CUSTOM,
	};

	size_t i;

	if (kb_backlight.state == KB_STATE_OFF)
		return;

	for (i = 0; i < ARRAY_SIZE(modes); i++) {
		if (modes[i] == kb_backlight.mode)
			break;
	}

	BUG_ON(i == ARRAY_SIZE(modes));

	kb_backlight.ops->set_mode(modes[(i + 1) % ARRAY_SIZE(modes)]);
}

static void kb_next_color(void)
{
	size_t i;
	unsigned int nc;

	if (kb_backlight.state == KB_STATE_OFF)
		return;

	for (i = 0; i < ARRAY_SIZE(kb_colors); i++) {
		if (i == kb_backlight.color.left)
			break;
	}

	if (i + 1 > ARRAY_SIZE(kb_colors))
		nc = 0;
	else
		nc = i + 1;

	kb_backlight.ops->set_color(nc, nc, nc, nc);
}

/* full color backlight keyboard */

static void kb_full_color__set_color(unsigned left, unsigned center,
	unsigned right, unsigned lower)
{
	u32 cmd;

	cmd = 0xF0000000;
	cmd |= kb_colors[left].value.b << 16;
	cmd |= kb_colors[left].value.r <<  8;
	cmd |= kb_colors[left].value.g <<  0;

	if (!clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED, cmd, NULL))
		kb_backlight.color.left = left;

	cmd = 0xF1000000;
	cmd |= kb_colors[center].value.b << 16;
	cmd |= kb_colors[center].value.r <<  8;
	cmd |= kb_colors[center].value.g <<  0;

	if (!clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED, cmd, NULL))
		kb_backlight.color.center = center;

	cmd = 0xF2000000;
	cmd |= kb_colors[right].value.b << 16;
	cmd |= kb_colors[right].value.r <<  8;
	cmd |= kb_colors[right].value.g <<  0;

	if (!clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED, cmd, NULL))
		kb_backlight.color.right = right;

	if (kb_backlight.lower == KB_HAS_LOWER_TRUE) {
		cmd = 0xF3000000;
		cmd |= kb_colors[lower].value.b << 16;
		cmd |= kb_colors[lower].value.r << 8;
		cmd |= kb_colors[lower].value.g << 0;

		if(!clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED, cmd, NULL))
			kb_backlight.color.lower = lower;
	}

	kb_backlight.mode = KB_MODE_CUSTOM;
}

static void kb_full_color__set_brightness(unsigned i)
{
	u8 lvl_to_raw[] = { 63, 126, 189, 252 };

	i = clamp_t(unsigned, i, 0, ARRAY_SIZE(lvl_to_raw) - 1);

	if (!clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED,
		0xF4000000 | lvl_to_raw[i], NULL))
		kb_backlight.brightness = i;
}

static void kb_full_color__set_mode(unsigned mode)
{
	static u32 cmds[] = {
		[KB_MODE_BREATHE]      = 0x1002a000,
		[KB_MODE_CUSTOM]       = 0,
		[KB_MODE_CYCLE]        = 0x33010000,
		[KB_MODE_DANCE]        = 0x80000000,
		[KB_MODE_FLASH]        = 0xA0000000,
		[KB_MODE_RANDOM_COLOR] = 0x70000000,
		[KB_MODE_TEMPO]        = 0x90000000,
		[KB_MODE_WAVE]         = 0xB0000000,
	};

	BUG_ON(mode >= ARRAY_SIZE(cmds));

	clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED, 0x10000000, NULL);

	if (mode == KB_MODE_CUSTOM) {
		kb_full_color__set_color(kb_backlight.color.left,
			kb_backlight.color.center,
			kb_backlight.color.right,
			kb_backlight.color.lower);
		kb_full_color__set_brightness(kb_backlight.brightness);
		return;
	}

	if (!clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED, cmds[mode], NULL))
		kb_backlight.mode = mode;
}

static void kb_full_color__set_state(enum kb_state state)
{
	u32 cmd = 0xE0000000;

	CLEVO_XSM_DEBUG("State: %d\n", state);

	switch (state) {
	case KB_STATE_OFF:
		cmd |= 0x003001;
		break;
	case KB_STATE_ON:
		cmd |= 0x07F001;
		break;
	default:
		BUG();
	}

	if (!clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED, cmd, NULL))
		kb_backlight.state = state;
}

static void kb_full_color__init(void)
{
	CLEVO_XSM_DEBUG();

	kb_backlight.lower = KB_HAS_LOWER_FALSE;

	kb_full_color__set_state(param_kb_off ? KB_STATE_OFF : KB_STATE_ON);
	kb_full_color__set_color(param_kb_color[0], param_kb_color[1],
		param_kb_color[2], param_kb_color[3]);
	kb_full_color__set_brightness(param_kb_brightness);
}

static struct kb_backlight_ops kb_full_color_ops = {
	.set_state      = kb_full_color__set_state,
	.set_color      = kb_full_color__set_color,
	.set_brightness = kb_full_color__set_brightness,
	.set_mode       = kb_full_color__set_mode,
	.init           = kb_full_color__init,
};

static void kb_full_color__init_lower(void)
{
	CLEVO_XSM_DEBUG();

	kb_backlight.lower = KB_HAS_LOWER_TRUE;

	kb_full_color__set_state(param_kb_off ? KB_STATE_OFF : KB_STATE_ON);
	kb_full_color__set_color(param_kb_color[0], param_kb_color[1],
		param_kb_color[2], param_kb_color[3]);
	kb_full_color__set_brightness(param_kb_brightness);
}

static struct kb_backlight_ops kb_full_color_with_lower_ops = {
	.set_state      = kb_full_color__set_state,
	.set_color      = kb_full_color__set_color,
	.set_brightness = kb_full_color__set_brightness,
	.set_mode       = kb_full_color__set_mode,
	.init           = kb_full_color__init_lower,
};

/* 8 color backlight keyboard */

static void kb_8_color__set_color(unsigned left, unsigned center,
	unsigned right, unsigned lower)
{
	u32 cmd = 0x02010000;

	cmd |= kb_backlight.brightness << 12;
	cmd |= right  << 8;
	cmd |= center << 4;
	cmd |= left;

	if (!clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED, cmd, NULL)) {
		kb_backlight.color.left   = left;
		kb_backlight.color.center = center;
		kb_backlight.color.right  = right;
	}

	kb_backlight.mode = KB_MODE_CUSTOM;
}

static void kb_8_color__set_brightness(unsigned i)
{
	u32 cmd = 0xD2010000;

	i = clamp_t(unsigned, i, 0, KB_BRIGHTNESS_MAX);

	cmd |= i << 12;
	cmd |= kb_backlight.color.right  << 8;
	cmd |= kb_backlight.color.center << 4;
	cmd |= kb_backlight.color.left;

	if (!clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED, cmd, NULL))
		kb_backlight.brightness = i;
}

static void kb_8_color__set_mode(unsigned mode)
{
	static u32 cmds[] = {
		[KB_MODE_BREATHE]      = 0x12010000,
		[KB_MODE_CUSTOM]       = 0,
		[KB_MODE_CYCLE]        = 0x32010000,
		[KB_MODE_DANCE]        = 0x80000000,
		[KB_MODE_FLASH]        = 0xA0000000,
		[KB_MODE_RANDOM_COLOR] = 0x70000000,
		[KB_MODE_TEMPO]        = 0x90000000,
		[KB_MODE_WAVE]         = 0xB0000000,
	};

	BUG_ON(mode >= ARRAY_SIZE(cmds));

	clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED, 0x20000000, NULL);

	if (mode == KB_MODE_CUSTOM) {
		kb_8_color__set_color(kb_backlight.color.left,
			kb_backlight.color.center,
			kb_backlight.color.right, kb_backlight.color.lower);
		kb_8_color__set_brightness(kb_backlight.brightness);
		return;
	}

	if (!clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED,
		cmds[mode], NULL))
		kb_backlight.mode = mode;
}

static void kb_8_color__set_state(enum kb_state state)
{
	CLEVO_XSM_DEBUG("State: %d\n", state);

	switch (state) {
	case KB_STATE_OFF:
		if (!clevo_xsm_wmi_evaluate_wmbb_method(SET_KB_LED,
			0x22010000, NULL))
			kb_backlight.state = state;
		break;
	case KB_STATE_ON:
		kb_8_color__set_mode(kb_backlight.mode);
		kb_backlight.state = state;
		break;
	default:
		BUG();
	}
}

static void kb_8_color__init(void)
{
	CLEVO_XSM_DEBUG();

	kb_8_color__set_state(KB_STATE_OFF);

	kb_backlight.color.left   = param_kb_color[0];
	kb_backlight.color.center = param_kb_color[1];
	kb_backlight.color.right  = param_kb_color[2];

	kb_backlight.brightness = param_kb_brightness;
	kb_backlight.mode       = KB_MODE_CUSTOM;
	kb_backlight.lower      = KB_HAS_LOWER_FALSE;

	if (!param_kb_off) {
		kb_8_color__set_color(kb_backlight.color.left,
			kb_backlight.color.center,
			kb_backlight.color.right, kb_backlight.color.lower);
		kb_8_color__set_brightness(kb_backlight.brightness);
		kb_8_color__set_state(KB_STATE_ON);
	}
}

static struct kb_backlight_ops kb_8_color_ops = {
	.set_state      = kb_8_color__set_state,
	.set_color      = kb_8_color__set_color,
	.set_brightness = kb_8_color__set_brightness,
	.set_mode       = kb_8_color__set_mode,
	.init           = kb_8_color__init,
};


static void clevo_xsm_wmi_notify(u32 value, void *context)
{
	static unsigned int report_cnt;

	u32 event;

	if (value != 0xD0) {
		CLEVO_XSM_INFO("Unexpected WMI event (%0#6x)\n", value);
		return;
	}

	clevo_xsm_wmi_evaluate_wmbb_method(GET_EVENT, 0, &event);

	switch (event) {
	case 0xF4:
		CLEVO_XSM_DEBUG("Airplane-Mode Hotkey pressed\n");

		if (clevo_xsm_input_polling_task) {
			CLEVO_XSM_INFO("Stopping polling thread\n");
			kthread_stop(clevo_xsm_input_polling_task);
			clevo_xsm_input_polling_task = NULL;
		}

		mutex_lock(&clevo_xsm_input_report_mutex);

		if (global_report_cnt > report_cnt) {
			mutex_unlock(&clevo_xsm_input_report_mutex);
			break;
		}

		clevo_xsm_input_report_key(KEY_RFKILL);
		report_cnt++;

		mutex_unlock(&clevo_xsm_input_report_mutex);
		break;
	default:
		if (!kb_backlight.ops)
			break;

		switch (event) {
		case 0x81:
			kb_dec_brightness();
			break;
		case 0x82:
			kb_inc_brightness();
			break;
		case 0x83:
			if (!param_kb_cycle_colors)
				kb_next_mode();
			else
				kb_next_color();
			break;
		case 0x9F:
			kb_toggle_state();
			break;
		}
		break;
	}
}

static int clevo_xsm_wmi_probe(struct platform_device *dev)
{
	int status;

	status = wmi_install_notify_handler(CLEVO_EVENT_GUID,
		clevo_xsm_wmi_notify, NULL);
	if (unlikely(ACPI_FAILURE(status))) {
		CLEVO_XSM_ERROR("Could not register WMI notify handler (%0#6x)\n",
			status);
		return -EIO;
	}

	clevo_xsm_wmi_evaluate_wmbb_method(GET_AP, 0, NULL);

	if (kb_backlight.ops)
		kb_backlight.ops->init();

	return 0;
}

static int clevo_xsm_wmi_remove(struct platform_device *dev)
{
	wmi_remove_notify_handler(CLEVO_EVENT_GUID);
	return 0;
}

static int clevo_xsm_wmi_resume(struct platform_device *dev)
{
	clevo_xsm_wmi_evaluate_wmbb_method(GET_AP, 0, NULL);

	if (kb_backlight.ops && kb_backlight.state == KB_STATE_ON)
		kb_backlight.ops->set_mode(kb_backlight.mode);

	return 0;
}

static struct platform_driver clevo_xsm_platform_driver = {
	.remove = clevo_xsm_wmi_remove,
	.resume = clevo_xsm_wmi_resume,
	.driver = {
		.name  = CLEVO_XSM_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};


/* RFKILL sub-driver */

static bool param_rfkill;
module_param_named(rfkill, param_rfkill, bool, 0);
MODULE_PARM_DESC(rfkill, "Enable WWAN-RFKILL capability.");

static struct rfkill *clevo_xsm_wwan_rfkill_device;

static int clevo_xsm_wwan_rfkill_set_block(void *data, bool blocked)
{
	CLEVO_XSM_DEBUG("blocked=%i\n", blocked);

	if (clevo_xsm_wmi_evaluate_wmbb_method(SET_3G, !blocked, NULL))
		CLEVO_XSM_ERROR("Setting 3G power state failed!\n");
	return 0;
}

static const struct rfkill_ops clevo_xsm_wwan_rfkill_ops = {
	.set_block = clevo_xsm_wwan_rfkill_set_block,
};

static int __init clevo_xsm_rfkill_init(void)
{
	int err;
	u32 unblocked = 0;

	if (!param_rfkill)
		return 0;

	clevo_xsm_wmi_evaluate_wmbb_method(TALK_BIOS_3G, 1, NULL);

	clevo_xsm_wwan_rfkill_device = rfkill_alloc("clevo_xsm-wwan",
		&clevo_xsm_platform_device->dev,
		RFKILL_TYPE_WWAN,
		&clevo_xsm_wwan_rfkill_ops, NULL);
	if (unlikely(!clevo_xsm_wwan_rfkill_device))
		return -ENOMEM;

	err = rfkill_register(clevo_xsm_wwan_rfkill_device);
	if (unlikely(err))
		goto err_destroy_wwan;

	if (clevo_xsm_wmi_evaluate_wmbb_method(GET_POWER_STATE_FOR_3G, 0,
		&unblocked))
		CLEVO_XSM_ERROR("Could not get 3G power state!\n");
	else
		rfkill_set_sw_state(clevo_xsm_wwan_rfkill_device, !unblocked);

	return 0;

err_destroy_wwan:
	rfkill_destroy(clevo_xsm_wwan_rfkill_device);
	clevo_xsm_wmi_evaluate_wmbb_method(TALK_BIOS_3G, 0, NULL);
	return err;
}

static void __exit clevo_xsm_rfkill_exit(void)
{
	if (!clevo_xsm_wwan_rfkill_device)
		return;

	clevo_xsm_wmi_evaluate_wmbb_method(TALK_BIOS_3G, 0, NULL);

	rfkill_unregister(clevo_xsm_wwan_rfkill_device);
	rfkill_destroy(clevo_xsm_wwan_rfkill_device);
}


/* Sysfs interface */

static ssize_t clevo_xsm_brightness_show(struct device *child,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", kb_backlight.brightness);
}

static ssize_t clevo_xsm_brightness_store(struct device *child,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int val;
	int ret;

	if (!kb_backlight.ops)
		return -EINVAL;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	kb_backlight.ops->set_brightness(val);

	return ret ? : size;
}

static DEVICE_ATTR(kb_brightness, 0644,
	clevo_xsm_brightness_show, clevo_xsm_brightness_store);

static ssize_t clevo_xsm_state_show(struct device *child,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", kb_backlight.state);
}

static ssize_t clevo_xsm_state_store(struct device *child,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int val;
	int ret;

	if (!kb_backlight.ops)
		return -EINVAL;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	val = clamp_t(unsigned, val, 0, 1);
	kb_backlight.ops->set_state(val);

	return ret ? : size;
}

static DEVICE_ATTR(kb_state, 0644,
	clevo_xsm_state_show, clevo_xsm_state_store);

static ssize_t clevo_xsm_mode_show(struct device *child,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", kb_backlight.mode);
}

static ssize_t clevo_xsm_mode_store(struct device *child,
	struct device_attribute *attr, const char *buf, size_t size)
{
	static enum kb_mode modes[] = {
		KB_MODE_RANDOM_COLOR,
		KB_MODE_CUSTOM,
		KB_MODE_BREATHE,
		KB_MODE_CYCLE,
		KB_MODE_WAVE,
		KB_MODE_DANCE,
		KB_MODE_TEMPO,
		KB_MODE_FLASH,
	};

	unsigned int val;
	int ret;

	if (!kb_backlight.ops)
		return -EINVAL;

	ret = kstrtouint(buf, 0, &val);
	if (ret)
		return ret;

	val = clamp_t(unsigned, val, 0, 7);
	kb_backlight.ops->set_mode(modes[val]);

	return ret ? : size;
}

static DEVICE_ATTR(kb_mode, 0644,
	clevo_xsm_mode_show, clevo_xsm_mode_store);

static ssize_t clevo_xsm_color_show(struct device *child,
	struct device_attribute *attr, char *buf)
{
	if (kb_backlight.lower == KB_HAS_LOWER_TRUE)
		return sprintf(buf, "%s %s %s %s\n",
			kb_colors[kb_backlight.color.left].name,
			kb_colors[kb_backlight.color.center].name,
			kb_colors[kb_backlight.color.right].name,
			kb_colors[kb_backlight.color.lower].name);
	else
		return sprintf(buf, "%s %s %s\n",
			kb_colors[kb_backlight.color.left].name,
			kb_colors[kb_backlight.color.center].name,
			kb_colors[kb_backlight.color.right].name);
}

static ssize_t clevo_xsm_color_store(struct device *child,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int i, j;
	unsigned int val[4] = {0};
	char left[8];
	char right[8];
	char center[8];
	char lower[8];

	if (!kb_backlight.ops)
		return -EINVAL;

	i = sscanf(buf, "%7s %7s %7s %7s", left, center, right, lower);

	if (i == 1) {
		for (j = 0; j < ARRAY_SIZE(kb_colors); j++) {
			if (!strcmp(left, kb_colors[j].name))
				val[0] = j;
		}
		val[0] = clamp_t(unsigned, val[0], 0, ARRAY_SIZE(kb_colors));
		val[3] = val[2] = val[1] = val[0];

	} else if (i == 3 || i == 4) {
		for (j = 0; j < ARRAY_SIZE(kb_colors); j++) {
			if (!strcmp(left, kb_colors[j].name))
				val[0] = j;
			if (!strcmp(center, kb_colors[j].name))
				val[1] = j;
			if (!strcmp(right, kb_colors[j].name))
				val[2] = j;
			if (!strcmp(lower, kb_colors[j].name))
				val[3] = j;
		}
		val[0] = clamp_t(unsigned, val[0], 0, ARRAY_SIZE(kb_colors));
		val[1] = clamp_t(unsigned, val[1], 0, ARRAY_SIZE(kb_colors));
		val[2] = clamp_t(unsigned, val[2], 0, ARRAY_SIZE(kb_colors));
		val[3] = clamp_t(unsigned, val[3], 0, ARRAY_SIZE(kb_colors));

	} else
		return -EINVAL;

	kb_backlight.ops->set_color(val[0], val[1], val[2], val[3]);

	return size;
}
static DEVICE_ATTR(kb_color, 0644,
	clevo_xsm_color_show, clevo_xsm_color_store);

/* dmi & init & exit */

static int __init clevo_xsm_dmi_matched(const struct dmi_system_id *id)
{
	CLEVO_XSM_INFO("Model %s found\n", id->ident);
	kb_backlight.ops = id->driver_data;

	return 1;
}

static struct dmi_system_id clevo_xsm_dmi_table[] __initdata = {
	{
		.ident = "Clevo P870DM",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "P870DM"),
		},
		.callback = clevo_xsm_dmi_matched,
		.driver_data = &kb_full_color_with_lower_ops,
	},
	{
		.ident = "Clevo P7xxDM(-G)",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "P7xxDM(-G)"),
		},
		.callback = clevo_xsm_dmi_matched,
		.driver_data = &kb_full_color_with_lower_ops,
	},
	{
		.ident = "Clevo P750ZM",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "P750ZM"),
		},
		.callback = clevo_xsm_dmi_matched,
		.driver_data = &kb_full_color_with_lower_ops,
	},
	{
		.ident = "Clevo P750ZM",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "P5 Pro SE"),
		},
		.callback = clevo_xsm_dmi_matched,
		.driver_data = &kb_full_color_with_lower_ops,
	},
	{
		.ident = "Clevo P370SM-A",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "P370SM-A"),
		},
		.callback = clevo_xsm_dmi_matched,
		.driver_data = &kb_full_color_ops,
	},
	{
		.ident = "Clevo P17SM-A",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "P17SM-A"),
		},
		.callback = clevo_xsm_dmi_matched,
		.driver_data = &kb_full_color_ops,
	},
	{
		.ident = "Clevo P15SM1-A",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "P15SM1-A"),
		},
		.callback = clevo_xsm_dmi_matched,
		.driver_data = &kb_full_color_ops,
	},
	{
		.ident = "Clevo P15SM-A",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "P15SM-A"),
		},
		.callback = clevo_xsm_dmi_matched,
		.driver_data = &kb_full_color_ops,
	},
	{
		.ident = "Clevo P17SM",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "P17SM"),
		},
		.callback = clevo_xsm_dmi_matched,
		.driver_data = &kb_8_color_ops,
	},
	{
		.ident = "Clevo P15SM",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "P15SM"),
		},
		.callback = clevo_xsm_dmi_matched,
		.driver_data = &kb_8_color_ops,
	},
	{
		.ident = "Clevo P150EM",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "P150EM"),
		},
		.callback = clevo_xsm_dmi_matched,
		.driver_data = &kb_8_color_ops,
	},
	{
		/* terminating NULL entry */
	},
};

MODULE_DEVICE_TABLE(dmi, clevo_xsm_dmi_table);

static int __init clevo_xsm_init(void)
{
	int err;

	switch (param_kb_color_num) {
	case 1:
		param_kb_color[1] = param_kb_color[2] = param_kb_color[0] = param_kb_color[3];
		break;
	case 2:
		return -EINVAL;
	}

	dmi_check_system(clevo_xsm_dmi_table);

	if (!wmi_has_guid(CLEVO_EVENT_GUID)) {
		CLEVO_XSM_INFO("No known WMI event notification GUID found\n");
		return -ENODEV;
	}

	if (!wmi_has_guid(CLEVO_GET_GUID)) {
		CLEVO_XSM_INFO("No known WMI control method GUID found\n");
		return -ENODEV;
	}

	clevo_xsm_platform_device =
		platform_create_bundle(&clevo_xsm_platform_driver,
			clevo_xsm_wmi_probe, NULL, 0, NULL, 0);

	if (unlikely(IS_ERR(clevo_xsm_platform_device)))
		return PTR_ERR(clevo_xsm_platform_device);

	err = clevo_xsm_rfkill_init();
	if (unlikely(err))
		CLEVO_XSM_ERROR("Could not register rfkill device\n");

	err = clevo_xsm_input_init();
	if (unlikely(err))
		CLEVO_XSM_ERROR("Could not register input device\n");

	err = clevo_xsm_led_init();
	if (unlikely(err))
		CLEVO_XSM_ERROR("Could not register LED device\n");

	if (device_create_file(&clevo_xsm_platform_device->dev,
		&dev_attr_kb_brightness) != 0)
		CLEVO_XSM_ERROR("Sysfs attribute creation failed for brightness\n");

	if (device_create_file(&clevo_xsm_platform_device->dev,
		&dev_attr_kb_state) != 0)
		CLEVO_XSM_ERROR("Sysfs attribute creation failed for state\n");

	if (device_create_file(&clevo_xsm_platform_device->dev,
		&dev_attr_kb_mode) != 0)
		CLEVO_XSM_ERROR("Sysfs attribute creation failed for mode\n");

	if (device_create_file(&clevo_xsm_platform_device->dev,
		&dev_attr_kb_color) != 0)
		CLEVO_XSM_ERROR("Sysfs attribute creation failed for color\n");

	return 0;
}

static void __exit clevo_xsm_exit(void)
{
	clevo_xsm_led_exit();
	clevo_xsm_input_exit();
	clevo_xsm_rfkill_exit();

	device_remove_file(&clevo_xsm_platform_device->dev,
		&dev_attr_kb_brightness);
	device_remove_file(&clevo_xsm_platform_device->dev, &dev_attr_kb_state);
	device_remove_file(&clevo_xsm_platform_device->dev, &dev_attr_kb_mode);
	device_remove_file(&clevo_xsm_platform_device->dev, &dev_attr_kb_color);

	platform_device_unregister(clevo_xsm_platform_device);
	platform_driver_unregister(&clevo_xsm_platform_driver);
}

module_init(clevo_xsm_init);
module_exit(clevo_xsm_exit);

MODULE_AUTHOR("Arnoud Willemsen <mail@lynthium.com>");
MODULE_DESCRIPTION("Clevo SM series laptop driver.");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.9");
