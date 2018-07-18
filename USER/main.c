#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
//#include "lcd.h"
#include "adc.h"
#include "dac.h"
#include "mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h" 
//Pins used��
//PB9,PB8 : I2C for MPU6050
//PC0 : INT for MPU6050
//PA5 : ADC CH5, 12bit mode
//PA4 : DAC CH1, 12bit

//����1����1���ַ� 
//c:Ҫ���͵��ַ�
void usart1_send_char(u8 c)
{

	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET); 
    USART_SendData(USART1,c);   

} 
//�������ݸ�����������λ�����(V2.6�汾)
//fun:������. 0XA0~0XAF
//data:���ݻ�����,���28�ֽ�!!
//len:data����Ч���ݸ���
void usart1_niming_report(u8 fun,u8*data,u8 len)
{
	u8 send_buf[32];
	u8 i;
	if(len>28)return;	//���28�ֽ����� 
	send_buf[len+3]=0;	//У��������
	send_buf[0]=0X88;	//֡ͷ
	send_buf[1]=fun;	//������
	send_buf[2]=len;	//���ݳ���
	for(i=0;i<len;i++)send_buf[3+i]=data[i];			//��������
	for(i=0;i<len+3;i++)send_buf[len+3]+=send_buf[i];	//����У���	
	for(i=0;i<len+4;i++)usart1_send_char(send_buf[i]);	//�������ݵ�����1 
}
//���ͼ��ٶȴ��������ݺ�����������
//aacx,aacy,aacz:x,y,z������������ļ��ٶ�ֵ
//gyrox,gyroy,gyroz:x,y,z�������������������ֵ
void mpu6050_send_data(short aacx,short aacy,short aacz,short gyrox,short gyroy,short gyroz)
{
	u8 tbuf[12]; 
	tbuf[0]=(aacx>>8)&0XFF;
	tbuf[1]=aacx&0XFF;
	tbuf[2]=(aacy>>8)&0XFF;
	tbuf[3]=aacy&0XFF;
	tbuf[4]=(aacz>>8)&0XFF;
	tbuf[5]=aacz&0XFF; 
	tbuf[6]=(gyrox>>8)&0XFF;
	tbuf[7]=gyrox&0XFF;
	tbuf[8]=(gyroy>>8)&0XFF;
	tbuf[9]=gyroy&0XFF;
	tbuf[10]=(gyroz>>8)&0XFF;
	tbuf[11]=gyroz&0XFF;
	usart1_niming_report(0XA1,tbuf,12);//�Զ���֡,0XA1
}	
//ͨ������1�ϱ���������̬���ݸ�����
//aacx,aacy,aacz:x,y,z������������ļ��ٶ�ֵ
//gyrox,gyroy,gyroz:x,y,z�������������������ֵ
//roll:�����.��λ0.01�ȡ� -18000 -> 18000 ��Ӧ -180.00  ->  180.00��
//pitch:������.��λ 0.01�ȡ�-9000 - 9000 ��Ӧ -90.00 -> 90.00 ��
//yaw:�����.��λΪ0.1�� 0 -> 3600  ��Ӧ 0 -> 360.0��
void usart1_report_imu(short aacx,short aacy,short aacz,short gyrox,short gyroy,short gyroz,short roll,short pitch,short yaw)
{
	u8 tbuf[28]; 
	u8 i;
	for(i=0;i<28;i++)tbuf[i]=0;//��0
	tbuf[0]=(aacx>>8)&0XFF;
	tbuf[1]=aacx&0XFF;
	tbuf[2]=(aacy>>8)&0XFF;
	tbuf[3]=aacy&0XFF;
	tbuf[4]=(aacz>>8)&0XFF;
	tbuf[5]=aacz&0XFF; 
	tbuf[6]=(gyrox>>8)&0XFF;
	tbuf[7]=gyrox&0XFF;
	tbuf[8]=(gyroy>>8)&0XFF;
	tbuf[9]=gyroy&0XFF;
	tbuf[10]=(gyroz>>8)&0XFF;
	tbuf[11]=gyroz&0XFF;	
	tbuf[18]=(roll>>8)&0XFF;
	tbuf[19]=roll&0XFF;
	tbuf[20]=(pitch>>8)&0XFF;
	tbuf[21]=pitch&0XFF;
	tbuf[22]=(yaw>>8)&0XFF;
	tbuf[23]=yaw&0XFF;
	usart1_niming_report(0XAF,tbuf,28);//�ɿ���ʾ֡,0XAF
} 

void MPU6050_MUST_Init(){
	while(mpu_dmp_init())// =0 means success
	{
		/*�˴������ʼ��ʧ�ܱ�־*/
		
		//LCD_ShowString(30,130,200,16,16,"MPU6050 Error");
		//delay_ms(200);
		//LCD_Fill(30,130,239,130+16,WHITE);
 		delay_ms(200);
		printf("MPU dmp init failing....\n");
	}
}

void MPU6050_Perform(){

}

int main(void)
{ 
	u8 report=1;			//Ĭ�Ͽ����ϱ�
	u8 key;
	u16 adcx;
	u16 dacval=0;
	float adcvalue;
	
	float pitch,roll,yaw; 		//ŷ����
	short aacx,aacy,aacz;		//���ٶȴ�����ԭʼ����
	short gyrox,gyroy,gyroz;	//������ԭʼ����
	short temp;					//�¶�
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	delay_init(168);  			//��ʼ����ʱ����
	uart_init(460800);			//��ʼ�����ڲ�����Ϊ50,0000//460800:OK
	LED_Init();					//��ʼ��LED 
	KEY_Init();					//��ʼ������
 	//LCD_Init();				//LCD��ʼ��
	MPU_Init();					//��ʼ��MPU6050
	Adc_Init();         		//��ʼ��ADC
	Dac1_Init();		 		//DACͨ��1��ʼ��	
	
	//MPU6050_MUST_Init();
	DAC_SetChannel1Data(DAC_Align_12b_R,dacval);//��ʼֵΪ0
	dacval=2000;
	
 	while(1)
	{
		printf("running....\n");
		delay_ms(500);
		/*  DAC output part  */
		DAC_SetChannel1Data(DAC_Align_12b_R, dacval);//����DACֵ
		
		/*  ADC read in part  */
		adcx=Get_Adc_Average(ADC_Channel_5,20);//��ȡͨ��5��ת��ֵ��20��ȡƽ��
		adcvalue=(float)adcx*(3.3/4096);          //��ȡ�����Ĵ�С����ʵ�ʵ�ѹֵ������3.1111
		adcx=adcvalue;                            //��ֵ�������ָ�adcx��������ΪadcxΪu16����
		delay_ms(250);	
		printf("ADC in : %f",adcvalue);
		
		/* MPU6050 measure part */
		if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)//=0 means success
		{ 
			temp=MPU_Get_Temperature();					//�õ��¶�ֵ
			MPU_Get_Accelerometer(&aacx,&aacy,&aacz);	//�õ����ٶȴ���������
			MPU_Get_Gyroscope(&gyrox,&gyroy,&gyroz);	//�õ�����������
			//if(report)mpu6050_send_data(aacx,aacy,aacz,gyrox,gyroy,gyroz);//���Զ���֡���ͼ��ٶȺ�������ԭʼ����
			//if(report)usart1_report_imu(aacx,aacy,aacz,gyrox,gyroy,gyroz,(int)(roll*100),(int)(pitch*100),(int)(yaw*10));
			//�������䲻Ӱ�촮�����
			printf("pitch=%f,roll=%f,yaw=%f;\n",pitch,roll,yaw);//printf������ʹ�ò���Ӱ�쵽����������λ�������ݵĽ��ܡ�
			//��ΪUsart_send�Ⱥ��������ǲ����ʶ��ˣ������Ҳ�Ǻ�printf�õ��ǲ�ͬ�ģ������޷��ڴ��ڳ�����鿴��
			
			delay_ms(100);
			//delay������ʹ�û�ʹ������������λ����̬��ʾ����ȥ�ܿ���
			//�������������۲鿴�������ݡ�
			
		}
		

	} 	
}
