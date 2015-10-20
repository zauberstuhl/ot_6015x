/*
 * as364x.c - Led flash driver
 *
 * Version:
 * 2011-01-14: v0.7 : first working version
 * 2011-01-18: v1.0 : adapt also to AS3647
 * 2011-09-26: v1.1 : adapt also to AS3643
 *
 * Copyright (C) 2010 Ulrich Herrmann <ulrich.herrmann@austriamicrosystems.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
//#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include "leds-as364x.h"

//#define CONFIG_FLASHLIGHT_DEBUG
#undef CDBG
#ifdef CONFIG_FLASHLIGHT_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

#ifdef CONFIG_AS3648
#define AS364X_CURR_STEP 3529 /* uA */
#define AS364X_CURR_STEP_BOOST 3921 /* uA */
#define AS364X_TXMASK_STEP 56467 /* uA */
#define AS364X_TXMASK_STEP_BOOST 62747 /* uA */
#define AS364X_NUM_REGS 14
#define AS364X_MAX_PEAK_CURRENT 1000
#define AS364X_MIN_ILIMIT 2000 /* mA*/
#endif

#ifdef CONFIG_AS3647
#define AS364X_CURR_STEP 6274U /* uA */
#define AS364X_TXMASK_STEP 100400 /* uA */
#define AS364X_NUM_REGS 11
#define AS364X_MAX_PEAK_CURRENT 1600
#define AS364X_MIN_ILIMIT 2000 /* mA */
#endif

#ifdef CONFIG_AS3643
#define AS364X_CURR_STEP 5098U /* uA */
#define AS364X_TXMASK_STEP 81600 /* uA */
#define AS364X_NUM_REGS 11
#define AS364X_MAX_PEAK_CURRENT 1300
#define AS364X_MIN_ILIMIT 1000 /* mA */
#endif

#define AS364X_MAX_ASSIST_CURRENT    \
	DIV_ROUND_UP((AS364X_CURR_STEP * 0xff * 0x7f / 0xff), 1000)
#define AS364X_MAX_INDICATOR_CURRENT \
	DIV_ROUND_UP((AS364X_CURR_STEP * 0xff * 0x3f / 0xff), 1000)

#define AS364X_REG_ChipID 0
#define AS364X_REG_Current_Set_LED1 1
#ifdef CONFIG_AS3648
#define AS364X_REG_Current_Set_LED2 2
#endif
#define AS364X_REG_TXMask 3
#define AS364X_REG_Low_Voltage 4
#define AS364X_REG_Flash_Timer 5
#define AS364X_REG_Control 6
#define AS364X_REG_Strobe_Signalling 7
#define AS364X_REG_Fault 8
#define AS364X_REG_PWM_and_Indicator 9
#define AS364X_REG_min_LED_Current 0xE
#define AS364X_REG_act_LED_Current 0xF
#ifdef CONFIG_AS3648
#define AS364X_REG_Password 0x80
#define AS364X_REG_Current_Boost 0x81
#endif

#define AS364X_WRITE_REG(a, b) i2c_smbus_write_byte_data(data->client, a, b)
#define AS364X_READ_REG(a) i2c_smbus_read_byte_data(data->client, a)

#define AS364X_LOCK()   mutex_lock(&(data)->update_lock)
#define AS364X_UNLOCK() mutex_unlock(&(data)->update_lock)

struct as364x_reg {
	const char *name;
	u8 id;
	u8 value;
};

struct as364x_data {
	struct as364x_platform_data *pdata;
	struct i2c_client *client;
	struct mutex update_lock;
	struct as364x_reg regs[AS364X_NUM_REGS];
	int flash_mode; //0-off; 1-assistant light; 2-flash mode; 3-ext_torch mode
	u8 flash_curr;
	u8 torch_curr;
#ifdef CONFIG_AS3648
	u8 flash_boost;
#endif
	u8 flash_time;
	u8 normal_curr;
	u8 normal_ctrl;
	u8 led_mask;
	u8 strobe_reg;
};

static struct as364x_data *as364x;

static void as364x_set_leds(struct as364x_data *data, u8 ledMask,
		u8 ctrl, u8 curr);

#define AS364X_REG(NAME, VAL) \
	{.name = __stringify(NAME), .id = AS364X_REG_##NAME, .value = (VAL)}

static const struct as364x_data as364x_default_data = {
	.client = NULL,
	.regs = {
		AS364X_REG(ChipID, 0xB1),
		AS364X_REG(Current_Set_LED1, 0x9C),
#ifdef CONFIG_AS3648
		AS364X_REG(Current_Set_LED2, 0x9C),
#endif
		AS364X_REG(TXMask, 0x68),
		AS364X_REG(Low_Voltage, 0x2C),
		AS364X_REG(Flash_Timer, 0xDC),
		AS364X_REG(Control, 0x00),
		AS364X_REG(Strobe_Signalling, 0xC0),
		AS364X_REG(Fault, 0x00),
		AS364X_REG(PWM_and_Indicator, 0x00),
		AS364X_REG(min_LED_Current, 0x00),
		AS364X_REG(act_LED_Current, 0x00),
#ifdef CONFIG_AS3648
		AS364X_REG(Password, 0x00),
		AS364X_REG(Current_Boost, 0x00),
#endif
	},
};

#ifdef CONFIG_PM
static int as364x_suspend(struct i2c_client *client, pm_message_t msg)
{
	dev_info(&client->dev, "Suspending AS364X\n");

	return 0;
}

static int as364x_resume(struct i2c_client *client)
{
	dev_info(&client->dev, "Resuming AS364X\n");

	return 0;
}

static void as364x_shutdown(struct i2c_client *client)
{
	struct as364x_data *data = i2c_get_clientdata(client);

	dev_info(&client->dev, "Shutting down AS364X\n");

	AS364X_LOCK();
	as364x_set_leds(data, data->led_mask, 0, 0);
	AS364X_UNLOCK();
}
#endif

static void as364x_set_leds(struct as364x_data *data,
			u8 ledMask, u8 ctrl, u8 curr)
{
    CDBG("%s,%d,ledMask=%d,ctrl=%d,curr=%d\n", __func__, __LINE__, ledMask, ctrl, curr);
	if (ledMask & 1)
		AS364X_WRITE_REG(AS364X_REG_Current_Set_LED1, curr);
	else
		AS364X_WRITE_REG(AS364X_REG_Current_Set_LED1, 0);
#ifdef CONFIG_AS3648
	if (ledMask & 2)
		AS364X_WRITE_REG(AS364X_REG_Current_Set_LED2, curr);
	else
		AS364X_WRITE_REG(AS364X_REG_Current_Set_LED2, 0);
#endif

	if (ledMask == 0 || curr == 0)
		AS364X_WRITE_REG(AS364X_REG_Control, ctrl & ~0x08);
	else
		AS364X_WRITE_REG(AS364X_REG_Control, ctrl);
}

static void as364x_set_txmask(struct as364x_data *data)
{
	u8 tm;
	u32 limit = 0, txmask;

	tm = (data->pdata->use_tx_mask ? 1 : 0);

	if (data->pdata->I_limit_mA > AS364X_MIN_ILIMIT)
		limit = (data->pdata->I_limit_mA - AS364X_MIN_ILIMIT) / 500;

	if (limit > 3)
		limit = 3;
	tm |= limit<<2;

	txmask = data->pdata->txmasked_current_mA * 1000;

#ifdef CONFIG_AS3648
	if (data->flash_boost)
		txmask /= AS364X_TXMASK_STEP_BOOST;
	else
#endif
		txmask /= AS364X_TXMASK_STEP;

	if (txmask > 0xf)
		txmask = 0xf;

	tm |= txmask<<4;

	AS364X_WRITE_REG(AS364X_REG_TXMask, tm);
}

static const u16 v_in_low[] = {3070, 3140, 3220, 3300, 3338, 3470};
static u8 as364x_get_vin_index(u16 mV)
{
	s8 vin;
	if (mV == 0)
		return 0;
	for (vin = ARRAY_SIZE(v_in_low) - 1; vin >= 0; vin--) {
		if (mV >= v_in_low[vin])
			break;
	}
	vin += 2;

	return vin;
}

static int turn_strobe_on(struct as364x_data *data, u8 ctrl)
{
    int err = 0;

	AS364X_LOCK();
	data->flash_mode = (ctrl == 0x0a) ? 1 : 2;
	/* reset fault reg */
	i2c_smbus_read_byte_data(data->client, AS364X_REG_Fault);
	//If control register set to 0x0a, it is assistant light mode, and current only use LSB 6:0.
	as364x_set_leds(data, data->led_mask, ctrl, (ctrl == 0x0a) ? data->torch_curr : data->flash_curr);
	if (data->pdata->gpio_strobe > 0 && (data->strobe_reg & 0x80))
		gpio_direction_output(data->pdata->gpio_strobe, 1);
	AS364X_UNLOCK();

    return err;
}

static int turn_strobe_off(struct as364x_data *data)
{
    int err = 0;

	AS364X_LOCK();
	data->flash_mode = 0;
	as364x_set_leds(data, data->led_mask, 0, 0);
	if (data->pdata->gpio_strobe > 0 && (data->strobe_reg & 0x80))
		gpio_direction_output(data->pdata->gpio_strobe, 0);
	AS364X_UNLOCK();

    return err;
}
#if 0
static int turn_ext_torch_on(struct as364x_data *data)
{
	int err = 0;
	u8 txMask = 0, control = 0;

	AS364X_LOCK();
	data->flash_mode = 3;
	txMask = AS364X_READ_REG(AS364X_REG_TXMask);
	txMask |= 2;
	txMask &= ~1;
	AS364X_WRITE_REG(AS364X_REG_TXMask, txMask);
	control = AS364X_READ_REG(AS364X_REG_Control);
	control &= ~3;
	AS364X_WRITE_REG(AS364X_REG_Control, control);
	AS364X_WRITE_REG(AS364X_REG_Current_Set_LED1, data->torch_curr);
	if (data->pdata->gpio_torch > 0)
		gpio_direction_output(data->pdata->gpio_torch, 1);
	AS364X_UNLOCK();
	
	return err;
}
#endif
static int turn_ext_torch_off(struct as364x_data *data)
{
	int err = 0;

	AS364X_LOCK();
	data->flash_mode = 0;
	if (data->pdata->gpio_torch > 0)
		gpio_direction_output(data->pdata->gpio_torch, 0);
	AS364X_UNLOCK();
	
	return err;
}

static void led_i2c_brightness_set_led_work(struct work_struct *work)
{
    if(as364x->pdata->brightness > LED_HALF) {
		as364x->flash_curr = (as364x->pdata->brightness < 0xE0) ? as364x->pdata->brightness : 0xE0;
        turn_strobe_on(as364x, 0x0b);
    } else if(as364x->pdata->brightness > LED_OFF) {
        as364x->torch_curr = as364x->pdata->brightness;
        turn_strobe_on(as364x, 0x0a);
        //turn_ext_torch_on(as364x);
    } else {
        if (3 != as364x->flash_mode)
	        turn_strobe_off(as364x);
		else
		    turn_ext_torch_off(as364x);
    }
}

static void led_i2c_brightness_set(struct led_classdev *led_cdev,
				    enum led_brightness value)
{
	as364x->pdata->brightness = value;
	schedule_work(&as364x->pdata->work);
}

static enum led_brightness led_i2c_brightness_get(struct led_classdev *led_cdev)
{
    struct as364x_platform_data *flash_led =
	    container_of(led_cdev, struct as364x_platform_data, cdev);
	return flash_led->brightness;
}

static int as364x_configure(struct i2c_client *client,
		struct as364x_data *data, struct as364x_platform_data *pdata)
{
	int err = 0;
	u8 lv, vin;

    pdata->use_tx_mask = false;
	as364x_set_txmask(data);

	vin = as364x_get_vin_index(pdata->vin_low_v_run_mV);
	lv = vin<<0;

	vin = as364x_get_vin_index(pdata->vin_low_v_mV);
	lv |= vin<<3;

	if (pdata->led_off_when_vin_low)
		lv |= 0x40;
	AS364X_WRITE_REG(AS364X_REG_Low_Voltage, lv);

	AS364X_WRITE_REG(AS364X_REG_PWM_and_Indicator,
			pdata->freq_switch_on ? 0x04 : 0);
	
	pdata->strobe_type = 1;
	data->strobe_reg = pdata->strobe_type ? 0xc0 : 0x80;
	AS364X_WRITE_REG(AS364X_REG_Strobe_Signalling, data->strobe_reg);

	if (data->pdata->max_peak_current_mA > AS364X_MAX_PEAK_CURRENT) {
		dev_warn(&client->dev,
				"max_peak_current_mA of %d higher than"
				" possible, reducing to %d\n",
				data->pdata->max_peak_current_mA,
				AS364X_MAX_PEAK_CURRENT);
		data->pdata->max_peak_current_mA = AS364X_MAX_PEAK_CURRENT;
	}
	if (data->pdata->max_sustained_current_mA > AS364X_MAX_ASSIST_CURRENT) {
		dev_warn(&client->dev,
				"max_sustained_current_mA of %d higher than"
				" possible, reducing to %d\n",
				data->pdata->max_sustained_current_mA,
				AS364X_MAX_ASSIST_CURRENT);
		data->pdata->max_sustained_current_mA =
			AS364X_MAX_ASSIST_CURRENT / 1000;
	}
	if ((1000*data->pdata->min_current_mA) < AS364X_CURR_STEP) {
		data->pdata->min_current_mA = AS364X_CURR_STEP / 1000;
		dev_warn(&client->dev,
				"min_current_mA lower than possible, icreasing"
				" to %d\n",
				data->pdata->min_current_mA);
	}
	if (data->pdata->min_current_mA > AS364X_MAX_INDICATOR_CURRENT) {
		dev_warn(&client->dev,
				"min_current_mA of %d higher than possible,"
				" reducing to %d",
				data->pdata->min_current_mA,
				AS364X_MAX_INDICATOR_CURRENT);
		data->pdata->min_current_mA =
			AS364X_MAX_INDICATOR_CURRENT / 1000;
	}

	data->flash_curr = 0xE0;
	data->torch_curr = 0x7F;
	data->led_mask = 3;
	data->normal_ctrl = 0;
	data->normal_curr = 0;
	as364x_set_leds(data, data->led_mask,
			data->normal_ctrl, data->normal_curr);
	
	return err;
}

static int as364x_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
    const char *temp_str;
    struct device_node *node = client->dev.of_node;
	struct as364x_data *data;
	struct as364x_platform_data *pdata;
	int id1, i;
	int err = 0;

    CDBG("%s,%d\n", __func__, __LINE__);
	id1 = i2c_smbus_read_byte_data(client, AS364X_REG_ChipID);
	if (id1 < 0 || ((id1 & 0xf8) != 0xb0)) {
		err = id1;
		dev_err(&client->dev, "no chip or wrong chip detected, ids=%d", id1);
		return -EINVAL;
	}
	dev_info(&client->dev, "AS364X driver v1.1: detected AS364X "
			"compatible chip with id %x\n", id1);
	
    if (node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct as364x_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
	} else {
		pdata = client->dev.platform_data;
    }

	if (!pdata) {
		dev_err(&client->dev, "Invalid pdata\n");
		return -EINVAL;
	}
	
    pdata->cdev.default_trigger = "none";
    err = of_property_read_string(node, "linux,default-trigger", &temp_str);
	if (!err) {
		pdata->cdev.default_trigger = temp_str;
    }

    err = of_property_read_string(node, "linux,name", &pdata->cdev.name);
	if (err) {
		dev_err(&client->dev, "%s: Failed to read linux name. rc = %d\n",
			__func__, err);
		goto free_platform_data;
	}

    INIT_WORK(&pdata->work, led_i2c_brightness_set_led_work);
    pdata->cdev.max_brightness = LED_FULL;
	pdata->cdev.brightness_set = led_i2c_brightness_set;
	pdata->cdev.brightness_get = led_i2c_brightness_get;

    err = led_classdev_register(&client->dev, &pdata->cdev);
	if (err) {
		dev_err(&client->dev, "%s: Failed to register led dev. rc = %d\n",
			__func__, err);
		goto free_platform_data;
	}

    // Get gpio number of flash from dts(i)
    pdata->gpio_torch = of_get_named_gpio(node, "qcom,gpio-torch", 0);
    if (pdata->gpio_torch < 0) {
		pr_err("%s,%d, get torch gpio fail\n", __func__, __LINE__);
    } else {
        err = gpio_request(pdata->gpio_torch, "gpio-torch");
		if (err) {
			pr_err("%s,%d, gpio request %d fail\n", __func__, __LINE__, pdata->gpio_torch);
			goto free_platform_data;
		}
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_torch, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_direction_output(pdata->gpio_torch, 0);
    }

	pdata->gpio_strobe = of_get_named_gpio(node, "qcom,gpio-strobe", 0);
	if (pdata->gpio_strobe < 0) {
		pr_err("%s,%d, get strobe gpio fail\n", __func__, __LINE__);
	} else {
	    err = gpio_request(pdata->gpio_strobe, "gpio-strobe");
		if (err) {
			pr_err("%s,%d, gpio request %d fail\n", __func__, __LINE__, pdata->gpio_strobe);
			goto free_gpio_torch;
		}
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_strobe, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_direction_output(pdata->gpio_strobe, 0);
	}

    // Check and setup i2c client
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C not supported\n");
		goto free_gpio_strobe;
	}
	
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK))
	{
	    dev_err(&client->dev, "%s: I2C error2!\n", __func__);
		goto free_gpio_strobe;
	}

	data = kzalloc(sizeof(struct as364x_data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev, "Not enough memory\n");
		goto free_gpio_strobe;
	}

	/* initialize with meaningful data (register names, etc.) */
	memcpy(data, &as364x_default_data, sizeof(struct as364x_data));
	mutex_init(&data->update_lock);
	data->pdata = pdata;
	data->client = client;
	i2c_set_clientdata(client, data);
    as364x = data;

	for (i = 0; i < ARRAY_SIZE(data->regs); i++) {
		if (data->regs[i].name)
			i2c_smbus_write_byte_data(client,
					data->regs[i].id, data->regs[i].value);
	}

	err = as364x_configure(client, data, pdata);
    if (err) 
		goto free_data;

	return 0;

free_data:
	mutex_destroy(&data->update_lock);
	kfree(data);
	i2c_set_clientdata(client, NULL);
free_gpio_strobe:
	if (pdata->gpio_strobe > 0)
        gpio_free(pdata->gpio_strobe);
free_gpio_torch:
	if (pdata->gpio_torch > 0)
		gpio_free(pdata->gpio_torch);
free_platform_data:
    if(node) {
        kfree(pdata);
    }
	
	return err;
}

static int as364x_remove(struct i2c_client *client)
{
	struct as364x_data *data = i2c_get_clientdata(client);
	
	if (data->pdata->gpio_strobe > 0)
		gpio_free(data->pdata->gpio_strobe);
	if (data->pdata->gpio_torch > 0)
		gpio_free(data->pdata->gpio_torch);
    if(client->dev.of_node)
        kfree(data->pdata);
	mutex_destroy(&data->update_lock);
	kfree(data);
	i2c_set_clientdata(client, NULL);
	
	return 0;
}

static const struct i2c_device_id as364x_id[] = {
	{ "as364x", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, as364x_id);

#ifdef CONFIG_OF
static struct of_device_id as364x_match_table[] = {
    {.compatible = "qcom,led-flash",},
    {},
};
#else
#define as364x_match_table NULL
#endif

static struct i2c_driver as364x_driver = {
	.driver = {
#ifdef CONFIG_AS3648
		.name   = "as3648",
#endif
#ifdef CONFIG_AS3647
		.name   = "as3647",
#endif
#ifdef CONFIG_AS3643
		.name   = "as3643",
#endif
        .owner = THIS_MODULE,
        .of_match_table = as364x_match_table,
	},
	.probe  = as364x_probe,
	.remove = as364x_remove,
#ifdef CONFIG_PM
	.shutdown = as364x_shutdown,
	.suspend  = as364x_suspend,
	.resume   = as364x_resume,
#endif
	.id_table = as364x_id,
};

static int __init as364x_init(void)
{
	return i2c_add_driver(&as364x_driver);
}

static void __exit as364x_exit(void)
{
	i2c_del_driver(&as364x_driver);
}


MODULE_AUTHOR("Ulrich Herrmann <ulrich.herrmann@austriamicrosystems.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AS364X LED flash light");

module_init(as364x_init);
module_exit(as364x_exit);

