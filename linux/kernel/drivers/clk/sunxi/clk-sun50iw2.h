/*
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#ifndef __MACH_SUNXI_CLK_SUN50IW2_H
#define __MACH_SUNXI_CLK_SUN50IW2_H

#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/io.h>
#define IO_ADDRESS(x) (void __iomem *)(((x) & 0x0fffffff) + \
					(((x) >> 4) & 0x0f000000) + \
					0xf0000000)
#include "clk-factors.h"

/* register list */
#define PLL_CPU             0x0000
#define PLL_AUDIO           0x0008
#define PLL_VIDEO           0x0010
#define PLL_VE              0x0018
#define PLL_DDR             0x0020
#define PLL_PERIPH0         0x0028
#define PLL_GPU             0x0038
#define PLL_PERIPH1         0x0044
#define PLL_DE              0x0048

#define CPU_CFG             0x0050
#define AHB1_CFG            0x0054
#define APB2_CFG            0x0058
#define AHB2_CFG            0x005c
#define BUS_GATE0           0x0060
#define BUS_GATE1           0x0064
#define BUS_GATE2           0x0068
#define BUS_GATE3           0x006c
#define BUS_GATE4           0x0070
#define THS_CFG             0x0074
#define NAND_CFG            0x0080
#define SD0_CFG             0x0088
#define SD1_CFG             0x008c
#define SD2_CFG             0x0090
#define TS_CFG              0x0098
#define CE_CFG              0x009c
#define SPI0_CFG            0x00A0
#define SPI1_CFG            0x00A4
#define I2S0_CFG            0x00B0
#define I2S1_CFG            0x00B4
#define I2S2_CFG            0x00B8
#define SPDIF_CFG           0x00C0
#define USB_CFG             0x00CC
#define DRAM_CFG            0x00F4
#define DDR_CFG             0x00F8 /* FIXME */
#define MBUS_RST            0x00FC
#define DRAM_GATE           0x0100

#define DE_CFG              0x0104
#define TCON0_CFG           0x0118
#define TVE_CFG             0x0120 /* FIXME */
#define DEINTERLACE_CFG     0x0124
#define CSI_MISC            0x0130
#define CSI_CFG             0x0134
#define VE_CFG              0x013C
#define ADDA_CFG            0x0140
#define AVS_CFG             0x0144
#define HDMI_CFG            0x0150
#define HDMI_SLOW           0x0154
#define MBUS_CFG            0x015C
#define GPU_CFG             0x01A0

#define PLL_CPUPAT          0x0280
#define PLL_AUDIOPAT        0x0284
#define PLL_VIDEOPAT        0x0288
#define PLL_VEPAT           0x028c
#define PLL_DRRPAT          0x0290
#define PLL_GPUPAT          0x029c
#define PLL_PERI1PAT        0x02A4
#define PLL_DEPAT           0x02A8

#define BUS_RST0            0x02C0
#define BUS_RST1            0x02C4
#define BUS_RST2            0x02C8
#define BUS_RST3            0x02D0
#define BUS_RST4            0x02D8

#define PS_CFG              0x0300
#define PS_CNT              0x0304
#define PLL_CLK_CTRL        0x0320 /* FIXME:sun50iw2 have no new mode */

#define SUNXI_CLK_MAX_REG   0x0320

#define CPUS_CFG            0x0000
#define CPUS_APB0           0x000c
#define CPUS_APB0_GATE      0x0028
#define CPUS_CIR            0x0054
#define CPUS_APB0_RST       0x00B0
#define CPUS_CLK_MAX_REG    0x00B0
#define ADDA_PR_CFG_REG     0x1C0
#define LOSC_OUT_GATE       0x01F00060

#define F_N8X7_M0X4(nv, mv) (FACTOR_ALL(nv, 8, 7, 0, 0, 0, mv, 0, 4, \
					0, 0, 0, 0, 0, 0, 0, 0, 0))
#define F_N8X5_K4X2(nv, kv) (FACTOR_ALL(nv, 8, 5, kv, 4, 2, 0, 0, 0, \
					0, 0, 0, 0, 0, 0, 0, 0, 0))
#define F_N8X7_M0X2(nv, mv) (FACTOR_ALL(nv, 8, 7, 0, 0, 0, mv, 0, 2, \
					0, 0, 0, 0, 0, 0, 0, 0, 0))
#define F_N8X5_K4X2_M0X2(nv, kv, mv) (FACTOR_ALL(nv, 8, 5, kv, 4, 2, \
					mv, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0))
#define F_N8X5_K4X2_M0X2_P16x2(nv, kv, mv, pv) \
			      (FACTOR_ALL(nv, 8, 5, \
			       kv, 4, 2, \
			       mv, 0, 2, \
			       pv, 16, 2, \
			       0, 0, 0, 0, 0, 0))

#define PLLCPU(n, k, m, p, freq)    {F_N8X5_K4X2_M0X2_P16x2(n, k, m, p), freq}
#define PLLVIDEO(n, m, freq)        {F_N8X7_M0X4(n, m), freq}
#define PLLVE(n, m, freq)           {F_N8X7_M0X4(n, m), freq}
#define PLLDDR(n, k, m, freq)       {F_N8X5_K4X2_M0X2(n, k, m), freq}
#define PLLPERIPH0(n, k, freq)      {F_N8X5_K4X2(n, k), freq}
#define PLLPERIPH1(n, k, freq)      {F_N8X5_K4X2(n, k), freq}
#define PLLGPU(n, m, freq)          {F_N8X7_M0X4(n, m), freq}
#define PLLDE(n, m, freq)           {F_N8X7_M0X4(n, m), freq}
#endif
