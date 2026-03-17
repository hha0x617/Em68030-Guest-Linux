// SPDX-License-Identifier: GPL-2.0-only
/*
 * em68030input - Em68030 virtual keyboard/mouse input driver
 *
 * Reads input events from the EMKM control registers at $FFFE9000
 * and reports them to the Linux input subsystem.
 *
 * Keyboard and mouse button events are read from the MMIO event FIFO.
 * Mouse position is polled from the MOUSE_ABS_X/Y registers and reported
 * via two devices:
 *   - Absolute tablet device (for X Window System / libinput)
 *   - Relative mouse device (for gpm / fbcon text selection)
 *
 * Copyright (C) 2026 hha0x617
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/timer.h>

#define EMKM_BASE    0xFFFE9000
#define EMKM_MAGIC   0x454D4B4D  /* "EMKM" */

/* Register offsets */
#define REG_MAGIC       0x00
#define REG_EVENT_COUNT 0x04
#define REG_EVENT_TYPE  0x05
#define REG_EVENT_CODE  0x06
#define REG_EVENT_VALUE 0x08
#define REG_EVENT_VALUE2 0x0A
#define REG_EVENT_ACK   0x0C
#define REG_MOUSE_ABS_X 0x14
#define REG_MOUSE_ABS_Y 0x16
#define REG_SCREEN_W    0x1C
#define REG_SCREEN_H    0x1E

/* Event types */
#define EVENT_KEY        1
#define EVENT_MOUSE_BTN  3

/* Poll interval: 10ms = 100 Hz */
#define POLL_INTERVAL_MS 10

static volatile void __iomem *regs;
static struct input_dev *kbd_dev;
static struct input_dev *tablet_dev;  /* absolute coordinates for X/libinput */
static struct input_dev *mouse_dev;   /* relative coordinates for gpm */
static struct timer_list poll_timer;
static u16 screen_w, screen_h;
static u16 last_abs_x, last_abs_y;

static void em68030input_poll(struct timer_list *t)
{
	int count, i;
	u8 type;
	u16 code, abs_x, abs_y;
	s16 value;

	/* Process keyboard and mouse button events from FIFO */
	count = ioread8(regs + REG_EVENT_COUNT);
	for (i = 0; i < count; i++) {
		type = ioread8(regs + REG_EVENT_TYPE);
		code = ioread16be(regs + REG_EVENT_CODE);
		value = (s16)ioread16be(regs + REG_EVENT_VALUE);

		iowrite8(0, regs + REG_EVENT_ACK);

		switch (type) {
		case EVENT_KEY:
			input_report_key(kbd_dev, code, value);
			input_sync(kbd_dev);
			break;

		case EVENT_MOUSE_BTN:
			/* Report button to both tablet and mouse devices */
			input_report_key(tablet_dev, code, value);
			input_sync(tablet_dev);
			input_report_key(mouse_dev, code, value);
			input_sync(mouse_dev);
			break;

		default:
			break;
		}
	}

	/* Read absolute mouse position from registers */
	abs_x = ioread16be(regs + REG_MOUSE_ABS_X);
	abs_y = ioread16be(regs + REG_MOUSE_ABS_Y);

	if (abs_x != last_abs_x || abs_y != last_abs_y) {
		/* Absolute device: report position directly */
		input_report_abs(tablet_dev, ABS_X, abs_x);
		input_report_abs(tablet_dev, ABS_Y, abs_y);
		input_sync(tablet_dev);

		/* Relative device: compute delta from previous position */
		input_report_rel(mouse_dev, REL_X, (s16)(abs_x - last_abs_x));
		input_report_rel(mouse_dev, REL_Y, (s16)(abs_y - last_abs_y));
		input_sync(mouse_dev);

		last_abs_x = abs_x;
		last_abs_y = abs_y;
	}

	mod_timer(&poll_timer, jiffies + msecs_to_jiffies(POLL_INTERVAL_MS));
}

static int __init em68030input_init(void)
{
	u32 magic;
	int err, i;

	regs = (volatile void __iomem *)EMKM_BASE;

	magic = ioread32be(regs + REG_MAGIC);
	if (magic != EMKM_MAGIC) {
		pr_err("em68030input: EMKM device not found (magic=0x%08x)\n",
		       magic);
		return -ENODEV;
	}

	screen_w = ioread16be(regs + REG_SCREEN_W);
	screen_h = ioread16be(regs + REG_SCREEN_H);

	/* --- Keyboard input device --- */
	kbd_dev = input_allocate_device();
	if (!kbd_dev)
		return -ENOMEM;

	kbd_dev->name = "Em68030 Virtual Keyboard";
	kbd_dev->phys = "em68030/input0";
	kbd_dev->id.bustype = BUS_HOST;
	kbd_dev->id.vendor  = 0x454D;  /* "EM" */
	kbd_dev->id.product = 0x4B42;  /* "KB" */
	kbd_dev->id.version = 1;

	set_bit(EV_KEY, kbd_dev->evbit);
	set_bit(EV_REP, kbd_dev->evbit);
	/* Enable standard keyboard keys only (KEY_ESC=1 through KEY_UNKNOWN=240).
	 * Registering up to KEY_MAX includes BTN_* codes (0x100+), which causes
	 * udev/libinput to misclassify the keyboard as a mouse device. */
	for (i = 1; i <= 240; i++)
		set_bit(i, kbd_dev->keybit);

	err = input_register_device(kbd_dev);
	if (err) {
		pr_err("em68030input: failed to register keyboard (%d)\n", err);
		input_free_device(kbd_dev);
		return err;
	}

	/* --- Tablet input device (absolute coordinates for X/libinput) --- */
	tablet_dev = input_allocate_device();
	if (!tablet_dev) {
		input_unregister_device(kbd_dev);
		return -ENOMEM;
	}

	tablet_dev->name = "Em68030 Virtual Tablet";
	tablet_dev->phys = "em68030/input1";
	tablet_dev->id.bustype = BUS_HOST;
	tablet_dev->id.vendor  = 0x454D;  /* "EM" */
	tablet_dev->id.product = 0x5442;  /* "TB" */
	tablet_dev->id.version = 1;

	set_bit(EV_KEY, tablet_dev->evbit);
	set_bit(EV_ABS, tablet_dev->evbit);

	set_bit(BTN_LEFT,   tablet_dev->keybit);
	set_bit(BTN_RIGHT,  tablet_dev->keybit);
	set_bit(BTN_MIDDLE, tablet_dev->keybit);

	input_set_abs_params(tablet_dev, ABS_X, 0, screen_w - 1, 0, 0);
	input_set_abs_params(tablet_dev, ABS_Y, 0, screen_h - 1, 0, 0);

	err = input_register_device(tablet_dev);
	if (err) {
		pr_err("em68030input: failed to register tablet (%d)\n", err);
		input_free_device(tablet_dev);
		input_unregister_device(kbd_dev);
		return err;
	}

	/* --- Mouse input device (relative coordinates for gpm) --- */
	mouse_dev = input_allocate_device();
	if (!mouse_dev) {
		input_unregister_device(tablet_dev);
		input_unregister_device(kbd_dev);
		return -ENOMEM;
	}

	mouse_dev->name = "Em68030 Virtual Mouse";
	mouse_dev->phys = "em68030/input2";
	mouse_dev->id.bustype = BUS_HOST;
	mouse_dev->id.vendor  = 0x454D;  /* "EM" */
	mouse_dev->id.product = 0x4D53;  /* "MS" */
	mouse_dev->id.version = 1;

	set_bit(EV_KEY, mouse_dev->evbit);
	set_bit(EV_REL, mouse_dev->evbit);

	set_bit(BTN_LEFT,   mouse_dev->keybit);
	set_bit(BTN_RIGHT,  mouse_dev->keybit);
	set_bit(BTN_MIDDLE, mouse_dev->keybit);

	set_bit(REL_X, mouse_dev->relbit);
	set_bit(REL_Y, mouse_dev->relbit);

	err = input_register_device(mouse_dev);
	if (err) {
		pr_err("em68030input: failed to register mouse (%d)\n", err);
		input_free_device(mouse_dev);
		input_unregister_device(tablet_dev);
		input_unregister_device(kbd_dev);
		return err;
	}

	/* Start polling timer */
	timer_setup(&poll_timer, em68030input_poll, 0);
	mod_timer(&poll_timer, jiffies + msecs_to_jiffies(POLL_INTERVAL_MS));

	pr_info("em68030input: keyboard + tablet + mouse registered (%ux%u)\n",
		screen_w, screen_h);

	return 0;
}

static void __exit em68030input_exit(void)
{
	del_timer_sync(&poll_timer);
	input_unregister_device(mouse_dev);
	input_unregister_device(tablet_dev);
	input_unregister_device(kbd_dev);
}

module_init(em68030input_init);
module_exit(em68030input_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("hha0x617");
MODULE_DESCRIPTION("Em68030 virtual keyboard/mouse input driver");
