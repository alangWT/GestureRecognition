#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "math.h"

#include "beep.h"
#include "led.h"
#include "key.h"
#include "lcd.h"

#include "matrixkey.h"
#include "fdc2214.h"
//****Pins used��  
//*Matrix keys:
//1.PB0-PB3/PA0-PA3 : 4X4 matrix keys

//*fdc2214:
//2.PC4(SCL) PC5(SDA) �� I2C for fdc2214
//3.PC6(SCL) PC7(SDA) �� I2C for Zfdc2214

//*LCD:
//4.PB15,PD0,PD1,PD4,PD5,PD8,PD9,PD10,PD14,PD15,PE7~15,PF12,PG12;

//*Mode keys:
//5.PF1 3 5:��椿���,����һЩ�ڲ�״̬

//*LED:
//6.PA11 12 13 14 15 Ϊ״ָ̬ʾLED
//7.PG2 3 4 5 6Ϊ����ʶ��ָʾ��
 
//*BEEP piano out
//8.PF7
 
//***���ڲ��뿪�ؼ�ģʽѡ���˵��������Ϊ̫���ӣ�������Ա�Լ���Ҫдһ���ֲ�����ס��ôʹ����������� =_= ��
//1.��λʱ���в��뿪�ض�������������ģʽ��
//2.��λʱ��1�Ų���(PF1)�����ϲ�on����Ϊ��̬ģʽ��
//3.��λ��2�Ų���(PF3)�����ϲ�on����PF3in���Ϊ�ͣ����붯̬ģʽ��Ȼ���ٽ����뿪��������ȥ��off��Ȼ����Խ���ѵ����ѵ�����˺�
//ͬʱ��סA��B+���������ϲ����������A-12345���ơ�B-ʯͷ���������ƶ�̬ʶ��ģʽ��
//4.��λʱ��3�Ų���(PF5)�����ϲ�on��Ȼ���ٽ�2�Ų������ϲ�������뵯����ģʽ��

#define TSAMPLETIMES 10//train sample times
#define DSAMPLETIMES 5//detect sample times
#define MAXTRAINTIMES 3
#define MAXRANGE 0.2 //��ѵ��ʱ�������������С֮�����0.2 pfʱ����Ϊ�˴�ѵ���������ã���Ҫ����ѵ��
#define NOHANDSTATUS 0.4  //�������ֵС��0.4����Ϊ��ʱ���ַ��ڰ���
//�õ�ѵ���������㣺��ֵ����0.4���ҵ���ѵ���Ĳ�ͬ���ݵ�֮�����С��0.2��
//��Ϊ��ֵС��0.4�൱����û�����ַ������档�������0.2˵�����ֶ��˻��߻������Ŵ�ʱ�ǳ���

//�洢˳�򣬵��ݴ�С����: 1 2 3 4 5 ST JD B,��λʱ�������봫������Զ��
float mean_g[8]={2.25,3.30,3.80,4.25,5.25,1.10,3.30,5.25};
//
float unst_g[8]={0,0,0,0,0,0,0,0};
float measurebuffer[10];
//ʹ�ò�椿��ظı䣺
int printfmode=1;//mode 
int compensation = 0;
int dynamicmode=0;
int pianomode=0;

//
int mode=0;//=0:training, =1:detect
int current_gesture=0;//1:1,2:2,6:ST,7:JD,8:B
int train_status=2;//=1:train finished, =-1:failed =0:training
int current_key=0;//1:1,16:D
float C0,C1,C2,C3,C4=0;
float ZC0,ZC1,ZC2,ZC3,ZC4=0;
float temp0;//float temp1,temp2,temp3;��ʼ����ֵ
float Ztemp0;//float Ztemp1,Ztemp2,Ztemp3;��ʼZ����ֵ

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


/*********************MAIN FUNCTIONS*********************/

int gesture_train(int n,float fdc2214temp){
	int i;
	int t;
	float trainmean;
	float trianmaxrange;
	
	LED_Training();
	
	for(t=0;t<MAXTRAINTIMES;t++){// 3 times max, for 1.5s
		
		for(i=0;i<TSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 sample for 10 times.
			if(compensation==1)
				measurebuffer[i]-=ZCap_Calculate(0);
			delay_ms(50);
		}
		trainmean=mean(measurebuffer,TSAMPLETIMES);
		
		trianmaxrange=MaxRange(measurebuffer,TSAMPLETIMES);
		if(trianmaxrange<MAXRANGE){//��ѵ�������
			if(trainmean<NOHANDSTATUS)continue;//�������
			mean_g[n-1]=mean(measurebuffer,TSAMPLETIMES);//
			unst_g[n-1]=stdsd(measurebuffer,TSAMPLETIMES);//
			printf("Mean=%f, Uncertainty=%f \r\n",mean_g[n-1],unst_g[n-1]);
			POINT_COLOR=GREEN;
			LCD_ShowString(30,210,210,16,16,"Train Status: OK.");
			LED_Trainsuccess();
			return 0;
		}
		//��ѵ�����������Ҫ����ѭ�������ѭ����������10�ͷ���ѵ����ʧ��
	}
	
	LED_Trainfail();
	POINT_COLOR=RED;
	LCD_ShowString(30,210,210,16,16,"Train Status: Failed.");
	printf("***��ManRange = %f, training failed��please have another try. \r\n",MaxRange(measurebuffer,TSAMPLETIMES));
	return 1;
}


int Parse_Key(int row,int column,float fdc2214temp){
	int i;
	float fdc2214in;
	int trainflag;//����ѵ���Ƿ�ɹ��ı�־
	
	//ѵ��������
	if(equal2(row,column,1,1)){//1
		printf("for Traning of gesture 1.\r\n");
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,345,200,12,12,"1:gesture 1 training.");
		trainflag=gesture_train(1,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,345,200,12,12,"1:gesture 1 Trained!!!");
		POINT_COLOR=BLACK;
		LCD_ShowNum(100,190,mean_g[0]*100,3,16);
	}
	else if(equal2(row,column,1,2)){//2
		printf("for Traning of gesture 2.\r\n");
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,360,200,12,12,"2:gesture 2 training.");
		trainflag=gesture_train(2,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,360,200,12,12,"2:gesture 2 Trained!!!");
		POINT_COLOR=BLACK;
		LCD_ShowNum(140,190,mean_g[1]*100,3,16);
	}
	else if(equal2(row,column,1,3)){//3
		printf("for Traning of gesture 3.\r\n");
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,375,200,12,12,"3:gesture 3 training.");
		trainflag=gesture_train(3,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,375,200,12,12,"3:gesture 3 Trained!!!");
		POINT_COLOR=BLACK;
		LCD_ShowNum(180,190,mean_g[2]*100,3,16);
	}
	else if(equal2(row,column,2,1)){//4
		printf("for Traning of gesture 4.\r\n");
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,390,200,12,12,"4:gesture 4 training.");
		trainflag=gesture_train(4,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,390,200,12,12,"4:gesture 4 Trained!!!");
		POINT_COLOR=BLACK;
		LCD_ShowNum(220,190,mean_g[3]*100,3,16);
	}
	else if(equal2(row,column,2,2)){//5
		printf("for Traning of gesture 5.\r\n");
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,405,200,12,12,"5:gesture 5 training.");
		trainflag=gesture_train(5,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,405,200,12,12,"5:gesture 5 Trained!!!");
		POINT_COLOR=BLACK;
		LCD_ShowNum(260,190,mean_g[4]*100,3,16);
	}
	else if(equal2(row,column,4,1)){//*
		printf("for Traning of gesture ST.\r\n");//��С
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,285,200,12,12,"*:gesture ST training.");
		trainflag=gesture_train(6,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,285,200,12,12,"*:gesture ST Trained!!!");
		POINT_COLOR=BLACK;
		LCD_ShowNum(100,170,mean_g[5]*100,3,16);
	}
	else if(equal2(row,column,4,3)){//#
		printf("for Traning of gesture JD.\r\n");//�м�
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,300,200,12,12,"#:gesture JD training.");
		trainflag=gesture_train(7,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,300,200,12,12,"#:gesture JD Trained!!!");
		POINT_COLOR=BLACK;
		LCD_ShowNum(160,170,mean_g[6]*100,3,16);
	}
	else if(equal2(row,column,4,4)){//D
		printf("for Traning of gesture B.\r\n");//���
		POINT_COLOR=YELLOW;
		LCD_ShowString(30,315,200,12,12,"D:gesture B training.");
		trainflag=gesture_train(8,fdc2214temp);
		POINT_COLOR=GREEN;
		if(trainflag==0)
			LCD_ShowString(30,315,200,12,12,"D:gesture B Trained!!!");
		POINT_COLOR=BLACK;
		LCD_ShowNum(220,170,mean_g[7]*100,3,16);
	}
	
	//�ж�������
	
	else if(equal2(row,column,1,4)){//key A, For Detection of ST/JD/B.
		printf("For Detection of STJDB.\r\n");
		POINT_COLOR=BLACK;
		LCD_ShowString(30,270,180,12,12,"A:Detection of STJDB");
		for(i=0;i<DSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 5 times for mean.
			if(compensation==1)
				measurebuffer[i]-=ZCap_Calculate(0);
			delay_ms(50);
		}
		fdc2214in=mean(measurebuffer,DSAMPLETIMES);
		
		if(fdc2214in < (mean_g[6-1]+mean_g[7-1])/2 ){//ST��ʯͷ����ֵ��С
			POINT_COLOR=BLUE;
			LCD_ShowString(30,30,210,16,16,"Gesture Recognized:  ");
			LCD_ShowString(260,30,100,16,16,"ST ");
			POINT_COLOR=BLACK;
			LCD_ShowString(30,230,210,16,16,"Last recognized: ");
			LCD_ShowString(260,230,100,16,16,"ST  ");
			LED_gestureST_on();
			printf("recognized : gesture ST .\r\n");
		}
		else if(fdc2214in < (mean_g[7-1]+mean_g[8-1])/2){
			POINT_COLOR=BLUE;
			LCD_ShowString(30,30,210,16,16,"Gesture Recognized:  ");
			LCD_ShowString(260,30,100,16,16,"JD ");
			POINT_COLOR=BLACK;
			LCD_ShowString(30,230,210,16,16,"Last recognized: ");
			LCD_ShowString(260,230,100,16,16,"JD  ");
			LED_gestureJD_on();
			printf("recognized : gesture JD .\r\n");
		}
		else{										//B��������ֵ���
			POINT_COLOR=BLUE;
			LCD_ShowString(30,30,210,16,16,"Gesture Recognized:  ");
			LCD_ShowString(260,30,100,16,16,"B  ");
			POINT_COLOR=BLACK;
			LCD_ShowString(30,230,210,16,16,"Last recognized: ");
			LCD_ShowString(260,230,100,16,16,"B  ");
			LED_gestureB_on();
			printf("recognized : gesture B .\r\n");
			//LCD_ShowNum(220,170,mean_g[7]*100,3,16);
		}

	}
	else if(equal2(row,column,2,4)){	//key B, For Detection of 12345.
		printf("For Detection of 12345.\r\n");
		POINT_COLOR=BLACK;
		LCD_ShowString(30,330,180,12,12,"B:Detection of 12345");
		//detect sample:
		for(i=0;i<DSAMPLETIMES;i++){
			measurebuffer[i]=Cap_Calculate(0)-fdc2214temp; // CH0 detect for 5 times for mean.
			if(compensation==1)
				measurebuffer[i]-=ZCap_Calculate(0);
			delay_ms(50);
		}
		fdc2214in=mean(measurebuffer,DSAMPLETIMES);
		//recognize:
		for(i=1;i<6;i++){
			if(i<5 && (fdc2214in <= (mean_g[i-1]+mean_g[i])/2) ){//fabs(fdc2214in-mean_g[i-1])<=unst_g[i-1]
				POINT_COLOR=BLUE;
				LCD_ShowString(30,30,210,16,16,"Gesture Recognized:  ");
				LCD_ShowNum(260,30,i,2,16);
				POINT_COLOR=BLACK;
				LCD_ShowString(30,230,210,16,16,"Last recognized: ");
				LCD_ShowNum(260,230,i,2,16);
				LED_gesture_on(i);
				printf("recognized : gesture %d.\r\n",i);
				break;
			}
			if(i==5){
				if(fdc2214in > (mean_g[i-2]+mean_g[i-1])/2){
					POINT_COLOR=BLUE;
					LCD_ShowString(30,30,180,16,16,"Gesture Recognized:  ");
					LCD_ShowNum(260,30,i,2,16);
					POINT_COLOR=BLACK;
					LCD_ShowString(30,230,210,16,16,"Last recognized: ");
					LCD_ShowNum(260,230,i,2,16);
					LED_gesture_on(i);
					printf("recognized : gesture %d.\r\n",i);
				}
			}
		}
		
		delay_ms(300);//
		
	}
	
	//���������԰�����//9,C
	
	else if(equal2(row,column,3,3)){//9,printf all existed data.
		int i=0;
		
		POINT_COLOR=BLACK;
		LCD_ShowString(30,420,200,12,12,"9:show ALL training data");
		for(i=0;i<5;i++){
			printf("Mean of gesture %d = %f, Uncertainty = %f\r\n",i+1,mean_g[i],unst_g[i]);
		}
		printf("Mean of gesture ST = %f, Uncertainty = %f\r\n",mean_g[5],unst_g[5]);
		printf("Mean of gesture JD = %f, Uncertainty = %f\r\n",mean_g[6],unst_g[6]);
		printf("Mean of gesture  B = %f, Uncertainty = %f\r\n",mean_g[7],unst_g[7]);
		POINT_COLOR=BLACK;
		LCD_ShowString(30,170,140,16,16,"STJDB C: ");
		LCD_ShowNum(100,170,mean_g[5]*100,3,16);
		LCD_ShowNum(160,170,mean_g[6]*100,3,16);
		LCD_ShowNum(220,170,mean_g[7]*100,3,16);
		LCD_ShowString(30,190,140,16,16,"12345 C: ");
		LCD_ShowNum(100,190,mean_g[0]*100,3,16);
		LCD_ShowNum(140,190,mean_g[1]*100,3,16);
		LCD_ShowNum(180,190,mean_g[2]*100,3,16);
		LCD_ShowNum(220,190,mean_g[3]*100,3,16);
		LCD_ShowNum(260,190,mean_g[4]*100,3,16);
	}
	else if(equal2(row,column,3,4)){//C, reset cap value
		POINT_COLOR=BLACK;
		LCD_ShowString(30,435,180,12,12,"C:reset cap value");	
		temp0 = Cap_Calculate(0);
	
		Ztemp0 = ZCap_Calculate(0);
	
		printf("temp0 = %f,Ztemp0 = %f. \r\n",temp0,Ztemp0);
		LCD_ShowString(30,50,100,16,16,"C:temp0=");
		LCD_ShowNum(120,50,temp0,3,16);
		LCD_ShowString(170,50,100,16,16," Z=");
		LCD_ShowNum(210,50,Ztemp0,3,16);
		LCD_ShowString(260,50,100,16,16," pf");
	}

	return 0;
}


void LCD_Shining(void){
	LCD_Clear(WHITE);
	POINT_COLOR=BLACK;	  
	if(compensation ==1)
		LCD_ShowString(30,10,210,16,16,"Current Mode: Compensation");
	else if(dynamicmode ==1)
		LCD_ShowString(30,10,210,16,16,"Current Mode: Dynamic     ");
	else if(pianomode==1)
		LCD_ShowString(30,10,210,16,16,"Current Mode: Piano       ");
	else
		LCD_ShowString(30,10,210,16,16,"Current Mode: Normal      ");
	LCD_ShowString(30,30,210,16,16,"Gssture Recognized:  ");
		//LCD_ShowString(30,40,210,24,24,"Mode:Judging");
		//LCD_ShowString(30,70,200,16,16,"posture:paper");
		//LCD_ShowString(30,70,200,16,16,"posture:scissors");
	LCD_ShowString(30,50,210,16,16,"Temp C: ");
	POINT_COLOR=BLUE;
	LCD_ShowString(30,70,210,16,16,"Current C0: ");
	LCD_ShowString(30,90,210,16,16,"Current C1: ");
	LCD_ShowString(30,110,210,16,16,"Current C2: ");
	LCD_ShowString(30,130,210,16,16,"Current C3: ");
	LCD_ShowString(30,150,210,16,16,"Current C4: ");
	POINT_COLOR=BLACK;
	LCD_ShowString(30,170,210,16,16,"STJDB C: ");
	LCD_ShowString(30,190,210,16,16,"12345 C: ");
	if(train_status==0)
		LCD_ShowString(30,210,210,16,16,"Train Status: ing...");
	else if(train_status==1)
		LCD_ShowString(30,210,210,16,16,"Train Status: OK.");
	else if(train_status==-1)
		LCD_ShowString(30,210,210,16,16,"Train Status: Failed.");
	else
		LCD_ShowString(30,210,210,16,16,"Train Status: ");
	
 	//LCD_ShowString(30,130,210,24,24,lcd_id);		//��ʾLCD ID
	LCD_ShowString(30,230,210,16,16,"Last recognized: ");
	
	POINT_COLOR=BROWN;
	LCD_ShowString(30,250,180,16,16,"***** Function of Keys ******");
	LCD_ShowString(30,270,180,12,12,"A:Detection of STJDB");
	POINT_COLOR=RED;
	LCD_ShowString(30,285,180,12,12,"*:gesture ST UNtrained");
	LCD_ShowString(30,300,180,12,12,"#:gesture JD UNtrained");
	LCD_ShowString(30,315,180,12,12,"D:gesture  B UNtrained");
	POINT_COLOR=BROWN;
	LCD_ShowString(30,330,180,12,12,"B:Detection of 12345");
	POINT_COLOR=RED;
	LCD_ShowString(30,345,180,12,12,"1:gesture 1 UNtrained");
	LCD_ShowString(30,360,180,12,12,"2:gesture 2 UNtrained");
	LCD_ShowString(30,375,180,12,12,"3:gesture 3 UNtrained");
	LCD_ShowString(30,390,180,12,12,"4:gesture 4 UNtrained");
	LCD_ShowString(30,405,180,12,12,"5:gesture 5 UNtrained");

	LCD_ShowString(30,420,200,12,12,"9:show ALL training data");
	LCD_ShowString(30,435,180,12,12,"C:reset cap value");	
	//MAX=450
}

void LCD_ShowNormalC(){
	
	POINT_COLOR=BLACK;
	LCD_ShowString(30,70,210,16,16,"Current C0: ");
	LCD_ShowNum(120,70,fabs(C0)*100,3,16);
	LCD_ShowString(170,70,100,16,16," Z=");
	LCD_ShowNum(210,70,fabs(ZC0)*100,3,16);
	LCD_ShowString(250,70,100,16,16,"/100 pf");
	
	LCD_ShowString(30,90,210,16,16,"Current C1: ");
	LCD_ShowNum(120,90,fabs(C1)*100,3,16);
	LCD_ShowString(170,90,100,16,16," Z=");
	LCD_ShowNum(210,90,fabs(ZC1)*100,3,16);
	LCD_ShowString(250,90,100,16,16,"/100 pf");
	
	LCD_ShowString(30,110,210,16,16,"Current C2: ");
	LCD_ShowNum(120,110,fabs(C2)*100,3,16);
	LCD_ShowString(170,110,100,16,16," Z=");
	LCD_ShowNum(210,110,fabs(ZC2)*100,3,16);
	LCD_ShowString(250,110,100,16,16,"/100 pf");
	
	LCD_ShowString(30,130,210,16,16,"Current C3: ");
	LCD_ShowNum(120,130,fabs(C3)*100,3,16);
	LCD_ShowString(170,130,100,16,16," Z=");
	LCD_ShowNum(210,130,fabs(ZC3)*100,3,16);
	LCD_ShowString(250,130,100,16,16,"/100 pf");
	
	LCD_ShowString(30,150,210,16,16,"Current C4: ");
	LCD_ShowNum(120,150,fabs(C4)*100,3,16);
	LCD_ShowString(170,150,100,16,16," Z=");
	LCD_ShowNum(210,150,fabs(ZC4)*100,3,16);
	LCD_ShowString(250,150,100,16,16,"/100 pf");
}


int main(void)
{ 
	int mine_or_others=1;
	int row=0;
	int column=0;
	int i=0;
	
	float res0;//float res1,res2,res3;
	float Zres0;//float Zres1,Zres2,Zres3;
	
	
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	delay_init(168);  			//��ʼ����ʱ����
	uart_init(9600);			//��ʼ�����ڲ�����Ϊ50,0000//460800:OK
	LED_Init();					//��ʼ��LED 
	KEY_Init();					//��ʼ������
	BEEP_Init();
 	LCD_Init();				    //LCD��ʼ��
	MATRI4X4KEY_Init();			//������̳�ʼ��
	
	//fdc2214 init :
	while(FDC2214_Init());
	for(i=0;i<5;i++){//�ڶ���fdc2214ֻ����5��
		ZFDC2214_Init();
		delay_ms(50);
	}
	
	if(PFin(1)==0)//���뿪��=ON����Ϊ0
		compensation=1;
	
	dynamicmode=!PFin(3);
	pianomode=!PFin(5);
	
	LCD_ShowString(30,210,210,16,16,"Init Status: OK.");
	printf("Init OK \r\n.");
	delay_ms(1000);//��ʱ1sʹ�������ֿ������ط�ֹ�ַ��ڸ�λ��ťʱӰ����ݡ�
	
	temp0 = Cap_Calculate(0);
	//temp1 = Cap_Calculate(1);//��ȡ��ʼֵ******
	//temp2 = Cap_Calculate(2);
	//temp3 = Cap_Calculate(3);
	Ztemp0 = ZCap_Calculate(0);
	//Ztemp1 = ZCap_Calculate(1);//��ȡ��ʼֵ******
	//Ztemp2 = ZCap_Calculate(2);
	//Ztemp3 = ZCap_Calculate(3);
	//printf("temp0 = %f,temp1 = %f,temp2 = %f,temp3 = %f\n",temp0,temp1,temp2,temp3);
	printf("temp0 = %f,Ztemp0 = %f. \r\n",temp0,Ztemp0);
	
	LCD_Shining();
	
	LCD_ShowString(30,50,100,16,16,"C:temp0=");
	LCD_ShowNum(120,50,temp0,3,16);
	LCD_ShowString(170,50,100,16,16," Z=");
	LCD_ShowNum(210,50,Ztemp0,3,16);
	LCD_ShowString(260,50,100,16,16," pf");
	
 	while(1)
	{
		LED_Off();
		LCD_ShowString(30,210,210,16,16,"Train Status:         ");
		LCD_ShowString(30,30,210,16,16,"Gesture Recognized:  ");
		LCD_ShowString(260,30,100,16,16,"   ");
		
		POINT_COLOR=BROWN;
		LCD_ShowString(30,330,180,12,12,"B:Detection of 12345");
		LCD_ShowString(30,270,180,12,12,"A:Detection of STJDB");
		LCD_ShowString(30,420,200,12,12,"9:show ALL training data");
		LCD_ShowString(30,435,180,12,12,"C:reset cap value");
		
		dynamicmode=!PFin(3);
		if(dynamicmode==0){
			row= -1;
			column= -1;
		}
		else if(dynamicmode==1){
			MATRI4_4KEY_Scan(&row, &column);
			while(!PFin(3)){
				Parse_Key(row,column,temp0);
			}
		}

		res0 = Cap_Calculate(0);//�ɼ�����
		//res1 = Cap_Calculate(1);
		//res2 = Cap_Calculate(2);
		//res3 = Cap_Calculate(3);
		Zres0 = ZCap_Calculate(0);//�ɼ�����
		//Zres1 = ZCap_Calculate(1);
		//Zres2 = ZCap_Calculate(2);
		//Zres3 = ZCap_Calculate(3);

		C4=C3;
		C3=C2;
		C2=C1;
		C1=C0;
		C0=res0-temp0;
		ZC4=ZC3;
		ZC3=ZC2;
		ZC2=ZC1;
		ZC1=ZC0;
		ZC0=Zres0-Ztemp0;
		
		LCD_ShowNormalC();//����������4��printf
		
		//printf("C0-C4:%f,%f,%f,%f,%f\n",C0,C1,C2,C3,C4);
		//printf("CH0;%3.3f CH1;%3.3f CH2;%3.3f CH3;%3.3f. \n",res0-temp0,res1-temp1,res2-temp2,res3-temp3);
		//printf("ZCH0;%3.3f ZCH1;%3.3f ZCH2;%3.3f ZCH3;%3.3f. \n",Zres0-Ztemp0,Zres1-Ztemp1,Zres2-Ztemp2,Zres3-Ztemp3);
		if(printfmode ==2)printf("Compensation CH = %3.3f \r\n",res0-2*Zres0-(temp0-2*Ztemp0));
		if(printfmode ==1)printf("CH0- = %3.3f, ZCH0- = %3.3f \r\n",res0-temp0,Zres0-Ztemp0);
		if(printfmode ==3)printf("CH0 res=%f, ZCH0 res=%f \r\n",res0,Zres0);
		if(printfmode ==4)printf("CH0 abs =%3.3f, ZCH0 abs =%3.3f \r\n",fabs(res0-temp0),fabs(Zres0-Ztemp0));
		

		if(pianomode==1){
			while(!PFin(5)){
				res0 = Cap_Calculate(0);//�ɼ�����
				dynamicmode=!PFin(3);
				C0=res0-temp0;
				if(dynamicmode==1)
					Piano(C0*100);
				LCD_ShowNormalC();
				delay_ms(5);
			}
		}
		else{
			if(MATRI4_4KEY_Scan(&row, &column)==0)
				printf("key pressed row=%d, column=%d \r\n",row,column);
			if(mine_or_others==1){
				if(compensation==1)
					Parse_Key(row,column,temp0-Ztemp0);
				else
					Parse_Key(row,column,temp0);
			}
		}
		
		delay_ms(400);
	} 	
}
