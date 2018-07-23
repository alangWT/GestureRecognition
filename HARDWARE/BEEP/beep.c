#include "beep.h" 
#include "delay.h"
#include "lcd.h"
//////////////////////////////////////////////////////////////////////////////////	 
	  
////////////////////////////////////////////////////////////////////////////////// 	 

//��ʼ��PF7Ϊ�����		    
//BEEP IO��ʼ��
void BEEP_Init(void)
{   
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);//ʹ��GPIOFʱ��
  
  //��ʼ����������Ӧ����GPIOF8
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;//����
  GPIO_Init(GPIOF, &GPIO_InitStructure);//��ʼ��GPIO
	
  GPIO_ResetBits(GPIOF,GPIO_Pin_7);  //��������Ӧ����GPIOF8���ͣ� 
}


void Piano(int f){
	f *=1.5;
	if(f>100&&f<800){//100-200,200-300,300-400,400-500,500-600,600-700,700-800
		u32 T = 0;//����usֵ
		u32 t =0;
		
		if(f<200){
			f=DOU;
			LCD_ShowString(30,30,210,16,16,"Gssture Recognized:  DOU");
		}
		else if(f<300){
			f=RUI;
			LCD_ShowString(30,30,210,16,16,"Gssture Recognized:  RUI");
		}
		else if(f<400){
			f=MI;
			LCD_ShowString(30,30,210,16,16,"Gssture Recognized:  MI ");
		}
		else if(f<500){
			f=FA;
			LCD_ShowString(30,30,210,16,16,"Gssture Recognized:  FA ");
		}
		else if(f<600){
			f=SOU;
			LCD_ShowString(30,30,210,16,16,"Gssture Recognized:  SOU");
		}
		else if(f<700){
			f=LA;
			LCD_ShowString(30,30,210,16,16,"Gssture Recognized:  LA ");
		}
		else {
			f=SI;
			LCD_ShowString(30,30,210,16,16,"Gssture Recognized:  SI ");
		}
		T = 1000000/f;
		for(t=0;t<400000;t+=T){
			PFout(7)=1;
			delay_us(T/2);
			PFout(7)=0;
			delay_us(T/2);
		}
	}
}



