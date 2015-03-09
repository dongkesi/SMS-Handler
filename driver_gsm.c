/****************************GroundWireMangement ******************************
* File Name          : driver_gsm.c
* Author             : Si Kedong
* Version            : V1.0
* Date               : 12/20/2011
* Description        : GSM驱动
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
#include "config.h"
#include "driver_includes.h"
#include "middle_includes.h"
#include "def.h"
#include <stdio.h>
/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static volatile uint8_t gsmCmdUpdateFlag    = 0;
static volatile uint8_t gsmTCFlag           = 0;
/* Private function prototypes -----------------------------------------------*/
static void GPIO_Configuration  (void);
static void USART_Configuration (void);
static void EXTI_Configuration  (void);
/* Private functions ---------------------------------------------------------*/

/*GSM设备初始化*/
int GSM_Init(void)
{
    InitQueue(&gsmRxQueue, gsmRxBuffer, GSM_RX_BUFFER_SIZE);
    InitQueue(&gsmTxQueue, gsmTxBuffer, GSM_TX_BUFFER_SIZE);
    InitQueue(&msgIndexQueue, msgIndexBuffer, MSG_INDEX_BUFFER_SIZE);
    /* Enable GPIOA clock */
    RCC_APB2PeriphClockCmd(GSM_RCC_APB2Periph_GPIO | GSMPOWER_RCC_APB2Periph_GPIO | RCC_APB2Periph_AFIO,
                             ENABLE);
    /* Enable USART2 clocks */
#ifdef STD
    RCC_APB1PeriphClockCmd(GSM_RCC_APB1Periph_USART, ENABLE);
#else
    RCC_APB2PeriphClockCmd(GSM_RCC_APB1Periph_USART, ENABLE);
#endif  
    GPIO_Configuration();
    USART_Configuration();
    EXTI_Configuration();
    return 0;
}

/*打开GSM设备文件*/
int GSM_Open(void)
{
    return 0;
}

/*关闭GSM设备文件*/
int GSM_Close(void)
{
    /* 为什么不能关闭UE？关闭后下次打开时传输第一字节出错；*/
    //USART_Cmd(GSM_USART, DISABLE);
    /* Disable GPIOA clock */
    /*GPIO别随便关闭，因为其他设备可能用到了该端口其他引脚*/
//  RCC_APB2PeriphClockCmd(GSM_RCC_APB2Periph_GPIO | GSMPOWER_RCC_APB2Periph_GPIO, DISABLE);
    /* Disable USART2 clocks */
//#ifdef STD
//      RCC_APB1PeriphClockCmd(GSM_RCC_APB1Periph_USART, DISABLE);
//#else
//  RCC_APB2PeriphClockCmd(GSM_RCC_APB1Periph_USART, DISABLE);
//#endif    
    return 0;       
}


/*******************************************************************************
* Function Name  : GSM_Block_Read
* Description    : 阻塞读GSM串口
* Input          : - nbytes: 要读取的字节数
*                  - timeout: 超时
* Output         : - buf: 读取字节的存储位置
* Return         : 实际读出的字节数
*******************************************************************************/
uint32_t GSM_Block_Read(void *buf, uint32_t nbytes,  uint32_t timeOut)
{
    volatile uint32_t   oldTime = 0;
    volatile uint32_t   newTime = 0;
    uint32_t            i       = 0;
    uint8_t            *pBuf    = buf;

    assert_param(buf != NULL);
    /*复位串口时间计数器*/
    ResetUsartTime();
    oldTime = GetUsartTime();
    /*查询是否收到数据*/
    do
    {
        newTime = GetUsartTime();   
    }
    while(GSM_GetUpdateBit() == 0 && newTime-oldTime<timeOut);

    if (GSM_GetUpdateBit() == 1)
        GSM_ResetUpdateBit();
    if (newTime-oldTime>timeOut)
        return TIMEOUT;

    ResetUsartTime();
    while(i<nbytes)
    {
        oldTime = GetUsartTime();
        /* 若缓冲区的数据为空，循环等待,直到超时*/
        while(IsEmptyQueue(&gsmRxQueue))
        {
            /*获得新时刻*/
            newTime = GetUsartTime();
            /*判断是否超时*/
            if (((newTime-oldTime>timeOut) && (timeOut != 0))    /*头超时*/
                || (newTime-oldTime>USART_TAIL_TIMEOUT && i!=0)) /*尾超时*/
            {
                goto QUIT;
            }
        }
        DeQueue(&gsmRxQueue, &pBuf[i]);
//      if(((uint8_t *)buf)[i] == '\0')
//          break;
        i++;
    }
QUIT:
    pBuf[i] = '\0';
    /*返回读取的字节数*/
    return i;   
}

/*******************************************************************************
* Function Name  : GSM_Block_Write
* Description    : 阻塞写GSM串口
* Input          : - buf: 写入的数据
*                  - nbytes: 写入的字节数
*                  - timeout: 超时
* Output         : none
* Return         : 实际写入的字节数
*******************************************************************************/
uint32_t GSM_Block_Write(const void *buf, uint32_t nbytes, uint32_t timeOut)
{
    volatile uint32_t   oldTime = 0;
    volatile uint32_t   newTime = 0;
    uint32_t            i       =0;
    const uint8_t      *pBuf    = buf;

    assert_param(buf != NULL);
    ResetUsartTime();
    /*关闭中断*/
    USART_ITConfig(GSM_USART, USART_IT_TXE, DISABLE);
    while(i<nbytes)
    {
        if(/*((const uint8_t *)buf)[i] == '\0' || */!EnQueue(&gsmTxQueue, pBuf[i]))
            break;
        i++;
        
    }    
    USART_ClearFlag(GSM_USART, USART_FLAG_TC);
    /*因为配置好USART时，进入中断后关闭发送中断，所以在写时重新开启*/
    USART_ITConfig(GSM_USART, USART_IT_TXE, ENABLE);

    oldTime = GetUsartTime();
    /*等待传输完成*/
    do
    {
        newTime = GetUsartTime();
    }
    while(USART_GetFlagStatus(GSM_USART, USART_FLAG_TC) == RESET && newTime - oldTime < timeOut);
    /*返回写入的字节数*/
    return i;
}
/*******************************************************************************
* Function Name  : GSM_Read
* Description    : 非阻塞读GSM串口
* Input          : - nbytes: 要读出的字节数
* Output         : - buf: 读出的数据存储位置
* Return         : 实际读出的字节数
*******************************************************************************/
unsigned int GSM_Read(void *buf, unsigned int nbytes)
{
    uint16_t    i       = 0;
    uint8_t    *pBuf    = buf;

    assert_param(buf != NULL);
//  USART_ITConfig(GSM_USART, USART_IT_RXNE, DISABLE);
    while(i<nbytes)
    {
    /*队列中无数据，或者收到’/0‘字符退出循环*/
        if(!DeQueue(&gsmRxQueue, &pBuf[i])/* || ((uint8_t *)buf)[i] == '\0'*/)
            break;
        i++;
    }
//  USART_ITConfig(GSM_USART, USART_IT_RXNE, ENABLE);
    return i;
}

/*******************************************************************************
* Function Name  : GSM_Write
* Description    : 非阻塞读GSM串口
* Input          : - buf: 待写入的数据
*                  - nbytes: 要写入的字节数
* Output         : none
* Return         : 实际写入的字节数
*******************************************************************************/
#if 0
unsigned int GSM_Write(const void *buf, unsigned int nbytes)
{
    unsigned int    count   = 0;
    const char     *string  = buf;  
    
    assert_param(buf != NULL);
    while(count < nbytes)
    {
        /*write a character to the USART */
        USART_SendData(GSM_USART, string[count]);
        
        /* Loop until the end of transmission */
        while(USART_GetFlagStatus(GSM_USART, USART_FLAG_TC) == RESET)
        {
        }
        count++;
    }
    return count;
}       
#else
unsigned int GSM_Write(const void *buf, unsigned int nbytes)
{
    uint16_t        i       = 0;
    const uint8_t  *pBuf    = buf;

    assert_param(buf != NULL);
    USART_ITConfig(GSM_USART, USART_IT_TXE, DISABLE);
    while(i<nbytes)
    {
        if(/*((const uint8_t *)buf)[i] == '\0' || */!EnQueue(&gsmTxQueue, pBuf[i]))
            break;
        i++;
    }
    USART_ITConfig(GSM_USART, USART_IT_TXE, ENABLE);

    return i;
}
#endif 
/*******************************************************************************
* Function Name  : GPIO_Configuration
* Description    : Configures the different GPIO ports.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* Configure GSM_GPIO RTS and GSM_USART Tx as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin     = GSM_GPIO_RTSPin | GSM_GPIO_TxPin;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF_PP;
    GPIO_Init(GSM_GPIO, &GPIO_InitStructure);
    
    /* Configure GSM_GPIO CTS and GSM_USART Rx as input floating */
    GPIO_InitStructure.GPIO_Pin     = GSM_GPIO_CTSPin | GSM_GPIO_RxPin | GSM_GPIO_VDDEXTPin | GSM_GPIO_RIPin;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GSM_GPIO, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin     = GSMPOWER_GPIO_Pin;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_Out_PP;
    GPIO_Init(GSMPOWER_GPIO, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin     = GSM_GPIO_DTRPin;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_Out_PP;
    GPIO_Init(GSM_GPIO, &GPIO_InitStructure);

}

/*******************************************************************************
* Function Name  : EXTI_Configuration
* Description    : GSM模块短信中断配置
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void EXTI_Configuration(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    GPIO_EXTILineConfig(GSM_GPIO_PortSourceGPIO, GSM_GPIO_PinSourceRI);
      
    EXTI_InitStructure.EXTI_Line    = EXTI_Line_GSM_RI;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

/*******************************************************************************
* Function Name  : USART_Configuration
* Description    : Configures the USART3.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void USART_Configuration(void)
{
    USART_InitTypeDef USART_InitStructure;
    
    /* USART2 configuration ------------------------------------------------------*/
    /* USART3 configured as follow:
        - BaudRate = 115200 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
    */
    /*配置GSM_USART*/
    USART_InitStructure.USART_BaudRate              = GSM_BAUDRATE;
    USART_InitStructure.USART_WordLength            = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits              = USART_StopBits_1;
    USART_InitStructure.USART_Parity                = USART_Parity_No;
    // USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
    USART_InitStructure.USART_HardwareFlowControl   = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                  = USART_Mode_Rx | USART_Mode_Tx;
    
    USART_Init(GSM_USART, &USART_InitStructure);

    /*为了使第一字节正确传输，清除TC位*/
    USART_ClearFlag(GSM_USART,USART_FLAG_TC); 
    /* Enable the USART Transmoit interrupt: this interrupt is generated when the 
     USART1 transmit data register is empty */
//  USART_ITConfig(GSM_USART, USART_IT_TC, ENABLE);  
    USART_ITConfig(GSM_USART, USART_IT_TXE, ENABLE);
    USART_ITConfig(GSM_USART, USART_IT_RXNE, ENABLE);
    USART_ITConfig(GSM_USART, USART_IT_IDLE, ENABLE);
    /* Enable GSM_USART */
    USART_Cmd(GSM_USART, ENABLE);

}



/*******************************************************************************
* Function Name  : fputc
* Description    : Retargets the C library printf function to the USART.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int fputc(int ch, FILE *f)
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART */
    USART_SendData(GSM_USART, (u8) ch);
    /* Loop until the end of transmission */
    while(USART_GetFlagStatus(GSM_USART, USART_FLAG_TC) == RESET)
    {
    }
    return ch;
}

/*******************************************************************************
* Function Name  : GSM_PowerOff
* Description    : GSM模块关机，POWERKEY引脚置高一秒
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void GSM_PowerOff(void)
{
    GPIO_SetBits(GSM_GPIO, GSM_GPIO_DTRPin);
    
    GPIO_ResetBits(GSMPOWER_GPIO, GSMPOWER_GPIO_Pin);
    Delay(1);   
    GPIO_SetBits(GSMPOWER_GPIO, GSMPOWER_GPIO_Pin);
    Delay(1500);
    GPIO_ResetBits(GSMPOWER_GPIO, GSMPOWER_GPIO_Pin);
    Delay(3000);
}
/*******************************************************************************
* Function Name  : GSM_PowerOn
* Description    : GSM模块开机；按步骤开机
* Input          : - step: 开机步骤
* Output         : None
* Return         : None
*******************************************************************************/
void GSM_PowerOn(uint8_t step)
{
    if (step == 1)
    {
        GPIO_SetBits(GSM_GPIO, GSM_GPIO_DTRPin);
        GPIO_ResetBits(GSMPOWER_GPIO, GSMPOWER_GPIO_Pin);       
    }
    else if (step == 2)
    {
        GPIO_SetBits(GSMPOWER_GPIO, GSMPOWER_GPIO_Pin);     
    }
    else if (step == 3)
    {
        GPIO_ResetBits(GSMPOWER_GPIO, GSMPOWER_GPIO_Pin);       
    }
}

/*设置GSM串口更新位，该位在串口空闲时设置*/
void GSM_SetUpdateBit(void)
{
    ENTER_CRITICAL();
    gsmCmdUpdateFlag = 1;
    EXIT_CRITICAL();
}

/*复位GSM串口更新位*/
void GSM_ResetUpdateBit(void)
{
    ENTER_CRITICAL();
    gsmCmdUpdateFlag = 0;
    EXIT_CRITICAL();
}

/*获取GSM串口更新位*/
uint8_t GSM_GetUpdateBit(void)
{
    return gsmCmdUpdateFlag;
}

/*********************************END OF FILE***********************************/
