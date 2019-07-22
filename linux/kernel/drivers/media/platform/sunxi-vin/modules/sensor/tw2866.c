
/*
 * A V4L2 driver for TW2866 YUV cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>

#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("zw");
MODULE_DESCRIPTION("A low-level driver for TW2866 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR 0x00c8

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 30

/*
 * The TW2866 sits on i2c with ID 0x50
 */
#define I2C_ADDR 0x50
#define SENSOR_NAME "tw2866"


#if 0
static struct regval_list read_reg[] = {
	{0x00, 0x00},
	{0x01, 0x00},
	{0x02, 0x00},
	{0x03, 0x00},
	{0xFA, 0x00},		/*output clock, base of pclk, 1ch*d1 = 27m */
	{0xFB, 0x00},
	{0xFC, 0x00},
	{0x9C, 0x00},		/*A0 */
	{0x9E, 0x00},
	{0xF9, 0x00},		/*Video misc */
	{0xAA, 0x00},		/*Video AGC */
	{0x6A, 0x00},		/*CLKPO2/CLKNO2 off */
	{0x6B, 0x00},		/*CLKPO3/CLKNO3 off */
	{0x6C, 0x00},		/*CLKPO4/CLKNO4 off */
	{0x60, 0x00},		/*0x15/0x05�� */
	{0x61, 0x00},
	{0xca, 0x00},		/*chmd: 0=1ch 1=2ch 2=4ch */
	{0xcd, 0x00},		/*1st */
	{0x42, 0x00},		/*testpattern 75%color bar */
};

#endif


static struct regval_list reg_d1_1ch[] = {
	{0x00, 0x00},
	{0x01, 0x00},
	{0x02, 0x64},
	{0x03, 0x11},
	{0xFA, 0x40},		/*output clock, base of pclk, 1ch*d1 = 27m */
	{0xFB, 0x2F},
	{0xFC, 0xFF},
	{0x9C, 0x20},		/*A0 */
	{0x9E, 0x52},
	{0xF9, 0x11},		/*Video misc */
	{0xAA, 0x00},		/*Video AGC */
	{0x6A, 0x0f},		/*CLKPO2/CLKNO2 off */
	{0x6B, 0x0f},		/*CLKPO3/CLKNO3 off */
	{0x6C, 0x0f},		/*CLKPO4/CLKNO4 off */
	{0x60, 0x15},		/*0x15/0x05*/
	{0x61, 0x03},
	{0xca, 0x00},		/*chmd: 0=1ch 1=2ch 2=4ch */
	{0xcd, 0xe4},		/*1st */
	{0x5b, 0x11},		/*pad drive set */
};


#if 0
static struct regval_list reg_d1_2ch[] = {
	{0x00, 0x00},
	{0x01, 0x00},
	{0x02, 0x64},
	{0x03, 0x11},
	{0xFA, 0x45},		/*0x45 *//*[7]:v-scale output clock, base of pclk, 2ch*d1 = 54m */
	{0xFB, 0x2F},
	{0xFC, 0xFF},
	{0x9C, 0x20},		/*A0 */
	{0x9E, 0x52},
	{0xF9, 0x11},		/*Video misc */
	{0xAA, 0x00},		/*Video AGC */
	{0xca, 0x01},		/*chmd: 0=1ch 1=2ch 2=4ch */
	{0xcd, 0xe4},		/*1st */
	{0xcc, 0x39},		/*2nd */
	{0xcb, 0x00},		/*4ch cif */
	{0x60, 0x15},		/*0x15/0x05*/
	{0x61, 0x03},
	{0x5b, 0x00},		/*pad drive set */
};
#endif

#if 0
static struct regval_list reg_d1_4ch[] = {
	{0x00, 0x00},
	{0x01, 0x00},
	{0x02, 0x64},
	{0x03, 0x11},
	{0xFA, 0x4a},		/*output clock, base of pclk, 4ch*cif = 54m  4ch*d1 = 108m */
	{0xFB, 0x2F},
	{0xFC, 0xFF},
	{0x9C, 0x20},		/*A0 */
	{0x9E, 0x52},
	{0xF9, 0x11},		/*Video misc */
	{0xAA, 0x00},		/*Video AGC */
	{0xca, 0x02},		/*chmd: 0=1ch 1=2ch 2=4ch */
	{0xcd, 0xe4},		/*1st */
	{0xcc, 0x39},		/*2nd */
	{0xcb, 0x00},		/*4ch cif */
	{0x60, 0x15},		/*0x15/0x05*/
	{0x61, 0x03},
	{0x5b, 0xff},		/*pad drive set */
	{REG_DLY, 0x20},
};
#endif

#if 0
static struct regval_list reg_cif_4ch[] = {
	/*CSI_MODE==CSI_MODE_TW2866_4CH_CIF */
	{0x00, 0x00},
	{0x01, 0x00},
	{0x02, 0x64},
	{0x03, 0x11},
	{0xFA, 0x45},		/*output clock, base of pclk, 4ch*cif = 54m  4ch*cif = 54m */
	{0xFB, 0x2F},
	{0xFC, 0xFF},
	{0x9C, 0x20},		/*A0 */
	{0x9E, 0x52},
	{0xF9, 0x11},		/*Video misc */
	{0xAA, 0x00},		/*Video AGC */
	{0xca, 0x00},		/*chmd: 0=1ch or 4ch-cif 1=2ch 2=4ch-d1 */
	{0xcd, 0xe4},		/*1st */
	{0xcc, 0x39},		/*2nd */
	{0xcb, 0x01},		/*4ch cif */
	{0x60, 0x17},		/*0x37/0x15/0x05*/
	{0x61, 0x03},
	{0x5b, 0x11},		/*pad drive set */
	{0x9f, 0x00},		/*p clock delay 7ns */
};
#endif

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	if (on_off)
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
	else
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
	return 0;
}

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	switch (on) {
	case STBY_ON:
		sensor_dbg("CSI_SUBDEV_STBY_ON!\n");
		sensor_s_sw_stby(sd, ON);
		break;
	case STBY_OFF:
		sensor_dbg("CSI_SUBDEV_STBY_OFF!\n");
		sensor_s_sw_stby(sd, OFF);
		break;
	case PWR_ON:
		sensor_dbg("CSI_SUBDEV_PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AFVDD, ON);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("CSI_SUBDEV_PWR_OFF!\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
	usleep_range(5000, 6000);
	vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
	usleep_range(5000, 6000);
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval;

	rdval = 0;
	sensor_read(sd, 0xff, &rdval);
	sensor_print("reg 0xff rdval = 0x%x\n", rdval);

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = VGA_WIDTH;
	info->height = VGA_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

	info->preview_first_flag = 1;
	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);
	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg,
			       info->current_wins,
			       sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case ISP_SET_EXP_GAIN:
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
	.desc = "BT656 4CH",
	.mbus_code = V4L2_MBUS_FMT_UYVY8_2X8,
	.regs = NULL,
	.regs_size = 0,
	.bpp = 2,
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size sensor_win_sizes[] = {
	/* 480p */
	{
	 .width = 704,
	 .height = 576,
	 .hoffset = 0,
	 .voffset = 0,
	 .regs = reg_d1_1ch,
	 .regs_size = ARRAY_SIZE(reg_d1_1ch),
	 .set_size = NULL,
	 },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_BT656;
	cfg->flags = CLK_POL | CSI_CH_0;
	return 0;
}

static int sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	switch (qc->id) {
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 0, 10000 * 10000, 1, 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 0, 10000 * 10000, 1, 16);
	}
	return 0;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	return 0;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	return 0;
}

static int sensor_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SENSOR, 0);
}

static int sensor_reg_init(struct sensor_info *info)
{
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	sensor_dbg("sensor_reg_init\n");

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);
	sensor_print("%s on = %d, %d*%d %x\n", __func__, enable,
		  info->current_wins->width,
		  info->current_wins->height, info->fmt->mbus_code);

	if (!enable)
		return 0;
	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.g_chip_ident = sensor_g_chip_ident,
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.queryctrl = sensor_queryctrl,
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.s_stream = sensor_s_stream,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};


/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_8,
	.data_width = CCI_BITS_8,
};

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	mutex_init(&info->lock);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_INTERLACED;
	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;

	sd = cci_dev_remove_helper(client, &cci_drv);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = SENSOR_NAME,
		   },
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};
static __init int init_sensor(void)
{
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);
