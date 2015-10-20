/* linux/drivers/input/touchscreen/s3c-ts.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *

 */
//#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <mach/irqs.h>
// Jack add
#include <linux/i2c.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>


#define ANDROID_TS

#ifdef CONFIG_ANDROID_POWER
#include <linux/android_power.h>
#include <linux/earlysuspend.h>
#endif
#include <linux/input/msg2133_ts.h>

#ifdef CONFIG_TOUCHSCREEN_MSG2133A_PSENSOR
#include <linux/wakelock.h>
#endif

//Below resolution depends on your system
#define ANDROID_TS_RESOLUTION_X 320
#define ANDROID_TS_RESOLUTION_Y 480

#define CNT_RESOLUTION_X  2048
#define CNT_RESOLUTION_Y  2048

#define Touch_State_No_Touch   0
#define Touch_State_One_Finger 1
#define Touch_State_Two_Finger 2
#define Touch_State_VK 3
#ifdef CONFIG_TOUCHSCREEN_MSG2133A_PSENSOR
#define Touch_State_PS 4
#endif

#define INT_GPIO 1
#define RST_GPIO 0

static struct i2c_client *this_client;
static struct workqueue_struct *CNT_wq;

struct  Data_CNT {
	struct input_dev	*input;
	char			Name[32];
	struct hrtimer		timer;
	struct work_struct      work;
	struct regulator 	*vdd;
	struct regulator 	*vcc_i2c;
	int 			irq;
	int 			rst;
	struct i2c_client	*client;	
	spinlock_t		lock;
	bool		suspended;
#ifdef CONFIG_TOUCHSCREEN_MSG2133A_PSENSOR
	struct input_dev	*ps_input;
	struct workqueue_struct 	*ps_wq;
	struct work_struct 	ps_work;
#endif
};

int msg2133_i2c_rxdata(char *rxdata, int length)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr   = this_client->addr,
			.flags  = 0,
			.len    = 1,
			.buf    = rxdata,
		},
		{
			.addr   = this_client->addr,
			.flags  = I2C_M_RD,
			.len    = length,
			.buf    = rxdata,
		},
	};

	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

	return ret;
}
#define __FIRMWARE_UPDATE_MSG2133A__
#ifdef __FIRMWARE_UPDATE_MSG2133A__
#define FW_ADDR_MSG21XX  0x62// (0xC4>>1)
#define FW_ADDR_MSG21XX_TP  0x26// (0x4C>>1)
#define FW_UPDATE_ADDR_MSG21XX  0x4F// (0x92>>1)
#define TP_DEBUG	printk//(x)		//x
#define DBUG	printk//(x) //x
static  char *fw_version;
static u8 temp[94][1024];
 u8  Fmr_Loader[1024];
     u32 crc_tab[256];
static u8 g_dwiic_info_data[1024];

static int FwDataCnt;
struct class *firmware_class;
struct device *firmware_cmd_dev;

void HalTscrCReadI2CSeq(u8 addr, u8* read_data, u16 size)
{
   //according to your platform.
   	int rc;

	struct i2c_msg msgs[] =
    {
		{
			.addr = addr,
			.flags = I2C_M_RD,
			.len = size,
			.buf = read_data,
		},
	};

	rc = i2c_transfer(this_client->adapter, msgs, 1);
	if( rc < 0 )
    {
		printk("HalTscrCReadI2CSeq error %d\n", rc);
	}
}

void HalTscrCDevWriteI2CSeq(u8 addr, u8* data, u16 size)
{
    //according to your platform.
   	int rc;
	struct i2c_msg msgs[] =
    {
		{
			.addr = addr,
			.flags = 0,
			.len = size,
			.buf = data,
		},
	};
	rc = i2c_transfer(this_client->adapter, msgs, 1);
	if( rc < 0 )
    {
		printk("HalTscrCDevWriteI2CSeq error %d,addr = %d\n", rc,addr);
	}
}

/*
static void Get_Chip_Version(void)
{
    printk("[%s]: Enter!\n", __func__);
    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[2];

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0xCE;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, &dbbus_tx_data[0], 3);
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
    if (dbbus_rx_data[1] == 0)
    {
        // it is Catch2
        TP_DEBUG(printk("*** Catch2 ***\n");)
        //FwVersion  = 2;// 2 means Catch2
    }
    else
    {
        // it is catch1
        TP_DEBUG(printk("*** Catch1 ***\n");)
        //FwVersion  = 1;// 1 means Catch1
    }

}
*/

static void dbbusDWIICEnterSerialDebugMode(void)
{
    u8 data[5];

    // Enter the Serial Debug Mode
    data[0] = 0x53;
    data[1] = 0x45;
    data[2] = 0x52;
    data[3] = 0x44;
    data[4] = 0x42;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 5);
}

static void dbbusDWIICStopMCU(void)
{
    u8 data[1];

    // Stop the MCU
    data[0] = 0x37;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICUseBus(void)
{
    u8 data[1];

    // IIC Use Bus
    data[0] = 0x35;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICReshape(void)
{
    u8 data[1];

    // IIC Re-shape
    data[0] = 0x71;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICNotUseBus(void)
{
    u8 data[1];

    // IIC Not Use Bus
    data[0] = 0x34;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}
static void dbbusDWIICNotStopMCU(void)
{
    u8 data[1];

    // Not Stop the MCU
    data[0] = 0x36;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICExitSerialDebugMode(void)
{
    u8 data[1];

    // Exit the Serial Debug Mode
    data[0] = 0x45;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);

    // Delay some interval to guard the next transaction
    udelay ( 150);//200 );        // delay about 0.2ms
}

static void drvISP_EntryIspMode(void)
{
    u8 bWriteData[5] =
    {
        0x4D, 0x53, 0x54, 0x41, 0x52
    };
	printk("\n******%s come in*******\n",__FUNCTION__);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5);
    udelay ( 150 );//200 );        // delay about 0.1ms
}

static u8 drvISP_Read(u8 n, u8* pDataToRead)    //First it needs send 0x11 to notify we want to get flash data back.
{
    u8 Read_cmd = 0x11;
    unsigned char dbbus_rx_data[2] = {0};
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &Read_cmd, 1);
    //msctpc_LoopDelay ( 1 );        // delay about 100us*****
    udelay( 800 );//200);
    if (n == 1)
    {
        HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
        *pDataToRead = dbbus_rx_data[0];
        TP_DEBUG("dbbus=%d,%d===drvISP_Read=====\n",dbbus_rx_data[0],dbbus_rx_data[1]);
  	}
    else
    {
        HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG21XX, pDataToRead, n);
    }

    return 0;
}

static void drvISP_WriteEnable(void)
{
    u8 bWriteData[2] =
    {
        0x10, 0x06
    };
    u8 bWriteData1 = 0x12;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    udelay(150);//1.16
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
}


static void drvISP_ExitIspMode(void)
{
    u8 bWriteData = 0x24;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData, 1);
    udelay( 150 );//200);
}

static u8 drvISP_ReadStatus(void)
{
    u8 bReadData = 0;
    u8 bWriteData[2] =
    {
        0x10, 0x05
    };
    u8 bWriteData1 = 0x12;

    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    //msctpc_LoopDelay ( 1 );        // delay about 100us*****
    udelay(150);//200);
    drvISP_Read(1, &bReadData);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
    return bReadData;
}


static void drvISP_BlockErase(u32 addr)
{
    u8 bWriteData[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
    u8 bWriteData1 = 0x12;
    u32 timeOutCount=0;
	printk("\n******%s come in*******\n",__FUNCTION__);
    drvISP_WriteEnable();

    //Enable write status register
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x50;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

    //Write Status
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x01;
    bWriteData[2] = 0x00;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 3);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

    //Write disable
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x04;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
	//msctpc_LoopDelay ( 1 );        // delay about 100us*****
	udelay(150);//200);
    timeOutCount=0;
	while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
	{
		timeOutCount++;
		if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
	}
    drvISP_WriteEnable();

    bWriteData[0] = 0x10;
    bWriteData[1] = 0xC7;//0xD8;        //Block Erase
    //bWriteData[2] = ((addr >> 16) & 0xFF) ;
    //bWriteData[3] = ((addr >> 8) & 0xFF) ;
    //bWriteData[4] = (addr & 0xFF) ;
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    //HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData, 5);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
		//msctpc_LoopDelay ( 1 );        // delay about 100us*****
	udelay(150);//200);
	timeOutCount=0;
	while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
	{
		timeOutCount++;
		if ( timeOutCount >= 500000 ) break; /* around 5 sec timeout */
	}
}
static void drvISP_Program(u16 k, u8* pDataToWrite)
{
    u16 i = 0;
    u16 j = 0;
    //u16 n = 0;
    u8 TX_data[133];
    u8 bWriteData1 = 0x12;
    u32 addr = k * 1024;
		u32 timeOutCount=0;
    for (j = 0; j < 8; j++)   //128*8 cycle
    {
        TX_data[0] = 0x10;
        TX_data[1] = 0x02;// Page Program CMD
        TX_data[2] = (addr + 128 * j) >> 16;
        TX_data[3] = (addr + 128 * j) >> 8;
        TX_data[4] = (addr + 128 * j);
        for (i = 0; i < 128; i++)
        {
            TX_data[5 + i] = pDataToWrite[j * 128 + i];
        }
        //msctpc_LoopDelay ( 1 );        // delay about 100us*****
        udelay(150);//200);
       
        timeOutCount=0;
		while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
		{
			timeOutCount++;
			if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
		}
  
        drvISP_WriteEnable();
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, TX_data, 133);   //write 133 byte per cycle
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
    }
}

/*reset the chip*/
static void _HalTscrHWReset(void)
{
	gpio_direction_output(RST_GPIO, 1);
	gpio_set_value(RST_GPIO, 1);
	gpio_set_value(RST_GPIO, 0);
	mdelay(10);  /* Note that the RST must be in LOW 10ms at least */
	gpio_set_value(RST_GPIO, 1);
	/* Enable the interrupt service thread/routine for INT after 50ms */
	mdelay(50);
}

static void drvISP_Verify ( u16 k, u8* pDataToVerify )
{
    u16 i = 0, j = 0;
    u8 bWriteData[5] ={ 0x10, 0x03, 0, 0, 0 };
    u8 RX_data[256];
    u8 bWriteData1 = 0x12;
    u32 addr = k * 1024;
    u8 index = 0;
    u32 timeOutCount;
    for ( j = 0; j < 8; j++ ) //128*8 cycle
    {
        bWriteData[2] = ( u8 ) ( ( addr + j * 128 ) >> 16 );
        bWriteData[3] = ( u8 ) ( ( addr + j * 128 ) >> 8 );
        bWriteData[4] = ( u8 ) ( addr + j * 128 );
        udelay ( 100 );        // delay about 100us*****

        timeOutCount = 0;
        while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
        {
            timeOutCount++;
            if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
        }

        HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 5 ); //write read flash addr
        udelay ( 100 );        // delay about 100us*****
        drvISP_Read ( 128, RX_data );
        HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 ); //cmd end
        for ( i = 0; i < 128; i++ ) //log out if verify error
        {
            if ( ( RX_data[i] != 0 ) && index < 10 )
            {
                //TP_DEBUG("j=%d,RX_data[%d]=0x%x\n",j,i,RX_data[i]);
                index++;
            }
            if ( RX_data[i] != pDataToVerify[128 * j + i] )
            {
                TP_DEBUG ( "k=%d,j=%d,i=%d===============Update Firmware Error================", k, j, i );
            }
        }
    }
}

static void drvISP_ChipErase(void)
{
    u8 bWriteData[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
    u8 bWriteData1 = 0x12;
    u32 timeOutCount = 0;
    drvISP_WriteEnable();

    //Enable write status register
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x50;
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 2 );
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 );

    //Write Status
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x01;
    bWriteData[2] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 3 );
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 );

    //Write disable
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x04;
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 2 );
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 );
    udelay ( 100 );        // delay about 100us*****
    timeOutCount = 0;
    while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
    {
        timeOutCount++;
        if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
    }
    drvISP_WriteEnable();

    bWriteData[0] = 0x10;
    bWriteData[1] = 0xC7;

    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 2 );
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 );
    udelay ( 100 );        // delay about 100us*****
    timeOutCount = 0;
    while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
    {
        timeOutCount++;
        if ( timeOutCount >= 500000 ) break; /* around 5 sec timeout */
    }
}

/* update the firmware part, used by apk*/
/*show the fw version*/
static ssize_t firmware_update_c2 ( struct device *dev,
                                    struct device_attribute *attr, const char *buf, size_t size )
{
    u8 i;
    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};

    // set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set MCU clock,SPI clock =FRO
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x22;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x23;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable slave's ISP ECO mode
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x08;
    dbbus_tx_data[2] = 0x0c;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable SPI Pad
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = ( dbbus_rx_data[0] | 0x20 ); //Set Bit 5
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // WP overwrite
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x0E;
    dbbus_tx_data[3] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set pin high
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x10;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbusDWIICIICNotUseBus();
    dbbusDWIICNotStopMCU();
    dbbusDWIICExitSerialDebugMode();

    drvISP_EntryIspMode();
    drvISP_ChipErase();
    _HalTscrHWReset();
    mdelay ( 300 );

    // Program and Verify
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    //Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set MCU clock,SPI clock =FRO
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x22;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x23;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable slave's ISP ECO mode
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x08;
    dbbus_tx_data[2] = 0x0c;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable SPI Pad
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = ( dbbus_rx_data[0] | 0x20 ); //Set Bit 5
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // WP overwrite
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x0E;
    dbbus_tx_data[3] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set pin high
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x10;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbusDWIICIICNotUseBus();
    dbbusDWIICNotStopMCU();
    dbbusDWIICExitSerialDebugMode();

    ///////////////////////////////////////
    // Start to load firmware
    ///////////////////////////////////////
    drvISP_EntryIspMode();

    for ( i = 0; i < 94; i++ ) // total  94 KB : 1 byte per R/W
    {
        drvISP_Program ( i, temp[i] ); // program to slave's flash
        drvISP_Verify ( i, temp[i] ); //verify data
    }
    TP_DEBUG ( "update OK\n" );
    drvISP_ExitIspMode();
    FwDataCnt = 0;
    return size;
}

static u32 Reflect ( u32 ref, char ch ) //unsigned int Reflect(unsigned int ref, char ch)
{
    u32 value = 0;
    u32 i = 0;

    for ( i = 1; i < ( ch + 1 ); i++ )
    {
        if ( ref & 1 )
        {
            value |= 1 << ( ch - i );
        }
        ref >>= 1;
    }
    return value;
}

u32 Get_CRC ( u32 text, u32 prevCRC, u32 *crc32_table )
{
    u32  ulCRC = prevCRC;
	ulCRC = ( ulCRC >> 8 ) ^ crc32_table[ ( ulCRC & 0xFF ) ^ text];
    return ulCRC ;
}
static void Init_CRC32_Table ( u32 *crc32_table )
{
    u32 magicnumber = 0x04c11db7;
    u32 i = 0, j;

    for ( i = 0; i <= 0xFF; i++ )
    {
        crc32_table[i] = Reflect ( i, 8 ) << 24;
        for ( j = 0; j < 8; j++ )
        {
            crc32_table[i] = ( crc32_table[i] << 1 ) ^ ( crc32_table[i] & ( 0x80000000L ) ? magicnumber : 0 );
        }
        crc32_table[i] = Reflect ( crc32_table[i], 32 );
    }
}

typedef enum
{
	EMEM_ALL = 0,
	EMEM_MAIN,
	EMEM_INFO,
} EMEM_TYPE_t;

static void drvDB_WriteReg8Bit ( u8 bank, u8 addr, u8 data )
{
    u8 tx_data[4] = {0x10, bank, addr, data};
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, tx_data, 4 );
}

static void drvDB_WriteReg ( u8 bank, u8 addr, u16 data )
{
    u8 tx_data[5] = {0x10, bank, addr, data & 0xFF, data >> 8};
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, tx_data, 5 );
}

static unsigned short drvDB_ReadReg ( u8 bank, u8 addr )
{
    u8 tx_data[3] = {0x10, bank, addr};
    u8 rx_data[2] = {0};

    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &rx_data[0], 2 );
    return ( rx_data[1] << 8 | rx_data[0] );
}

static int drvTP_erase_emem_c32 ( void )
{
    /////////////////////////
    //Erase  all
    /////////////////////////
    
    //enter gpio mode
    drvDB_WriteReg ( 0x16, 0x1E, 0xBEAF );

    // before gpio mode, set the control pin as the orginal status
    drvDB_WriteReg ( 0x16, 0x08, 0x0000 );
    drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x10 );
    mdelay ( 10 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );

    // ptrim = 1, h'04[2]
    drvDB_WriteReg8Bit ( 0x16, 0x08, 0x04 );
    drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x10 );
    mdelay ( 10 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );

    // ptm = 6, h'04[12:14] = b'110
    drvDB_WriteReg8Bit ( 0x16, 0x09, 0x60 );
    drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x10 );

    // pmasi = 1, h'04[6]
    drvDB_WriteReg8Bit ( 0x16, 0x08, 0x44 );
    // pce = 1, h'04[11]
    drvDB_WriteReg8Bit ( 0x16, 0x09, 0x68 );
    // perase = 1, h'04[7]
    drvDB_WriteReg8Bit ( 0x16, 0x08, 0xC4 );
    // pnvstr = 1, h'04[5]
    drvDB_WriteReg8Bit ( 0x16, 0x08, 0xE4 );
    // pwe = 1, h'04[9]
    drvDB_WriteReg8Bit ( 0x16, 0x09, 0x6A );
    // trigger gpio load
    drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x10 );

    return ( 1 );
}

static ssize_t firmware_update_c32 ( struct device *dev, struct device_attribute *attr,
                                     const char *buf, size_t size,  EMEM_TYPE_t emem_type )
{
  // struct msg2133_ts_data *msg2133_ts = dev_get_drvdata(dev);
//    u8  dbbus_tx_data[4];
//    u8  dbbus_rx_data[2] = {0};
      // Buffer for slave's firmware


    u32 i, j;
    u32 crc_main, crc_main_tp;
    u32 crc_info, crc_info_tp;
    u16 reg_data = 0;

    crc_main = 0xffffffff;
    crc_info = 0xffffffff;

	printk("%s+++++++ start+++\n",__func__);
#if 1
    /////////////////////////
    // Erase  all
    /////////////////////////
    drvTP_erase_emem_c32();
    mdelay ( 1000 ); //MCR_CLBK_DEBUG_DELAY ( 1000, MCU_LOOP_DELAY_COUNT_MS );

    //ResetSlave();
    _HalTscrHWReset();
    //drvDB_EnterDBBUS();
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    // Reset Watchdog
    drvDB_WriteReg8Bit ( 0x3C, 0x60, 0x55 );
    drvDB_WriteReg8Bit ( 0x3C, 0x61, 0xAA );

    /////////////////////////
    // Program
    /////////////////////////

    //polling 0x3CE4 is 0x1C70
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x1C70 );


    drvDB_WriteReg ( 0x3C, 0xE4, 0xE38F );  // for all-blocks

    //polling 0x3CE4 is 0x2F43
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x2F43 );


    //calculate CRC 32
    Init_CRC32_Table ( &crc_tab[0] );

	printk("%s+++++++for start+++\n",__func__);
    for ( i = 0; i < 33; i++ ) // total  33 KB : 2 byte per R/W
    {
        if ( i < 32 )   //emem_main
        {
            if ( i == 31 )
            {
                temp[i][1014] = 0x5A; //Fmr_Loader[1014]=0x5A;
                temp[i][1015] = 0xA5; //Fmr_Loader[1015]=0xA5;

                for ( j = 0; j < 1016; j++ )
                {
                    //crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
                    crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
                }
            }
            else
            {
                for ( j = 0; j < 1024; j++ )
                {
                    //crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
                    crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
                }
            }
        }
        else  // emem_info
        {
            for ( j = 0; j < 1024; j++ )
            {
                //crc_info=Get_CRC(Fmr_Loader[j],crc_info,&crc_tab[0]);
                crc_info = Get_CRC ( temp[i][j], crc_info, &crc_tab[0] );
            }
        }

        //drvDWIIC_MasterTransmit( DWIIC_MODE_DWIIC_ID, 1024, Fmr_Loader );
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP, temp[i], 1024 );

        // polling 0x3CE4 is 0xD0BC
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }
        while ( reg_data != 0xD0BC );

        drvDB_WriteReg ( 0x3C, 0xE4, 0x2F43 );
    }

	printk("%s+++++++for end+++\n",__func__);
    //write file done
    drvDB_WriteReg ( 0x3C, 0xE4, 0x1380 );

    mdelay ( 10 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );
    // polling 0x3CE4 is 0x9432
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x9432 );

    crc_main = crc_main ^ 0xffffffff;
    crc_info = crc_info ^ 0xffffffff;

    // CRC Main from TP
    crc_main_tp = drvDB_ReadReg ( 0x3C, 0x80 );
    crc_main_tp = ( crc_main_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0x82 );
 
    //CRC Info from TP
    crc_info_tp = drvDB_ReadReg ( 0x3C, 0xA0 );
    crc_info_tp = ( crc_info_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0xA2 );

    TP_DEBUG ( "crc_main=0x%x, crc_info=0x%x, crc_main_tp=0x%x, crc_info_tp=0x%x\n",
               crc_main, crc_info, crc_main_tp, crc_info_tp );

    //drvDB_ExitDBBUS();
    if ( ( crc_main_tp != crc_main ) || ( crc_info_tp != crc_info ) )
    {
        printk ( "update FAILED\n" );
		_HalTscrHWReset();
        FwDataCnt = 0;
//	enable_irq(msg2133_ts->pdata->irq);
        return ( 0 );
    }

    printk ( "update OK\n" );
	_HalTscrHWReset();
    FwDataCnt = 0;
//	enable_irq(msg2133_ts->pdata->irq);

    return size;
#endif
}

static int drvTP_erase_emem_c33 ( EMEM_TYPE_t emem_type )
{
    // stop mcu
    drvDB_WriteReg ( 0x0F, 0xE6, 0x0001 );

    //disable watch dog
    drvDB_WriteReg8Bit ( 0x3C, 0x60, 0x55 );
    drvDB_WriteReg8Bit ( 0x3C, 0x61, 0xAA );

    // set PROGRAM password
    drvDB_WriteReg8Bit ( 0x16, 0x1A, 0xBA );
    drvDB_WriteReg8Bit ( 0x16, 0x1B, 0xAB );

    //proto.MstarWriteReg(F1.loopDevice, 0x1618, 0x80);
    drvDB_WriteReg8Bit ( 0x16, 0x18, 0x80 );

    if ( emem_type == EMEM_ALL )
    {
        drvDB_WriteReg8Bit ( 0x16, 0x08, 0x10 ); //mark
    }

    drvDB_WriteReg8Bit ( 0x16, 0x18, 0x40 );
    mdelay ( 10 );

    drvDB_WriteReg8Bit ( 0x16, 0x18, 0x80 );

    // erase trigger
    if ( emem_type == EMEM_MAIN )
    {
        drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x04 ); //erase main
    }
    else
    {
        drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x08 ); //erase all block
    }

    return ( 1 );
}

static int drvTP_read_info_dwiic_c33 ( void )
{
    u8  dwiic_tx_data[5];
    u16 reg_data=0;
    mdelay ( 300 );

    // Stop Watchdog
    drvDB_WriteReg8Bit ( 0x3C, 0x60, 0x55 );
    drvDB_WriteReg8Bit ( 0x3C, 0x61, 0xAA );

    drvDB_WriteReg ( 0x3C, 0xE4, 0xA4AB );

	drvDB_WriteReg ( 0x1E, 0x04, 0x7d60 );

    // TP SW reset
    drvDB_WriteReg ( 0x1E, 0x04, 0x829F );
    mdelay ( 1 );
    dwiic_tx_data[0] = 0x10;
    dwiic_tx_data[1] = 0x0F;
    dwiic_tx_data[2] = 0xE6;
    dwiic_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dwiic_tx_data, 4 );	
    mdelay ( 100 );

    do{
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x5B58 );

    dwiic_tx_data[0] = 0x72;
    dwiic_tx_data[1] = 0x80;
    dwiic_tx_data[2] = 0x00;
    dwiic_tx_data[3] = 0x04;
    dwiic_tx_data[4] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP , dwiic_tx_data, 5 );

    mdelay ( 50 );

    // recive info data
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX_TP, &g_dwiic_info_data[0], 1024 );

    return ( 1 );
}

static ssize_t firmware_update_c33 ( struct device *dev, struct device_attribute *attr,
                                     const char *buf, size_t size, EMEM_TYPE_t emem_type )
{
//   struct msg2133_ts_data *msg2133_ts = dev_get_drvdata(dev);
  //  u8  dbbus_tx_data[4];
  //  u8  dbbus_rx_data[2] = {0};
    u8  life_counter[2];
    u32 i, j;
    u32 crc_main, crc_main_tp;
    u32 crc_info, crc_info_tp;
  
    int update_pass = 1;
    u16 reg_data = 0;

    crc_main = 0xffffffff;
    crc_info = 0xffffffff;

    drvTP_read_info_dwiic_c33();
	
	printk("%s+++++++\n",__func__);
    if ( g_dwiic_info_data[0] == 'M' && g_dwiic_info_data[1] == 'S' && g_dwiic_info_data[2] == 'T' && g_dwiic_info_data[3] == 'A' && g_dwiic_info_data[4] == 'R' && g_dwiic_info_data[5] == 'T' && g_dwiic_info_data[6] == 'P' && g_dwiic_info_data[7] == 'C' )
    {
        // updata FW Version
        //drvTP_info_updata_C33 ( 8, &temp[32][8], 5 );

		g_dwiic_info_data[8]=temp[32][8];
		g_dwiic_info_data[9]=temp[32][9];
		g_dwiic_info_data[10]=temp[32][10];
		g_dwiic_info_data[11]=temp[32][11];
        // updata life counter
        life_counter[1] = (( ( (g_dwiic_info_data[13] << 8 ) | g_dwiic_info_data[12]) + 1 ) >> 8 ) & 0xFF;
        life_counter[0] = ( ( (g_dwiic_info_data[13] << 8 ) | g_dwiic_info_data[12]) + 1 ) & 0xFF;
		g_dwiic_info_data[12]=life_counter[0];
		g_dwiic_info_data[13]=life_counter[1];
        //drvTP_info_updata_C33 ( 10, &life_counter[0], 3 );
        drvDB_WriteReg ( 0x3C, 0xE4, 0x78C5 );
		drvDB_WriteReg ( 0x1E, 0x04, 0x7d60 );
        // TP SW reset
        drvDB_WriteReg ( 0x1E, 0x04, 0x829F );

        mdelay ( 50 );

	printk("%s+++++11111++\n",__func__);
        //polling 0x3CE4 is 0x2F43
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );

        }
        while ( reg_data != 0x2F43 );

        // transmit lk info data
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP , &g_dwiic_info_data[0], 1024 );

        //polling 0x3CE4 is 0xD0BC
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }
        while ( reg_data != 0xD0BC );

    }

    //erase main
    drvTP_erase_emem_c33 ( EMEM_MAIN );
    mdelay ( 1000 );

    //ResetSlave();
    _HalTscrHWReset();

    //drvDB_EnterDBBUS();
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    /////////////////////////
    // Program
    /////////////////////////

    //polling 0x3CE4 is 0x1C70
    if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }
        while ( reg_data != 0x1C70 );
    }

	printk("%s++i======emem_type = %d\n",__func__,emem_type);
    switch ( emem_type )
    {
        case EMEM_ALL:
            drvDB_WriteReg ( 0x3C, 0xE4, 0xE38F );  // for all-blocks
            break;
        case EMEM_MAIN:
            drvDB_WriteReg ( 0x3C, 0xE4, 0x7731 );  // for main block
            break;
        case EMEM_INFO:
            drvDB_WriteReg ( 0x3C, 0xE4, 0x7731 );  // for info block

            drvDB_WriteReg8Bit ( 0x0F, 0xE6, 0x01 );

            drvDB_WriteReg8Bit ( 0x3C, 0xE4, 0xC5 ); //
            drvDB_WriteReg8Bit ( 0x3C, 0xE5, 0x78 ); //

            drvDB_WriteReg8Bit ( 0x1E, 0x04, 0x9F );
            drvDB_WriteReg8Bit ( 0x1E, 0x05, 0x82 );

            drvDB_WriteReg8Bit ( 0x0F, 0xE6, 0x00 );
            mdelay ( 100 );
            break;
    }

    // polling 0x3CE4 is 0x2F43
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x2F43 );

    // calculate CRC 32
    Init_CRC32_Table ( &crc_tab[0] );

	printk("%s+++++++for start+++\n",__func__);
	for ( i = 0; i < 33; i++ ) // total  33 KB : 2 byte per R/W
	{
		if ( emem_type == EMEM_INFO )
			i = 32;

		if ( i < 32 )   //emem_main
		{
			if ( i == 31 )
			{
				temp[i][1014] = 0x5A; //Fmr_Loader[1014]=0x5A;
				temp[i][1015] = 0xA5; //Fmr_Loader[1015]=0xA5;

				for ( j = 0; j < 1016; j++ )
				{
					//crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
					crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
				}
			}
			else
			{
				for ( j = 0; j < 1024; j++ )
				{
					//crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
					crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
				}
			}
		}
		else  //emem_info
		{
			for ( j = 0; j < 1024; j++ )
			{
				//crc_info=Get_CRC(Fmr_Loader[j],crc_info,&crc_tab[0]);
				crc_info = Get_CRC ( g_dwiic_info_data[j], crc_info, &crc_tab[0] );
			}
			if ( emem_type == EMEM_MAIN ) break;
		}

		//drvDWIIC_MasterTransmit( DWIIC_MODE_DWIIC_ID, 1024, Fmr_Loader );
		HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP, temp[i], 1024 );

		// polling 0x3CE4 is 0xD0BC
		do
		{
			reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
		}
		while ( reg_data != 0xD0BC );

		drvDB_WriteReg ( 0x3C, 0xE4, 0x2F43 );
	}

	printk("%s+++++++for end+++\n",__func__);

	if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
	{
		// write file done and check crc
		drvDB_WriteReg ( 0x3C, 0xE4, 0x1380 );
	}
	mdelay ( 10 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );

	if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
	{
		// polling 0x3CE4 is 0x9432
		do
		{
			reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
		}while ( reg_data != 0x9432 );
	}

	crc_main = crc_main ^ 0xffffffff;
	crc_info = crc_info ^ 0xffffffff;

	if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
	{
		// CRC Main from TP
		crc_main_tp = drvDB_ReadReg ( 0x3C, 0x80 );
		crc_main_tp = ( crc_main_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0x82 );

		// CRC Info from TP
		crc_info_tp = drvDB_ReadReg ( 0x3C, 0xA0 );
		crc_info_tp = ( crc_info_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0xA2 );
	}
	TP_DEBUG ( "crc_main=0x%x, crc_info=0x%x, crc_main_tp=0x%x, crc_info_tp=0x%x\n",
               crc_main, crc_info, crc_main_tp, crc_info_tp );

    //drvDB_ExitDBBUS();

    update_pass = 1;
	if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        if ( crc_main_tp != crc_main )
            update_pass = 0;

        if ( crc_info_tp != crc_info )
            update_pass = 0;
    }

    if ( !update_pass )
    {
        printk ( "update FAILED\n" );
		_HalTscrHWReset();
        FwDataCnt = 0;
//	enable_irq(msg2133_ts->pdata->irq);
        return ( 0 );
    }

    printk ( "update OK\n" );
	_HalTscrHWReset();
    FwDataCnt = 0;
//	enable_irq(msg2133_ts->pdata->irq);
	printk("%s+++++++c33 end+++\n",__func__);
    return size;
}

static ssize_t firmware_update_show ( struct device *dev,struct device_attribute *attr, char *buf )
{
    return sprintf ( buf, "%s\n", fw_version );
}

static ssize_t firmware_update_store ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t size )
{
   // u8 i;
//	struct msg2133_ts_data *msg2133_ts = dev_get_drvdata(dev);
    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};
//    disable_irq(msg2133_ts->pdata->irq);
//	disable_irq(msg2133_ts->pdata->irq);

    _HalTscrHWReset();

	printk("%s++++++\n",__func__);
    // Erase TP Flash first
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    // Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    /////////////////////////
    // Difference between C2 and C3
    /////////////////////////
	// c2:2133 c32:2133a(2) c33:2138
    //check id
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0xCC;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    if ( dbbus_rx_data[0] == 2 )
    {
        // check version
        dbbus_tx_data[0] = 0x10;
        dbbus_tx_data[1] = 0x3C;
        dbbus_tx_data[2] = 0xEA;
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
        HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
        TP_DEBUG ( "dbbus_rx version[0]=0x%x", dbbus_rx_data[0] );
        printk ( "dbbus_rx version[0]=0x%x", dbbus_rx_data[0] );

        if ( dbbus_rx_data[0] == 3 ){
        printk ( "===========c33=====\n" );
           // return firmware_update_c33 ( dev, attr, buf, size, EMEM_MAIN );
            return firmware_update_c32 ( dev, attr, buf, size, EMEM_ALL );
		}
        else{
        printk ( "--c32--------\n" );

            return firmware_update_c33 ( dev, attr, buf, size, EMEM_MAIN );
           // return firmware_update_c32 ( dev, attr, buf, size, EMEM_ALL );
        }
    }
    else
    {
        printk ( "+++++c2+++++\n" );
        return firmware_update_c2 ( dev, attr, buf, size );
    } 
}

static DEVICE_ATTR(update, 0777, firmware_update_show, firmware_update_store);
/***********************************************************************************************
Name	:	msg2133_i2c_rxdata

Input	:	*rxdata
*length

Output	:	ret

function	:

***********************************************************************************************/
/*static int msg2133_i2c_rxdata(char *rxdata, int length)
{

	int ret;
	struct i2c_msg msg;
	msg.addr = this_client->addr;
	msg.flags = I2C_M_RD;
	msg.len = length;
	msg.buf = rxdata;
	ret = i2c_transfer(this_client->adapter, &msg, 1);
	if (ret < 0)
		pr_err("msg %s i2c Read error: %d, addr:0x%x\n", __func__, ret,this_client->addr);
	return ret;
}*/
int HalTscrCReadI2CSeq_1(u8 addr, u8* read_data, u8 size)
{
	//according to your platform.
	int rc;
	struct i2c_msg msg;
	msg.addr = addr;
	msg.flags = I2C_M_RD;
	msg.len = size;
	msg.buf = read_data;
	rc = i2c_transfer(this_client->adapter, &msg, 1);
	if (rc < 0)
		pr_err("msg %s i2c Read error: %d\n", __func__, rc);

	return rc;
}

int HalTscrCDevWriteI2CSeq_1(u8 addr, u8* data, u16 size)
{
	//according to your platform.
	int rc;
	struct i2c_msg msg;
	msg.addr = addr;
	msg.flags = 0;
	msg.len = size;
	msg.buf = data;
	rc = i2c_transfer(this_client->adapter, &msg, 1);
	if (rc < 0)
		pr_err("msg %s i2c Write error: %d\n", __func__, rc);

	return rc;
}


static ssize_t firmware_version_show(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
    TP_DEBUG("*** firmware_version_show fw_version = %s***\n", fw_version);
    return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_version_store(struct device *dev,
                                      struct device_attribute *attr, const char *buf, size_t size)
{
    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[4] ;
    unsigned short major=0, minor=0;
/*
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();

*/
    fw_version = kzalloc(sizeof(char), GFP_KERNEL);

	printk("%s+++++++++\n",__func__);
    //Get_Chip_Version();
    dbbus_tx_data[0] = 0x53;
    dbbus_tx_data[1] = 0x00;
    dbbus_tx_data[2] = 0x2A;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_tx_data[0], 3);
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_rx_data[0], 4);

    major = (dbbus_rx_data[1]<<8)+dbbus_rx_data[0];
    minor = (dbbus_rx_data[3]<<8)+dbbus_rx_data[2];

    TP_DEBUG("***major = %d ***\n", major);
    printk("***+++++++++major = %d ***\n", major);
    TP_DEBUG("***minor = %d ***\n", minor);
    printk("***+++++++++++minor = %d ***\n", minor);
    sprintf(fw_version,"%03d%03d", major, minor);
    printk("***fw_version = %s ***\n", fw_version);

    return size;
}
static DEVICE_ATTR(version, 0777, firmware_version_show, firmware_version_store);

static ssize_t firmware_data_show(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
    return FwDataCnt;
}

static ssize_t firmware_data_store(struct device *dev,
                                   struct device_attribute *attr, const char *buf, size_t size)
{
    int i;
	TP_DEBUG("***FwDataCnt = %d ***\n", FwDataCnt);
    for (i = 0; i < 1024; i++)
    {
        memcpy(temp[FwDataCnt], buf, 1024);
    }
    FwDataCnt++;
    return size;
}
static DEVICE_ATTR(data, 0777, firmware_data_show, firmware_data_store);
static ssize_t firmware_clear_show(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
	u16 k=0,i = 0, j = 0;
	u8 bWriteData[5] =
	{
        0x10, 0x03, 0, 0, 0
	};
	u8 RX_data[256];
	u8 bWriteData1 = 0x12;
	u32 addr = 0;
	u32 timeOutCount=0;
	printk(" +++++++ [%s] Enter!++++++\n", __func__);
	for (k = 0; k < 94; i++)   // total  94 KB : 1 byte per R/W
	{
		addr = k * 1024;
		for (j = 0; j < 8; j++)   //128*8 cycle
		{
			bWriteData[2] = (u8)((addr + j * 128) >> 16);
			bWriteData[3] = (u8)((addr + j * 128) >> 8);
			bWriteData[4] = (u8)(addr + j * 128);
			//msctpc_LoopDelay ( 1 );        // delay about 100us*****
			udelay(150);//200);

			timeOutCount=0;
			while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
			{
				timeOutCount++;
				if ( timeOutCount >= 100000 ) 
					break; /* around 1 sec timeout */
	  		}
        
			HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5);    //write read flash addr
			//msctpc_LoopDelay ( 1 );        // delay about 100us*****
			udelay(150);//200);
			drvISP_Read(128, RX_data);
			HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);    //cmd end
			for (i = 0; i < 128; i++)   //log out if verify error
			{
				if (RX_data[i] != 0xFF)
				{
					//TP_DEBUG(printk("k=%d,j=%d,i=%d===============erase not clean================",k,j,i);)
					printk("k=%d,j=%d,i=%d  erase not clean !!",k,j,i);
				}
			}
		}
	}
	TP_DEBUG("read finish\n");
	return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_clear_store(struct device *dev,
                                     struct device_attribute *attr, const char *buf, size_t size)
{

	u8 dbbus_tx_data[4];
	unsigned char dbbus_rx_data[2] = {0};
	printk(" +++++++ [%s] Enter!++++++\n", __func__);
	//msctpc_LoopDelay ( 100 ); 	   // delay about 100ms*****

	// Enable slave's ISP ECO mode

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x08;
	dbbus_tx_data[2] = 0x0c;
	dbbus_tx_data[3] = 0x08;
	
	// Disable the Watchdog
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x11;
	dbbus_tx_data[2] = 0xE2;
	dbbus_tx_data[3] = 0x00;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x3C;
	dbbus_tx_data[2] = 0x60;
	dbbus_tx_data[3] = 0x55;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x3C;
	dbbus_tx_data[2] = 0x61;
	dbbus_tx_data[3] = 0xAA;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	//Stop MCU
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x0F;
	dbbus_tx_data[2] = 0xE6;
	dbbus_tx_data[3] = 0x01;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	//Enable SPI Pad
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x02;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	printk("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
	printk("++++++++++dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
	dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20);  //Set Bit 5
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x25;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);

	dbbus_rx_data[0] = 0;
	dbbus_rx_data[1] = 0;
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	printk("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
	printk("+++++++++++dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
	dbbus_tx_data[3] = dbbus_rx_data[0] & 0xFC;  //Clear Bit 1,0
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	//WP overwrite
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x0E;
	dbbus_tx_data[3] = 0x02;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	//set pin high
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x10;
	dbbus_tx_data[3] = 0x08;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
	//set FRO to 50M
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x11;
	dbbus_tx_data[2] = 0xE2;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
	dbbus_rx_data[0] = 0;
	dbbus_rx_data[1] = 0;
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	printk("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
	printk("++++++++++dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);
	dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 1,0
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbusDWIICIICNotUseBus();
	dbbusDWIICNotStopMCU();
	dbbusDWIICExitSerialDebugMode();

    ///////////////////////////////////////
    // Start to load firmware
    ///////////////////////////////////////
    drvISP_EntryIspMode();
	printk("chip erase+\n");
	printk("+++++++chip erase+\n");
    drvISP_BlockErase(0x00000);
	printk("chip erase-\n");
	printk("+++++++chip erase---\n");
    drvISP_ExitIspMode();
    return size;
}
static DEVICE_ATTR(clear, 0777, firmware_clear_show, firmware_clear_store);

#endif

#ifdef CONFIG_MSG2133A_ENABLE_AUTO_UPDATA
static const unsigned char MSG21XX_OLD_ITO_update_bin[]=
{
	#include "MSG2133A_update_0x000a_20140307.i"
};

static const unsigned char MSG21XX_NEW_ITO_update_bin[]=
{
	#include "MSG2133A_update_V0x0002.0x0003_20140305.i"
};
static unsigned short curr_ic_major=0;
static unsigned short curr_ic_minor=0;

static void getFWPrivateVersion(void)
{
	u8 tx_data[3] = {0};
	u8 rx_data[4] = {0};
	u16 Major = 0, Minor = 0;
	int rc_w = 0, rc_r = 0;

	//printk("_msg_GetVersion\n");

	tx_data[0] = 0x53;
	tx_data[1] = 0x00;
	tx_data[2] = 0x2A;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &tx_data[0], 3);
	mdelay(50);
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX_TP, &rx_data[0], 4);
	mdelay(50);

	Major = (rx_data[1]<<8) + rx_data[0];
	Minor = (rx_data[3]<<8) + rx_data[2];

	if(rc_w<0||rc_r<0)
	{
		curr_ic_major = 0xffff;
		curr_ic_minor = 0xffff;
	}
	else
	{
		curr_ic_major = Major;
		curr_ic_minor = Minor;
	}
	//printk("***major = %d ***\n", curr_ic_major);
	//printk("***minor = %d ***\n", curr_ic_minor);
}

static void msg2133a_auto_update(void)
{
	u32 update_old_bin_major = 0;
	u32 update_old_bin_minor = 0;
	u32 update_new_bin_major = 0;
	u32 update_new_bin_minor = 0;

	//printk("[TP] check auto updata\n");

	getFWPrivateVersion();
	update_old_bin_major = MSG21XX_OLD_ITO_update_bin[0x7f4f]<<8|MSG21XX_OLD_ITO_update_bin[0x7f4e];
	update_old_bin_minor = MSG21XX_OLD_ITO_update_bin[0x7f51]<<8|MSG21XX_OLD_ITO_update_bin[0x7f50];
	update_new_bin_major = MSG21XX_NEW_ITO_update_bin[0x7f4f]<<8|MSG21XX_NEW_ITO_update_bin[0x7f4e];
	update_new_bin_minor = MSG21XX_NEW_ITO_update_bin[0x7f51]<<8|MSG21XX_NEW_ITO_update_bin[0x7f50];
	//printk("bin_major = %d \n",update_old_bin_major);
	//printk("bin_minor = %d \n",update_old_bin_minor);
	//printk("bin_major = %d \n",update_new_bin_major);
	//printk("bin_minor = %d \n",update_new_bin_minor);

	if(curr_ic_major == 790)
	{
		if(update_old_bin_minor> curr_ic_minor)
		{
			u32 i,j;
			for (i = 0; i < 33; i++)
			{
				for (j = 0; j < 1024; j++)
				{
					temp[i][j] = MSG21XX_OLD_ITO_update_bin[i*1024+j];
				}
			}
			firmware_update_store(NULL, NULL, NULL, 0);
		}
	}
	else if(curr_ic_major == 2)
	{
	 	if(update_new_bin_minor> curr_ic_minor)
		{
			u32 i,j;
			for (i = 0; i < 33; i++)
			{
				for (j = 0; j < 1024; j++)
				{
					temp[i][j] = MSG21XX_NEW_ITO_update_bin[i*1024+j];
				}
			}
			firmware_update_store(NULL, NULL, NULL, 0);
		}
	}
}
#endif


#ifdef CONFIG_TOUCHSCREEN_MSG2133A_PSENSOR
struct Data_CNT *msg2133a_data = NULL;
static struct wake_lock ps_wake_lock;
char ps_data_state[1] = {1};
int proximity_enable = 0;
enum
{
       DISABLE_CTP_PS = 0,
       ENABLE_CTP_PS = 1,
       RESET_CTP_PS
};

static ssize_t show_proximity_sensor_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("%s : proximity_sensor_status = %d\n",__func__,ps_data_state[0]);
	return sprintf(buf, "0x%02x   \n",ps_data_state[0]);
}

static ssize_t show_proximity_sensor_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
	//unsigned char temp;
	printk("%s : proximity_enable = %d\n",__func__,proximity_enable);
	return sprintf(buf, "0x%02x   \n",proximity_enable);
}
static ssize_t store_proximity_sensor_enable(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int val;
	u8 ps_store_data[4];
	if(buf != NULL && size != 0)
	{
		sscanf(buf,"%d",&val);
		printk("%s : val =%d\n",__func__,val);
		_HalTscrHWReset();
		if(DISABLE_CTP_PS == val)
		{
			proximity_enable = 0;
			printk("DISABLE_CTP_PS buf=%d,size=%d,val=%d\n", *buf, size,val);
			ps_store_data[0] = 0x52;
			ps_store_data[1] = 0x00;
			ps_store_data[2] = 0x4a;
			ps_store_data[3] = 0xa1;
			HalTscrCDevWriteI2CSeq(0x26, ps_store_data,4);
			msleep(200);
			printk("RESET_CTP_PS buf=%d\n", *buf);
			wake_unlock(&ps_wake_lock);
		}

		else if(ENABLE_CTP_PS == val)
		{
			wake_lock(&ps_wake_lock);
			proximity_enable = 1;
			printk("ENABLE_CTP_PS buf=%d,size=%d,val=%d\n", *buf, size,val);
			ps_store_data[0] = 0x52;
			ps_store_data[1] = 0x00;
			ps_store_data[2] = 0x4a;
			ps_store_data[3] = 0xa0;
			HalTscrCDevWriteI2CSeq(0x26, ps_store_data,4);
			if(msg2133a_data)
			{
				input_report_abs(msg2133a_data->ps_input, ABS_DISTANCE, ps_data_state[0]);
			}
		}
	}

	return size;
}
static DEVICE_ATTR(enable, 0777, show_proximity_sensor_enable, store_proximity_sensor_enable);
static DEVICE_ATTR(status, 0777, show_proximity_sensor_status,NULL);
static struct attribute *msg2133_proximity_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_status.attr,
	NULL
};

static struct attribute_group msg2133_proximity_attribute_group = {
	.attrs = msg2133_proximity_attributes
};

#endif




unsigned char tpd_check_sum(unsigned char *pval)
{
	int i, sum = 0;

	for(i = 0; i < 7; i++)
	{
		sum += pval[i];
	}

	return (unsigned char)((-sum) & 0xFF);
}

/*
 * The function_st_finger_infoss for inserting/removing us as a module.
 */
static  int key_id = 0,kave_VK = 0;
static u8 Buf[8]={0};
struct finger_infos
{
	int i2_x;
	int i2_y;
};
static struct finger_infos _st_finger_infos[2];
static int tsp_keycodes_q100[4]={KEY_MENU,KEY_HOMEPAGE,KEY_BACK,KEY_SEARCH};
static int Touch_State = 0;


int  msg2133_read_data(void)
{
	u16 temp_x0 = 0, temp_y0 = 0, temp_x1 = 0,temp_y1 = 0;
	s16  temp_x1_dst = 0, temp_y1_dst = 0;
	int temp;
	int ret=-1;
	Touch_State=0;	
	ret = msg2133_i2c_rxdata(Buf,8);
	if(ret<0){
		printk("msg2133 get TP info error\n");
	}

	temp_x0 = ((Buf[1] & 0xF0) << 4) | Buf[2];
	temp_y0= ((Buf[1] & 0x0F) << 8) | Buf[3];
	temp_x1_dst  = ((Buf[4] & 0xF0) << 4) | Buf[5];
	temp_y1_dst = ((Buf[4] & 0x0F) << 8) | Buf[6];

	if((temp_x1_dst) & 0x0800)
	{
		temp_x1_dst |= 0xF000;
	}

	if((temp_y1_dst) & 0x0800)
	{
		temp_y1_dst |= 0xF000;
	}

	//printk("x0 %d, y0 %d, dsx1 %d,dsy1 %d\n",temp_x0,temp_y0,temp_x1_dst,temp_y1_dst);
	temp_x1 =  temp_x0 + temp_x1_dst;
	temp_y1 =  temp_y0 + temp_y1_dst;

	temp = tpd_check_sum(Buf);
	if(temp==Buf[7]){
		if(Buf[0] == 0x52) //CTP  ID
		{
			if(Buf[1] == 0xFF&& Buf[4] == 0xFF)
			{
				Touch_State =  Touch_State_VK;
				if(Buf[5] == 0x01)
				{
					key_id = 0x01;
				}
				else if(Buf[5] == 0x02)
				{
					key_id = 0x02;
				}
				else if(Buf[5] == 0x04)
				{
					key_id = 0x04;
				}
				else if(Buf[5] == 0x08)
				{
					key_id = 0x08;
				}

#ifdef CONFIG_TOUCHSCREEN_MSG2133A_PSENSOR
				else if(Buf[5] == 0x80) // close to
				{
					printk("msg2133_read_data psensor on !!! \n");
					ps_data_state[0] = 0;
					Touch_State = Touch_State_PS;
				}
				else if(Buf[5] == 0x40) // leave
				{
					printk("msg2133_read_data psensor off !!! \n");
					ps_data_state[0] = 1;
					Touch_State = Touch_State_PS;
				}
#endif
				else
				{
					Touch_State = Touch_State_No_Touch;
				}
				                     // printk("\n++++++++ line = %d, key id = %d\n++++++++\n",__LINE__, key_id);
			}
			else if((temp_x1_dst  == 0) && (temp_y1_dst == 0))
			{
				_st_finger_infos[0].i2_x = (temp_x0*ANDROID_TS_RESOLUTION_X) / CNT_RESOLUTION_X ;
				_st_finger_infos[0].i2_y = (temp_y0*ANDROID_TS_RESOLUTION_Y) / CNT_RESOLUTION_Y ;
				Touch_State = Touch_State_One_Finger;
				//printk("\n+++ android x0 = %d, android y0 = %d,+++\n",_st_finger_infos[0].i2_x, _st_finger_infos[0].i2_y);
			}
			else
			{
				/*Mapping CNT touch coordinate to Android coordinate*/
				_st_finger_infos[0].i2_x = (temp_x0*ANDROID_TS_RESOLUTION_X) / CNT_RESOLUTION_X ;
				_st_finger_infos[0].i2_y = (temp_y0*ANDROID_TS_RESOLUTION_Y) / CNT_RESOLUTION_Y ;

				_st_finger_infos[1].i2_x = (temp_x1*ANDROID_TS_RESOLUTION_X) / CNT_RESOLUTION_X ;
				_st_finger_infos[1].i2_y = (temp_y1*ANDROID_TS_RESOLUTION_Y) / CNT_RESOLUTION_Y ;
				Touch_State = Touch_State_Two_Finger;
				//printk("\n++++++++ line = %d , android x0 = %d, android y0 = %d, android x1 = %d, android y1 = %d\n++++++++\n",
				//		__LINE__, _st_finger_infos[0].i2_x, _st_finger_infos[0].i2_y, _st_finger_infos[1].i2_x, _st_finger_infos[1].i2_y);
			}

		}
		return 1;
	}
	else
	{
		printk("msg2133 checksum error\n");
		return 0;
	}
}

static int msg2133_report_data(struct Data_CNT *device_contex)
{
	_st_finger_infos[0].i2_x=0;
	_st_finger_infos[0].i2_y=0;
	_st_finger_infos[1].i2_x=0;
	_st_finger_infos[1].i2_y=0;

	if(msg2133_read_data()){
		if(Touch_State == Touch_State_One_Finger)
		{
			//printk("one finger x %d y %d \n",_st_finger_infos[0].i2_x,_st_finger_infos[0].i2_y);
			input_report_key(device_contex->input, BTN_TOUCH,1);
			input_report_abs(device_contex->input, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(device_contex->input, ABS_MT_POSITION_X, _st_finger_infos[0].i2_x);
			input_report_abs(device_contex->input, ABS_MT_POSITION_Y, _st_finger_infos[0].i2_y);
			input_report_abs(device_contex->input, ABS_MT_TRACKING_ID,0);
			input_mt_sync(device_contex->input);
			input_sync(device_contex->input);

		}
		else if(Touch_State == Touch_State_Two_Finger)
		{
			//printk("two finger\n");
			/*report first point*/
			input_report_key(device_contex->input, BTN_TOUCH,1);
			input_report_abs(device_contex->input, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(device_contex->input, ABS_MT_POSITION_X, _st_finger_infos[0].i2_x);
			input_report_abs(device_contex->input, ABS_MT_POSITION_Y, _st_finger_infos[0].i2_y);
			input_report_abs(device_contex->input, ABS_MT_TRACKING_ID,0);
			input_mt_sync(device_contex->input);
			/*report second point*/
			input_report_abs(device_contex->input, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(device_contex->input, ABS_MT_POSITION_X, _st_finger_infos[1].i2_x);
			input_report_abs(device_contex->input, ABS_MT_POSITION_Y, _st_finger_infos[1].i2_y);
			input_report_abs(device_contex->input, ABS_MT_TRACKING_ID,1);
			input_mt_sync(device_contex->input);

			input_sync(device_contex->input);

		}
#ifdef CONFIG_TOUCHSCREEN_MSG2133A_PSENSOR
		else if(Touch_State == Touch_State_PS)
		{
			printk(" psensor  input_report ps_data = %d\n",ps_data_state[0]);
			input_report_abs(device_contex->ps_input, ABS_DISTANCE, ps_data_state[0]);
			input_sync(device_contex->ps_input);
		}
#endif
		else if(Touch_State == Touch_State_VK)
		{
			/*you can implement your VK releated function here*/
			if(key_id == 0x01)
			{
				input_report_key(device_contex->input, tsp_keycodes_q100[0], 1);
				//printk(KERN_DEBUG "\n########### line = %d, key id = %d\n###########\n",__LINE__, key_id);
			}
			else if(key_id == 0x02)
			{
				input_report_key(device_contex->input, tsp_keycodes_q100[1], 1);
				//printk(KERN_DEBUG "\n########### line = %d, key id = %d\n###########\n",__LINE__, key_id);
			}
			else if(key_id == 0x04)
			{
				input_report_key(device_contex->input, tsp_keycodes_q100[2], 1);
				//printk(KERN_DEBUG "\n########### line = %d, key id = %d\n###########\n",__LINE__, key_id);
			}
			else if(key_id == 0x08)
			{
				input_report_key(device_contex->input, tsp_keycodes_q100[3], 1);
				//printk(KERN_DEBUG "\n########### line = %d, key id = %d\n###########\n",__LINE__, key_id);
			}
			//input_report_key(device_contex->input, BTN_TOUCH, 1);
			kave_VK=1;
			input_sync(device_contex->input);
		}
		else/*Finger up*/
		{
			if(kave_VK==1)
			{
				if(key_id == 0x01)
				{
					input_report_key(device_contex->input, tsp_keycodes_q100[0], 0);
				}
				else if(key_id == 0x02)
				{
					input_report_key(device_contex->input, tsp_keycodes_q100[1], 0);
				}
				else if(key_id == 0x04)
				{
					input_report_key(device_contex->input, tsp_keycodes_q100[2], 0);
				}
				else if(key_id == 0x08)
				{
					input_report_key(device_contex->input, tsp_keycodes_q100[3], 0);
				}
			//	input_report_key(device_contex->input, BTN_TOUCH, 0);
				kave_VK=0;
				input_sync(device_contex->input);	
			}
			else
			{
				//printk("figer up\n");
				input_report_key(device_contex->input, BTN_TOUCH,0);
				input_report_abs(device_contex->input, ABS_MT_TOUCH_MAJOR, 0);
				input_mt_sync(device_contex->input);
				input_sync(device_contex->input);	
			}
		}


	}
	return 0;	
}

static void CNT_ts_work_func(struct work_struct *work)
{
	struct Data_CNT *device_contex  = container_of(work, struct Data_CNT, work);
	msg2133_report_data(device_contex);
	enable_irq(device_contex->irq);
}

static irqreturn_t ts_irq(int irq, void *dev_id)
{
	struct Data_CNT *device_contex=(struct Data_CNT *)dev_id;
	//printk("[%s]\n",__func__);
	disable_irq_nosync(device_contex->irq);
	queue_work(CNT_wq, &device_contex->work);
	return IRQ_HANDLED;
}

static int msg2133a_power_on(struct Data_CNT *device_contex, bool on)
{
	int rc;

	if (!on)
		goto power_off;

	rc = regulator_enable(device_contex->vdd);
	if (rc) {
		dev_err(&device_contex->client->dev,
			"Regulator vdd enable failed rc=%d\n", rc);
		return rc;
	}

	rc = regulator_enable(device_contex->vcc_i2c);
	if (rc) {
		dev_err(&device_contex->client->dev,
			"Regulator vcc_i2c enable failed rc=%d\n", rc);
		regulator_disable(device_contex->vdd);
	}

	return rc;

power_off:
	rc = regulator_disable(device_contex->vdd);
	if (rc) {
		dev_err(&device_contex->client->dev,
			"Regulator vdd disable failed rc=%d\n", rc);
		return rc;
	}

	rc = regulator_disable(device_contex->vcc_i2c);
	if (rc) {
		dev_err(&device_contex->client->dev,
			"Regulator vcc_i2c disable failed rc=%d\n", rc);
		regulator_enable(device_contex->vdd);
	}

	return rc;
}

static int msg2133a_chip_init(struct Data_CNT *device_contex,struct i2c_client *client)
{
	device_contex->rst = RST_GPIO;
	device_contex->vdd = regulator_get(&client->dev, "vdd");
	device_contex->vcc_i2c = regulator_get(&client->dev, "vcc_i2c");

	regulator_set_voltage(device_contex->vdd,2700000,3300000);
	regulator_set_voltage(device_contex->vcc_i2c,1800000,1800000);
	regulator_enable(device_contex->vdd);
	regulator_enable(device_contex->vcc_i2c);

	gpio_request(device_contex->rst,"msg2133a_reset");
	gpio_direction_output(device_contex->rst,1);
	gpio_set_value(device_contex->rst,1);
	msleep(50);
	return 0;
}

#ifdef CONFIG_PM
static int msg2133a_ts_suspend(struct device *dev)
{
	struct Data_CNT *device_contex = dev_get_drvdata(dev);
	int err;

	if (device_contex->suspended) {
		dev_info(dev, "Already in suspend state\n");
		return 0;
	}
	gpio_set_value_cansleep(device_contex->rst, 0);
	disable_irq(device_contex->irq);

	err = msg2133a_power_on(device_contex, false);
	if (err) {
		dev_err(dev, "power off failed");
		goto pwr_off_fail;
	}
	device_contex->suspended = true;
	return 0;

pwr_off_fail:
	if (gpio_is_valid(device_contex->rst)) {
		gpio_set_value_cansleep(device_contex->rst, 1);
		msleep(200);
	}
	enable_irq(device_contex->irq);
	return err;
}

static int msg2133a_ts_resume(struct device *dev)
{
	struct Data_CNT *device_contex = dev_get_drvdata(dev);
	int err;

	err = msg2133a_power_on(device_contex, true);
	if (err) {
		dev_err(dev, "power on failed");
		return err;
	}
	if (gpio_is_valid(device_contex->rst)) {
		gpio_set_value_cansleep(device_contex->rst, 1);
		msleep(50);
	}

	enable_irq(device_contex->irq);
	device_contex->suspended = false;

	return 0;
}

static const struct dev_pm_ops msg2133a_ts_pm_ops = {
	.suspend = msg2133a_ts_suspend,
	.resume = msg2133a_ts_resume,
};
#endif
static int  s3c_ts_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct Data_CNT *device_contex;
	struct input_dev *input_dev;
	int err;
	struct msg2133_platform_data *pdata;
	dev_err(&client->dev, "PAP CNT\n");
	if (!i2c_check_functionality(client->adapter,I2C_FUNC_SMBUS_READ_WORD_DATA))
	{
		dev_err(&client->dev, "I2C bus error!\n");
		return -EIO;
	}

	 if (client->dev.of_node) {
	        pdata = devm_kzalloc(&client->dev,
	                sizeof(struct msg2133_platform_data ), GFP_KERNEL);
	        if (!pdata) {
	                dev_err(&client->dev, "Failed to allocate memory\n");
                return -ENOMEM;
	        }
#if 0
       	 err = msg2133_parse_dt(&client->dev, pdata);
	       	 if (err)
	                return err;
#endif
	 pdata->irq_gpio =gpio_to_irq(INT_GPIO); 
	 printk("-->[%s] irq_gpio = %d\n", __func__, pdata->irq_gpio);
	} else
        	pdata = client->dev.platform_data;

	printk("-->[%s] I2C addr = 0x%x\n", __func__, client->addr);
	
	if (!pdata)
	        return -EINVAL;

	// allocte device contex
	device_contex = kzalloc(sizeof(struct Data_CNT ), GFP_KERNEL);
	if (!device_contex){
		dev_err(&client->dev, "%s: Alloc mem fail!", __func__);
		goto exit_alloc_data_failed;
	}

	msg2133a_chip_init(device_contex,client);
	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "%s: failed to allocate input device\n", __func__);
		goto exit_input_dev_alloc_failed;
	}
#ifdef CONFIG_TOUCHSCREEN_MSG2133A_PSENSOR
	device_contex->ps_input = input_allocate_device();
	if (!device_contex->ps_input) {
		dev_err(&client->dev, "%s: failed to allocate input device\n", __func__);
		goto exit_input_dev_alloc_failed;
	}
#endif
	device_contex->client = client;
	device_contex->input  = input_dev;

	this_client = client;
	i2c_set_clientdata(client, device_contex);
	strcpy(device_contex->Name,"PAP CNT driver");


	spin_lock_init(&device_contex->lock);

	// Initial input device
	input_dev->name = "TSC2007 Touchscreen";
#ifdef CONFIG_TOUCHSCREEN_MSG2133A_PSENSOR
	device_contex->ps_input->name = "msg2133a-ps";
#endif
	input_dev->id.bustype = BUS_I2C;

	input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ANDROID_TS_RESOLUTION_X, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ANDROID_TS_RESOLUTION_Y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
        input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 2, 0, 0);

	set_bit(BTN_2, input_dev->keybit);

	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
#ifdef CONFIG_TOUCHSCREEN_MSG2133A_PSENSOR
	set_bit(EV_ABS, device_contex->ps_input->evbit);
	input_set_abs_params(device_contex->ps_input, ABS_DISTANCE, 0,1, 0, 0);
#endif
	set_bit(BTN_MISC, input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	set_bit(BTN_TOUCH, input_dev->keybit);

	input_set_abs_params(input_dev,
			ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	set_bit(KEY_MENU, input_dev->keybit);
	set_bit(KEY_BACK, input_dev->keybit);
	set_bit(KEY_HOMEPAGE, input_dev->keybit);
	set_bit(KEY_SEARCH, input_dev->keybit);

	
	err = input_register_device(input_dev);
	if (err){
		dev_err(&client->dev, "Register input device fail\n");
		goto exit_input_register_device;
	}
#ifdef CONFIG_TOUCHSCREEN_MSG2133A_PSENSOR
	err = input_register_device(device_contex->ps_input);
	if (err){
		dev_err(&client->dev, "Register input device fail\n");
		goto exit_ps_input_register_device;
	}

	wake_lock_init(&ps_wake_lock, WAKE_LOCK_SUSPEND, "ps_wakelock");
	msg2133a_data = device_contex;
#endif
	// Initial a thread
	CNT_wq = create_singlethread_workqueue("CNT_wq");
	if (!CNT_wq) {
		dev_err(&client->dev, "%s: fail to create wq\n", __func__);
		goto exit_create_singlethread;
	}
	INIT_WORK(&device_contex->work, CNT_ts_work_func);

	printk("msg2133 0000\n");
	device_contex->irq = pdata->irq_gpio;	
	err = request_irq(device_contex->irq, ts_irq, IRQF_TRIGGER_RISING,
			client->dev.driver->name, device_contex);

	if (err < 0) {
		dev_err(&client->dev,
				"failed to allocate irq %d\n", device_contex->irq);
		goto exit_irq_request_failed;
	}

#ifdef CONFIG_TOUCHSCREEN_MSG2133A_PSENSOR
	err= sysfs_create_group(&device_contex->ps_input->dev.kobj, &msg2133_proximity_attribute_group);
	if(err < 0)
	{
		printk("Failed to create devices file:msg2133_proximity_attribute_group\n");
	}
#endif

#ifdef __FIRMWARE_UPDATE_MSG2133A__
	firmware_class = class_create(THIS_MODULE, "ms-touchscreen-msg20xx");
	if (IS_ERR(firmware_class))
		pr_err("Failed to create class(firmware)!\n");

	firmware_cmd_dev = device_create(firmware_class,NULL, 0, NULL, "device");
	if (IS_ERR(firmware_cmd_dev))
		pr_err("Failed to create device(firmware_cmd_dev)!\n");

	// version
	if (device_create_file(firmware_cmd_dev, &dev_attr_version) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_version.attr.name);
	// update
	if (device_create_file(firmware_cmd_dev, &dev_attr_update) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_update.attr.name);
	// data
	if (device_create_file(firmware_cmd_dev, &dev_attr_data) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_data.attr.name);

	if (device_create_file(firmware_cmd_dev, &dev_attr_clear) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_clear.attr.name);

#endif

#ifdef CONFIG_MSG2133A_ENABLE_AUTO_UPDATA
	msg2133a_auto_update();
#endif

	return 0;

exit_irq_request_failed:
	cancel_work_sync(&device_contex->work);
	destroy_workqueue(CNT_wq);
exit_create_singlethread:
	input_free_device(input_dev);
#ifdef CONFIG_TOUCHSCREEN_MSG2133A_PSENSOR
exit_ps_input_register_device:
	input_free_device(device_contex->ps_input);
#endif
exit_input_register_device:
	i2c_set_clientdata(client, NULL);
exit_input_dev_alloc_failed:
	kfree(device_contex);
exit_alloc_data_failed:
	return 0;
}
static int s3c_ts_remove(struct i2c_client *client)
{
	return 0;
}
#ifdef CONFIG_OF
static struct of_device_id msg2133_match_table[] = {
        { .compatible = "mstar,msg2133",},
        { },
};
#else
#define msg2133_match_table NULL
#endif

static struct i2c_device_id PACNT_idtable[] = {
	{ "msg2133_ts", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, PACNT_idtable);

static struct i2c_driver d3d = {
	.driver = {
		.name	= "msg2133_ts",
		.owner	= THIS_MODULE,
		.of_match_table = msg2133_match_table,
#ifdef CONFIG_PM
		.pm = &msg2133a_ts_pm_ops,
#endif
	},
	.id_table	= PACNT_idtable,
	.probe		= s3c_ts_probe,
	.remove		=s3c_ts_remove,
};

static char banner[] __initdata = KERN_INFO "CNT TP \n";

static int __init s3c_ts_init(void)
{
	printk(banner);
	return i2c_add_driver(&d3d);
}

static void __exit s3c_ts_exit(void)
{
	i2c_del_driver(&d3d);
}

module_init(s3c_ts_init);
module_exit(s3c_ts_exit);

MODULE_AUTHOR("Jack Hsueh");
MODULE_DESCRIPTION("X9 CNT driver");
MODULE_LICENSE("GPL");

