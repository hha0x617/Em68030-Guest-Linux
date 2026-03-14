// SPDX-License-Identifier: GPL-2.0-only
/*
 * em68030input - Em68030 virtual keyboard/mouse input driver
 *
 * Reads input events from the EMKM control registers at $FFFE9000
 * and reports them to the Linux input subsystem.
 *
 * The emulator host pushes keyboard and mouse events into an MMIO FIFO.
 * This driver polls the FIFO at 100 Hz and reports events via evdev.
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
#define REG_MAGIC       0x00  /* R   u32: magic number */
#define REG_EVENT_COUNT 0x04  /* R   u8:  pending event count */
#define REG_EVENT_TYPE  0x05  /* R   u8:  front event type */
#define REG_EVENT_CODE  0x06  /* R   u16: scancode / button code */
#define REG_EVENT_VALUE 0x08  /* R   s16: key value / delta-X */
#define REG_EVENT_VALUE2 0x0A /* R   s16: delta-Y / button state */
#define REG_EVENT_ACK   0x0C  /* W   u8:  dequeue front event */
#define REG_MOUSE_ABS_X 0x14  /* R   u16: absolute mouse X */
#define REG_MOUSE_ABS_Y 0x16  /* R   u16: absolute mouse Y */
#define REG_MOUSE_MODE  0x18  /* RW  u8:  0=relative, 1=absolute */
#define REG_SCREEN_W    0x1C  /* R   u16: screen width */
#define REG_SCREEN_H    0x1E  /* R   u16: screen height */

/* Event types (must match emulator's InputDevice) */
#define EVENT_KEY        1
#define EVENT_MOUSE_MOVE 2
#define EVENT_MOUSE_BTN  3

/* Poll interval: 10ms = 100 Hz */
#define POLL_INTERVAL_MS 10

static volatile void __iomem *regs;
static struct input_dev *kbd_dev;
static struct input_dev *mouse_dev;
static struct timer_list poll_timer;
static u16 screen_w, screen_h;

static void em68030input_poll(struct timer_list *t)
{
	int count, i;
	u8 type;
	u16 code;
	s16 value, value2;

	count = ioread8(regs + REG_EVENT_COUNT);

	for (i = 0; i < count; i++) {
		type   = ioread8(regs + REG_EVENT_TYPE);
		code   = ioread16be(regs + REG_EVENT_CODE);
		value  = (s16)ioread16be(regs + REG_EVENT_VALUE);
		value2 = (s16)ioread16be(regs + REG_EVENT_VALUE2);

		/* Dequeue this event */
		iowrite8(0, regs + REG_EVENT_ACK);

		switch (type) {
		case EVENT_KEY:
			input_report_key(kbd_dev, code, value);
			input_sync(kbd_dev);
			break;

		case EVENT_MOUSE_MOVE:
			if (ioread8(regs + REG_MOUSE_MODE)) {
				/* Absolute mode: value=X, value2=Y */
				input_report_abs(mouse_dev, ABS_X, (u16)value);
				input_report_abs(mouse_dev, ABS_Y, (u16)value2);
			} else {
				/* Relative mode: value=dX, value2=dY */
				input_report_rel(mouse_dev, REL_X, value);
				input_report_rel(mouse_dev, REL_Y, value2);
			}
			input_sync(mouse_dev);
			break;

		case EVENT_MOUSE_BTN:
			input_report_key(mouse_dev, code, value);
			input_sync(mouse_dev);
			break;
		}
	}

	mod_timer(&poll_timer, jiffies + msecs_to_jiffies(POLL_INTERVAL_MS));
}

static int __init em68030input_init(void)
{
	u32 magic;
	int err;
	int i;

	/*
	 * On m68k, I/O addresses above 0xFF000000 are identity-mapped.
	 * Use a direct pointer cast to avoid iounmap "bad pmd" warnings.
	 */
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
	/* Enable all standard keys (KEY_ESC=1 through KEY_MAX) */
	for (i = 1; i < KEY_MAX; i++)
		set_bit(i, kbd_dev->keybit);

	err = input_register_device(kbd_dev);
	if (err) {
		pr_err("em68030input: failed to register keyboard (%d)\n", err);
		input_free_device(kbd_dev);
		return err;
	}

	/* --- Mouse/tablet input device --- */
	mouse_dev = input_allocate_device();
	if (!mouse_dev) {
		input_unregister_device(kbd_dev);
		return -ENOMEM;
	}

	mouse_dev->name = "Em68030 Virtual Mouse";
	mouse_dev->phys = "em68030/input1";
	mouse_dev->id.bustype = BUS_HOST;
	mouse_dev->id.vendor  = 0x454D;  /* "EM" */
	mouse_dev->id.product = 0x4D53;  /* "MS" */
	mouse_dev->id.version = 1;

	set_bit(EV_KEY, mouse_dev->evbit);
	set_bit(EV_ABS, mouse_dev->evbit);
	set_bit(EV_REL, mouse_dev->evbit);

	/* Mouse buttons */
	set_bit(BTN_LEFT,   mouse_dev->keybit);
	set_bit(BTN_RIGHT,  mouse_dev->keybit);
	set_bit(BTN_MIDDLE, mouse_dev->keybit);

	/* Absolute axes (tablet mode, default) */
	input_set_abs_params(mouse_dev, ABS_X, 0, screen_w - 1, 0, 0);
	input_set_abs_params(mouse_dev, ABS_Y, 0, screen_h - 1, 0, 0);

	/* Relative axes (for optional relative mode) */
	set_bit(REL_X, mouse_dev->relbit);
	set_bit(REL_Y, mouse_dev->relbit);

	err = input_register_device(mouse_dev);
	if (err) {
		pr_err("em68030input: failed to register mouse (%d)\n", err);
		input_free_device(mouse_dev);
		input_unregister_device(kbd_dev);
		return err;
	}

	/* Start polling timer */
	timer_setup(&poll_timer, em68030input_poll, 0);
	mod_timer(&poll_timer, jiffies + msecs_to_jiffies(POLL_INTERVAL_MS));

	pr_info("em68030input: keyboard + mouse registered (%ux%u)\n",
		screen_w, screen_h);

	return 0;
}

static void __exit em68030input_exit(void)
{
	del_timer_sync(&poll_timer);
	input_unregister_device(mouse_dev);
	input_unregister_device(kbd_dev);
}

module_init(em68030input_init);
module_exit(em68030input_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("hha0x617");
MODULE_DESCRIPTION("Em68030 virtual keyboard/mouse input driver");
