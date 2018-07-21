#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "math.h"
#include "lcd.h"
//#include "timer.h"
//#include "adc.h"
//#include "dac.h"
#include "matrixkey.h"
#include "fdc2214.h"
//#include "nixietube.h"

//Pins used��
//1.PB9,PB8 : I2C for MPU6050
//2.PC0 : INT for MPU6050
//3.PA5 : ADC Input  CH5, 12bit mode
//4.PA4 : DAC Output CH1, 12bit
//5.PB0-PB7 : 4X4 matrix keys
//7.PC4(SCL) PC5(SDA) �� I2C for fdc2214 PC

//Abilities:
//1.PID ����Ƶ����TIM3��ʱ������
//

#define TSAMPLETIMES 20//train sample times
#define DSAMPLETIMES 5//detect sample times
#define MAXTRAINTIMES 5
#define MAXRANGE 0.2 //��ѵ��ʱ�������������С֮�����0.2pfʱ����Ϊ�˴�ѵ���������ã���Ҫ����ѵ��
//�õ�ѵ���������㣺��ֵ����0.8���ҵ���ѵ���Ĳ�ͬ���ݵ�֮�����С��0.2��
//��Ϊ��ֵС��0.8�൱����û�����ַ������档�������0.2˵�����ֶ��˻��߻������Ŵ�ʱ�ǳ���

//�洢˳�򣬵��ݴ�С����
float mean_g[8]={-100,-100,-100,-100,-100,-100,-100,-100};
float unst_g[8]={0,0,0,0,0,0,0,0};
float measurebuffer[10];

float MaxRange(float measurebuffer[],int len){//�������������С�Ĳ�ֵ
	int i=0;
	float max=measurebuffer[0];
	float min=measurebuffer[0];
	for(i=0;i<len;i++){
		if(measurebuffer[i]>max){
			max = measurebuffer[i];
		}
		if(measurebuffer[i]<min){
			min = measurebuffer[i];
		}
	}
	return max-min;
}

float mean(float measurebuffer[],int len){//calcul mean of an array
	int i=0;
	float meansum=0;
	for(i=0;i<len;i++){
		meansum+=measurebuffer[i];
	}
	meansum=meansum/len;
	return meansum;
}

float stdsd(float measurebuffer[],int len){//calculate the std err of an array 
	int i=0;
	float errsum=0;
	float mean_g = mean(measurebuffer,len);
	for(i=0;i<len;i++){
		errsum+=abs((10000*measurebuffer[i]-10000*mean_g));
	}
	errsum=10*errsum/(len-1);//adjust sensitivity
	return errsum/10000;
}

int equal2(int row,int column,int expect_row,int expect_column){
	return row==expect_row&&column==expect_column?1:0;
}

void gesture_train(int n,float fdc2214temp){
	int i;
	int t;
	float trainmean;
	float trianmaxrange;
	for(t=0;t<MAXTRAINTIMES;t++){// 5 times max, for 10s
		for(i=0;i<TSAMPLETIMES;i++){
				measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 sample for 20 times.
				delay_ms(100);
		}
		trainmean=mean(measurebuffer,TSAMPLETIMES);
		trianmaxrange=MaxRange(measurebuffer,TSAMPLETIMES);
		if(trianmaxrange<MAXRANGE){
			if(trainmean<0.8)continue;//�������
			mean_g[n-1]=mean(measurebuffer,TSAMPLETIMES);//
			unst_g[n-1]=stdsd(measurebuffer,TSAMPLETIMES);//
			printf("Mean=%f, Uncertainty=%f \n",mean_g[n-1],unst_g[n-1]);
			return;
		}
	}
	printf("ManRange = %f, training failed��please have another try. \n",MaxRange(measurebuffer,TSAMPLETIMES));
}


int Parse_Key(int row,int column,float fdc2214temp){
	int i;
	float fdc2214in=Cap_Calculate(0)-fdc2214temp;
	
	if(equal2(row,column,1,1)){//1
		printf("for Traning of gesture 1.\n");
		gesture_train(1,fdc2214temp);
	}
	else if(equal2(row,column,1,2)){//2
		printf("for Traning of gesture 2.\n");
		gesture_train(2,fdc2214temp);
	}
	else if(equal2(row,column,1,3)){//3
		printf("for Traning of gesture 3.\n");
		gesture_train(3,fdc2214temp);
	}
	else if(equal2(row,column,2,1)){//4
		printf("for Traning of gesture 4.\n");
		gesture_train(4,fdc2214temp);
	}
	else if(equal2(row,column,2,2)){//5
		printf("for Traning of gesture 5.\n");
		gesture_train(5,fdc2214temp);
	}
	else if(equal2(row,column,4,1)){//*
		printf("for Traning of gesture ST.\n");//��С
		gesture_train(6,fdc2214temp);
	}
	else if(equal2(row,column,4,3)){//#
		printf("for Traning of gesture JD.\n");//�м�
		gesture_train(7,fdc2214temp);
	}
	else if(equal2(row,column,4,4)){//D
		printf("for Traning of gesture B.\n");//���
		gesture_train(8,fdc2214temp);
	}
	else if(equal2(row,column,1,4)){//A, For Detection of ST/JD/B.
		printf("For Detection of STJDB.\n");
		for(i=0;i<DSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 10 times for mean.
			delay_ms(100);
		}
		fdc2214in=mean(measurebuffer,DSAMPLETIMES);
		
		if(fdc2214in < (mean_g[6-1]+mean_g[7-1])/2 ){//ST��ʯͷ����ֵ��С
			printf("recognized : gesture ST .\n");
		}
		else if(fdc2214in < (mean_g[7-1]+mean_g[8-1])/2){
			printf("recognized : gesture JD .\n");
		}
		else{										//B��������ֵ���
			printf("recognized : gesture B .\n");
		}
		
		/*
		if(fabs(fdc2214in-mean_g[6-1])<=unst_g[6-1]){
			printf("recognized : gesture JD .\n");
		}
		else if(fabs(fdc2214in-mean_g[7-1])<=unst_g[7-1]){
			printf("recognized : gesture ST .\n");
		}
		else if(fabs(fdc2214in-mean_g[8-1])<=unst_g[8-1]){
			printf("recognized : gesture B .\n");
		}*/
	}
	else if(equal2(row,column,2,4)){//B, For Detection of 12345.
		printf("For Detection of 12345.\n");
		for(i=0;i<DSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 10 times for mean.
			delay_ms(100);
		}
		fdc2214in=mean(measurebuffer,DSAMPLETIMES);
		for(i=1;i<6;i++){
			if(i<5 && (fdc2214in <= (mean_g[i-1]+mean_g[i])/2) ){//fabs(fdc2214in-mean_g[i-1])<=unst_g[i-1]
				printf("recognized : gesture %d.\n",i);
				break;
			}
			if(i==5){
				if(fdc2214in > (mean_g[i-2]+mean_g[i-1])/2){
					printf("recognized : gesture %d.\n",i);
				}
			}
		}
		
	}
	///////////////
	else if(equal2(row,column,4,2)){//0,��ѵ�����ģʽ,JD/ST/B
		mean_g[6-1]=2.5; unst_g[6-1]=0.5;//JD: 2-3
		mean_g[7-1]=1.4; unst_g[7-1]=0.6;//ST: 0.8-2
		mean_g[8-1]=3.5;  unst_g[8-1]=0.5;// B: 3-4
		
		printf("For Untrained Detection of JDSTB.\n");
		for(i=0;i<DSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 10 times for mean.
			delay_ms(100);
		}
		fdc2214in=mean(measurebuffer,DSAMPLETIMES);
		printf("detect value: %f \n",fdc2214in);
		
		if(fabs(fdc2214in-mean_g[6-1])<=unst_g[6-1]){
			printf("recognized : gesture JD .\n");
		}
		else if(fabs(fdc2214in-mean_g[7-1])<=unst_g[7-1]){
			printf("recognized : gesture ST .\n");
		}
		else if(fabs(fdc2214in-mean_g[8-1])<=unst_g[8-1]){
			printf("recognized : gesture B .\n");
		}
		else{
			printf("Confused with gesture recognition......\n");
		}
		delay_ms(1000);
	}
	else if(equal2(row,column,3,4)){//C,��ѵ�����ģʽ,1/2/3/4/5
		mean_g[1-1]=1.70; unst_g[1-1]=0.15;
		mean_g[2-1]=2.05; unst_g[2-1]=0.2;
		mean_g[3-1]=2.4;  unst_g[3-1]=0.15;
		mean_g[4-1]=2.7; unst_g[4-1]=0.15;
		mean_g[5-1]=3.5; unst_g[5-1]=0.5;
		
		printf("For Untrained Detection of 12345.\n");
		for(i=0;i<DSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 10 times for mean.
			delay_ms(100);
		}
		fdc2214in=mean(measurebuffer,DSAMPLETIMES);
		printf("detect value: %f \n",fdc2214in);
		
		for(i=1;i<6;i++){
			if(fabs(fdc2214in-mean_g[i-1])<=unst_g[i-1]){
				printf("recognized : gesture %d.\n",i);
				delay_ms(1000);
				break;
			}
		}
	}
	else if(equal2(row,column,3,3)){//9,printf existed data.
		int i=0;
		for(i=0;i<5;i++){
			printf("Mean of gesture %d = %f, Uncertainty = %f\r\n",i+1,mean_g[i],unst_g[i]);
		}
		printf("Mean of gesture JD = %f, Uncertainty = %f\r\n",mean_g[5],unst_g[5]);
		printf("Mean of gesture ST = %f, Uncertainty = %f\r\n",mean_g[6],unst_g[6]);
		printf("Mean of gesture  B = %f, Uncertainty = %f\r\n",mean_g[7],unst_g[7]);
	}
	return 0;
}


int main(void)
{ 
	int mine_or_others=1;
	int row=0;
	int column=0;
	
	float res0,res1,res2,res3;
	float temp0,temp1,temp2,temp3;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	delay_init(168);  			//��ʼ����ʱ����
	uart_init(460800);			//��ʼ�����ڲ�����Ϊ50,0000//460800:OK
	LED_Init();					//��ʼ��LED 
	KEY_Init();					//��ʼ������
 	LCD_Init();				//LCD��ʼ��
	//TIM3_Int_Init(1000-1,8400-1);	//Timer for PID ��ʱ��ʱ��84M����Ƶϵ��8400������84M/8400=10Khz�ļ���Ƶ�ʣ�����1000��Ϊ100ms��every 0.1sһ���ж� 
	MATRI4X4KEY_Init();
	while(FDC2214_Init());
	
	printf("Init OK \n.");
	delay_ms(1000);//��ʱ1sʹ�������ֿ������ط�ֹ�ַ��ڸ�λ��ťʱӰ����ݡ�
	
	temp0 = Cap_Calculate(0);//��ȡ��ʼֵ******
	temp1 = Cap_Calculate(1);
	temp2 = Cap_Calculate(2);
	temp3 = Cap_Calculate(3);
	printf("temp0 = %f,temp1 = %f,temp2 = %f,temp3 = %f\n",temp0,temp1,temp2,temp3);
	
 	while(1)
	{
		row= -1;
		column= -1;
		res0 = Cap_Calculate(0);//�ɼ�����
		res1 = Cap_Calculate(1);
		res2 = Cap_Calculate(2);
		res3 = Cap_Calculate(3);
		printf("CH0;%3.3f CH1;%3.3f CH2;%3.3f CH3;%3.3f. \n",res0-temp0,res1-temp1,res2-temp2,res3-temp3);
		if(MATRI4_4KEY_Scan(&row, &column)==0)
			printf("key pressed row=%d, column=%d \n",row,column);
		if(mine_or_others==1){
			Parse_Key(row,column,temp0);
		}
		delay_ms(1000);
	} 	
}
