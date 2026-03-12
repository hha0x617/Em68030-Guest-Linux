// SPDX-License-Identifier: GPL-2.0-only
/*
 * em68030fb - Em68030 virtual framebuffer driver (simplefb)
 *
 * Registers a simple-framebuffer platform device by reading
 * framebuffer parameters from the EMFB control registers at $FFFE8000.
 *
 * Copyright (C) 2026 hha0x617
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/platform_data/simplefb.h>
#include <linux/io.h>

#define EMFB_BASE  0xFFFE8000
#define EMFB_MAGIC 0x454D4642  /* "EMFB" */

static struct simplefb_platform_data em68030fb_pdata;
static struct resource em68030fb_res;

static struct platform_device em68030fb_dev = {
	.name          = "simple-framebuffer",
	.id            = 0,
	.num_resources = 1,
	.resource      = &em68030fb_res,
	.dev.platform_data = &em68030fb_pdata,
};

static int __init em68030fb_init(void)
{
	void __iomem *regs;
	u32 magic, vbase, vsize;
	u16 width, height, stride;
	u8 bpp;

	regs = ioremap(EMFB_BASE, 64);
	if (!regs)
		return -ENOMEM;

	magic = ioread32be(regs + 0x00);
	if (magic != EMFB_MAGIC) {
		iounmap(regs);
		pr_err("em68030fb: EMFB device not found (magic=0x%08x)\n",
		       magic);
		return -ENODEV;
	}

	width  = ioread16be(regs + 0x04);
	height = ioread16be(regs + 0x06);
	bpp    = ioread8(regs + 0x08);
	stride = ioread16be(regs + 0x0A);
	vbase  = ioread32be(regs + 0x0C);
	vsize  = ioread32be(regs + 0x10);
	iounmap(regs);

	em68030fb_pdata.width  = width;
	em68030fb_pdata.height = height;
	em68030fb_pdata.stride = stride;
	em68030fb_pdata.format = (bpp == 32) ? "a8r8g8b8" :
	                         (bpp == 8)  ? "r3g3b2"   : "r5g6b5";

	em68030fb_res.start = vbase;
	em68030fb_res.end   = vbase + vsize - 1;
	em68030fb_res.flags = IORESOURCE_MEM;

	pr_info("em68030fb: %ux%u %ubpp stride=%u @ 0x%08x (%u bytes)\n",
		width, height, bpp, stride, vbase, vsize);

	return platform_device_register(&em68030fb_dev);
}

static void __exit em68030fb_exit(void)
{
	platform_device_unregister(&em68030fb_dev);
}

module_init(em68030fb_init);
module_exit(em68030fb_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("hha0x617");
MODULE_DESCRIPTION("Em68030 virtual framebuffer (simplefb)");
