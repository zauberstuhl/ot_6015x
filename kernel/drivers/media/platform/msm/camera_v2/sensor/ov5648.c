/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#define pr_fmt(fmt) "%s:%d "fmt, __func__, __LINE__

#include "msm_sensor.h"

#define OV5648_SENSOR_NAME "ov5648"
DEFINE_MSM_MUTEX(ov5648_mut);

#define OV5648_DEBUG
#undef CDBG
#ifdef OV5648_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif //OV5648_DEBUG

#define OTP_FEATURE
#ifdef OTP_FEATURE
#define RG_RATIO_TYPICAL 353
#define BG_RATIO_TYPICAL 340

struct ov5648_otp_struct {
	int rg_ratio;
	int bg_ratio;
};
#endif //OTP_FEATURE

static struct msm_sensor_ctrl_t ov5648_s_ctrl;

static struct msm_sensor_power_setting ov5648_power_setting[] = {
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VANA,
		.config_val = 0,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VAF,
		.config_val = 0,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_LOW,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_AF_PWDM,
		.config_val = GPIO_OUT_HIGH,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 24000000,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};

static struct v4l2_subdev_info ov5648_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};

static const struct i2c_device_id ov5648_i2c_id[] = {
	{OV5648_SENSOR_NAME,
		(kernel_ulong_t)&ov5648_s_ctrl},
	{ }
};

static int32_t msm_ov5648_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &ov5648_s_ctrl);
}

static struct i2c_driver ov5648_i2c_driver = {
	.id_table = ov5648_i2c_id,
	.probe  = msm_ov5648_i2c_probe,
	.driver = {
		.name = OV5648_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov5648_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

#ifdef OTP_FEATURE
static void ov5648_write_i2c(struct msm_sensor_ctrl_t *s_ctrl, uint32_t addr, uint16_t data)
{
	long rc = 0;

	rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(s_ctrl->sensor_i2c_client, \
		addr, data, MSM_CAMERA_I2C_BYTE_DATA);
	if(0 > rc)
		CDBG("%s, %d, i2c write(0x%x) fail:%ld\n", __func__, __LINE__, addr, rc);
}

static int ov5648_read_i2c(struct msm_sensor_ctrl_t *s_ctrl, uint32_t addr)
{
	uint16_t data = 0;
	long rc = 0;
	
	rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(s_ctrl->sensor_i2c_client, \
		addr, &data, MSM_CAMERA_I2C_BYTE_DATA);
	if(0 > rc)
		CDBG("%s, %d, i2c read(0x%x) fail:%ld\n", __func__, __LINE__, addr, rc);

	return data;
}

static int ov5648_otp_readWB(struct msm_sensor_ctrl_t *s_ctrl, int nGroup, 
	struct ov5648_otp_struct *current_otp)
{
	int rc = 0, i = 0, flag = 0, rg = 0, bg = 0, lsb = 0;

	if(NULL == current_otp) {
		CDBG("%s, %d, OTP struct point is NULL\n", __func__, __LINE__);
		return 0;
	}

	if(0 == nGroup) {
		ov5648_write_i2c(s_ctrl, 0x3D84, 0xC0); //OTP read enable
		ov5648_write_i2c(s_ctrl, 0x3D85, 0); //bank0
		ov5648_write_i2c(s_ctrl, 0x3D86, 0x0F); //end address
		ov5648_write_i2c(s_ctrl, 0x3D81, 0x01); //load OTP
		msleep(5);
		ov5648_write_i2c(s_ctrl, 0x3D81, 0); //disable load OTP
		flag = ov5648_read_i2c(s_ctrl, 0x3D05);
		rg = ov5648_read_i2c(s_ctrl, 0x3D07);
		bg = ov5648_read_i2c(s_ctrl, 0x3D08);
		if(!(flag & 0x80) && (rg || bg)){ //group 0
			lsb = ov5648_read_i2c(s_ctrl, 0x3D0B);
			current_otp->rg_ratio = (rg << 2) | ((lsb >> 6) & 0x03);
			current_otp->bg_ratio = (bg << 2) | ((lsb >> 4) & 0x03);
			rc = 1;
		} else {
			rc = 0;
		}
		for(i = 0; i < 16; i ++) { //clear OTP buffer
			ov5648_write_i2c(s_ctrl, 0x3d00 + i, 0);
		}
	} else if(1 == nGroup) {
		ov5648_write_i2c(s_ctrl, 0x3d84, 0xc0); //OTP read enable
		ov5648_write_i2c(s_ctrl, 0x3d85, 0); //bank0
		ov5648_write_i2c(s_ctrl, 0x3d86, 0x0f); //end address
		ov5648_write_i2c(s_ctrl, 0x3d81, 0x01); //load OTP
		msleep(5);
		ov5648_write_i2c(s_ctrl, 0x3d81, 0); //disable load OTP
		flag = ov5648_read_i2c(s_ctrl, 0x3d0e);
		ov5648_write_i2c(s_ctrl, 0x3d84, 0xc0); //OTP read enable
		ov5648_write_i2c(s_ctrl, 0x3d85, 0x10); //bank1
		ov5648_write_i2c(s_ctrl, 0x3d86, 0x1f);
		ov5648_write_i2c(s_ctrl, 0x3d81, 0x01); //load OTP
		msleep(5);
		ov5648_write_i2c(s_ctrl, 0x3d81, 0); //disable load OTP
		rg = ov5648_read_i2c(s_ctrl, 0x3d00);
		bg = ov5648_read_i2c(s_ctrl, 0x3d01);
		if(!(flag & 0x80) && (rg || bg)){ //group 1
			lsb = ov5648_read_i2c(s_ctrl, 0x3d04);
			current_otp->rg_ratio = (rg << 2) | ((lsb >> 6) & 0x03);
			current_otp->bg_ratio = (bg << 2) | ((lsb >> 4) & 0x03);
			rc = 1;
		} else {
			rc = 0;
		}
		for(i = 0; i < 16; i ++) { //clear OTP buffer
			ov5648_write_i2c(s_ctrl, 0x3d00 + i, 0);
		}
	} else if(2 == nGroup) {
		ov5648_write_i2c(s_ctrl, 0x3d84, 0xc0); //OTP read enable
		ov5648_write_i2c(s_ctrl, 0x3d85, 0x10); //bank1
		ov5648_write_i2c(s_ctrl, 0x3d86, 0x1f);
		ov5648_write_i2c(s_ctrl, 0x3d81, 0x01); //load OTP
		msleep(5);
		ov5648_write_i2c(s_ctrl, 0x3d81, 0); //disable load OTP
		flag = ov5648_read_i2c(s_ctrl, 0x3d07);
		rg = ov5648_read_i2c(s_ctrl, 0x3d09);
		bg = ov5648_read_i2c(s_ctrl, 0x3d0a);
		if(!(flag & 0x80) && (rg || bg)){ //group 2
			lsb = ov5648_read_i2c(s_ctrl, 0x3d0d);
			current_otp->rg_ratio = (rg << 2) | ((lsb >> 6) & 0x03);
			current_otp->bg_ratio = (bg << 2) | ((lsb >> 4) & 0x03);
			rc = 1;
		} else {
			rc = 0;
		}
		for(i = 0; i < 16; i ++) { //clear OTP buffer
			ov5648_write_i2c(s_ctrl, 0x3d00 + i, 0);
		}
	} else {
		rc = 0;
	}
	
	if(rc) 
		CDBG("%s, %d, OTP WB group(%d), rg_ratio:%d, bg_ratio:%d\n", \
			__func__, __LINE__, nGroup, \
			current_otp->rg_ratio, current_otp->bg_ratio);
	else 
		CDBG("%s, %d, OTP WB group(%d) invalid or empty\n", __func__, __LINE__, nGroup);

	return rc;
}

static int32_t ov5648_otp_config(struct msm_sensor_ctrl_t *s_ctrl,
	void __user *argp)
{
	struct ov5648_otp_struct current_otp = {0};
	int i = 0, R_gain = 0, B_gain = 0, G_gain = 0, G_gain_B = 0, G_gain_R = 0;

	if (NULL == s_ctrl) {
		CDBG("%s, %d, msm_sensor_ctrl_t struct point is NULL\n", __func__, __LINE__);
		return 0;
	}

    mutex_lock(s_ctrl->msm_sensor_mutex);
	//WB
	for(i = 0; i < 3; i++) {
		if (ov5648_otp_readWB(s_ctrl, i, &current_otp))
			break;
	}
	if(3 > i) {
	    if (!current_otp.bg_ratio || !current_otp.rg_ratio) {
	        pr_err("otp read zero! bg:%d, rg:%d\n", current_otp.bg_ratio, current_otp.rg_ratio);
	        goto ex;
	    }
		//calculate G gain
		//0x400 = 1x gain
		if(current_otp.bg_ratio < BG_RATIO_TYPICAL) {
			if(current_otp.rg_ratio < RG_RATIO_TYPICAL) {
				G_gain = 0x400;
				B_gain = 0x400 * BG_RATIO_TYPICAL / current_otp.bg_ratio;
				R_gain = 0x400 * RG_RATIO_TYPICAL / current_otp.rg_ratio; 
			} else {
				R_gain = 0x400;
				G_gain = 0x400 * current_otp.rg_ratio / RG_RATIO_TYPICAL;
				B_gain = G_gain * BG_RATIO_TYPICAL / current_otp.bg_ratio;
			}
		} else {
			if(current_otp.rg_ratio < RG_RATIO_TYPICAL) {
				B_gain = 0x400;
				G_gain = 0x400 * current_otp.bg_ratio / BG_RATIO_TYPICAL;
				R_gain = G_gain * RG_RATIO_TYPICAL / current_otp.rg_ratio;
			} else {
				G_gain_B = 0x400 * current_otp.bg_ratio / BG_RATIO_TYPICAL;
				G_gain_R = 0x400 * current_otp.rg_ratio / RG_RATIO_TYPICAL;
				if(G_gain_B > G_gain_R) {
					B_gain = 0x400;
					G_gain = G_gain_B;
					R_gain = G_gain * RG_RATIO_TYPICAL / current_otp.rg_ratio;
				} else {
					R_gain = 0x400;
					G_gain = G_gain_R;
					B_gain = G_gain * BG_RATIO_TYPICAL / current_otp.bg_ratio;
				}
			}    
		}
		CDBG("%s, %d, OTP WB after calculate, G_gain:%d, B_gain:%d, R_gain:%d\n", __func__, __LINE__, \
			G_gain, B_gain, R_gain);
		//update WB
		if(R_gain > 0x400) {
			ov5648_write_i2c(s_ctrl, 0x5186, R_gain >> 8);
			ov5648_write_i2c(s_ctrl, 0x5187, R_gain & 0x00ff);
		}
		if(G_gain > 0x400) {
			ov5648_write_i2c(s_ctrl, 0x5188, G_gain >> 8);
			ov5648_write_i2c(s_ctrl, 0x5189, G_gain & 0x00ff);
		}
		if(B_gain > 0x400) {
			ov5648_write_i2c(s_ctrl, 0x518a, B_gain >> 8);
			ov5648_write_i2c(s_ctrl, 0x518b, B_gain & 0x00ff);
		}
	}

ex:
    mutex_unlock(s_ctrl->msm_sensor_mutex);
    CDBG("exit\n");
    
	return 0;
}
#endif //OTP_FEATURE

static struct msm_sensor_fn_t ov5648_sensor_func_tbl = {
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_match_id = msm_sensor_match_id,
#ifdef OTP_FEATURE
	.otp_config = ov5648_otp_config,
#endif //OTP_FEATURE
};

static struct msm_sensor_ctrl_t ov5648_s_ctrl = {
	.sensor_i2c_client = &ov5648_sensor_i2c_client,
	.power_setting_array.power_setting = ov5648_power_setting,
	.power_setting_array.size =
			ARRAY_SIZE(ov5648_power_setting),
	.msm_sensor_mutex = &ov5648_mut,
	.sensor_v4l2_subdev_info = ov5648_subdev_info,
	.sensor_v4l2_subdev_info_size =
			ARRAY_SIZE(ov5648_subdev_info),
	.func_tbl = &ov5648_sensor_func_tbl,
};

static const struct of_device_id ov5648_dt_match[] = {
	{
		.compatible = "t2m,ov5648",
		.data = &ov5648_s_ctrl
	},
	{}
};

MODULE_DEVICE_TABLE(of, ov5648_dt_match);

static struct platform_driver ov5648_platform_driver = {
	.driver = {
		.name = "t2m,ov5648",
		.owner = THIS_MODULE,
		.of_match_table = ov5648_dt_match,
	},
};

static int32_t ov5648_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;

	match = of_match_device(ov5648_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init ov5648_init_module(void)
{
	int32_t rc = 0;

	rc = platform_driver_probe(&ov5648_platform_driver,
		ov5648_platform_probe);
	if (!rc)
		return rc;
	return i2c_add_driver(&ov5648_i2c_driver);
}

static void __exit ov5648_exit_module(void)
{
	if (ov5648_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&ov5648_s_ctrl);
		platform_driver_unregister(&ov5648_platform_driver);
	} else
		i2c_del_driver(&ov5648_i2c_driver);
	return;
}

module_init(ov5648_init_module);
module_exit(ov5648_exit_module);
MODULE_DESCRIPTION("ov5648");
MODULE_LICENSE("GPL v2");
