#include "delay.h"
#include "sys.h"
#include "timer.h"
#include "led.h"
#include "dac.h"
#include "adc.h"
#include "usart.h"
#include "mypid.h"
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
	TIM_Cmd(TIM3,ENABLE); //ʹ�ܶ�ʱ��3
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //��ʱ��3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //��ռ���ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
}

double pidin,pidout,pidref;
long i=0;
//��ʱ��3�жϷ�����
void TIM3_IRQHandler(void)
{	
	
	u16 adcx;
	u16 dacval=0;
	u16 adc_raw=0;
	float adcvalue;
	dacval=1000;
	if(i%100==0){//����100�Σ�10s ��ӡһ��PID����
		PID_ShowConfig();
		delay_ms(1000);
	}
	//printf("Into interupt of TIM3 \n");
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //����ж�
	{	//�ڴ˴������ж�ʱҪ���������
		
		
		/*  DAC output part  */
		//DAC_SetChannel1Data(DAC_Align_12b_R, dacval);//����DAC���ֵ
		
		/*  ADC read in part  */
		adc_raw=Get_Adc_Average(ADC_Channel_5,20);	  //��ȡͨ��5��ת��ֵ��20��ȡƽ��
		adcvalue=(float)adc_raw*(3.3/4096);          //��ȡ�����Ĵ�С����ʵ�ʵ�ѹֵ������3.1111
		adcx=adcvalue;                            	//��ֵ�������ָ�adcx��������ΪadcxΪu16����
		//delay_ms(250);	
		printf("ADC read in : %f \n",adcvalue);
		
		/* PID part */
		//experiment : PA5/ PA4����һ�𣬿���PA5���
		pidin = adc_raw;
		PID_Compute();
		DAC_SetChannel1Data(DAC_Align_12b_R, pidout);
		printf("PID in = %f, PID out = %f",pidin,pidout);
		
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //����жϱ�־λ
	i++;
	if(i>50000)i=0;
}







