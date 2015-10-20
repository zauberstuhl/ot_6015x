/*
 *
 * Mstar Msg2133A TouchScreen driver header file.
 *
 * Copyright (c) 2010  Focal tech Ltd.
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 
#ifndef __LINUX_MSG2133_TS_H__
#define __LINUX_MSG2133_TS_H__

struct msg2133_platform_data {
	char *name;
	int numtouch;
	int irq_gpio;
	int max_x;
	int max_y;
	void (*init_platform_hw)(void);
	void (*exit_platform_hw) (void);
};

#endif
