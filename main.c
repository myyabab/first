#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "oled.h"
#include "key.h"
#include "includes.h"
/************************************************
 ALIENTEKս��STM32������UCOSʵ��
 ��10-3 UCOSIII �ź�����������ͬ��
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/

//UCOSIII���������ȼ��û�������ʹ�ã�ALIENTEK
//����Щ���ȼ��������UCOSIII��5��ϵͳ�ڲ�����
//���ȼ�0���жϷ������������� OS_IntQTask()
//���ȼ�1��ʱ�ӽ������� OS_TickTask()
//���ȼ�2����ʱ���� OS_TmrTask()
//���ȼ�OS_CFG_PRIO_MAX-2��ͳ������ OS_StatTask()
//���ȼ�OS_CFG_PRIO_MAX-1���������� OS_IdleTask()
//����֧�֣�www.openedv.com
//�Ա����̣�http://eboard.taobao.com  
//������������ӿƼ����޹�˾  
//���ߣ�����ԭ�� @ALIENTEK

//�������ȼ�
#define START_TASK_PRIO		3
//�����ջ��С	
#define START_STK_SIZE 		128
//������ƿ�
OS_TCB StartTaskTCB;
//�����ջ	
CPU_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *p_arg);

//�������ȼ�
#define USART3_TASK_PRIO		6
//�����ջ��С	
#define USART3_STK_SIZE 		128
//������ƿ�
OS_TCB Usart3_TaskTCB;
//�����ջ	
CPU_STK USART3_TASK_STK[USART3_STK_SIZE];
void usart3_task(void *p_arg);


//�������ȼ�
#define KEY_TASK_PRIO		4
//�����ջ��С	
#define KEY_STK_SIZE 		128
//������ƿ�
OS_TCB Key_TaskTCB;
//�����ջ	
CPU_STK KEY_TASK_STK[KEY_STK_SIZE];
void key_task(void *p_arg);

//�������ȼ�
#define OLED_TASK_PRIO		5
//�����ջ��С	
#define OLED_STK_SIZE 		128
//������ƿ�
OS_TCB Oled_TaskTCB;
//�����ջ	
CPU_STK OLED_TASK_STK[OLED_STK_SIZE];
void oled_task(void *p_arg);


OS_SEM	SYNC_USART;		//����һ���ź��������ڴ��ڷ���ͬ��
OS_SEM  SYNC_OLED;  //����һ���ź���������oled��ʾͬ��

//������
int main(void)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	
	delay_init();  //ʱ�ӳ�ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�жϷ�������
	uart_init(1200);   //���ڳ�ʼ��
	LED_Init();         //LED��ʼ��	
	KEY_Init();			//������ʼ��
	OLED_Init();		//oled��ʼ��
	
	
	OSInit(&err);		    	//��ʼ��UCOSIII
	OS_CRITICAL_ENTER();	//�����ٽ���			 
	//������ʼ����
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//������ƿ�
				 (CPU_CHAR	* )"start task", 		//��������
                 (OS_TASK_PTR )start_task, 			//������
                 (void		* )0,					//���ݸ��������Ĳ���
                 (OS_PRIO	  )START_TASK_PRIO,     //�������ȼ�
                 (CPU_STK   * )&START_TASK_STK[0],	//�����ջ����ַ
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//�����ջ�����λ
                 (CPU_STK_SIZE)START_STK_SIZE,		//�����ջ��С
                 (OS_MSG_QTY  )0,					//�����ڲ���Ϣ�����ܹ����յ������Ϣ��Ŀ,Ϊ0ʱ��ֹ������Ϣ
                 (OS_TICK	  )0,					//��ʹ��ʱ��Ƭ��תʱ��ʱ��Ƭ���ȣ�Ϊ0ʱΪĬ�ϳ��ȣ�
                 (void   	* )0,					//�û�����Ĵ洢��
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //����ѡ��
                 (OS_ERR 	* )&err);				//��Ÿú�������ʱ�ķ���ֵ
	OS_CRITICAL_EXIT();	//�˳��ٽ���	 
	OSStart(&err);      //����UCOSIII
}


//��ʼ������
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;
	
	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//ͳ������                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//���ʹ���˲����жϹر�ʱ��
    CPU_IntDisMeasMaxCurReset();	
#endif
	
#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //��ʹ��ʱ��Ƭ��ת��ʱ��
	 //ʹ��ʱ��Ƭ��ת���ȹ���,ʱ��Ƭ����Ϊ1��ϵͳʱ�ӽ��ģ���1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif	
		
	OS_CRITICAL_ENTER();	//�����ٽ���
	//���������ź���
	OSSemCreate ((OS_SEM*	)&SYNC_USART,
                 (CPU_CHAR*	)"SYNC_USART",
                 (OS_SEM_CTR)0,		
                 (OS_ERR*	)&err);
	OSSemCreate ((OS_SEM*	)&SYNC_OLED,
                 (CPU_CHAR*	)"SYNC_OLED",
                 (OS_SEM_CTR)0,		
                 (OS_ERR*	)&err);
	//����USART3����
	OSTaskCreate((OS_TCB 	* )&Usart3_TaskTCB,		
				 (CPU_CHAR	* )"Usart3 task", 		
                 (OS_TASK_PTR )usart3_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )USART3_TASK_PRIO,     
                 (CPU_STK   * )&USART3_TASK_STK[0],	
                 (CPU_STK_SIZE)USART3_STK_SIZE/10,	
                 (CPU_STK_SIZE)USART3_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,  					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);			
	//����KEY����
	OSTaskCreate((OS_TCB 	* )&Key_TaskTCB,		
				 (CPU_CHAR	* )"Key task", 		
                 (OS_TASK_PTR )key_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )KEY_TASK_PRIO,     
                 (CPU_STK   * )&KEY_TASK_STK[0],	
                 (CPU_STK_SIZE)KEY_STK_SIZE/10,	
                 (CPU_STK_SIZE)KEY_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,  					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);			
	//����OLED����
	OSTaskCreate((OS_TCB 	* )&Oled_TaskTCB,		
				 (CPU_CHAR	* )"Oled task", 		
                 (OS_TASK_PTR )oled_task, 			
                 (void		* )0,					
                 (OS_PRIO	  )OLED_TASK_PRIO,     
                 (CPU_STK   * )&OLED_TASK_STK[0],	
                 (CPU_STK_SIZE)OLED_STK_SIZE/10,	
                 (CPU_STK_SIZE)OLED_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,  					
                 (void   	* )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR 	* )&err);				 
	OS_CRITICAL_EXIT();	//�˳��ٽ���
	OSTaskDel((OS_TCB*)0,&err);	//ɾ��start_task��������
}


//KEY��������
void key_task(void *p_arg)
{
	u8 key = 0;
	OS_ERR err;
	u8 rkey = 0;
	u32 w = 0;
	LED0 = 0;
	while(1)
	{
		key = KEY_Scan(1);  //ɨ�谴��
		if(key == 3 | key == 1){
			rkey = key;
		}
		if(key == KEY0_PRES){
			w = 0;
			OLED_Display_On();
		}else if(key == KEY2_PRES){
			w = 0;
			OLED_Display_Off();
		}
		if(rkey == KEY0_PRES && key != WKUP_PRES && key != KEY1_PRES)	
		{
			LED0 = 1;
			LED1 = 0;
			OSTaskQFlush(&Usart3_TaskTCB, &err);
			OSTaskQFlush(&Oled_TaskTCB, &err);
			OSTaskQPost(&Usart3_TaskTCB, (void*)&w, (OS_MSG_SIZE)4, OS_OPT_POST_FIFO, &err);
			OSTaskQPost(&Oled_TaskTCB, (void*)&w, (OS_MSG_SIZE)4, OS_OPT_POST_FIFO, &err);
			OSSemPost(&SYNC_OLED,OS_OPT_POST_1,&err);//�����ź���
			OSSemPost(&SYNC_USART,OS_OPT_POST_1,&err);//�����ź���
		}else if(rkey == KEY2_PRES){
			w = 0;
			LED0 = 0;
			LED1 = 1;
		}
		if(key == WKUP_PRES){
			if(w<=0XFFFFFF9B){
				w += 100;
			}else{
				w =0XFFFFFFFF;
			}
			OSTaskQFlush(&Usart3_TaskTCB, &err);
			OSTaskQPost(&Usart3_TaskTCB, (void*)&w, (OS_MSG_SIZE)sizeof(u32), OS_OPT_POST_FIFO, &err);
		}else if(key == KEY1_PRES){
			if(w>=100){
				w -= 100;
			}else {
				w = 0;
			}
			OSTaskQFlush(&Usart3_TaskTCB, &err);
			OSTaskQPost(&Usart3_TaskTCB, (void*)&w, (OS_MSG_SIZE)sizeof(u32), OS_OPT_POST_FIFO, &err);
		}
		OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_PERIODIC,&err);   //��ʱ100ms
	}
}

//USART3��������
void usart3_task(void *p_arg)
{	
	OS_ERR err;
	u32 zhong = 0;
	u8 len = 0;
	while(1)
	{
		OSSemPend(&SYNC_USART,0,OS_OPT_PEND_BLOCKING,0,&err); //�����ź���
		zhong = *(u32*)OSTaskQPend(0, OS_OPT_PEND_BLOCKING, (OS_MSG_SIZE*)&len, NULL, &err);//�ȴ���Ϣ����
		printf("+%.8xB", zhong);
		zhong = 0;
		OSTaskQFlush(&Usart3_TaskTCB, &err);
		//OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_PERIODIC,&err);   //��ʱ100ms
	}
}
//OLED��������
void oled_task(void *p_arg){
	OS_ERR err;
	u32 kg = 0;
	u8 len = 0;
	u8 kgg[10] = " KG";
	u8 kt[12] = "Weight is :";
	OLED_ShowString(0, 0, kt, 16);			//��ʾ��������
	OLED_ShowNum(0, 16, kg, 8, 12);	//��ʾ��ǰ����
	OLED_ShowString(60, 16, kgg, 12);		//��ʾ������λ
	OLED_Display_Off();
	while(1)
	{
		OSSemPend(&SYNC_OLED,0,OS_OPT_PEND_BLOCKING,0,&err);
		kg = *(u32*)OSTaskQPend(0, OS_OPT_PEND_BLOCKING, (OS_MSG_SIZE*)&len, NULL, &err);//�ȴ���Ϣ����
		OLED_ShowNum(0,16, kg, 10, 12);
		OLED_Refresh_Gram();					//������ʾ��OLED
		OSTaskQFlush(&Usart3_TaskTCB, &err);
		OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_PERIODIC,&err);   //��ʱ100ms
	}
}
