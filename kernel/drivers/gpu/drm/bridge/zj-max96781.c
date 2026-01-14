// SPDX-License-Identifier: GPL-2.0-only

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/slab.h>

#include <media/cec.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_edid.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>

//#include <linux/hdmi.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_bridge.h>
#include <drm/drm_connector.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>

//#define PATGEN_EN
#define REMOTE_IIC

#define I2CRegWrite(a,b) regmap_write(regmap_max96781, a, b);
#define I2CRegRead(a,b)  regmap_read(regmap_max96781, a, b);

static const struct regmap_range ti_sn_bridge_volatile_ranges[] = {
    { .range_min = 0, .range_max = 0xFF },
};

static const struct regmap_access_table ti_sn_bridge_volatile_table = {
    .yes_ranges = ti_sn_bridge_volatile_ranges,
    .n_yes_ranges = ARRAY_SIZE(ti_sn_bridge_volatile_ranges),
};

static const struct regmap_config max96781_regmap_config = {
    .reg_bits = 16, 
    .val_bits = 8, 
    .volatile_table = &ti_sn_bridge_volatile_table,
    .cache_type = REGCACHE_NONE,
};

static int I2CRegConfig(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	
	struct regmap *regmap_max96781;
	unsigned int val;
	int ret = 0;

	printk("jin debug: func=%s start\n", __FUNCTION__);
	regmap_max96781 = devm_regmap_init_i2c(i2c, &max96781_regmap_config);
	if (IS_ERR(regmap_max96781)) 
	{
		ret = PTR_ERR(regmap_max96781);
		return ret;
	}

	printk("jin debug: func=%s second\n", __FUNCTION__);

	ret = I2CRegRead(0x00, &val);
	if (ret)
		return ret;
	
	printk("jin debug: val=0x%08x\n", val);
	if(val!=0x80)
	{
		printk("jin debug: address error!\n");

		return ret;		
	}
	//I2CRegWrite(0x0010,0x91);//Activate chip reset
#if 0
	I2CRegWrite(0x00A3,0x10);//Packets are transmitted over GMSLA insplitter mode
	I2CRegWrite(0x00A7,0x21);
	I2CRegWrite(0x0421,0x00);

	I2CRegWrite(0x0422,0x00);//VS delay in terms of PCLK cycles
	I2CRegWrite(0x0423,0x00);
	I2CRegWrite(0x0424,0x00);

	I2CRegWrite(0x0425,0x00);//11800
	I2CRegWrite(0x0426,0x2E);//VS high period in terms of PCLK cycles
	I2CRegWrite(0x0427,0x18);

	I2CRegWrite(0x0428,0x18);//1604800 
	I2CRegWrite(0x0429,0x7C);//VS low period in terms of PCLK cycles
	I2CRegWrite(0x042A,0xC0);


	I2CRegWrite(0x042B,0x00);//VS edge to the rising edge of the first HS interms of PCLK cycles
	I2CRegWrite(0x042C,0x00);
	I2CRegWrite(0x042D,0x00);

	I2CRegWrite(0x042E,0x00);//HS high period in terms of PCLK cycles 44
	I2CRegWrite(0x042F,0x2C);

	I2CRegWrite(0x0430,0x09);//HS low period in terms of PCLK cycles 2316
	I2CRegWrite(0x0431,0x0C);

	I2CRegWrite(0x0432,0x02);//HS pulses per frame 685
	I2CRegWrite(0x0433,0xAD);	
	
	I2CRegWrite(0x0434,0x01);//VS edge to the rising edge of the first DE interms of PCLK cycles 96952
	I2CRegWrite(0x0435,0x7A);
	I2CRegWrite(0x0436,0xB8);

	I2CRegWrite(0x0437,0x08);//DE high period in terms of PCLK cycles 2080
	I2CRegWrite(0x0438,0x20);
	
	I2CRegWrite(0x0439,0x01);// DE low period in terms of PCLK cycles 280
	I2CRegWrite(0x043A,0x18);
	
	I2CRegWrite(0x043B,0x02);//Active lines per frame 640
	I2CRegWrite(0x043C,0x80);

	I2CRegWrite(0x043D,0x02);//0b10: Generate gradient pattern
	//I2CRegWrite(0x043D,0x00);//Pattern generator disabled - use video fromthe serializer input jhh
	
	I2CRegWrite(0x043E,0x04);
	
	I2CRegWrite(0x0004,0x0D);//PAR_VID_EN
	//I2CRegWrite(0x0004,0x20);//HS VS DE plck outpus enable  jhh

	//I2CRegWrite(0x0005,0xc0);//Link A OR Link B is lockedg
#ifdef REMOTE_IIC
	I2CRegWrite(0x0076,0x18);//enable I2C remote control
#endif
#ifdef PATGEN_EN
	I2CRegWrite(0x04C0,0xE4);//96996  M
	I2CRegWrite(0x04C1,0x7A);
	I2CRegWrite(0x04C2,0x01);
	
	I2CRegWrite(0x04C3,0xE0);//300000  n
	I2CRegWrite(0x04C4,0x93);
	I2CRegWrite(0x04C5,0x04);

	I2CRegWrite(0x04CE,0x5F);//95  asym_enable
	
	I2CRegWrite(0x6420,0x00);
	I2CRegWrite(0x6421,0x01);//plck_run 
#endif	
	I2CRegWrite(0x0420,0xFF);//vpg_enable
	I2CRegWrite(0x04D1,0xF0);

	I2CRegWrite(0x6424,0x0A);
#endif
#if 1  //jinbiao
	printk("jin debug: 96781 Congfig start!\n");

	I2CRegWrite(0x0005,0xc0);//Link A OR Link B is lockedg
	I2CRegWrite(0x0010,0x01);//Disable auto-link config; Single A  Activate chip reset

	printk("jhh debug: 96781 LinkA over!\n");

	I2CRegWrite(0x0076,0x18);//enable I2C remote control
	I2CRegWrite(0x0100,0x61);//Video transmitport enabled

	I2CRegWrite(0x6421,0x0F);//Set pclks to run continuously  no find register
	//I2CRegWrite(0x6421,0x03);//Set pclks to run continuously

	//I2CRegWrite(0x7F11,0x05);//Enable MST mode on GM03-DEBUG	//0x07  no find register
	I2CRegWrite(0x7F11,0x06);

	//Configure GM03 DP_RX Payload IDs
	I2CRegWrite(0x7904,0x01);//Set video payload ID1 for video output port0 on GM03	
	I2CRegWrite(0x7904,0x02);//Set video payload ID2 for video output port1 on GM03
	I2CRegWrite(0x6420,0x10);//Turn off video
	//I2CRegWrite(0x6420,0x00);//Turn off video

	I2CRegWrite(0x7A14,0x00);//Disable MST_VS0_DTG_ENABLE
	I2CRegWrite(0x7B14,0x00);//Disable MST_VS1_DTG_ENABLE

	I2CRegWrite(0x7000,0x00);//Disable LINK_ENABLE
	I2CRegWrite(0x7054,0x01);//Reset DPRX core(VIDEO_INPUT_RESET)
	//delay100
	mdelay(100);

	//I2CRegWrite(0x7074,0x14);//Set MAX_LINK_RATE to 5.4Gb/s
	I2CRegWrite(0x7074,0x0a);//Set MAX_LINK_RATE to 5.4Gb/s
	I2CRegWrite(0x7070,0x02);//Set MAX_LINK_COUNT to 2
	//delay100
	mdelay(100);

	//I2CRegWrite(0x00A3,0x11);//StreamID//0x10-->0x11
	I2CRegWrite(0x00A3,0x00);//StreamID//0x10-->0x11  Packets are not transmitted over GMSLB in splitter mode

	//I2CRegWrite(0x00A7,0x20);//0x21-->0x20
	I2CRegWrite(0x00A7,0x01);//0x21-->0x20  Stream ID for packets from this channel

	I2CRegWrite(0x7000,0x01);//Enable LINK_ENABLE
	//delay1000
	mdelay(100);

	I2CRegWrite(0x7A18,0x05);//Disable MSA reset  no find registe
	I2CRegWrite(0x7B18,0x05);
	I2CRegWrite(0x7C18,0x05);
	I2CRegWrite(0x7D18,0x05);
	
	//I2CRegWrite(0x7A28,0xFF);//Adjust VS0_DMA_HSYNC
	//I2CRegWrite(0x7A2A,0xFF);
	//I2CRegWrite(0x7A24,0xFF);//Adjust VS0_DMA_VSYNC
	//I2CRegWrite(0x7A27,0x0F);
	
	I2CRegWrite(0x7A28,0x64);//Adjust VS0_DMA_HSYNC  DMA_HSYNC_PULSE_WIDTH_L[7:0]
	I2CRegWrite(0x7A2A,0x40);//HFP old 0x14 jhh  DMA_HSYNC_FRONT_PORCH_L[7:0]
	I2CRegWrite(0x7A24,0x02);//Adjust VS0_DMA_VSYNC  DMA_VSYNC_PULSE_WIDTH
	I2CRegWrite(0x7A26,0x04);//VFP   DMA_VSYNC_FRONT_PORCH_L
	
	//I2CRegWrite(0x7B28,0xFF);//Adjust VS1_DMA_HSYNC
	//I2CRegWrite(0x7B2A,0xFF);
	//I2CRegWrite(0x7B24,0xFF);//Adjust VS1_DMA_VSYNC
	//I2CRegWrite(0x7B27,0x0F);
	
	I2CRegWrite(0x7A14,0x01);//Enable MST_VS0_DTG_ENABLE
	//I2CRegWrite(0x7B14,0x01);//Enable MST_VS1_DTG_ENABLE
	I2CRegWrite(0x7B14,0x00);//Disable MST_VS1_DTG_ENABLE
	
	//add
	I2CRegWrite(0x7A00,0x01);//Enable VS0
	//I2CRegWrite(0x7B00,0x01);//Enable VS1
	I2CRegWrite(0x7B00,0x00);//Disable VS1
	
	I2CRegWrite(0x6420,0x13);//Turnonvideo
	//I2CRegWrite(0x6420,0x11);//Turnonvideo
	I2CRegWrite(0x6420,0x10);//Turnoffvideo
	//I2CRegWrite(0x6420,0x10);//Turnoffvideo
	I2CRegWrite(0x6420,0x13);//Turnonvideo
	//I2CRegWrite(0x6420,0x11);//Turnonvideo
	
	I2CRegWrite(0x0028,0x88);//Link A is enabled,6G
	//I2CRegWrite(0x0032,0x80);//Link B is enabled
	I2CRegWrite(0x0032,0x00);//Link B is disabled

	//I2CRegWrite(0x0010,0x01);//MAX96781 link A one short reset//650x03-->0x01
	I2CRegWrite(0x0010,0x01);//MAX96781 link A one short reset//650x03-->0x01  0b01: Single A
	I2CRegWrite(0x0029,0x02);//Delay200ms waiting link lock  0b1: Reset data path
	//delay200
	mdelay(100);

	I2CRegWrite(0x0029, 0x01); //Reset GMSL link  0b1: Activate link reset
	I2CRegWrite(0x0076, 0x98); //Disable remote control channel  0b1: Remote control channel disabled  0b1: Lock status automatically determines the
	I2CRegWrite(0x0100, 0x61); //Ensure the VID enabled  0b10: HS, VS, and DE encoding on, color bits sentonly when DE is high
	I2CRegWrite(0x7000, 0x00); //Disable LINK_ENABLE  DPRX disabled - HPD = 0
	I2CRegWrite(0x7019, 0x00); //Disable reporting MST Capability
	I2CRegWrite(0x70a0, 0x04); //Set AUX_RD_INTERVAL to 16ms
	I2CRegWrite(0x7074, 0x14); //Set MAX_LINK_RATE to 5.4Gb/s
	I2CRegWrite(0x7070, 0x02); //Set MAX_LINK_COUNT to 2  0x02: 2 lanes supported (lane 0, 1)
	I2CRegWrite(0x7000, 0x01); //Enable LINK_ENABLE  0b1: DPRX enabled - HPD active

	I2CRegWrite(0x0028, 0x88); //Link A is enabled + 6G
	//I2CRegWrite(0x0032, 0x80); //Link B is enabled 
	//I2CRegWrite(0x0010, 0x00); //Dual Link
	I2CRegWrite(0x0010, 0x01); //Link A
	I2CRegWrite(0x0076, 0x18); //Enable remote control channel

#ifdef  PATGEN_EN
//I2CRegWrite(0x043d, 0x82); //enable PATGEN_EN
#endif
	I2CRegWrite(0x0029, 0x00); //Release GMSL link
	printk("jhh debug: 96781 Congfig end---!\n");
#endif

	return ret;
}

static int max96781_encoder_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{

    printk("96781 is started\n");
	I2CRegConfig(i2c, id);

	return 0;
}

static int max96781_encoder_remove(struct i2c_client *i2c)
{
	return 0;
}

static const struct dev_pm_ops max96781_encoder_pm_ops = {
};

static const struct i2c_device_id max96781_encoder_i2c_ids[] = {
	{ "max96781-encoder", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, max96781_encoder_i2c_ids);

static const struct of_device_id max96781_encoder_of_ids[] = {
	{ .compatible = "max96781-encoder", .data = (void *)0 },
	{ }
};
MODULE_DEVICE_TABLE(of, max96781_encoder_of_ids);

static struct i2c_driver max96781_encoder_driver = {
	.driver = {
		.name = "max96781_encoder",
		.of_match_table = max96781_encoder_of_ids,
		.pm = &max96781_encoder_pm_ops,
	},
	.id_table = max96781_encoder_i2c_ids,
	.probe = max96781_encoder_probe,
	.remove = max96781_encoder_remove,
};

static int __init max96781_encoder_init(void)
{
	return i2c_add_driver(&max96781_encoder_driver);
}
module_init(max96781_encoder_init);

static void __exit max96781_encoder_exit(void)
{

}
module_exit(max96781_encoder_exit);

MODULE_AUTHOR("Jin");
MODULE_DESCRIPTION("max96781 encoder transmitter driver");
MODULE_LICENSE("GPL");
