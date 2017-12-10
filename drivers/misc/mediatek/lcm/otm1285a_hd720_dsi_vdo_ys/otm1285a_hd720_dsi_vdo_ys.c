#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
#endif

#ifdef BUILD_LK
#include <platform/gpio_const.h>
#include <platform/mt_gpio.h>
#include <platform/upmu_common.h>
#else
#include <mach/gpio_const.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include <linux/string.h>
#endif

#define LCM_ID_OTM1285 0x40
const static unsigned char LCD_MODULE_ID = 0x01;
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)


#define REGFLAG_DELAY             								0xFC
#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------


static LCM_UTIL_FUNCS lcm_util;

#define __SAME_IC_COMPATIBLE__

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define MDELAY(n) 											(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
	{0x00,1,{0x00}},         
{0xff,3,{0x12,0x85,0x01}},                //EXTC=1 
{0x00,1,{0x80}},                                        //Orise,mode,enable 
{0xff,2,{0x12,0x85}}, 
{0x00,1,{0x80}},                                        //TCON,Setting,(RTN, 
{0xc0,2,{0x00,0x7F}}, 
{0x00,1,{0x82}},                                        //TCON,Setting,(VFP,VBP, 
{0xc0,3,{0x00,0x0c,0x08}}, 
{0x00,1,{0x90}},                                        //,Oscillator,Divided,mclk/pclk=6+1=7,(defaul=8+1=9, 
{0xc1,1,{0x55}},         
{0x00,1,{0x80}},                                        //,Oscillator,idle/norm/pwron/vdo,64.61MHz 
{0xc1,4,{0x1D,0x1D,0x1D,0x1D}}, 
{0x00,1,{0x90}},         
{0xc2,10,{0x86,0x20,0x00,0x0F,0x00,0x87,0x20,0x00,0x0F,0x00}},        //CKV1/2,shift,switch,width,chop_t1,shift_t2         
{0x00,1,{0xec}},                                        //duty,block,block,width? 
{0xc2,1,{0x00}},         
{0x00,1,{0x80}},                                //,LTPS,STV,Setting 
{0xc2,4,{0x82,0x01,0x08,0x08}}, 
{0x00,1,{0xa0}},         
{0xc0,7,{0x00,0x0f,0x00,0x00,0x0a,0x1A,0x00}},        //CKH 
{0x00,1,{0xfb}},                                                                        //PCH_SET 
{0xc2,1,{0x80}},         
{0x00,1,{0x80}},         
{0xcc,12,{0x03,0x04,0x01,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b}},                                        //Step1:SIGIN_SEL,U2D 
{0x00,1,{0xb0}},         
{0xcc,12,{0x04,0x03,0x01,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b}},                                        //Step1:(add,SIGIN_SEL,D2U 
{0x00,1,{0x80}},         
{0xcd,15,{0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x15,0x0B,0x0b,0x12,0x13,0x14,0x0b,0x0b,0x0b}},        //Step1:SIGIN_SEL 
{0x00,1,{0xc0}},         
{0xcb,15,{0x05,0x05,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},        //try,Step2:ENMODE,(address,1-27在各個state狀態, 
{0x00,1,{0xd0}},         
{0xcb,15,{0x00,0x00,0x00,0x0A,0x05,0x00,0x05,0x05,0x05,0x00,0x00,0x00,0x00,0x00,0x00}},        //Step2:ENMODE,(address,1-27在各個state狀態, 
{0x00,1,{0xe0}},         
{0xcc,4,{0x00,0x00,0x00,0x00}},                                                                                                                        //Step3:Hi-Z,mask,(若reg_hiz,=,0，則維持STEP2的設定, 
{0x00,1,{0xd0}},         
{0xcd,15,{0x01,0x02,0x04,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f}},        //Step4:決定address1-27,mapping,到,R_CGOUT1-15 
{0x00,1,{0xe0}},         
{0xcd,12,{0x10,0x11,0x12,0x13,0x03,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b}},                                        //Step4:決定address1-27,mapping,到,R_CGOUT16-27 
{0x00,1,{0xa0}},         
{0xcd,15,{0x01,0x02,0x04,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f}},        //(add,Step4:決定address1-27,mapping,到,L_CGOUT1-15 
{0x00,1,{0xb0}},         
{0xcd,12,{0x10,0x11,0x12,0x13,0x03,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b}},                                        //(add,Step4:決定address1-27,mapping,到,L_CGOUT16-27 
//-------------power,setting----------------------------------------------------------------------------------------------         
{0x00,1,{0x00}},                                                                                                                                                        //,set,GVDD/NGVDD=+/-4.1V(measure=+/-, 
{0xD8,2,{0x26,0x26}}, 
{0x00,1,{0x90}},                                                                                                                                                        //,power,control,setting,for,normal,mode}},,(set,AVDD,VGL,VGH,AVDD,NAVDD, 
{0xC5,6,{0xB2,0xD6,0xA0,0x0F,0xA0,0x14}},                                                                                                //,AVDD,clamp,at,6V,AVDD=2VCI}},,VGH=4xVCI,VGHS=,8.1V}},VGL_DM=3xVCI,VGLS=,-8V 
{0x00,1,{0x96}}, 
{0xc5,4,{0xa0,0x0d,0xaa,0x0c}}, 
//----------------------------------------------------------------------------------------------------------------------         
//-------------------------------,tunig,default,--------------------------------------------------- 
{0x00,1,{0xc1}},         
{0xc5,1,{0x33}},                //VDD_18/LVDSVDD=1.6V 
{0x00,1,{0xa3}},         
{0xc5,1,{0x0f}},         
{0x00,1,{0xa5}},         
{0xc5,5,{0x0f,0xa0,0x0d,0xaa,0x0c}},         
{0x00,1,{0x93}},         
{0xc5,1,{0x0f}},         
{0x00,1,{0x95}},         
{0xc5,5,{0x0f,0xa0,0x0d,0xaa,0x0c}},         
{0x00,1,{0x90}},         
{0xf5,14,{0x03,0x15,0x09,0x15,0x07,0x15,0x0c,0x15,0x0a,0x15,0x09,0x15,0x0a,0x15}},         
{0x00,1,{0xa0}},         
{0xf5,14,{0x12,0x11,0x03,0x15,0x09,0x15,0x11,0x15,0x08,0x15,0x07,0x15,0x09,0x15}},         
{0x00,1,{0xc0}},         
{0xf5,14,{0x0e,0x15,0x0e,0x15,0x00,0x15,0x00,0x15,0x0e,0x15,0x14,0x11,0x00,0x15}},         
{0x00,1,{0xd0}},         
{0xf5,15,{0x07,0x15,0x0a,0x15,0x10,0x11,0x00,0x10,0x90,0x90,0x90,0x02,0x90,0x00,0x00}}, 
{0x00,1,{0xC2}},
{0xC5,1,{0x08}},
{0x00,1,{0xA8}},
{0xC4,1,{0x11}},
{0x00,1,{0x00}},	
{0xE1,24,{0x10,0x29,0x30,0x3c,0x44,0x4b,0x57,0x67,0x6F,0x82,0x8c,0x95,0x65,0x5F,0x5a,0x4C,0x3D,0x2C,0x22,0x1C,0x17,0x0c,0x09,0x5}},	
{0x00,1,{0x00}},	
{0xE2,24,{0x10,0x29,0x30,0x3c,0x44,0x4C,0x57,0x67,0x6F,0x82,0x8c,0x95,0x65,0x5F,0x5a,0x4C,0x3D,0x2C,0x22,0x1C,0x17,0x0c,0x09,0x5}},	
{0x00,1,{0x00}},	
{0xE3,24,{0x10,0x29,0x30,0x3b,0x43,0x49,0x55,0x65,0x6e,0x81,0x8c,0x95,0x65,0x5F,0x5a,0x4C,0x3D,0x2C,0x22,0x1B,0x17,0x0E,0x0A,0x5}},	
{0x00,1,{0x00}},	
{0xE4,24,{0x10,0x29,0x30,0x3b,0x43,0x4a,0x54,0x65,0x6f,0x82,0x8c,0x95,0x65,0x5F,0x5a,0x4C,0x3D,0x2C,0x22,0x1B,0x17,0x0E,0x0A,0x5}},	
{0x00,1,{0x00}},	
{0xE5,24,{0x10,0x26,0x2c,0x37,0x40,0x46,0x52,0x63,0x6c,0x7f,0x8c,0x95,0x65,0x5F,0x5a,0x4C,0x3D,0x2C,0x25,0x1a,0x15,0x0b,0x09,0x5}},	
{0x00,1,{0x00}},	
{0xE6,24,{0x10,0x26,0x2c,0x37,0x40,0x46,0x52,0x63,0x6c,0x7f,0x8c,0x95,0x65,0x5F,0x5a,0x4C,0x3D,0x2C,0x25,0x1a,0x16,0x0b,0x09,0x5}},	
{0x00,1,{0x00}},         
{0xff,3,{0x00,0x00,0x00}}, 

{0x36,1,{0x01}},
{0x11,	1,		{0x00}},       
{REGFLAG_DELAY, 120, {}},		
{0x29,	1,		{0x00}},	        
{REGFLAG_DELAY, 20, {}},	      
{REGFLAG_END_OF_TABLE, 0x00, {}}

};
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    //Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
		memset(params, 0, sizeof(LCM_PARAMS));
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

		params->dbi.te_mode				= LCM_DBI_TE_MODE_VSYNC_ONLY;
		//LCM_DBI_TE_MODE_DISABLED;
		//LCM_DBI_TE_MODE_VSYNC_ONLY;  
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING; 
		/////////////////////   
		//if(params->dsi.lcm_int_te_monitor)  
		//params->dsi.vertical_frontporch *=2;  
		//params->dsi.lcm_ext_te_monitor= 0;//TRUE;
		//zhounengwen@wind-mobi.com begin
		//params->dsi.noncont_clock= TRUE;//FALSE;   
		//params->dsi.noncont_clock_period=2;
		params->dsi.cont_clock=1;    //modify  yudengwu
		//zhounengwen@wind-mobi.com end
		////////////////////          
		params->dsi.mode   = SYNC_PULSE_VDO_MODE;  
		// DSI    /* Command mode setting */  
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;      
		//The following defined the fomat for data coming from LCD engine.  
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;   
		params->dsi.data_format.trans_seq	= LCM_DSI_TRANS_SEQ_MSB_FIRST; 
		params->dsi.data_format.padding 	= LCM_DSI_PADDING_ON_LSB;    
		params->dsi.data_format.format	  = LCM_DSI_FORMAT_RGB888;       
		// Video mode setting		   
		params->dsi.intermediat_buffer_num = 2;  
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;  
		params->dsi.packet_size=256;    
		// params->dsi.word_count=480*3;	
		//DSI CMD mode need set these two bellow params, different to 6577   
		// params->dsi.vertical_active_line=800;   
		params->dsi.vertical_sync_active				= 6; //4   
		params->dsi.vertical_backporch				       = 20;  //14  
		params->dsi.vertical_frontporch				       = 14;  //16  
		params->dsi.vertical_active_line				       = FRAME_HEIGHT;     
		params->dsi.horizontal_sync_active				= 60;   //4
		params->dsi.horizontal_backporch				= 100;  //60  
		params->dsi.horizontal_frontporch				= 100;    //60
		params->dsi.horizontal_blanking_pixel				= 60;   
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;  
		
	//	params->dsi.pll_div1=1;		   
	//	params->dsi.pll_div2=1;		   
	//	params->dsi.fbk_div =28;//28	
//liqiang@wind-mobi.com 20140820 beign
// To fix boot error
		params->dsi.PLL_CLOCK = 225;	   // 245;

	//	params->dsi.ss

//		params->dsi.CLK_TRAIL = 17;
			
//liqiang@wind-mobi.com 20140820 end
		params->dsi.esd_check_enable = 1;
                params->dsi.customization_esd_check_enable = 1;
                params->dsi.lcm_esd_check_table[0].cmd                  = 0x0a;
                params->dsi.lcm_esd_check_table[0].count                = 1;
                params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
	
}


static unsigned int lcm_compare_id(void)
{
		unsigned int id = 0;
		unsigned char buffer[2];
		unsigned int array[16];
		unsigned char LCD_ID_value = 0;
		mt_set_gpio_mode(GPIO_LCM_PWR_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_PWR_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_PWR_EN, GPIO_OUT_ONE);
	MDELAY(10);
		
		
			SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
			SET_RESET_PIN(0);
			MDELAY(1);
			SET_RESET_PIN(1);
			MDELAY(150);
	
	
	
	
	//	push_table(lcm_compare_id_setting, sizeof(lcm_compare_id_setting) / sizeof(struct LCM_setting_table), 1);
	
		array[0] = 0x00023700;// read id return two byte,version and id
		dsi_set_cmdq(array, 1, 1);
		read_reg_v2(0xDA, buffer, 1);
	
		id = buffer[0]; //we only need ID
      #ifdef BUILD_LK
		printf("%s,  id otm1285A= 0x%08x\n", __func__, id);
	  #endif
               
                if(LCM_ID_OTM1285 == id)
                {
                   	LCD_ID_value = which_lcd_module_triple();

	                if(LCD_MODULE_ID == LCD_ID_value)
                   	{ 
                         	return 1;
    	                }
                 	else
                	{
                        	return 0;
    	                }
                    
                }
                else
                   return 0;                 

//		return (LCM_ID_OTM1285 == id)?1:0;


}


static void lcm_init(void)
{
	
    mt_set_gpio_mode(GPIO_LCM_PWR_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_PWR_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_PWR_EN, GPIO_OUT_ONE);
	
    MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);
	
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1); 
	
} 
static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	
	mt_set_gpio_mode(GPIO_LCM_PWR_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_PWR_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_PWR_EN, GPIO_OUT_ZERO);
	MDELAY(10);
}

static void lcm_resume(void)
{

	lcm_init();
}


static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);	
}


LCM_DRIVER otm1285a_hd720_dsi_vdo_ys_lcm_drv = 
{
    .name			= "otm1285a_dsi_vdo_ys",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
//	.init_power		= lcm_init_power,
//  .resume_power = lcm_resume_power,
//  .suspend_power = lcm_suspend_power,
//	.set_backlight	= lcm_setbacklight,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
};