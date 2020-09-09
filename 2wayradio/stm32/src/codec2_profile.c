/*---------------------------------------------------------------------------*\

  FILE........: codec2_profile.c
  AUTHOR......: David Rowe
  DATE CREATED: 30 May 2013

  Profiling Codec 2 operation on the STM32F4.

\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 2014 David Rowe

  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 2.1, as
  published by the Free Software Foundation.  This program is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
#include "gdb_stdio.h"
#include "codec2.h"
#include "dump.h"
#include "sine.h"
#include "machdep.h"
#include  "1278.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_it.h"


#ifdef __EMBEDDED__
#define printf gdb_stdio_printf
#define fopen gdb_stdio_fopen
#define fclose gdb_stdio_fclose
#define fread gdb_stdio_fread
#define fwrite gdb_stdio_fwrite
#endif


#define USART1_Remap_PB6_PB7

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

 //short in[320],out[320];
//static short I2S_Rx_ptr=0;
//short I2S_Rx_Buffer[320],I2S_Tx_Buffer[320];
//short I2S_Rx_Buffer_2[320],I2S_Tx_Buffer_2[320];

#define MODEM_FSK    0		
#define MODEM_LORA   1


volatile float RSSI;
char Get_NIRQ=0;
char send_over=0;
char receive=0;
 char receive_over,receive_over2; 
  char en_code,en_decode;
int loadLength=24;   //��һ��ͬ����
volatile unsigned char wireless_receive[24],wireless_send[24],uart_receive_0[252],uart_receive_1[252],uart_send_0[252],uart_send_1[252];

   uint32_t buffer0[2560], buffer1[2560],buffer2[2560],buffer3[2560];
   uint16_t buffer_encode_0[2560],buffer_decode_0[2560],buffer_encode_1[2560],buffer_decode_1[2560];  
volatile int TX_RX_Flag=1;
void Voice_Realtime_Transfer(void);
volatile int TX_start,RX_start,FM,again,TX_power;
float SX1276ReadRssi(int modem);


static void c2demo(int mode, short inputfile[], short outputfile[])
{
	   struct CODEC2 *codec2;
	    short        inbuf[320], outbuf[320];
	    unsigned char *bits;
	    int            nsam, nbit;
	   // FILE          *fin, *fout;
	    int            frame;
	    PROFILE_VAR(enc_start, dec_start);

	    codec2 = codec2_create(mode);
	    nsam = codec2_samples_per_frame(codec2);
	   // outbuf = (short*)malloc(nsam*sizeof(short));
	   // inbuf = (short*)malloc(nsam*sizeof(short));
	    nbit = codec2_bits_per_frame(codec2);
	    bits = (unsigned char*)malloc(nbit*sizeof(char));


	    /******1600bps   80��??1??************/

	   /* for(int i=0;i<200;i++)
	    {   for(int ii=0;ii<320;ii++)
	         {
	            inbuf[ii]=inputfile[i*320+ii*2];
	            inbuf[ii]=(inbuf[ii]<<8)+inputfile[i*320+ii*2+1];
	          }

        PROFILE_SAMPLE(enc_start);*/
       for(int q=0;q<100;q++)
       {  for(int i=0;i<320;i++)
	    	  inbuf[i]=inputfile[i];


        	codec2_encode(codec2, bits, inbuf);

	      	codec2_decode(codec2,outbuf, bits);

	      	for(int i=0;i<320;i++)
	      		outputfile[i]=outbuf[i];

       }

}

void Sysclock_Init(void)
{


}


 void TIM8_PWM_Init(u32 arr,u32 psc,u32 CCR2_Val)
  {
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8,ENABLE);  	//TIM3ʱ��ʹ��
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); 	//ʹ��PORTAʱ��

	GPIO_PinAFConfig(GPIOC,GPIO_PinSource7,GPIO_AF_TIM8); //GPIOA6����Ϊ��ʱ��3

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        //���ù���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//�ٶ�100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      //���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        //����
	GPIO_Init(GPIOC,&GPIO_InitStructure);              //��ʼ��PC7

	TIM_TimeBaseStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseStructure.TIM_Period=arr;   //�Զ���װ��ֵ
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;

	TIM_TimeBaseInit(TIM8,&TIM_TimeBaseStructure);//��ʼ����ʱ��3

	//��ʼ��TIM14 Channel1 PWMģʽ
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //ѡ��ʱ��ģʽ:TIM�����ȵ���ģʽ2
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //�Ƚ����ʹ��
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //�������:TIM����Ƚϼ��Ե�
        TIM_OCInitStructure.TIM_Pulse = CCR2_Val;	  //����??�����̨�2��?��???��?��??�̡�?��?3?������a��???????������?PWM
	TIM_OC2Init(TIM8, &TIM_OCInitStructure);  //����Tָ���Ĳ�����ʼ������TIM1 4OC1

	TIM_OC2PreloadConfig(TIM8, TIM_OCPreload_Enable);  //ʹ��TIM3��CCR1�ϵ�Ԥװ�ؼĴ���

      TIM_ARRPreloadConfig(TIM8,ENABLE);//ARPEʹ��
      TIM_CtrlPWMOutputs(TIM8, ENABLE);
	TIM_Cmd(TIM8, ENABLE);  //ʹ��TIM8

  } 

//ͨ�ö�ʱ��3�жϳ�ʼ��
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//��ʱ�����ʱ����㷽��:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=��ʱ������Ƶ��,��λ:Mhz
//����ʹ�õ��Ƕ�ʱ��3!
void TIM3_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///ʹ��TIM3ʱ��

        TIM_TimeBaseInitStructure.TIM_Period = arr; 	//�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//��ʼ��TIM3
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //����ʱ��3�����ж�	

	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //��ʱ��3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x00; //��ռ���ȼ�
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
         TIM_Cmd(TIM3,ENABLE); //ʹ�ܶ�ʱ��3
}


char  encode_en=0;
char aaa=0;
char buffer_select0,buffer_select1;
int I2S_Rx_ptr=0;
int voice_test;

void LED_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//��1?��GPIOA����?��

  //GPIOF9,F103?��??������??
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_SetBits(GPIOB,GPIO_Pin_0);
}





void RCC_Configuration(void)
{
	RCC_ClocksTypeDef RCC_ClockFreq;
	  /* This function fills the RCC_ClockFreq structure with the current
	  frequencies of different on chip clocks (for debug purpose) **************/
	  RCC_GetClocksFreq(&RCC_ClockFreq);


	  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);


	#ifdef USART1_Remap_PB6_PB7
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	#else
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	#endif
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

}

/*******************new****************************************/

void WM8974_I2C_write(unsigned char data1,unsigned char data2)
{
      I2C_GenerateSTART(I2C3, ENABLE);
      while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_MODE_SELECT));
      I2C_Send7bitAddress(I2C3,0x34, I2C_Direction_Transmitter);// 8974's  address
      while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
       I2C_SendData(I2C3, data1);
      while(!I2C_CheckEvent(I2C3,  I2C_EVENT_MASTER_BYTE_TRANSMITTED ));
      I2C_SendData(I2C3, data2);
       while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
}

void WM8974()
{
    volatile int aa=0x10;//0x9D;//0x91;
     volatile int bb=0x10;
    while(I2C_GetFlagStatus(I2C3, I2C_FLAG_BUSY));

    WM8974_I2C_write(0,0); //RESET
    WM8974_I2C_write(0x03,0x1F);//Power management1                     DEC1   B100011111
    WM8974_I2C_write(0x04,0x15);//Power management2                     DEC2   B000010101
    WM8974_I2C_write(0x06,0xFF);//Power management3                     DEC3   B011111111   (bit7=1 monoout Enable)
    WM8974_I2C_write(0x08,aa);//Audio Interface                       DEC4   I2S format
    WM8974_I2C_write(0x0A,0x00);//Companding ctrl                       DEC5       0x01  �ػ�   ��Ч��
    WM8974_I2C_write(0x0C,bb);//Clock Gen ctrl                        DEC6       BCLK and FRAME clock are outputs    0x04inputs
    WM8974_I2C_write(0x0E,0x0A);//Additional ctrl                       DEC7   B000001010
    WM8974_I2C_write(0x10,0x06);//GPIO                                  DEC8
    WM8974_I2C_write(0x14,0x00);//DAC Control                           DEC10
    WM8974_I2C_write(0x16,0xF3);//DAC digital Vol                       DEC11  B011110011
  //WM8974_I2C_write(0x1D,0x80);//ADC Control                           DEC14  B110000000  high pass
    WM8974_I2C_write(0x1E,0xFF);//ADC Digital Vol                       DEC15  B011111111
  //WM8974_I2C_write(0x40,0x00);//ALC control1                          DEC32
    WM8974_I2C_write(0x46,0x08);//ALC Noise Gate Control                DEC35  B000001000
  //WM8974_I2C_write(0x48,0x08);//PLL N                                 DEC36  B000001000
  //WM8974_I2C_write(0x4A,0x31);//PLL K1                                DEC37
  //WM8974_I2C_write(0x4C,0x26);//PLL K2                                DEC38
  //WM8974_I2C_write(0x4E,0xE8);//PLL K3                                DEC39
    WM8974_I2C_write(0x50,0x00);//Input control                         DEC40  0x02 monoout -10dB   0x00 0dB
    WM8974_I2C_write(0x58,0x02);//Input control                         DEC44  0x02��˷�����ͨ  0.9xAVDD
    WM8974_I2C_write(0x5A,0x20);//INP PGA gain ctrl                     DEC45  0x00=-12dB  0x01=-11.25dB 0x10=0dB    0x3F=35.25dB
  //WM8974_I2C_write(0x21,0xFF);//ALC control2                          DEC46
    WM8974_I2C_write(0x5F,0x70);//ADC Boost ctrl                        DEC47
    WM8974_I2C_write(0x62,0x00);//Output ctrl                           DEC49  B000001110
   if(FM==1)
      WM8974_I2C_write(0x64,0x02);//SPK mixer ctrl                        DEC50  0x01 DACֱͨSPK(when SX1278 work)    0x02 MICPֱͨSPK(when AT1846S work)
  else
        WM8974_I2C_write(0x64,0x01);
   WM8974_I2C_write(0x6C,0x3F);//SPK volume ctrl                       DEC54
    WM8974_I2C_write(0x70,0x02);//momo mixer control                    DEC56  byapass path to mono

    I2C_GenerateSTOP(I2C3, ENABLE);
  //I2C_AcknowledgeConfig(I2C1,ENABLE);
}

void I2C3_Congiguration(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
   I2C_InitTypeDef  I2C_InitStructure;

  RCC_AHB1PeriphClockCmd(  RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOC , ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C3,ENABLE);


 GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_I2C3);
 GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_I2C3);



         GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
         GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
         GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
         GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
         GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
         GPIO_Init(GPIOA, &GPIO_InitStructure);

         GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
         GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
         GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
         GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
         GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
         GPIO_Init(GPIOC, &GPIO_InitStructure);


           /* I2C ���� */
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 =0x0A;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable ;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = 200000;

  /* ʹ�� I2C1 */
    I2C_Cmd(I2C3, ENABLE);

  /* I2C1 ��ʼ�� */
     I2C_Init(I2C3, &I2C_InitStructure);


}



 void I2S3_Mode_Config(const uint16_t _usStandard, const uint16_t _usWordLen, const uint32_t _usAudioFreq  )
    {
      I2S_InitTypeDef I2S_InitStructure;
      uint32_t n = 0;
      FlagStatus status = RESET;
      /**
      * For I2S mode, make sure that either:
      *   - I2S PLL is configured using the functions RCC_I2SCLKConfig
      *     (RCC_I2S2CLKSource_PLLI2S),
      *   RCC_PLLI2SCmd(ENABLE) and RCC_GetFlagStatus(RCC_FLAG_PLLI2SRDY).
      */
      RCC_I2SCLKConfig(RCC_I2S2CLKSource_PLLI2S);
      RCC_PLLI2SCmd(ENABLE);
      for (n = 0; n < 500; n++) {
        status = RCC_GetFlagStatus(RCC_FLAG_PLLI2SRDY);
        if (status == 1)break;
      }
      
      /* �� I2S3 APB1 ʱ�� */
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);//xwl add
  	 
    
      //RCC_APB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOC, ENABLE);
      
      /* ��λ SPI3 ���赽ȱʡ״̬ */
      SPI_I2S_DeInit(SPI3);
      /* I2S3 �������� */
      /* ���� I2S ����ģʽ */
      I2S_InitStructure.I2S_Mode =I2S_Mode_MasterTx  ;
      /* �ӿڱ�׼ */
      I2S_InitStructure.I2S_Standard = _usStandard;
      /* ���ݸ�ʽ��16bit */
      I2S_InitStructure.I2S_DataFormat = _usWordLen;
      /* ��ʱ��ģʽ */
      I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
      /* ��Ƶ����Ƶ�� */
      I2S_InitStructure.I2S_AudioFreq = _usAudioFreq;
      I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;
      I2S_Init(SPI3, &I2S_InitStructure);
      /* ʹ�� SPI3/I2S3 ���� */
    
      
    
        I2S_Cmd(SPI3, ENABLE);
      //I2S_Cmd(I2S3ext, ENABLE);//xwl add
    }






#define WM8974_I2Sx_SPI                          SPI3
#define I2Sx_TX_DMA_STREAM                 DMA1_Stream5
#define I2Sx_TX_DMA_CHANNEL                DMA_Channel_0
#define I2Sx_DMA_CLK                     RCC_AHB1Periph_DMA1
#define I2Sx_TX_DMA_STREAM_IRQn         DMA1_Stream5_IRQn




#define WM8974_I2Sx_ext                          I2S3ext
#define I2Sxext_RX_DMA_STREAM                    DMA1_Stream0
#define I2Sxext_RX_DMA_CHANNEL                   DMA_Channel_3
#define I2Sxext_RX_DMA_STREAM_IRQn               DMA1_Stream0_IRQn
#define I2Sxext_RX_DMA_IT_TCIF                   DMA_IT_HTIF5

 void I2S3ext_Mode_Config(const uint16_t _usStandard,
                          const uint16_t _usWordLen,const uint32_t _usAudioFreq)
 {
   
   /* �� I2S3 APB1 ʱ�� */
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);//xwl add
   
   I2S_InitTypeDef I2Sext_InitStructure;
   /* I2S2 �������� */
   /* ���� I2S ����ģʽ */
   I2Sext_InitStructure.I2S_Mode = I2S_Mode_MasterTx; 
   /* �ӿڱ�׼ */
   I2Sext_InitStructure.I2S_Standard = _usStandard;
    /* ���ݸ�ʽ��16bit */
   I2Sext_InitStructure.I2S_DataFormat = _usWordLen;
   /* ��ʱ��ģʽ */
   I2Sext_InitStructure.I2S_MCLKOutput =I2S_MCLKOutput_Disable ;
   /* ��Ƶ����Ƶ�� */
   I2Sext_InitStructure.I2S_AudioFreq = _usAudioFreq;
   I2Sext_InitStructure.I2S_CPOL = I2S_CPOL_Low;
   I2S_FullDuplexConfig(WM8974_I2Sx_ext, &I2Sext_InitStructure);
   /* ʹ�� SPI2/I2S2 ���� */

   I2S_Cmd(WM8974_I2Sx_ext, ENABLE); 
 }


#define WM8974_LRC_GPIO_CLK        RCC_AHB1Periph_GPIOA
#define WM8974_BCLK_GPIO_CLK       RCC_AHB1Periph_GPIOC
#define WM8974_ADCDAT_GPIO_CLK     RCC_AHB1Periph_GPIOC
#define WM8974_DACDAT_GPIO_CLK     RCC_AHB1Periph_GPIOC
#define WM8974_MCLK_GPIO_CLK       RCC_AHB1Periph_GPIOC


#define WM8974_LRC_PIN           GPIO_Pin_15    //PA15
#define WM8974_BCLK_PIN          GPIO_Pin_10    //PC10
#define WM8974_ADCDAT_PIN        GPIO_Pin_11   //PC11
#define WM8974_DACDAT_PIN        GPIO_Pin_12   //PC12
#define WM8974_MCLK_PIN          GPIO_Pin_7     //PC7


#define WM8974_LRC_PORT       GPIOA
#define WM8974_BCLK_PORT      GPIOC
#define WM8974_ADCDAT_PORT    GPIOC
#define WM8974_DACDAT_PORT    GPIOC
#define WM8974_MCLK_PORT      GPIOC

#define WM8974_LRC_SOURCE     GPIO_PinSource15
#define WM8974_BCLK_SOURCE      GPIO_PinSource10
#define WM8974_ADCDAT_SOURCE    GPIO_PinSource11
#define WM8974_DACDAT_SOURCE    GPIO_PinSource12
#define WM8974_MCLK_SOURCE      GPIO_PinSource7

#define WM8974_LRC_AF            GPIO_AF_SPI3
#define WM8974_BCLK_AF           GPIO_AF_SPI3
#define WM8974_ADCDAT_AF         GPIO_AF_SPI3  //?
#define WM8974_DACDAT_AF         GPIO_AF_SPI3
#define WM8974_MCLK_AF           GPIO_AF_SPI3

/**
	* @brief  ����GPIO��������codecӦ��
	* @param  ��
	* @retval ��
	*/
void I2S_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

/**
	* I2S���ߴ�����Ƶ���ݿ���
	* WM8974_LRC    -> PA15/I2S2_WS
	* WM8974_BCLK   -> PC10/I2S2_CK
	* WM8974_ADCDAT -> PC11/I2S2ext_SD
	* WM8974_DACDAT -> PC12/I2S2_SD
	* WM8974_MCLK   -> PC7/I2S2_MCK
	*/
	/* Enable GPIO clock */
	RCC_AHB1PeriphClockCmd(WM8974_LRC_GPIO_CLK|WM8974_BCLK_GPIO_CLK| \
                         WM8974_ADCDAT_GPIO_CLK|WM8974_DACDAT_GPIO_CLK| \
	                       WM8974_MCLK_GPIO_CLK, ENABLE);
         RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3,ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_InitStructure.GPIO_Pin = WM8974_LRC_PIN;
	GPIO_Init(WM8974_LRC_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = WM8974_BCLK_PIN;
	GPIO_Init(WM8974_BCLK_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = WM8974_ADCDAT_PIN;
	GPIO_Init(WM8974_ADCDAT_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = WM8974_DACDAT_PIN;
	GPIO_Init(WM8974_DACDAT_PORT, &GPIO_InitStructure);
        
    

	GPIO_InitStructure.GPIO_Pin = WM8974_MCLK_PIN;
	GPIO_Init(WM8974_MCLK_PORT, &GPIO_InitStructure);

	/* Connect pins to I2S peripheral  */
	GPIO_PinAFConfig(WM8974_LRC_PORT,    WM8974_LRC_SOURCE,    WM8974_LRC_AF);
	GPIO_PinAFConfig(WM8974_BCLK_PORT,   WM8974_BCLK_SOURCE,   WM8974_BCLK_AF);
	GPIO_PinAFConfig(WM8974_ADCDAT_PORT, WM8974_ADCDAT_SOURCE, WM8974_ADCDAT_AF);
	GPIO_PinAFConfig(WM8974_DACDAT_PORT, WM8974_DACDAT_SOURCE, WM8974_DACDAT_AF);
	GPIO_PinAFConfig(WM8974_MCLK_PORT,   WM8974_MCLK_SOURCE,   WM8974_MCLK_AF);
        //GPIO_PinAFConfig(WM8974_MCLK_PORT,   WM8974_MCLK_SOURCE,   GPIO_AF_TIM8);

}



#define CLOCK 72/8 //����?��=72M

void delay_us(unsigned int us)
{
	u8 n;

	while(us--)for(n=0;n<CLOCK;n++);
}


void delay_ms(unsigned int ms)
{
	while(ms--)delay_us(1000);
}

void SPI1_GPIO_Init(void)
{
  SPI_InitTypeDef  SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);


  /*SCK,MOSI*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_7;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//����
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  
    /*NSS*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  
  
  /*MISO*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//����
    GPIO_InitStructure.GPIO_OType = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  
 
  GPIO_PinAFConfig(GPIOA,GPIO_PinSource5,GPIO_AF_SPI1); //
  GPIO_PinAFConfig(GPIOA,GPIO_PinSource6,GPIO_AF_SPI1); //
  GPIO_PinAFConfig(GPIOA,GPIO_PinSource7,GPIO_AF_SPI1); //
 
  /* SPI1 configuration */
  //SPI_Cmd(SPI1,DISABLE);
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,ENABLE);//��λSPI1
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,DISABLE);//ֹͣ��λSPI1

  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS =  SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI1, &SPI_InitStructure);


  /* Enable SPI1  */
  SPI_Cmd(SPI1, ENABLE);
  }



   
void RF_switch1278()
{
   GPIO_SetBits(GPIOC, GPIO_Pin_4); //RF_switch to 1278
}
void RF_switch1846()
{
   GPIO_ResetBits(GPIOC, GPIO_Pin_4); //RF_switch to 1846
}


void PA_En()
{
  
 GPIO_ResetBits(GPIOB, GPIO_Pin_4); //PA_EN //
  GPIO_SetBits(GPIOD, GPIO_Pin_2); //LNA_En
}
void PaOff()
{
  GPIO_SetBits(GPIOB, GPIO_Pin_4); //PA_off
}

void LNA_En()
{
 
   //GPIO_SetBits(GPIOB, GPIO_Pin_4); //PA_off
  GPIO_ResetBits(GPIOD, GPIO_Pin_2); //LNA_En
}

void PAandLNA_En()
{
 
GPIO_ResetBits(GPIOB, GPIO_Pin_4); //PA_EN 
  GPIO_ResetBits(GPIOD, GPIO_Pin_2); //LNA_En
}


void PA_Trans_En()
{
   GPIO_SetBits(GPIOB, GPIO_Pin_3); //PA_Trans_En
   GPIO_ResetBits(GPIOC, GPIO_Pin_5); //LNA_Trans_En
  
}

void LNA_Trans_En()
{
  GPIO_ResetBits(GPIOB, GPIO_Pin_3); //PA_Trans_En
  GPIO_SetBits(GPIOC, GPIO_Pin_5); //LNA_Trans_En
  
}       
volatile char start,start2;

unsigned int arr=700;//658  3.11Hz
unsigned char  communication_num=0;


void GPIO_EXTI_Callback( uint16_t GPIO_Pin)
{  
   if(GPIO_Pin== GPIO_Pin_11)//DIO0
   {
         sx1276_LoRaClearIrq();         
           if(send_over)           
		{   send_over=0;   
		    receive=1;   //������������                                                                  
               } 
                  
         else if(receive)     
		  {   u8  test;
                    //TIM3->ARR= arr;   //tim3 again initial value                    
                     RSSI = SX1276ReadRssi( MODEM_LORA);
                  start=1;                  
                    receive=0; // �򿪷���ʱ�ָ� receive=0;                    
                   receive_over=1;
                   communication_num=0;
                   
		   }     
    }
    else if(GPIO_Pin==GPIO_Pin_0)
    {
       TX_RX_Flag=!TX_RX_Flag;FM++,again=0;TX_power++;
    }
}



volatile char wireless_transport_en=0;


void tt(int ENABLE_DISABLE)
{
  GPIO_InitTypeDef  GPIO_InitStructure; 
  NVIC_InitTypeDef   NVIC_InitStructure;
  EXTI_InitTypeDef   EXTI_InitStructure;
 
  NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;//�ⲿ�ж�11
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;//��ռ���ȼ�0
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;//�����ȼ�2
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE_DISABLE;//ʹ���ⲿ�ж�ͨ��
  NVIC_Init(&NVIC_InitStructure);//����
}	    


 
//�ⲿ�жϳ�ʼ������
//��ʼ��PE2~4,PA0Ϊ�ж�����.
void EXTIX_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOB, &GPIO_InitStructure);


  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); 
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  //GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd =GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  NVIC_InitTypeDef   NVIC_InitStructure;
  EXTI_InitTypeDef   EXTI_InitStructure;
 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);//ʹ��SYSCFGʱ��
	
 
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource11);//PB11 ���ӵ��ж���11
        SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource0);  
	
  /* ����EXTI_Line0 */
  EXTI_InitStructure.EXTI_Line = EXTI_Line11;//LINE11
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//�ж��¼�
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //�����ش��� 
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;//ʹ��LINE11
  EXTI_Init(&EXTI_InitStructure);//����
	
  EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
  
  
 
  NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;//�ⲿ�ж�11
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;//��ռ���ȼ�0
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;//�����ȼ�2
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//ʹ���ⲿ�ж�ͨ��
  NVIC_Init(&NVIC_InitStructure);//����

  /* Enable the USARTx Interrupt */
   NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_Init(&NVIC_InitStructure);
}
  

void PA_LNA_configure(void)
{
   GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); 
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

}



/*********************************************************************/
#define WM8974_I2Sx_SPI                          SPI3
#define I2Sx_TX_DMA_STREAM                 DMA1_Stream5
#define I2Sx_TX_DMA_CHANNEL                DMA_Channel_0 
#define I2Sx_DMA_CLK                     RCC_AHB1Periph_DMA1
#define I2Sx_TX_DMA_STREAM_IRQn         DMA1_Stream5_IRQn 
void I2S3_TX_DMA_Init(const uint32_t *buffer0,const uint32_t *buffer1,const uint32_t num)  
{  
  NVIC_InitTypeDef   NVIC_InitStructure;  
  DMA_InitTypeDef  DMA_InitStructure;  
  
  RCC_AHB1PeriphClockCmd(I2Sx_DMA_CLK,ENABLE);//DMA1 ʱ��ʹ��  
  DMA_DeInit(I2Sx_TX_DMA_STREAM); 
  //�ȴ� DMA1_Stream4 ������ 
  while (DMA_GetCmdStatus(I2Sx_TX_DMA_STREAM) != DISABLE) {} ;
  //��� DMA1_Stream5 �������жϱ�־ 
  DMA_ClearITPendingBit(I2Sx_TX_DMA_STREAM,     
                        DMA_IT_FEIF5|DMA_IT_DMEIF5|DMA_IT_TEIF5|DMA_IT_HTIF5|DMA_IT_TCIF5);   
 
  /* ���� DMA Stream */ 
  //ͨ�� 0 SPIx_TX ͨ�� 
  DMA_InitStructure.DMA_Channel = I2Sx_TX_DMA_CHANNEL; 
  //�����ַΪ:(u32)&SPI2->DR
  DMA_InitStructure.DMA_PeripheralBaseAddr =(uint32_t)&WM8974_I2Sx_SPI->DR;   
   //DMA �洢�� 0 ��ַ 
   DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)buffer0; 
   //�洢��������ģʽ 
   DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
   //���ݴ����� 
   DMA_InitStructure.DMA_BufferSize = num; 
   //���������ģʽ 
   DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
   //�洢������ģʽ
   DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
   //�������ݳ���:16 λ 
   DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; 
   //�洢�����ݳ��ȣ�16 λ 
   DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; 
   // ʹ��ѭ��ģʽ 
   DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; 
   //�����ȼ� 
   DMA_InitStructure.DMA_Priority = DMA_Priority_High;
   //��ʹ�� FIFO ģʽ 
   DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable; 
   DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull; 
    //����ͻ�����δ��� 
   DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
   //�洢��ͻ�����δ���
   DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; 
   DMA_Init(I2Sx_TX_DMA_STREAM, &DMA_InitStructure);//��ʼ�� DMA Stream
   //˫����ģʽ���� 
   //DMA_DoubleBufferModeConfig(I2Sx_TX_DMA_STREAM,(uint32_t)buffer0, DMA_Memory_0);
   DMA_DoubleBufferModeConfig(I2Sx_TX_DMA_STREAM,(uint32_t)buffer1, DMA_Memory_1); 
   //˫����ģʽ���� 
   DMA_DoubleBufferModeCmd(I2Sx_TX_DMA_STREAM,ENABLE); 
   //������������ж� 
    DMA_ITConfig(I2Sx_TX_DMA_STREAM,DMA_IT_TC,ENABLE);
    
   //SPI3 TX DMA ����ʹ��. 
   SPI_I2S_DMACmd(WM8974_I2Sx_SPI,SPI_I2S_DMAReq_Tx,ENABLE); 
   NVIC_InitStructure.NVIC_IRQChannel = I2Sx_TX_DMA_STREAM_IRQn; 
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
   NVIC_Init(&NVIC_InitStructure);
}

#define WM8974_I2Sx_ext                          I2S3ext  
#define I2Sxext_RX_DMA_STREAM                    DMA1_Stream0
#define I2Sxext_RX_DMA_CHANNEL                   DMA_Channel_3
#define I2Sxext_RX_DMA_STREAM_IRQn               DMA1_Stream0_IRQn     
#define I2Sxext_RX_DMA_IT_TCIF                   DMA_IT_HTIF5

 

 void I2Sxext_RX_DMA_Init(const uint32_t *buffer0,const uint32_t *buffer1, const uint32_t num)  
 {  
   NVIC_InitTypeDef   NVIC_InitStructure;   
    DMA_InitTypeDef  DMA_InitStructure;  
    RCC_AHB1PeriphClockCmd(I2Sx_DMA_CLK,ENABLE);  
    DMA_DeInit(I2Sxext_RX_DMA_STREAM); 
    while (DMA_GetCmdStatus(I2Sxext_RX_DMA_STREAM) != DISABLE) {} ;
    
    DMA_ClearITPendingBit(I2Sxext_RX_DMA_STREAM, 
                          DMA_IT_FEIF3|DMA_IT_DMEIF3|DMA_IT_TEIF3|DMA_IT_HTIF3|DMA_IT_TCIF3);  
    /* ���� DMA Stream */ 
    DMA_InitStructure.DMA_Channel = I2Sxext_RX_DMA_CHANNEL; 
    DMA_InitStructure.DMA_PeripheralBaseAddr =(uint32_t)&WM8974_I2Sx_ext->DR; 
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)buffer0; 
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory; 
    DMA_InitStructure.DMA_BufferSize = num; 
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord ;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; 
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; 
    DMA_InitStructure.DMA_Priority = DMA_Priority_High; 
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable ; 
    DMA_InitStructure.DMA_FIFOThreshold =  DMA_FIFOThreshold_1QuarterFull; 
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_MemoryBurst_Single; 
    DMA_Init(I2Sxext_RX_DMA_STREAM, &DMA_InitStructure); 
   
     //˫����ģʽ���� 
    DMA_DoubleBufferModeConfig(I2Sxext_RX_DMA_STREAM, (uint32_t)buffer1,DMA_Memory_1); 
    //˫����ģʽ���� 
    DMA_DoubleBufferModeCmd(I2Sxext_RX_DMA_STREAM,ENABLE);
    //������������ж� 
    DMA_ITConfig(I2Sxext_RX_DMA_STREAM,DMA_IT_TC,DISABLE);
    //DMA ����ʹ��. 
    SPI_I2S_DMACmd(WM8974_I2Sx_ext,SPI_I2S_DMAReq_Rx,ENABLE); 
    
    NVIC_InitStructure.NVIC_IRQChannel = I2Sxext_RX_DMA_STREAM_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; 
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1; 
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE; 
    NVIC_Init(&NVIC_InitStructure); 
 }





/*#define I2Sx_TX_DMA_IT_TCIF           DMA_IT_HTIF0

 //I2S DMA �ص�����

void (*I2S_DMA_TX_Callback)(void);

void I2Sx_TX_DMA_STREAM_IRQFUN(void)
{
  //DMA ������ɱ�־
  if (DMA_GetITStatus(I2Sx_TX_DMA_STREAM,I2Sx_TX_DMA_IT_TCIF)==SET)
  {
    //�� DMA ������ɱ�׼

    DMA_ClearITPendingBit(I2Sx_TX_DMA_STREAM,I2Sx_TX_DMA_IT_TCIF);
    //ִ�лص�����,��ȡ���ݵȲ����������洦��
    I2S_DMA_TX_Callback();
  }
}*/

int LED_flag=0;
//��ʱ��3�жϷ�����  �Զ�����
void TIM3_IRQHandler(void)
{
  if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //����ж�
  {
    TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //����жϱ�־λ
   // TIM_Cmd(TIM3,DISABLE); //�رն�ʱ��3
       start2=1;                  
     receive=0;                
      receive_over=1;
    
     
    if(LED_flag==0)
    {
    //   GPIO_SetBits(GPIOB,GPIO_Pin_0); 
       LED_flag=1;
    }
    else
    {    LED_flag=0;
     //  GPIO_ResetBits(GPIOB,GPIO_Pin_0); 
    
    } 
   
  }}

int www;

//DMA �����ж�
void DMA1_Stream5_IRQHandler (void)          //  send  interrupt
{
  
if (DMA_GetITStatus(DMA1_Stream5, DMA_IT_TCIF5) == SET)
    {    
     //  DMA_ClearFlag(DMA1_Stream5, DMA_IT_TC);      
         DMA_ClearITPendingBit(DMA1_Stream5, DMA_IT_TCIF5);
      /*  www=!www;  
        if(www==1)
           GPIO_SetBits(GPIOB, GPIO_Pin_0);
        else if(www==0)
           GPIO_ResetBits(GPIOB, GPIO_Pin_0);
         */  
        //TIM_Cmd(TIM3,ENABLE); //ʹ�ܶ�ʱ��3      
     buffer_select1=!buffer_select1; 
      en_code=1;
         buffer_select0=!buffer_select0; 
      if( buffer_select1==1)
         { for(int i=0;i<1280;i++)
            {                        
             // buffer3[i]=0;
              buffer2[i]=buffer_decode_0[i];  
               buffer_encode_0[i]=buffer0[i];
            }           
         }
         else   if( buffer_select1==0)     
         {  for(int i=0;i<1280;i++)
             {  
             //  buffer2[i]=0; 
               buffer3[i]=buffer_decode_1[i]; 
                buffer_encode_1[i]=buffer1[i]; 
             }
         }
 
    }
}

void DMA1_Stream0_IRQHandler (void)   //receive  interrupt
{
  
/*if (DMA_GetITStatus(DMA1_Stream0, DMA_IT_TCIF5) == SET)
    {    
           
         DMA_ClearITPendingBit(DMA1_Stream0, DMA_IT_TCIF5);
      
       en_code=1;
      buffer_select0=!buffer_select0; 
      if( buffer_select0==1)
         { for(int i=0;i<1280;i++)
            {                      
              buffer_encode_0[i]=buffer0[i];
            }           
         }
         else   if( buffer_select0==0)      
         {  for(int i=0;i<1280;i++)
             {
               buffer_encode_1[i]=buffer1[i]; 
             }
         }
 
    }*/
}


void NVIC_Config()
{
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x03;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;
    NVIC_InitStructure.NVIC_IRQChannel  = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        
    NVIC_Init(&NVIC_InitStructure);
}

void USART_Gpio_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA  , ENABLE);
    
    //PA2->TX  PA3->Rx
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2);
}

void  usart_configure(void)
{
    NVIC_Config();
    USART_Gpio_Config();
  
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);
    USART_InitTypeDef USART_InitStructure;
    
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    
    USART_Init(USART2,&USART_InitStructure);   
    USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);
    
    USART_Cmd(USART2,ENABLE);
  
}

char uart_trans_En=0;



/***************************************************/

char test=0;

 

//I2S2 ��ʼ�� //���� I2S_Standard: @ref SPI_I2S_Standard  I2S ��׼, //���� I2S_Mode: @ref SPI_I2S_Mode //���� I2S_Clock_Polarity    @ref SPI_I2S_Clock_Polarity: //���� I2S_DataFormat��@ref SPI_I2S_Data_Format : 
/*void I2S2_Init(u16 I2S_Standard,u16 I2S_Mode,u16 I2S_Clock_Polarity,u16 I2S_DataFormat) 
{      I2S_InitTypeDef I2S_InitStructure;  
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);//ʹ�� SPI2 ʱ��  
      RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2,ENABLE); //��λ SPI2 
      RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2,DISABLE);//������λ     
      I2S_InitStructure.I2S_Mode=I2S_Mode;//IIS ģʽ  
      I2S_InitStructure.I2S_Standard=I2S_Standard;//IIS ��׼ 
      I2S_InitStructure.I2S_DataFormat=I2S_DataFormat;//IIS ���ݳ��� 
      I2S_InitStructure.I2S_MCLKOutput=I2S_MCLKOutput_Disable;//��ʱ�������ֹ  
      I2S_InitStructure.I2S_AudioFreq=I2S_AudioFreq_Default;//IIS Ƶ������ 
      I2S_InitStructure.I2S_CPOL=I2S_Clock_Polarity;//����״̬ʱ�ӵ�ƽ  
      I2S_Init(SPI2,&I2S_InitStructure);//��ʼ�� IIS 
      SPI_I2S_DMACmd(SPI2,SPI_I2S_DMAReq_Tx,ENABLE);//SPI2 TX DMA ����ʹ��.     
      I2S_Cmd(SPI2,ENABLE);//SPI2 I2S EN ʹ��. 
} */
   
/*u8 I2S2_SampleRate_Set(u32 samplerate) 
{   u8 i=0; 
    u32 tempreg=0;
    samplerate/=10;//��С 10 ��    
    for(i=0;i<(sizeof(I2S_PSC_TBL)/10);i++)//�����Ĳ������Ƿ����֧�� 
    {   if(samplerate==I2S_PSC_TBL[i][0])break;  } 
    RCC_PLLI2SCmd(DISABLE);//�ȹر� PLLI2S  
    if(i==(sizeof(I2S_PSC_TBL)/10))
      return 1;//�ѱ���Ҳ�Ҳ���  
    RCC_PLLI2SConfig((u32)I2S_PSC_TBL[i][1],(u32)I2S_PSC_TBL[i][2]);     //���� I2SxCLK ��Ƶ��(x=2)  ���� PLLI2SN PLLI2SR 
    RCC->CR|=1<<26;     //���� I2S ʱ��  
    while((RCC->CR&1<<27)==0);  //�ȴ� I2S ʱ�ӿ����ɹ�.   
    tempreg=I2S_PSC_TBL[i][3]<<0; //���� I2SDIV 
    tempreg|=I2S_PSC_TBL[i][3]<<8; //���� ODD λ  
    tempreg|=1<<9;     //ʹ�� MCKOE λ,��� MCK  
    SPI2->I2SPR=tempreg;   //���� I2SPR �Ĵ���  
    return 0; 
} */

void TX()
{ 
   PA_En();        
  PA_Trans_En();
  wireless_transport_en=0;             	           
  sx1276_LoRaEntryTx();  //����02ELS����ʱע�͵�
  sx1276_LoRaTxPacket();
  send_over=1;
  receive_over=0;
}

void I2C_write(char addr,char data1,char data0)
{
       I2C_GenerateSTART(I2C3, ENABLE);
   while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_MODE_SELECT)); 
  I2C_Send7bitAddress(I2C3,0xE2, I2C_Direction_Transmitter);//
    while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));  
      I2C_SendData(I2C3, addr);                       
    while(!I2C_CheckEvent(I2C3,  I2C_EVENT_MASTER_BYTE_TRANSMITTED )); 
    I2C_SendData(I2C3, data1);   
    while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_SendData(I2C3, data0);     
    while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
}

void AT1846S()
{   unsigned int test1846_0,test1846_1;  
  while(1)
 {  
    if(again==0)
  {  again=1;
    
    while(I2C_GetFlagStatus(I2C3, I2C_FLAG_BUSY)); 
    
   { I2C_write(0x30,0x00,0x01);
    delay_ms(500);
    I2C_write(0x30,0x00,0x04);
    I2C_write(0x04,0x0F,0xD1);
    I2C_write(0x31,0x00,0x31);
    I2C_write(0x33,0x44,0xA5);
    I2C_write(0x34,0x2B,0x87);
    I2C_write(0x41,0x47,0x0F);
    I2C_write(0x44,0x0D,0xFF);
    I2C_write(0x47,0x7F,0xFF);
    I2C_write(0x4F,0x2C,0x62);
    I2C_write(0x53,0x00,0x94);
    I2C_write(0x55,0x00,0x81);
    I2C_write(0x56,0x0B,0x22);
    I2C_write(0x57,0x1C,0x00);
    I2C_write(0x5A,0x0E,0xDB);
    I2C_write(0x60,0x10,0x1E);
    I2C_write(0x63,0x16,0xAD);
    I2C_write(0x30,0x40,0xA4);
    delay_ms(500);
    I2C_write(0x30,0x40,0xA6);
    delay_ms(500);
    I2C_write(0x30,0x40,0x06);
    }
    
     
     /*********У׼***************/
     I2C_write(0x27,0x74,0xA4);
      delay_ms(10);
     I2C_write(0x27,0x7C,0xA4);
     I2C_write(0x27,0x6C,0xA4);
       
   
   /********խ������**********/  
    
    /*    I2C_write(0x15,0x11,0x00);
        I2C_write(0x32,0x44,0x95);
         I2C_write(0x3A,0x40,0xC3);
          I2C_write(0x3C,0x04,0x07);
          I2C_write(0x3F,0x28,0xD0);
           I2C_write(0x48,0x20,0x3E);
            I2C_write(0x60,0x1B,0xB7);
             I2C_write(0x62,0x14,0x25);
              I2C_write(0x65,0x24,0x94);
               I2C_write(0x66,0xEB,0x2E);
      I2C_write(0x7F,0x00,0x01);
       I2C_write(0x06,0x00,0x14);
        I2C_write(0x07,0x02,0x0C);
         I2C_write(0x08,0x02,0x14);
          I2C_write(0x09,0x03,0x0C);
           I2C_write(0x0A,0x03,0x14);
            I2C_write(0x0B,0x03,0x24);
             I2C_write(0x0C,0x03,0x44);
              I2C_write(0x0D,0x13,0x44);
               I2C_write(0x0E,0x1B,0x44);
                I2C_write(0x0F,0x3F,0x44);
                 I2C_write(0x12,0xE0,0xEB);
                  I2C_write(0x7F,0x00,0x00);*/
     
      /********�������**********/  
    
        I2C_write(0x15,0x1F,0x00);
        I2C_write(0x32,0x75,0x64);
         I2C_write(0x3A,0x44,0xC3);
          I2C_write(0x3C,0x17,0x2C);
          I2C_write(0x3F,0x29,0xD2);
           I2C_write(0x48,0x21,0x41);
           I2C_write(0x59,0x0A,0x50);
             I2C_write(0x62,0x37,0x67);
              I2C_write(0x65,0x24,0x8A);
               I2C_write(0x66,0xFF,0x2E);
      I2C_write(0x7F,0x00,0x01);
       I2C_write(0x06,0x00,0x24);
        I2C_write(0x07,0x02,0x14);
         I2C_write(0x08,0x02,0x24);
          I2C_write(0x09,0x03,0x14);
           I2C_write(0x0A,0x03,0x24);
            I2C_write(0x0B,0x03,0x44);
             I2C_write(0x0C,0x03,0x84);
              I2C_write(0x0D,0x13,0x84);
               I2C_write(0x0E,0x1B,0x84);
                I2C_write(0x0F,0x3F,0x84);
                 I2C_write(0x12,0xE0,0xEB);
                  I2C_write(0x7F,0x00,0x00);
   
      if(TX_RX_Flag==0)     
      {  
          PaOff();
         LNA_En();        
         LNA_Trans_En();
        
        I2C_write(0x30,0x00,0x00); //shut down TX or Rx     
        I2C_write(0x05,0x87,0x63);
        I2C_write(0x29,0x00,0x69);
       I2C_write(0x2A,0x1A,0x40);//430.5M
   //  I2C_write(0x41,0x47,0x1F);
      /*********����**********������WM8974 DEC50=0x02***/
        I2C_write(0x49,0x0E,0xEF);     //sq level
        I2C_write(0x30,0x70,0x06);   //RX  
         I2C_write(0x30,0x70,0x2E);   //RX      I2C_write(0x30,0x70,0x2E); //SQ       
         I2C_write(0x44,0x0D,0xAA);//volume
         I2C_write(0x58,0xFF,0x40);  //filter
        GPIO_SetBits(GPIOB, GPIO_Pin_0);
      }
      
     else if(TX_RX_Flag==1)
       {  
          
         
          PA_En();        
          PA_Trans_En();
         I2C_write(0x30,0x00,0x00); //shut down TX or Rx     
        I2C_write(0x05,0x87,0x63);
        I2C_write(0x29,0x00,0x69);
       I2C_write(0x2A,0x1A,0xB0);//0x2A,0x1A,0xA0 430.501M
   //  I2C_write(0x41,0x47,0x1F);
     
    /*********����******/ 
      I2C_write(0x0A,0x7B,0x7F);//  0x7B=7dBm 3.3V  
      I2C_write(0x30,0x70,0x06);   //TX
       I2C_write(0x30,0x70,0x46);   //TX
          GPIO_ResetBits(GPIOB, GPIO_Pin_0);
      }
    
 /************************��ȡ�Ĵ���ֵ****************************/ 
 /*   I2C_GenerateSTOP(I2C3, ENABLE);
     while(I2C_GetFlagStatus(I2C3, I2C_FLAG_BUSY)); 
     I2C_GenerateSTART(I2C3, ENABLE);
     while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_MODE_SELECT)); 
  I2C_Send7bitAddress(I2C3,0xE2, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED )); 
     I2C_SendData(I2C3, 0x3A);        
    while(!I2C_CheckEvent(I2C3,I2C_EVENT_MASTER_BYTE_TRANSMITTED ));
      I2C_GenerateSTART(I2C3, ENABLE);
       while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_MODE_SELECT));
        I2C_Send7bitAddress(I2C3, 0xE3, I2C_Direction_Receiver);
           while(!(I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_RECEIVED)));
             test1846_0 = I2C_ReceiveData(I2C3);
             while(!(I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_RECEIVED)));
             test1846_1 = I2C_ReceiveData(I2C3);
    */  
    
     I2C_GenerateSTOP(I2C3, ENABLE);
     // I2C_AcknowledgeConfig(I2C1,DISABLE);

  }
 }
}

float SX1276ReadRssi(int modem)
{
	float rssi; u8 temp;
        float  PacketRssi=113;

	switch( modem )
	{
	case MODEM_FSK:
		rssi = -(SX1276Read( LR_RegPktRssiValue,&temp ) >> 1 );
		break;
	case MODEM_LORA:		
		{
		  rssi = -164 + SX1276Read( LR_RegPktRssiValue ,&temp);
		}
		break;
	default:
		rssi = -1;
		break;
	}
       // rssi = -164+16/15 * PacketRssi;        
	return rssi;
}


int main(int argc, char *argv[]) {

         
	 int i;
     //    usart_configure();
      //   while(1);

	PA_LNA_configure();
        EXTIX_Init(); 
        LED_Init();
        I2C3_Congiguration();
        delay_ms(2000);
        I2S_GPIO_Config();
		  // TIM3_Int_Init(51,200) ;  //8KHz 		    
		   TIM8_PWM_Init(5,13,3); 
        SPI1_GPIO_Init();   
       // TX();
      // TIM3_Int_Init(2000,20000); //(658,20000)
        
    
     /* I2S3_Init(I2S_Standard_Phillips,I2S_Mode_MasterTx,I2S_CPOL_Low, I2S_DataFormat_16b);
        I2S2ext_Init(I2S_Standard_Phillips,I2S_Mode_SlaveRx,I2S_CPOL_Low, I2S_DataFormat_16b);//�����ֱ�׼,�ӻ�����,ʱ�ӵ͵�ƽ��Ч,16 λ֡����  
        I2S3_SampleRate_Set(8000); //���ò�����  */
       /**********I2S ��ģʽ DMA�ж�Ϊɶ�ӱ���**************/ 
        
 /*xx:   DMA_Cmd(I2Sxext_RX_DMA_STREAM,DISABLE);//�ر�DMA RX����,�ر�¼��  
           DMA_Cmd(I2Sx_TX_DMA_STREAM,DISABLE);//�ر�DMA TX����,�رղ���   
           delay_ms(10);
        
     I2S3_Mode_Config(I2S_Standard_Phillips  ,I2S_DataFormat_16b,I2S_AudioFreq_8k); 
    I2S3ext_Mode_Config(I2S_Standard_Phillips ,I2S_DataFormat_16b,I2S_AudioFreq_8k);        
     I2Sxext_RX_DMA_Init(buffer0,buffer1,2560) ;
    I2S3_TX_DMA_Init(buffer1,buffer0,2560)  ;
     DMA_Cmd(I2Sxext_RX_DMA_STREAM,ENABLE);//����DMA RX����,��ʼ¼��  
     DMA_Cmd(I2Sx_TX_DMA_STREAM,ENABLE);//����DMA TX����,��ʼ����  
     delay_ms(10);
     
    //  DMA_Cmd(I2Sxext_RX_DMA_STREAM,DISABLE);//����DMA RX����,��ʼ¼��  
    // DMA_Cmd(I2Sx_TX_DMA_STREAM,DISABLE);//����DMA TX����,��ʼ����  
    //  delay_ms(10);
      goto xx ; */
     
     
      /* LNA_En();        
       LNA_Trans_En();
      sx1276_LoRaEntryRx();  //���óɽ���*/
   
        WM8974();
     if(FM==1)
     { RF_switch1846();
       AT1846S();
     }
      else  
      
      {   RF_switch1278();
         sx1276_Config();  
         SX1276Read(0x42,&test);//test=0x12  
         SX1276Read(0x44,&test);//test=0x2D
         

	machdep_profile_init ();
        Voice_Realtime_Transfer();
      }
 
}

#pragma optimize=speed  medium
void Voice_Realtime_Transfer(void)
{
   struct CODEC2 *codec2;
   unsigned char   *bits;
int   nsam, nbit;
codec2 = codec2_create(CODEC2_MODE_1200);
 nsam = codec2_samples_per_frame(codec2);
nbit = codec2_bits_per_frame(codec2);
bits = (unsigned char*)malloc(nbit*sizeof(char));

    static unsigned char char_wireless_send,char_wireless_receive;
      
 wireless_transport_en=1;
 char wireless_send_pointer,wireless_receive_pointer;
 int xwl_point;
 int xwl_delay;
 
    start=0;     
TX_RX_Flag=TX_start=0,RX_start=1;
 while(1)
  {   
     if(TX_RX_Flag==1)
     {   
  TX:  if( TX_start==1)
       {  
           DMA_Cmd(I2Sxext_RX_DMA_STREAM,DISABLE);//�ر�DMA RX����,�ر�¼��  
           DMA_Cmd(I2Sx_TX_DMA_STREAM,DISABLE);//�ر�DMA TX����,�رղ���   
           delay_ms(10);        
           I2S3_Mode_Config(I2S_Standard_Phillips  ,I2S_DataFormat_16b,I2S_AudioFreq_8k); 
           I2S3ext_Mode_Config(I2S_Standard_Phillips ,I2S_DataFormat_16b,I2S_AudioFreq_8k);        
           I2Sxext_RX_DMA_Init(buffer0,buffer1,2560) ;
           I2S3_TX_DMA_Init(buffer3,buffer2,2560)  ;
           DMA_Cmd(I2Sxext_RX_DMA_STREAM,ENABLE);//����DMA RX����,��ʼ¼��  
           DMA_Cmd(I2Sx_TX_DMA_STREAM,ENABLE);//����DMA TX����,��ʼ����          
           TX_start=0;
        }
    
          {           
             PA_En();        
             PA_Trans_En();
             wireless_transport_en=0;             	           
             sx1276_LoRaEntryTx();  //����02ELS����ʱע�͵�
             sx1276_LoRaTxPacket();
             send_over=1;
             receive_over=0;
            }
       
       while(en_code==0);
             en_code =0; 
           xwl_point=0;
          
          
        if( buffer_select0==0)      
        {    
            GPIO_SetBits(GPIOB, GPIO_Pin_0);
            for(int AAA=0;AAA<4;AAA++) 
          { 
               codec2_encode(codec2, bits, &buffer_encode_1[xwl_point]);
            for(char i=0;i<6;i++)
              {   wireless_send[i+wireless_send_pointer]=*(bits+i);
                  
              } 
             wireless_send_pointer+=6;   
            xwl_point+=320;
           }  
          }
          
        
         else  if( buffer_select0==1)      
         {      GPIO_ResetBits(GPIOB, GPIO_Pin_0);
           for(int AAA=0;AAA<4;AAA++) 
          {
              codec2_encode(codec2, bits, &buffer_encode_0[xwl_point]); 
              for(char i=0;i<6;i++)
               { 
                   wireless_send[i+wireless_send_pointer]=*(bits+i);  
               } 
               wireless_send_pointer+=6;
                xwl_point+=320;
            }            
          }   
        
          //  if( wireless_send_pointer>=loadLength)//24���ֽڴ��
              {  
                           
                  wireless_send_pointer=0;                 
              }
      }   
     
      else if(TX_RX_Flag==0)  //receive
      { 
      if(RX_start==1)    
      {   PaOff();  
          LNA_En();        
           LNA_Trans_En();
           sx1276_LoRaEntryRx();  //���óɽ���*/    
      }
             xwl_point=0;
              send_over=0;   
	       receive=1;   //������������
               
             while(start==0){
               if(TX_RX_Flag==1){
                 TX_start=1; goto TX ;}  }               
              start=0;
        
         if(RX_start==1)     
         {
           DMA_Cmd(I2Sxext_RX_DMA_STREAM,DISABLE);//�ر�DMA RX����,�ر�¼��  
           DMA_Cmd(I2Sx_TX_DMA_STREAM,DISABLE);//�ر�DMA TX����,�رղ���   
           delay_ms(10);        
           I2S3_Mode_Config(I2S_Standard_Phillips  ,I2S_DataFormat_16b,I2S_AudioFreq_8k); 
           I2S3ext_Mode_Config(I2S_Standard_Phillips ,I2S_DataFormat_16b,I2S_AudioFreq_8k);        
           I2Sxext_RX_DMA_Init(buffer0,buffer1,2560) ;
           I2S3_TX_DMA_Init(buffer3,buffer2,2560)  ;
           DMA_Cmd(I2Sxext_RX_DMA_STREAM,ENABLE);//����DMA RX����,��ʼ¼��  
           DMA_Cmd(I2Sx_TX_DMA_STREAM,ENABLE);//����DMA TX����,��ʼ����      
           RX_start=0;
          }   
              
           
           { 
               SX1276ReadBuffer( 0x00,wireless_receive, loadLength);             
             
            }
             
        if( buffer_select1==0)      
        {    
            GPIO_SetBits(GPIOB, GPIO_Pin_0);
            for(int AAA=0;AAA<4;AAA++) 
          { 
            for(char i=0;i<6;i++)
              {   
                  *(bits+i)=wireless_receive[i+wireless_send_pointer];
                // wireless_send[i+wireless_send_pointer]=wireless_receive[i+wireless_send_pointer];
              }    
             // codec2_encode(codec2, bits, &buffer_encode_0[xwl_point]);             
               codec2_decode(codec2, &buffer_decode_1[xwl_point], bits);   
              wireless_send_pointer+=6;   
                xwl_point+=320;
          }
          
        }
         else  if( buffer_select1==1)      
         {      GPIO_ResetBits(GPIOB, GPIO_Pin_0);
           for(int AAA=0;AAA<4;AAA++) 
          {
              for(char i=0;i<6;i++)
               {                  
                   *(bits+i)=wireless_receive[i+wireless_send_pointer]; 
                 // wireless_send[i+wireless_send_pointer]=wireless_receive[i+wireless_send_pointer];
               } 
              codec2_decode(codec2, &buffer_decode_0[xwl_point], bits);  
              wireless_send_pointer+=6;
                xwl_point+=320;
                       
            }   
         }          
                  wireless_send_pointer=0;   
      }
    }      
  }
