#ifndef __NIXIETUBE_H
#define __NIXIETUBE_H	 
#include "sys.h" 
//////////////////////////////////////////////////////////////////////////////////	 									  
////////////////////////////////////////////////////////////////////////////////// 	


void DigitalTube_Init(void); //����ܳ�ʼ��
void Led_Set(int A,int B,int C,int D,int E,int F,int G,int DP);
void SingleTube_Set(int num);
void DigitalTube_Set(int num);		//�������ʾ����
void Tube_delay(int time,int num);	//�������ʱ����
void Tube_clear(int num);			//�������������������λС��5�Ȼ��ߴ���135��ʱ
void Tube_scan_all(void);
void Tube_set_all(void);
void Tube_demo(void);
#endif


