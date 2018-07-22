#include "led.h" 								  
////////////////////////////////////////////////////////////////////////////////// 	 

//��ʼ��PA11 12 13 14 15 Ϊ�����.��ʹ���������ڵ�ʱ��		    
//LED IO��ʼ��
void LED_Init(void)
{    	 
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitTypeDef  GPIO_InitStructure2;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//ʹ��GPIOAʱ��

  //GPIOPA11 12 13 14 15��ʼ������
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12| GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��
	
  GPIO_SetBits(GPIOA,GPIO_Pin_11 | GPIO_Pin_12| GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15);//GPIOA10,A11���øߣ�����
		
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);//ʹ��GPIOGʱ��
	
  GPIO_InitStructure2.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3| GPIO_Pin_4| GPIO_Pin_5| GPIO_Pin_6;
  GPIO_InitStructure2.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
  GPIO_InitStructure2.GPIO_OType = GPIO_OType_PP;//�������
  GPIO_InitStructure2.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure2.GPIO_PuPd = GPIO_PuPd_UP;//����
  GPIO_Init(GPIOG, &GPIO_InitStructure2);//��ʼ��	
  GPIO_SetBits(GPIOG,GPIO_Pin_2 | GPIO_Pin_3| GPIO_Pin_4| GPIO_Pin_5| GPIO_Pin_6);//GPIOG2 - G6���øߣ�����

}

void LED_Training(void){
	PAout(11)=0;
	PAout(12)=1;
	PAout(13)=1;
}
void LED_Trainsuccess(void){
	PAout(11)=1;
	PAout(12)=0;
	PAout(13)=1;
}
void LED_Trainfail(void){
	PAout(11)=1;
	PAout(12)=1;
	PAout(13)=0;
}

void LED_Off(void){
	PAout(11)=1;
	PAout(12)=1;
	PAout(13)=1;
	PGout(2)=1;
	PGout(3)=1;
	PGout(4)=1;
	PGout(5)=1;
	PGout(6)=1;
}

void LED_gesture_on(int i){
	i++;
	PGout(2)=(i==2?0:1);
	PGout(3)=(i==3?0:1);
	PGout(4)=(i==4?0:1);
	PGout(5)=(i==5?0:1);
	PGout(6)=(i==6?0:1);
}

void LED_gestureST_on(void){
	PGout(2)=0;
	PGout(3)=1;
	PGout(4)=1;
	PGout(5)=1;
	PGout(6)=1;
}
void LED_gestureJD_on(void){
	PGout(2)=1;
	PGout(3)=0;
	PGout(4)=1;
	PGout(5)=1;
	PGout(6)=1;
}
void LED_gestureB_on(void){
	PGout(2)=1;
	PGout(3)=1;
	PGout(4)=0;
	PGout(5)=1;
	PGout(6)=1;
}
