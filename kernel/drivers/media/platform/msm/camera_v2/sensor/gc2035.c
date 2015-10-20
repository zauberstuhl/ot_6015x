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
 *         Modify History For This Module
 * When           Who             What,Where,Why
 * --------------------------------------------------------------------------------------
 * 13/11/25       Yixiang WU      Add GC2035 camera driver code   
 * --------------------------------------------------------------------------------------
*/
#include "msm_sensor.h"
#include "msm_cci.h"
#include "msm_camera_io_util.h"
#define GC2035_SENSOR_NAME "gc2035"

#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

DEFINE_MSM_MUTEX(gc2035_mut);
static struct msm_sensor_ctrl_t gc2035_s_ctrl;

static struct msm_sensor_power_setting gc2035_power_setting[] = {
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
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
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 24000000,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_LOW,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
};


#if 0
static struct msm_camera_i2c_reg_conf gc2035_full_settings[] = {
	//{0xfe, 0x00},
};
#endif


static struct msm_camera_i2c_reg_conf gc2035_start_settings[] = {
	{0xfe, 0x03},
	{0x10, 0x94},
	{0xfe, 0x00},
};

static struct msm_camera_i2c_reg_conf gc2035_stop_settings[] = {
	{0xfe, 0x03},
	{0x10, 0x00},
	{0xfe, 0x00},
};


//for preview setting
static struct msm_camera_i2c_reg_conf gc2035_svga_settings[] = {
 // vga = 640*480
	{0xfa, 0x00},
	{0xb6, 0x03},//turn on AEC
	{0xc8, 0x15},

	{0x90, 0x01},
	{0x95, 0x01},
	{0x96, 0xe0},
	{0x97, 0x02},
	{0x98, 0x80},

	{0xfe, 0x03},
	{0x12, 0x00},
	{0x13, 0x05},
	{0x04, 0x90},
	{0x05, 0x00},
	//{0x10, 0x94},
	{0xfe, 0x00},
};



//for snapshot
static struct msm_camera_i2c_reg_conf gc2035_uxga_settings[] = {
//uxga = 1600*1200, 2M
	{0x99, 0x11},
	{0x9a, 0x06},
	{0x9b, 0x00},
	{0x9c, 0x00},
	{0x9d, 0x00},
	{0x9e, 0x00},
	{0x9f, 0x00},
	{0xa0, 0x00},
	{0xa1, 0x00},
	{0xa2, 0x00},

	{0xfa, 0x11},
	{0xc8, 0x00},
	{0x90, 0x01},
	{0x95, 0x04},
	{0x96, 0xb0},
	{0x97, 0x06},
	{0x98, 0x40},

	{0xfe, 0x03},
	{0x12, 0x80},
	{0x13, 0x0c},
	{0x04, 0x20},
	{0x05, 0x00},
	//{0x10, 0x94},
	{0xfe, 0x00},
};


//set sensor init setting
static struct msm_camera_i2c_reg_conf gc2035_recommend_settings[] = {
	{0xfe, 0x80},  
	{0xfe, 0x80},  
	{0xfe, 0x80},  
	{0xfc, 0x06},  
	{0xf9, 0xfe},  
	{0xfa, 0x00},  
	{0xf6, 0x00},  
	{0xf7, 0x05},  //by wangpl, down clk, adjust whole MIPI timing, such as LP11 00 01
	{0xf8, 0x85},  //82
	{0xfe, 0x00},  
	{0x82, 0x00},  
	{0xb3, 0x60},  
	{0xb4, 0x40},  
	{0xb5, 0x60},  
	{0x03, 0x02},  
	{0x04, 0xee},  
	{0xfe, 0x00},  
	{0xec, 0x04},  
	{0xed, 0x04},  
	{0xee, 0x60},  
	{0xef, 0x90},  
	{0x0a, 0x00},  
	{0x0c, 0x00},  
	{0x0d, 0x04},  
	{0x0e, 0xc0},  
	{0x0f, 0x06},  
	{0x10, 0x58},  
	{0x17, 0x17},//0x14
	{0x18, 0x0e},  
	{0x19, 0x0c},  
	{0x1a, 0x01},  
	{0x1b, 0x8b},  
	{0x1c, 0x05}, 	
	{0x1e, 0x88},  
	{0x1f, 0x08},  
	{0x20, 0x05},  
	{0x21, 0x0f},  
	{0x22, 0xf0},  
	{0x23, 0xc3},  
	{0x24, 0x16},  
	{0xfe, 0x01},
	{0x4f, 0x00},
	{0x4d, 0x32}, // 30
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4d, 0x42}, // 40
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4d, 0x52}, // 50
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4d, 0x62}, // 60
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4d, 0x72}, // 70
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4d, 0x82}, // 80
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4d, 0x92}, // 90
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4f, 0x01},
	{0x50, 0x88},
	{0xfe, 0x00},
	{0x82, 0xfe},
	{0xfe, 0x01},  
	{0x09, 0x00},  
	{0x0b, 0x90},  
	{0x13, 0x80}, //7c 
	{0xfe, 0x00},  
	{0xfe, 0x00},  
//Hsync blank, dummy pixel, exporse time per line
	{0x05, 0x01},//01
	{0x06, 0x58},  //0a
//Vsync blank, exporse time per frame
	{0x07, 0x00},//
	{0x08, 0x86}, 

	{0xfe, 0x01},  
	
//related to 50Hz 60Hz antibanding
	{0x27, 0x00},//
	{0x28, 0x96}, //a1 
//exporse level 1
	{0x29, 0x05},//05
	{0x2a, 0x46},  //08
//exp level 2
	{0x2b, 0x05},//05
	{0x2c, 0xdc},  //a9
//exp level 3
	{0x2d, 0x05},////06
	{0x2e, 0xdc},  //a4
//exp level 4
	{0x2f, 0x07},//0c
	{0x30, 0x9e},  //94
	
	{0x3e, 0x40},//limit level, effect frame rate, can not modify to 0x30, will increase exp, more noise

	{0xfe, 0x00}, 
	{0xb6, 0x03},  
	{0xfe, 0x00},  
	{0x3f, 0x00},  
	{0x40, 0x77},  
	{0x42, 0x7f},  
	{0x43, 0x30},  
	{0x5c, 0x08},  
	{0x5e, 0x20},//28  
	{0x5f, 0x20},  
	{0x60, 0x20},  
	{0x61, 0x20},  
	{0x62, 0x20},  
	{0x63, 0x20},  
	{0x64, 0x20},  
	{0x65, 0x20},  
	{0x66, 0x00},  //current ratio
	{0x67, 0x00},  
	{0x68, 0x00},  
	{0x69, 0x00}, 
	{0x90, 0x01},  
	{0x95, 0x04},  
	{0x96, 0xb0},  
	{0x97, 0x06},  
	{0x98, 0x40},  
	{0xfe, 0x03},  
	{0x42, 0x80},  
	{0x43, 0x06},  
	{0x41, 0x00},  
	{0x40, 0x00},  
	{0x17, 0x01},  
	{0xfe, 0x00},  
	{0x80, 0xff},  
	{0x81, 0x26},  
	{0x84, 0x02},  
	{0x86, 0x02},  
	{0x87, 0x80},  
	{0x8b, 0xbc},  
	{0xa7, 0x80},  
	{0xa8, 0x80},  
	{0xb0, 0x80},  
	{0xc0, 0x40}, 
	/////lsc lens shading control//////// 
	{0xfe, 0x01},
	{0xc2, 0x2e},
	{0xc3, 0x28},
	{0xc4, 0x27},
	{0xc8, 0x29},
	{0xc9, 0x27},
	{0xca, 0x20},
	{0xbc, 0x4c},
	{0xbd, 0x49},
	{0xbe, 0x2f},
	{0xb6, 0x45},
	{0xb7, 0x33},
	{0xb8, 0x3a},
	{0xc5, 0x14},
	{0xc6, 0x07},
	{0xc7, 0x1d},
	{0xcb, 0x0c},
	{0xcc, 0x16},
	{0xcd, 0x1e},
	{0xbf, 0x17},
	{0xc0, 0x00},
	{0xc1, 0x10},
	{0xb9, 0x09},
	{0xba, 0x00},
	{0xbb, 0x14},
	{0xaa, 0x1c},
	{0xab, 0x10},
	{0xac, 0x0d},
	{0xad, 0x1d},
	{0xae, 0x0f},
	{0xaf, 0x0b},
	{0xb0, 0x2b},
	{0xb1, 0x1d},
	{0xb2, 0x1c},
	{0xb3, 0x14},
	{0xb4, 0x0a},
	{0xb5, 0x0b},
	{0xd0, 0x0e},
	{0xd2, 0x09},
	{0xd3, 0x17},
	{0xd8, 0x37},
	{0xda, 0x2d},
	{0xdb, 0x3d},
	{0xdc, 0x00},
	{0xde, 0x00},
	{0xdf, 0x00},
	{0xd4, 0x00},
	{0xd6, 0x0b},
	{0xd7, 0x13},
	{0xa4, 0x00},
	{0xa5, 0x00},
	{0xa6, 0x30},
	{0xa7, 0x77},
	{0xa8, 0x00},
	{0xa9, 0x00},
	{0xa1, 0x7b},
	{0xa2, 0x71},
	{0xfe, 0x00},
	///end lsc////
	{0xfe, 0x02},  
	{0xa4, 0x00},  
	{0xfe, 0x00},  
	{0xfe, 0x02},  
	{0xc0, 0x01},  
	{0xc1, 0x40},
	{0xc2, 0xfc},
	{0xc3, 0x05},
	{0xc4, 0xf4},
	{0xc5, 0x42},
	{0xc6, 0xf8},
	{0xc7, 0x40},
	{0xc8, 0xf8},
	{0xc9, 0x06},
	{0xca, 0xfd},
	{0xcb, 0x3e},
	{0xcc, 0xf3},
	{0xcd, 0x30},//34
	{0xce, 0xf4},//f6
	{0xcf, 0x02}, //04
	{0xe3, 0x02},  //0c
	{0xe4, 0x4a},  //44
	{0xe5, 0xf4}, //e5
	{0xfe, 0x00},
	////for awb clear/// 
	{0xfe, 0x01},
	{0x4f, 0x00}, 
	{0x4d, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x10}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4d, 0x20},  ///////////////20
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x30}, //////////////////30
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x40},  //////////////////////40
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x50}, //////////////////50
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x60}, /////////////////60
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x70}, ///////////////////70
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x80}, /////////////////////80
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 		  
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4d, 0x90}, //////////////////////90
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4d, 0xa0}, /////////////////a0
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4d, 0xb0}, //////////////////////////////////b0
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0xc0}, //////////////////////////////////c0
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0xd0}, ////////////////////////////d0
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4d, 0xe0}, /////////////////////////////////e0
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0xf0}, /////////////////////////////////f0
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4f, 0x01}, 
	////for awb vlaue//
	{0xfe, 0x01},
	{0x4f, 0x00},
	{0x4d, 0x33},
	{0x4e, 0x08},
	{0x4e, 0x04},
	{0x4e, 0x32}, 
	{0x4e, 0x02},
	{0x4d, 0x43},
	{0x4e, 0x08}, 
	{0x4e, 0x04},
	{0x4e, 0x34},
	{0x4e, 0x02},
	{0x4d, 0x53},
	{0x4e, 0x08}, 
	{0x4d, 0x62},
	{0x4e, 0x20}, 
	{0x4e, 0x08}, 
	{0x4d, 0x72},
	{0x4e, 0x20},
	{0x4f, 0x01},

	{0x50, 0x80},
	{0x56, 0x06},
	
	//{0x50, 0x80},
	{0x52, 0x40},
	{0x54, 0x60},
	//{0x56, 0x00},
	{0x57, 0x20},
	{0x58, 0x01}, 
	{0x5b, 0x04},
	{0x61, 0xaa},
	{0x62, 0xaa},
	{0x71, 0x00},
	{0x72, 0x25},
	{0x74, 0x10},
	{0x77, 0x08},
	{0x78, 0xfd},
	{0x86, 0x30},
	{0x87, 0x00},
	{0x88, 0x04},
	{0x8a, 0xc0},
	{0x89, 0x71},
	{0x84, 0x08},
	{0x8b, 0x00},
	{0x8d, 0x70},
	{0x8e, 0x70},
	{0x8f, 0xf4},
	{0xfe, 0x00},  
	{0x82, 0x02},  
	{0xfe, 0x01},  
	{0x21, 0xbf},  
	{0xfe, 0x02},  
	{0xa5, 0x50}, 
	{0xa2, 0xb0},  
	{0xa6, 0x50},  
	{0xa7, 0x30},  
	{0xab, 0x31},  
	{0x88, 0x1c},  
	{0xa9, 0x6c},  
	{0xb0, 0x55},  
	{0xb3, 0x70},  
	{0x8c, 0xf6},  
	{0x89, 0x03},   
	{0xde, 0xb6},  
	{0x38, 0x08},  
	{0x39, 0x50},  
	{0xfe, 0x00},  
	{0x81, 0x26},  
	{0x87, 0x80},  
	{0xfe, 0x02},  
	{0x83, 0x00},  
	{0x84, 0x45},  
	{0xd1, 0x38},  
	{0xd2, 0x38},
	{0xd3, 0x40},
	{0xd4, 0x80},
	{0xd5, 0x00},
	{0xdc, 0x30},
	{0xdd, 0x38},
	{0xfe, 0x00},  
	{0xfe, 0x02},
	{0x88, 0x15},
	{0x8c, 0xf6},
	{0x89, 0x03},
	{0xfe, 0x02},
	{0x90, 0x6c},
	{0x97, 0x45}, 
	{0xfe , 0x02},
	{0x15 , 0x0b},
	{0x16 , 0x0e},
	{0x17 , 0x10},
	{0x18 , 0x12},
	{0x19 , 0x19},
	{0x1a , 0x21},
	{0x1b , 0x29},
	{0x1c , 0x31},
	{0x1d , 0x41},
	{0x1e , 0x50},
	{0x1f , 0x5f},
	{0x20 , 0x6d},
	{0x21 , 0x79},
	{0x22 , 0x91},
	{0x23 , 0xa5},
	{0x24 , 0xb9},
	{0x25 , 0xc9},
	{0x26 , 0xe1},
	{0x27 , 0xee},
	{0x28 , 0xf7},
	{0x29 , 0xff},

	{0xfe, 0x02},  
	{0x2b, 0x00},  
	{0x2c, 0x04},  
	{0x2d, 0x09},  
	{0x2e, 0x18},  
	{0x2f, 0x27},  
	{0x30, 0x37},  
	{0x31, 0x49},  
	{0x32, 0x5c},  
	{0x33, 0x7e},  
	{0x34, 0xa0},  
	{0x35, 0xc0},  
	{0x36, 0xe0},  
	{0x37, 0xff},  
	{0xfe, 0x00},  
	{0x82, 0xfe},



	
	{0xad, 0x80},	
	{0xae, 0x80},	
	{0xaf, 0x80},
	
	//MIPI timing setting begin
	{0xf2, 0x00},
	{0xf3, 0x00},
	{0xf4, 0x00},
	{0xf5, 0x00},
	{0xfe, 0x01},
	{0x0b, 0x90},
	{0x87, 0x10},
	{0xfe, 0x00},

	{0xfe, 0x03},
	{0x01, 0x03},
	{0x02, 0x37},
	{0x03, 0x10},
	{0x06, 0x90},//leo changed
	{0x11, 0x1E},
	{0x12, 0x80},
	{0x13, 0x0c},
	{0x15, 0x10},
	{0x04, 0x20},
	{0x05, 0x00},
	{0x17, 0x00},

	{0x21, 0x01},
	{0x22, 0x03},
	{0x23, 0x01},
	{0x29, 0x03},
	{0x2a, 0x01},
	{0xfe, 0x00},
//MIPI timing setting end
};

static struct msm_camera_i2c_reg_conf GC2035_reg_saturation[11][5] = {
	{
{0xfe, 0x02},{0xd1, 0x10},{0xd2, 0x10},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x18},{0xd2, 0x18},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x20},{0xd2, 0x20},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x28},{0xd2, 0x28},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x30},{0xd2, 0x30},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x34},{0xd2, 0x34},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x40},{0xd2, 0x40},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x48},{0xd2, 0x48},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x50},{0xd2, 0x50},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x58},{0xd2, 0x58},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x60},{0xd2, 0x60},{0xfe, 0x00}
	},
};

static struct msm_camera_i2c_reg_conf GC2035_reg_contrast[11][3] = {
	{
{0xfe, 0x02},{0xd3, 0x18},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x20},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x28},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x30},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x38},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x40},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x48},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x50},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x58},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x60},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x68},{0xfe, 0x00}
	},
};

static struct msm_camera_i2c_reg_conf GC2035_reg_sharpness[6][3] = {
	{{0xfe, 0x02},{0x97, 0x26},{0xfe, 0x00}},//Sharpness -2
	{{0xfe, 0x02},{0x97, 0x37},{0xfe, 0x00}},//Sharpness -1
	{{0xfe, 0x02},{0x97, 0x45},{0xfe, 0x00}},//Sharpness
	{{0xfe, 0x02},{0x97, 0x59},{0xfe, 0x00}},//Sharpness +1
	{{0xfe, 0x02},{0x97, 0x6a},{0xfe, 0x00}},//Sharpness +2
	{{0xfe, 0x02},{0x97, 0x7b},{0xfe, 0x00}},//Sharpness +3
};
static struct msm_camera_i2c_reg_conf GC2035_reg_exposure_compensation[5][3] = {
	{{0xfe, 0x01},{0x13, 0x60},{0xfe, 0x00}},//Exposure -2
	{{0xfe, 0x01},{0x13, 0x70},{0xfe, 0x00}},//Exposure -1
	{{0xfe, 0x01},{0x13, 0x80},{0xfe, 0x00}},//Exposure
	{{0xfe, 0x01},{0x13, 0x98},{0xfe, 0x00}},//Exposure +1
	{{0xfe, 0x01},{0x13, 0xa0},{0xfe, 0x00}},//Exposure +2
};
static struct msm_camera_i2c_reg_conf GC2035_reg_antibanding[][17] = {
	/* OFF */
	{
	{0xfe, 0x00},{0x05, 0x01},{0x06, 0x58},{0x07, 0x00},{0x08, 0x22},{0xfe, 0x01},{0x27, 0x00},{0x28, 0x7d}, 
	{0x29, 0x04},{0x2a, 0xe2}, {0x2b, 0x05},{0x2c, 0xdc},{0x2d, 0x05},{0x2e, 0xdc},{0x2f, 0x07},{0x30, 0x53}
	}, /*ANTIBANDING 60HZ*/

	
	/* 50Hz */
	{
	{0xfe, 0x00},{0x05, 0x01},{0x06, 0x58},{0x07, 0x00},{0x08, 0x86},{0xfe, 0x01},{0x27, 0x00},{0x28, 0x96}, 
	{0x29, 0x05},{0x2a, 0x46}, {0x2b, 0x05},{0x2c, 0xdc},{0x2d, 0x05},{0x2e, 0xdc},{0x2f, 0x07},{0x30, 0x9e}
	}, /*ANTIBANDING 50HZ*/


	/* 60Hz */
	{
	{0xfe, 0x00},{0x05, 0x01},{0x06, 0x58},{0x07, 0x00},{0x08, 0x22},{0xfe, 0x01},{0x27, 0x00},{0x28, 0x7d}, 
	{0x29, 0x04},{0x2a, 0xe2}, {0x2b, 0x05},{0x2c, 0xdc},{0x2d, 0x05},{0x2e, 0xdc},{0x2f, 0x07},{0x30, 0x53}
	}, /*ANTIBANDING 60HZ*/

	
	/* AUTO */
	{
	{0xfe, 0x00},{0x05, 0x01},{0x06, 0x58},{0x07, 0x00},{0x08, 0x86},{0xfe, 0x01},{0x27, 0x00},{0x28, 0x96}, 
	{0x29, 0x05},{0x2a, 0x46}, {0x2b, 0x05},{0x2c, 0xdc},{0x2d, 0x05},{0x2e, 0xdc},{0x2f, 0x07},{0x30, 0x9e}
	},/*ANTIBANDING 50HZ*/
};

//begin effect
static struct msm_camera_i2c_reg_conf GC2035_reg_effect_normal[] = {
	/* normal: */
	{0x83, 0xe0},
};

static struct msm_camera_i2c_reg_conf GC2035_reg_effect_black_white[] = {
	/* B&W: */
{0x83, 0x12}
};

static struct msm_camera_i2c_reg_conf GC2035_reg_effect_negative[] = {
	/* Negative: */
{0x83, 0x01}
};

static struct msm_camera_i2c_reg_conf GC2035_reg_effect_old_movie[] = {
	/* Sepia(antique): */
{0x83, 0x82}
};

static struct msm_camera_i2c_reg_conf GC2035_reg_effect_solarize[] = {
	{0xfe, 0x00},
};
// end effect


//begin scene, not realised
static struct msm_camera_i2c_reg_conf GC2035_reg_scene_auto[] = {
	/* <SCENE_auto> */
		{0xfe, 0x00},

};

static struct msm_camera_i2c_reg_conf GC2035_reg_scene_portrait[] = {
	/* <CAMTUNING_SCENE_PORTRAIT> */
		{0xfe, 0x00},

};

static struct msm_camera_i2c_reg_conf GC2035_reg_scene_landscape[] = {
	/* <CAMTUNING_SCENE_LANDSCAPE> */
		{0xfe, 0x00},

};

static struct msm_camera_i2c_reg_conf GC2035_reg_scene_night[] = {
	/* <SCENE_NIGHT> */
		{0xfe, 0x00},

};
//end scene


//begin white balance
static struct msm_camera_i2c_reg_conf GC2035_reg_wb_auto[] = {
	/* Auto: */
{0xb3, 0x61},{0xb4, 0x40},{0xb5, 0x61},{0x82, 0xfe}
};

static struct msm_camera_i2c_reg_conf GC2035_reg_wb_sunny[] = {
	/* Sunny: */
{0x82, 0xfc},{0xb3, 0x58},{0xb4, 0x40},{0xb5, 0x50}
};

static struct msm_camera_i2c_reg_conf GC2035_reg_wb_cloudy[] = {
	/* Cloudy: */
{0x82, 0xfc},{0xb3, 0x8c},{0xb4, 0x50},{0xb5, 0x40}
};

static struct msm_camera_i2c_reg_conf GC2035_reg_wb_office[] = {
	/* Office: */
{0x82, 0xfc},{0xb3, 0x72},{0xb4, 0x40},{0xb5, 0x5b}
};

static struct msm_camera_i2c_reg_conf GC2035_reg_wb_home[] = {
	/* Home: */
{0x82, 0xfc},{0xb3, 0x50},{0xb4, 0x40},{0xb5, 0xa8}
};
//end white balance



static void gc2035_set_stauration(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	pr_debug("%s %d", __func__, value);

	s_ctrl->sensor_i2c_client->i2c_func_tbl->
	i2c_write_conf_tbl(
	s_ctrl->sensor_i2c_client, GC2035_reg_saturation[value],
	ARRAY_SIZE(GC2035_reg_saturation[value]),
	MSM_CAMERA_I2C_BYTE_DATA);
}

static void gc2035_set_contrast(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	pr_debug("%s %d", __func__, value);

	s_ctrl->sensor_i2c_client->i2c_func_tbl->
	i2c_write_conf_tbl(
	s_ctrl->sensor_i2c_client, GC2035_reg_contrast[value],
	ARRAY_SIZE(GC2035_reg_contrast[value]),
	MSM_CAMERA_I2C_BYTE_DATA);
}

static void gc2035_set_sharpness(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int val = value / 6;
	pr_debug("%s %d", __func__, value);
	
	s_ctrl->sensor_i2c_client->i2c_func_tbl->
	i2c_write_conf_tbl(
	s_ctrl->sensor_i2c_client, GC2035_reg_sharpness[val],
	ARRAY_SIZE(GC2035_reg_sharpness[val]),
	MSM_CAMERA_I2C_BYTE_DATA);
}
static void gc2035_set_exposure_compensation(struct msm_sensor_ctrl_t *s_ctrl,
	int value)
{
	int val = (value + 12) / 6;
	pr_debug("%s %d", __func__, val);

	s_ctrl->sensor_i2c_client->i2c_func_tbl->
	i2c_write_conf_tbl(
	s_ctrl->sensor_i2c_client, GC2035_reg_exposure_compensation[val],
	ARRAY_SIZE(GC2035_reg_exposure_compensation[val]),
	MSM_CAMERA_I2C_BYTE_DATA);
}
static void gc2035_set_effect(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	pr_debug("%s %d", __func__, value);
	switch (value) {
	case MSM_CAMERA_EFFECT_MODE_OFF: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_effect_normal,
		ARRAY_SIZE(GC2035_reg_effect_normal),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_MONO: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_effect_black_white,
		ARRAY_SIZE(GC2035_reg_effect_black_white),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_NEGATIVE: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_effect_negative,
		ARRAY_SIZE(GC2035_reg_effect_negative),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_SEPIA: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_effect_old_movie,
		ARRAY_SIZE(GC2035_reg_effect_old_movie),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_SOLARIZE: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_effect_solarize,
		ARRAY_SIZE(GC2035_reg_effect_solarize),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	default:
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_effect_normal,
		ARRAY_SIZE(GC2035_reg_effect_normal),
		MSM_CAMERA_I2C_BYTE_DATA);
	}
}

		
static void gc2035_set_antibanding(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	pr_debug("%s %d", __func__, value);

	s_ctrl->sensor_i2c_client->i2c_func_tbl->
	i2c_write_conf_tbl(
	s_ctrl->sensor_i2c_client, GC2035_reg_antibanding[value],
	ARRAY_SIZE(GC2035_reg_antibanding[value]),
	MSM_CAMERA_I2C_BYTE_DATA);	
}
static void gc2035_set_scene_mode(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	pr_debug("%s %d", __func__, value);
	switch (value) {
	case MSM_CAMERA_SCENE_MODE_OFF: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_scene_auto,
		ARRAY_SIZE(GC2035_reg_scene_auto),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	case MSM_CAMERA_SCENE_MODE_NIGHT: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_scene_night,
		ARRAY_SIZE(GC2035_reg_scene_night),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	case MSM_CAMERA_SCENE_MODE_LANDSCAPE: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_scene_landscape,
		ARRAY_SIZE(GC2035_reg_scene_landscape),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	case MSM_CAMERA_SCENE_MODE_PORTRAIT: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_scene_portrait,
		ARRAY_SIZE(GC2035_reg_scene_portrait),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	default:
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_scene_auto,
		ARRAY_SIZE(GC2035_reg_scene_auto),
		MSM_CAMERA_I2C_BYTE_DATA);
	}
}

static void gc2035_set_white_balance_mode(struct msm_sensor_ctrl_t *s_ctrl,
	int value)
{
	pr_debug("%s %d", __func__, value);
	switch (value) {
	case MSM_CAMERA_WB_MODE_AUTO: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_wb_auto,
		ARRAY_SIZE(GC2035_reg_wb_auto),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	case MSM_CAMERA_WB_MODE_INCANDESCENT: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_wb_home,
		ARRAY_SIZE(GC2035_reg_wb_home),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	case MSM_CAMERA_WB_MODE_DAYLIGHT: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_wb_sunny,
		ARRAY_SIZE(GC2035_reg_wb_sunny),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	case MSM_CAMERA_WB_MODE_FLUORESCENT: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_wb_office,
		ARRAY_SIZE(GC2035_reg_wb_office),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	case MSM_CAMERA_WB_MODE_CLOUDY_DAYLIGHT: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_wb_cloudy,
		ARRAY_SIZE(GC2035_reg_wb_cloudy),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	default:
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_reg_wb_auto,
		ARRAY_SIZE(GC2035_reg_wb_auto),
		MSM_CAMERA_I2C_BYTE_DATA);
	}
}


static struct v4l2_subdev_info gc2035_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};
#if 0
static struct msm_camera_i2c_reg_conf gc2035_config_change_settings[] = {
    
};
#endif
static const struct i2c_device_id gc2035_i2c_id[] = {
	{GC2035_SENSOR_NAME, (kernel_ulong_t)&gc2035_s_ctrl},
	{ }
};

static int32_t msm_gc2035_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &gc2035_s_ctrl);
}

static struct i2c_driver gc2035_i2c_driver = {
	.id_table = gc2035_i2c_id,
	.probe  = msm_gc2035_i2c_probe,
	.driver = {
		.name = GC2035_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client gc2035_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static const struct of_device_id gc2035_dt_match[] = {
	{.compatible = "shinetech,gc2035", .data = &gc2035_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, gc2035_dt_match);

static struct platform_driver gc2035_platform_driver = {
	.driver = {
		.name = "shinetech,gc2035",
		.owner = THIS_MODULE,
		.of_match_table = gc2035_dt_match,
	},
};

static int32_t gc2035_platform_probe(struct platform_device *pdev)
{
	int32_t rc;
	const struct of_device_id *match;

	match = of_match_device(gc2035_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init gc2035_init_module(void)
{
	int32_t rc;
	
	rc = platform_driver_probe(&gc2035_platform_driver,
		gc2035_platform_probe);
	if (!rc)
		return rc;
	return i2c_add_driver(&gc2035_i2c_driver);
}

static void __exit gc2035_exit_module(void)
{
	if (gc2035_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&gc2035_s_ctrl);
		platform_driver_unregister(&gc2035_platform_driver);
	} else
		i2c_del_driver(&gc2035_i2c_driver);
	return;
}

void gc2035_set_shutter(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t temp = 1;
	uint16_t shutter = 1;
	uint16_t shutter_h = 1,shutter_L = 1 ;
	s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
		s_ctrl->sensor_i2c_client,
		0xb6,
		0x00, MSM_CAMERA_I2C_BYTE_DATA);  //AEC off
	s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
			s_ctrl->sensor_i2c_client,
			0x03,
			&shutter_h, MSM_CAMERA_I2C_BYTE_DATA);
	s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
			s_ctrl->sensor_i2c_client,
			0x04,
			&shutter_L, MSM_CAMERA_I2C_BYTE_DATA);
        shutter = (shutter_h << 8) | (shutter_L & 0xff) ;
        CDBG("=====>preview_shutter=  %x  \n", shutter);
       	shutter = shutter / 2;
	if (shutter < 1)
		shutter = 1;
	shutter = shutter & 0x1fff;
        temp = shutter & 0xff;
	s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
		s_ctrl->sensor_i2c_client,
		0x04,
		temp, MSM_CAMERA_I2C_BYTE_DATA);
	temp = (shutter >> 8) & 0xff;		
	s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
		s_ctrl->sensor_i2c_client,
		0x03,
		temp, MSM_CAMERA_I2C_BYTE_DATA);		
}
int32_t gc2035_sensor_config(struct msm_sensor_ctrl_t *s_ctrl,
	void __user *argp)
{
	struct sensorb_cfg_data *cdata = (struct sensorb_cfg_data *)argp;
	long rc = 0;
	int32_t i = 0;
    
	mutex_lock(s_ctrl->msm_sensor_mutex);
	CDBG("%s:%d %s cfgtype = %d\n", __func__, __LINE__,
		s_ctrl->sensordata->sensor_name, cdata->cfgtype);
	switch (cdata->cfgtype) {
	case CFG_GET_SENSOR_INFO:
		memcpy(cdata->cfg.sensor_info.sensor_name,
			s_ctrl->sensordata->sensor_name,
			sizeof(cdata->cfg.sensor_info.sensor_name));
		cdata->cfg.sensor_info.session_id =
			s_ctrl->sensordata->sensor_info->session_id;
		for (i = 0; i < SUB_MODULE_MAX; i++)
			cdata->cfg.sensor_info.subdev_id[i] =
				s_ctrl->sensordata->sensor_info->subdev_id[i];
		CDBG("%s:%d sensor name %s\n", __func__, __LINE__,
			cdata->cfg.sensor_info.sensor_name);
		CDBG("%s:%d session id %d\n", __func__, __LINE__,
			cdata->cfg.sensor_info.session_id);
		for (i = 0; i < SUB_MODULE_MAX; i++)
			CDBG("%s:%d subdev_id[%d] %d\n", __func__, __LINE__, i,
				cdata->cfg.sensor_info.subdev_id[i]);

		break;
	case CFG_SET_INIT_SETTING:
		/* 1. Write Recommend settings */
		/* 2. Write change settings */
#if 0
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc2035_recommend_settings_0,
			ARRAY_SIZE(gc2035_recommend_settings_0),
			MSM_CAMERA_I2C_BYTE_DATA);
        msleep(50);

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc2035_recommend_settings_1,
			ARRAY_SIZE(gc2035_recommend_settings_1),
			MSM_CAMERA_I2C_BYTE_DATA);
        
        
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc2035_full_settings,
			ARRAY_SIZE(gc2035_full_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
#else

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
		    i2c_write_conf_tbl(
		    s_ctrl->sensor_i2c_client, gc2035_recommend_settings,
		    ARRAY_SIZE(gc2035_recommend_settings),
		    MSM_CAMERA_I2C_BYTE_DATA);
			mdelay(150);			
#endif
#if 0
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client,
			gc2035_config_change_settings,
			ARRAY_SIZE(gc2035_config_change_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
#endif
		break;
	case CFG_SET_RESOLUTION:
		{
		int val = 0;
		if (copy_from_user(&val,
			(void *)cdata->cfg.setting, sizeof(int))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
	if (val == 0)
		{
			gc2035_set_shutter(s_ctrl);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc2035_uxga_settings,
			ARRAY_SIZE(gc2035_uxga_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
		}
	else if (val == 1)
	


		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc2035_svga_settings,
			ARRAY_SIZE(gc2035_svga_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
		}
		break;
	case CFG_SET_STOP_STREAM:
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc2035_stop_settings,
			ARRAY_SIZE(gc2035_stop_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
		break;
	case CFG_SET_START_STREAM:
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc2035_start_settings,
			ARRAY_SIZE(gc2035_start_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
#if 0
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, gc2035_recommend_settings,
		ARRAY_SIZE(gc2035_recommend_settings),
		MSM_CAMERA_I2C_BYTE_DATA);
		//msleep(50);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, gc2035_full_settings,
		ARRAY_SIZE(gc2035_full_settings),
		MSM_CAMERA_I2C_BYTE_DATA);
#endif
#if 0
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client,
			gc2035_config_change_settings,
			ARRAY_SIZE(gc2035_config_change_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
#endif
		break;
	case CFG_GET_SENSOR_INIT_PARAMS:
		cdata->cfg.sensor_init_params =
			*s_ctrl->sensordata->sensor_init_params;
		CDBG("%s:%d init params mode %d pos %d mount %d\n", __func__,
			__LINE__,
			cdata->cfg.sensor_init_params.modes_supported,
			cdata->cfg.sensor_init_params.position,
			cdata->cfg.sensor_init_params.sensor_mount_angle);
		break;
	case CFG_SET_SLAVE_INFO: {
		struct msm_camera_sensor_slave_info sensor_slave_info;
		struct msm_sensor_power_setting_array *power_setting_array;
		int slave_index = 0;
		if (copy_from_user(&sensor_slave_info,
		    (void *)cdata->cfg.setting,
		    sizeof(struct msm_camera_sensor_slave_info))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		/* Update sensor slave address */
		if (sensor_slave_info.slave_addr) {
			s_ctrl->sensor_i2c_client->cci_client->sid =
				sensor_slave_info.slave_addr >> 1;
		}

		/* Update sensor address type */
		s_ctrl->sensor_i2c_client->addr_type =
			sensor_slave_info.addr_type;

		/* Update power up / down sequence */
		s_ctrl->power_setting_array =
			sensor_slave_info.power_setting_array;
		power_setting_array = &s_ctrl->power_setting_array;
		power_setting_array->power_setting = kzalloc(
			power_setting_array->size *
			sizeof(struct msm_sensor_power_setting), GFP_KERNEL);
		if (!power_setting_array->power_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(power_setting_array->power_setting,
		    (void *)sensor_slave_info.power_setting_array.power_setting,
		    power_setting_array->size *
		    sizeof(struct msm_sensor_power_setting))) {
			kfree(power_setting_array->power_setting);
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		s_ctrl->free_power_setting = true;
		CDBG("%s sensor id %x\n", __func__,
			sensor_slave_info.slave_addr);
		CDBG("%s sensor addr type %d\n", __func__,
			sensor_slave_info.addr_type);
		CDBG("%s sensor reg %x\n", __func__,
			sensor_slave_info.sensor_id_info.sensor_id_reg_addr);
		CDBG("%s sensor id %x\n", __func__,
			sensor_slave_info.sensor_id_info.sensor_id);
		for (slave_index = 0; slave_index <
			power_setting_array->size; slave_index++) {
			CDBG("%s i %d power setting %d %d %ld %d\n", __func__,
				slave_index,
				power_setting_array->power_setting[slave_index].
				seq_type,
				power_setting_array->power_setting[slave_index].
				seq_val,
				power_setting_array->power_setting[slave_index].
				config_val,
				power_setting_array->power_setting[slave_index].
				delay);
		}
		kfree(power_setting_array->power_setting);
		break;
	}
	case CFG_WRITE_I2C_ARRAY: {
		struct msm_camera_i2c_reg_setting conf_array;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;

		if (copy_from_user(&conf_array,
			(void *)cdata->cfg.setting,
			sizeof(struct msm_camera_i2c_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(
			s_ctrl->sensor_i2c_client, &conf_array);
		kfree(reg_setting);
		break;
	}
	case CFG_WRITE_I2C_SEQ_ARRAY: {
		struct msm_camera_i2c_seq_reg_setting conf_array;
		struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;

		if (copy_from_user(&conf_array,
			(void *)cdata->cfg.setting,
			sizeof(struct msm_camera_i2c_seq_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_seq_reg_array)),
			GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_seq_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_seq_table(s_ctrl->sensor_i2c_client,
			&conf_array);
		kfree(reg_setting);
		break;
	}

	case CFG_POWER_UP:
		if (s_ctrl->func_tbl->sensor_power_up)
			rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
		else
			rc = -EFAULT;
		break;

	case CFG_POWER_DOWN:
#if 1
		if (s_ctrl->func_tbl->sensor_power_down)
			rc = s_ctrl->func_tbl->sensor_power_down(
				s_ctrl);
		else
			rc = -EFAULT;
#endif
		break;

	case CFG_SET_STOP_STREAM_SETTING: {
		struct msm_camera_i2c_reg_setting *stop_setting =
			&s_ctrl->stop_setting;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;
		if (copy_from_user(stop_setting, (void *)cdata->cfg.setting,
		    sizeof(struct msm_camera_i2c_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = stop_setting->reg_setting;
		stop_setting->reg_setting = kzalloc(stop_setting->size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!stop_setting->reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(stop_setting->reg_setting,
		    (void *)reg_setting, stop_setting->size *
		    sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(stop_setting->reg_setting);
			stop_setting->reg_setting = NULL;
			stop_setting->size = 0;
			rc = -EFAULT;
			break;
		}
		break;
		}
		case CFG_SET_SATURATION: {
			int32_t sat_lev;
			if (copy_from_user(&sat_lev, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("%s:%d failed\n", __func__, __LINE__);
				rc = -EFAULT;
				break;
			}
		pr_debug("%s: Saturation Value is %d", __func__, sat_lev);
#if 1
		gc2035_set_stauration(s_ctrl, sat_lev);
#endif
		break;
		}
		case CFG_SET_CONTRAST: {
			int32_t con_lev;
			if (copy_from_user(&con_lev, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("%s:%d failed\n", __func__, __LINE__);
				rc = -EFAULT;
				break;
			}
		pr_debug("%s: Contrast Value is %d", __func__, con_lev);
#if 1
		gc2035_set_contrast(s_ctrl, con_lev);
#endif
		break;
		}
		case CFG_SET_SHARPNESS: {
			int32_t shp_lev;
			if (copy_from_user(&shp_lev, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("%s:%d failed\n", __func__, __LINE__);
				rc = -EFAULT;
				break;
			}
		pr_debug("%s: Sharpness Value is %d", __func__, shp_lev);
#if 1
		gc2035_set_sharpness(s_ctrl, shp_lev);
#endif
		break;
		}
		case CFG_SET_AUTOFOCUS: {
		/* TO-DO: set the Auto Focus */
		pr_debug("%s: Setting Auto Focus", __func__);
		break;
		}
		case CFG_CANCEL_AUTOFOCUS: {
		/* TO-DO: Cancel the Auto Focus */
		pr_debug("%s: Cancelling Auto Focus", __func__);
		break;
		}
	case CFG_SET_ANTIBANDING: {
#if 1
		int32_t anti_mode;
		
		if (copy_from_user(&anti_mode,
			(void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		pr_debug("%s: anti-banding mode is %d", __func__,
			anti_mode);
		gc2035_set_antibanding(s_ctrl, anti_mode);
#endif
		break;
		}
	case CFG_SET_EXPOSURE_COMPENSATION: {
#if 1
		int32_t ec_lev;
		if (copy_from_user(&ec_lev, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		pr_debug("%s: Exposure compensation Value is %d",
			__func__, ec_lev);
		gc2035_set_exposure_compensation(s_ctrl, ec_lev);
#endif	
		break;
		}
	case CFG_SET_WHITE_BALANCE: {
#if 1
		int32_t wb_mode;
		if (copy_from_user(&wb_mode, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		pr_debug("%s: white balance is %d", __func__, wb_mode);
		gc2035_set_white_balance_mode(s_ctrl, wb_mode);
#endif	
		break;
		}
	case CFG_SET_EFFECT: {
#if 1
		int32_t effect_mode;
		if (copy_from_user(&effect_mode, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		pr_debug("%s: Effect mode is %d", __func__, effect_mode);
		gc2035_set_effect(s_ctrl, effect_mode);
#endif	
		break;
		}
	case CFG_SET_BESTSHOT_MODE: {
#if 1
		int32_t bs_mode;
		if (copy_from_user(&bs_mode, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		pr_debug("%s: best shot mode is %d", __func__, bs_mode);
		gc2035_set_scene_mode(s_ctrl, bs_mode);
#endif
		break;
		}
	case CFG_SET_ISO:
		break;		
	default:
		rc = -EFAULT;
		break;
	}

	mutex_unlock(s_ctrl->msm_sensor_mutex);

	return rc;
}

static struct msm_sensor_fn_t gc2035_sensor_func_tbl = {
	.sensor_config = gc2035_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_match_id = msm_sensor_match_id,
};

static struct msm_sensor_ctrl_t gc2035_s_ctrl = {
	.sensor_i2c_client = &gc2035_sensor_i2c_client,
	.power_setting_array.power_setting = gc2035_power_setting,
	.power_setting_array.size = ARRAY_SIZE(gc2035_power_setting),
	.msm_sensor_mutex = &gc2035_mut,
	.sensor_v4l2_subdev_info = gc2035_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(gc2035_subdev_info),
	.func_tbl = &gc2035_sensor_func_tbl,
};

module_init(gc2035_init_module);
module_exit(gc2035_exit_module);
MODULE_DESCRIPTION("GC2035 2MP YUV sensor driver");
MODULE_LICENSE("GPL v2");

