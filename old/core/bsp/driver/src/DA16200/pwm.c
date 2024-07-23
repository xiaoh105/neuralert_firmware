/**
 ****************************************************************************************
 *
 * @file pwm.c
 *
 * @brief 
 *
 * Copyright (c) 2016-2022 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hal.h"

#define DRIVER_DBG_NONE		DRV_PRINTF
#define DRIVER_DBG_ERROR	DRV_PRINTF

#ifdef PWM_DEBUG_ON
#define PRINT_PWM 	DRV_PRINTF
#else
#define PRINT_PWM
#endif

//---------------------------------------------------------
//
//---------------------------------------------------------

static int	_pwm_instance[PWM_MAX];
static UINT8	drv_pwm_flag = FALSE;

#define	PWM_INSTANCE		0	//pwm->dev_unit

//---------------------------------------------------------
//
//---------------------------------------------------------

static UINT32 					g_period_us[PWM_MAX], g_hduty_percent[PWM_MAX];
static CLOCK_LIST_TYPE			pwm_pll_info;


static void pwm_pll_callback(UINT32 clock, void *param);

HANDLE	DRV_PWM_CREATE( UINT32 dev_id )
{
	PWM_HANDLER_TYPE	*pwm;

	// Allocate
	if((pwm = (PWM_HANDLER_TYPE *) pvPortMalloc(sizeof(PWM_HANDLER_TYPE)) ) == NULL){
		return NULL;
	}

	// Clear
	memset(pwm, 0, sizeof(PWM_HANDLER_TYPE));

	// Address Mapping
	switch((PWM_LIST)dev_id){
		case	pwm_0:
				pwm->dev_addr = SYS_PWM_BASE;
				pwm->instance = (UINT32)_pwm_instance[pwm_0];
				_pwm_instance[pwm_0] ++;
				break;
		case	pwm_1:
				pwm->dev_addr = SYS_PWM_BASE;
				pwm->instance = (UINT32)_pwm_instance[pwm_1];
				_pwm_instance[pwm_1] ++;
				break;
		case	pwm_2:
				pwm->dev_addr = SYS_PWM_BASE;
				pwm->instance = (UINT32)_pwm_instance[pwm_2];
				_pwm_instance[pwm_2] ++;
				break;
		case	pwm_3:
				pwm->dev_addr = SYS_PWM_BASE;
				pwm->instance = (UINT32)_pwm_instance[pwm_3];
				_pwm_instance[pwm_3] ++;
				break;


		default:
				DRIVER_DBG_ERROR("illegal unit, %d\r\n", dev_id);
				vPortFree(pwm);
				return NULL;
	}

	// Set Default Para
	pwm->regmap   = (PWM_REG_MAP *)(pwm->dev_addr);
	pwm->dev_unit = dev_id ;
	pwm->clock = 0;

	//PRINT_PWM("(%p) pwm Driver create, base %p\r\n", (UINT32 *)pwm, (UINT32 *)pwm->dev_addr );
	return (HANDLE) pwm;
}


int	DRV_PWM_INIT (HANDLE handler)
{
	PWM_HANDLER_TYPE	*pwm;

	if(handler == NULL){
		return FALSE;
	}
#if 0//For ASIC Pin Configuration

//		*(unsigned int *)(0x50001100) &= ~(0x00080000);//PWM not GPIO
//		*(unsigned int *)(0x50001104) &= ~(0x1);//PWM not INT
		FC9000_IOMUX->FSEL_GPIO1 &= ~(0x00080000);//PWM not GPIO;
		FC9000_IOMUX->FSEL_GPIO2 &= ~(0x1);//PWM not INT
#endif
	DA16X_CLKGATE_ON(clkoff_PWM);

//	da16x_io_pinmux(PIN_GMUX, GMUX_INTRPWM);
	pwm = (PWM_HANDLER_TYPE *)handler ;

	// Normal Mode
	if(pwm->instance == 0){
//		HW_REG_AND_WRITE32_PWM(	pwm->regmap->i2c_cr1, (~PWM_CTRL_ENABLE) ); //power gating

		// Post-Action
		if( drv_pwm_flag == FALSE ){
			drv_pwm_flag = TRUE ;
		}

	}
	//pll callback
		pwm_pll_info.callback.func = pwm_pll_callback;
		pwm_pll_info.callback.param = handler;
		pwm_pll_info.next = NULL;

		_sys_clock_ioctl(SYSCLK_SET_CALLACK, &(pwm_pll_info));
	return TRUE;
}



extern int	_sys_clock_read(UINT32 *clock, int len);


int DRV_PWM_START(HANDLE handler,  UINT32 period_us, UINT32 hduty_percent, UINT32 mode)
{
	PWM_HANDLER_TYPE    *pwm;
	unsigned int    period = 0;
	unsigned int    hduty = 0;
	unsigned int clock = 0;
	unsigned long long period_l, hduty_l;

	if(handler == NULL){
		return FALSE;
	}
	pwm = (PWM_HANDLER_TYPE *)handler ;

	PRINT_PWM("period = %d, hduty = %d\r\n",period_us,hduty_percent);
	_sys_clock_read(&clock,sizeof(UINT32));

	if(mode == PWM_DRV_MODE_US)
	{
		period_l = ((((unsigned long long)period_us * 10) * ((unsigned long long)clock / 1000000))/10)-1 ;// minimum sys clock 1mhz
		hduty_l = (((period_l + 1) * (unsigned long long)hduty_percent) / 100)-1 ;
		if((hduty_l > period_l))//klocwork
			hduty_l = 0;
		period = (unsigned int)period_l;
		hduty = (unsigned int)hduty_l;
	}
	else if(mode == PWM_DRV_MODE_CYC)//direct register access
	{
		period = period_us;
		hduty = hduty_percent;
	}

	switch(pwm->dev_unit){
		case    pwm_0:
					pwm->regmap->pwm_maxcy0 = period;
					pwm->regmap->pwm_hduty0 = hduty;
					pwm->regmap->pwm_en0 = PWM_CTRL_ENABLE;
					g_period_us[pwm_0] = period_us;
					g_hduty_percent[pwm_0] = hduty_percent;
				break;

		case	pwm_1:
					pwm->regmap->pwm_maxcy1 = period;
					pwm->regmap->pwm_hduty1 = hduty;
					pwm->regmap->pwm_en1 = PWM_CTRL_ENABLE;
					g_period_us[pwm_1] = period_us;
					g_hduty_percent[pwm_1] = hduty_percent;
				break;

		case	pwm_2:
					pwm->regmap->pwm_maxcy2 = period;
					pwm->regmap->pwm_hduty2 = hduty;
					pwm->regmap->pwm_en2 = PWM_CTRL_ENABLE;
					g_period_us[pwm_2] = period_us;
					g_hduty_percent[pwm_2] = hduty_percent;
				break;

		case	pwm_3:
					pwm->regmap->pwm_maxcy3 = period;
					pwm->regmap->pwm_hduty3 = hduty;
					pwm->regmap->pwm_en3 = PWM_CTRL_ENABLE;
					g_period_us[pwm_3] = period_us;
					g_hduty_percent[pwm_3] = hduty_percent;
				break;

		default:
				DRIVER_DBG_ERROR("illegal unit,\r\n");
				return FALSE;
	}

	return TRUE;
}

int	DRV_PWM_STOP(HANDLE handler, UINT32 dummy)
{
	DA16X_UNUSED_ARG(dummy);

	PWM_HANDLER_TYPE    *pwm;

	if(handler == NULL){
		return FALSE;
	}
	pwm = (PWM_HANDLER_TYPE *)handler ;


	switch(pwm->dev_unit){
		case    pwm_0:
					pwm->regmap->pwm_en0 &= ~PWM_CTRL_ENABLE;
				break;

		case    pwm_1:
					pwm->regmap->pwm_en1 &= ~PWM_CTRL_ENABLE;
				break;

		case    pwm_2:
					pwm->regmap->pwm_en2 &= ~PWM_CTRL_ENABLE;
				break;

		case    pwm_3:
					pwm->regmap->pwm_en3 &= ~PWM_CTRL_ENABLE;
				break;


		default:
				DRIVER_DBG_ERROR("illegal unit, \r\n");
				return FALSE;
	}

    return TRUE;
}


int	DRV_PWM_CLOSE(HANDLE handler)
{
	PWM_HANDLER_TYPE	*pwm;
//	UNSIGNED 			masked_evt;

	if(handler == NULL){
		return FALSE;
	}
	pwm = (PWM_HANDLER_TYPE *)handler ;

	if ( _pwm_instance[pwm->dev_unit] > 0) {
		_pwm_instance[pwm->dev_unit] -- ;

		if(_pwm_instance[pwm->dev_unit] == 0) {


			switch(pwm->dev_unit){
                case    pwm_0:
                            pwm->regmap->pwm_en0 &= ~PWM_CTRL_ENABLE;
                        break;

                case    pwm_1:
                            pwm->regmap->pwm_en1 &= ~PWM_CTRL_ENABLE;
                        break;
                
                case    pwm_2:
                            pwm->regmap->pwm_en2 &= ~PWM_CTRL_ENABLE;
                        break;
                
                case    pwm_3:
                            pwm->regmap->pwm_en3 &= ~PWM_CTRL_ENABLE;
                        break;
                

                default:
                        DRIVER_DBG_ERROR("illegal unit, \r\n");
                        return FALSE;
			}
		}
	}
	_sys_clock_ioctl(SYSCLK_RESET_CALLACK, &(pwm_pll_info));

//	HAL_FREE(handler);
//	DA16X_CLKGATE_OFF(clkoff_PWM);
	return TRUE;
}


static void pwm_pll_callback(UINT32 clock, void *param)
{
	PWM_HANDLER_TYPE	*pwm = (PWM_HANDLER_TYPE *) param;

	if( clock == 0 ){
		// "clock == 0" means "before CPU throttling".
		// skip
		return;
	}

	if(pwm->regmap->pwm_en0 | PWM_CTRL_ENABLE)
		DRV_PWM_START(pwm, g_period_us[pwm_0], g_hduty_percent[pwm_0], PWM_DRV_MODE_US);

	if(pwm->regmap->pwm_en1 | PWM_CTRL_ENABLE)
		DRV_PWM_START(pwm, g_period_us[pwm_1], g_hduty_percent[pwm_1], PWM_DRV_MODE_US);

	if(pwm->regmap->pwm_en2 | PWM_CTRL_ENABLE)
		DRV_PWM_START(pwm, g_period_us[pwm_2], g_hduty_percent[pwm_2], PWM_DRV_MODE_US);

	if(pwm->regmap->pwm_en3 | PWM_CTRL_ENABLE)
		DRV_PWM_START(pwm, g_period_us[pwm_3], g_hduty_percent[pwm_3], PWM_DRV_MODE_US);


}

int	DRV_PWM_IOCTL(HANDLE handler, UINT32 cmd , VOID *data )
{
	DA16X_UNUSED_ARG(handler);
	DA16X_UNUSED_ARG(cmd);
	DA16X_UNUSED_ARG(data);
#if _ENABLE_UNUSED_
	PWM_HANDLER_TYPE	*pwm;
//	UNSIGNED			masked_evt;
	UINT32				regvalue;

	if(handler == NULL){
		return FALSE;
	}
	pwm = (I2C_HANDLER_TYPE *)handler ;

	switch(cmd){
		case I2C_SET_CONFIG:
			break;
		case I2C_GET_CONFIG:
			*((UINT32 *)data) =  HW_REG_READ32_I2C( i2c->regmap->i2c_cr0);
			break;



		case I2C_GET_STATUS:
			if( data != NULL ){
				*((UINT32 *)data) = HW_REG_READ32_I2C( i2c->regmap->i2c_sr);
			}else{
				while( (HW_REG_READ32_I2C(i2c->regmap->i2c_sr) & I2C_STATUS_TFE) != I2C_STATUS_TFE ){
					//OAL_RETRIEVE_EVENTS((OAL_EVENT_GROUP *)(i2c->mutex), (i2c_DMA_HISR_DONE) , OAL_OR, &masked_evt, OAL_SUSPEND );	// Wait
					//OAL_SET_EVENTS((OAL_EVENT_GROUP *)(i2c->mutex), ~(i2c_DMA_HISR_DONE) , OAL_AND );								// Clear
					OAL_MSLEEP(1);
				}
			}
			break;

		case I2C_SET_DMA_WR:
			if( *((UINT32 *)data) == TRUE ){
				HW_REG_OR_WRITE32_I2C( i2c->regmap->i2c_dmacr, I2C_DMA_TXEN ) ;
			}else{
				HW_REG_AND_WRITE32_I2C( i2c->regmap->i2c_dmacr, (~I2C_DMA_TXEN) ) ;
			}
			break;
        case I2C_SET_DMA_RD:
            if( *((UINT32 *)data) == TRUE ){
                HW_REG_OR_WRITE32_I2C( i2c->regmap->i2c_dmacr, I2C_DMA_RXEN ) ;
            }else{
                HW_REG_AND_WRITE32_I2C( i2c->regmap->i2c_dmacr, (~I2C_DMA_RXEN) ) ;
            }
            break;

		case I2C_GET_DMA_WR:
			*((UINT32 *)data) = HW_REG_READ32_I2C(i2c->regmap->i2c_dmacr)&I2C_DMA_TXEN;
			break;
        case I2C_GET_DMA_RD:
            *((UINT32 *)data) = HW_REG_READ32_I2C(i2c->regmap->i2c_dmacr)&I2C_DMA_RXEN;
            break;

		case I2C_SET_MAXSIZE:
//			i2c->dma_maxlli = ( ( (*((UINT32 *)data)) / DAC_SAMPLE_ALIGN(DMA_TRANSFER_SIZE) ) * 1 ) + 1 ;
			break;

		case I2C_SET_RESET:
			if( *((UINT32 *)data) == TRUE ){
				HW_REG_WRITE32_I2C( i2c->regmap->i2c_dmacr,  0 ) ;

			}else{
				HW_REG_AND_WRITE32_I2C(	i2c->regmap->i2c_cr0, (~I2C_CFG_POWER_DOWN) );


			}
			break;

		case I2C_SET_HISR_CALLACK:
			drv_i2c_callback_registry( i2c, ((UINT32 *)data)[0], ((UINT32 *)data)[1], ((UINT32 *)data)[2] );
			break;


        case I2C_SET_CHIPADDR:
            HW_REG_WRITE32_I2C(i2c->regmap->i2c_drvid, *((char*)data));
            break;

        case I2C_SET_CLOCK:
            {
                unsigned int val = (40000000 / ((*((unsigned int *) data))*2000))-1;

    			HW_REG_WRITE32_I2C( i2c->regmap->i2c_cr0,(val&0xff)<<16|0x7); //400khz set

            }
            break;

		default:
			break;
	}
#endif	//_ENABLE_UNUSED_
	return TRUE;
}

