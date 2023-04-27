#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "oled.h"
#include "key.h"
#include "includes.h"
/************************************************
 ALIENTEK战舰STM32开发板UCOS实验
 例10-3 UCOSIII 信号量用于任务同步
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

//UCOSIII中以下优先级用户程序不能使用，ALIENTEK
//将这些优先级分配给了UCOSIII的5个系统内部任务
//优先级0：中断服务服务管理任务 OS_IntQTask()
//优先级1：时钟节拍任务 OS_TickTask()
//优先级2：定时任务 OS_TmrTask()
//优先级OS_CFG_PRIO_MAX-2：统计任务 OS_StatTask()
//优先级OS_CFG_PRIO_MAX-1：空闲任务 OS_IdleTask()
//技术支持：www.openedv.com
//淘宝店铺：http://eboard.taobao.com  
//广州市星翼电子科技有限公司  
//作者：正点原子 @ALIENTEK

//任务优先级
#define START_TASK_PRIO		3
//任务堆栈大小	
#define START_STK_SIZE 		128
//任务控制块
OS_TCB StartTaskTCB;
//任务堆栈	
CPU_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *p_arg);

//任务优先级
#define USART3_TASK_PRIO		6
//任务堆栈大小	
#define USART3_STK_SIZE 		128
//任务控制块
OS_TCB Usart3_TaskTCB;
//任务堆栈	
CPU_STK USART3_TASK_STK[USART3_STK_SIZE];
void usart3_task(void *p_arg);


//任务优先级
#define KEY_TASK_PRIO		4
//任务堆栈大小	
#define KEY_STK_SIZE 		128
//任务控制块
OS_TCB Key_TaskTCB;
//任务堆栈	
CPU_STK KEY_TASK_STK[KEY_STK_SIZE];
void key_task(void *p_arg);

//任务优先级
#define OLED_TASK_PRIO		5
//任务堆栈大小	
#define OLED_STK_SIZE 		128
//任务控制块
OS_TCB Oled_TaskTCB;
//任务堆栈	
CPU_STK OLED_TASK_STK[OLED_STK_SIZE];
void oled_task(void *p_arg);


OS_SEM	SYNC_USART;		//定义一个信号量，用于串口发送同步
OS_SEM  SYNC_OLED;  //定义一个信号量，用于oled显示同步

//主函数
int main(void)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	
	delay_init();  //时钟初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//中断分组配置
	uart_init(1200);   //串口初始化
	LED_Init();         //LED初始化	
	KEY_Init();			//按键初始化
	OLED_Init();		//oled初始化
	
	
	OSInit(&err);		    	//初始化UCOSIII
	OS_CRITICAL_ENTER();	//进入临界区			 
	//创建开始任务
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//任务控制块
				 (CPU_CHAR	* )"start task", 		//任务名字
                 (OS_TASK_PTR )start_task, 			//任务函数
                 (void		* )0,					//传递给任务函数的参数
                 (OS_PRIO	  )START_TASK_PRIO,     //任务优先级
                 (CPU_STK   * )&START_TASK_STK[0],	//任务堆栈基地址
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//任务堆栈深度限位
                 (CPU_STK_SIZE)START_STK_SIZE,		//任务堆栈大小
                 (OS_MSG_QTY  )0,					//任务内部消息队列能够接收的最大消息数目,为0时禁止接收消息
                 (OS_TICK	  )0,					//当使能时间片轮转时的时间片长度，为0时为默认长度，
                 (void   	* )0,					//用户补充的存储区
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //任务选项
                 (OS_ERR 	* )&err);				//存放该函数错误时的返回值
	OS_CRITICAL_EXIT();	//退出临界区	 
	OSStart(&err);      //开启UCOSIII
}


//开始任务函数
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;
	
	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//统计任务                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//如果使能了测量中断关闭时间
    CPU_IntDisMeasMaxCurReset();	
#endif
	
#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //当使用时间片轮转的时候
	 //使能时间片轮转调度功能,时间片长度为1个系统时钟节拍，既1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif	
		
	OS_CRITICAL_ENTER();	//进入临界区
	//创建两个信号量
	OSSemCreate ((OS_SEM*	)&SYNC_USART,
                 (CPU_CHAR*	)"SYNC_USART",
                 (OS_SEM_CTR)0,		
                 (OS_ERR*	)&err);
	OSSemCreate ((OS_SEM*	)&SYNC_OLED,
                 (CPU_CHAR*	)"SYNC_OLED",
                 (OS_SEM_CTR)0,		
                 (OS_ERR*	)&err);
	//创建USART3任务
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
	//创建KEY任务
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
	//创建OLED任务
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
	OS_CRITICAL_EXIT();	//退出临界区
	OSTaskDel((OS_TCB*)0,&err);	//删除start_task任务自身
}


//KEY的任务函数
void key_task(void *p_arg)
{
	u8 key = 0;
	OS_ERR err;
	u8 rkey = 0;
	u32 w = 0;
	LED0 = 0;
	while(1)
	{
		key = KEY_Scan(1);  //扫描按键
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
			OSSemPost(&SYNC_OLED,OS_OPT_POST_1,&err);//发送信号量
			OSSemPost(&SYNC_USART,OS_OPT_POST_1,&err);//发送信号量
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
		OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_PERIODIC,&err);   //延时100ms
	}
}

//USART3的任务函数
void usart3_task(void *p_arg)
{	
	OS_ERR err;
	u32 zhong = 0;
	u8 len = 0;
	while(1)
	{
		OSSemPend(&SYNC_USART,0,OS_OPT_PEND_BLOCKING,0,&err); //请求信号量
		zhong = *(u32*)OSTaskQPend(0, OS_OPT_PEND_BLOCKING, (OS_MSG_SIZE*)&len, NULL, &err);//等待消息队列
		printf("+%.8xB", zhong);
		zhong = 0;
		OSTaskQFlush(&Usart3_TaskTCB, &err);
		//OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_PERIODIC,&err);   //延时100ms
	}
}
//OLED的任务函数
void oled_task(void *p_arg){
	OS_ERR err;
	u32 kg = 0;
	u8 len = 0;
	u8 kgg[10] = " KG";
	u8 kt[12] = "Weight is :";
	OLED_ShowString(0, 0, kt, 16);			//显示数据内容
	OLED_ShowNum(0, 16, kg, 8, 12);	//显示当前重量
	OLED_ShowString(60, 16, kgg, 12);		//显示重量单位
	OLED_Display_Off();
	while(1)
	{
		OSSemPend(&SYNC_OLED,0,OS_OPT_PEND_BLOCKING,0,&err);
		kg = *(u32*)OSTaskQPend(0, OS_OPT_PEND_BLOCKING, (OS_MSG_SIZE*)&len, NULL, &err);//等待消息队列
		OLED_ShowNum(0,16, kg, 10, 12);
		OLED_Refresh_Gram();					//更新显示到OLED
		OSTaskQFlush(&Usart3_TaskTCB, &err);
		OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_PERIODIC,&err);   //延时100ms
	}
}
