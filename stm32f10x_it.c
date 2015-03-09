/**
  ******************************************************************************
  * @file    Project/Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.0.0
  * @date    04/06/2009
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */ 

/* Includes ------------------------------------------------------------------*/

#include "includes.h"

/** @addtogroup Template_Project
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t g_secTicks = 0;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval : None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval : None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval : None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval : None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval : None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval : None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval : None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval : None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval : None
  */
void SysTick_Handler(void)
{
    DecreaseTimingDelay();
    DecreaseShortTime();
    DecreaseLongTime();
    DecreaseFaultTime();
    DecreaseQueryPeriod();

//  KEY_Delay();
    KEY_Scan();
    Flash();
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval : None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */
void EXTI9_5_IRQHandler(void)
{
    SetLedFlashMode(MODE3);
    
#if CFG_POWER_MANAGEMENT > 0
    /*设置停机延时*/
    SetStopDelayTime(STOP_DELAY);
#endif

    if(EXTI_GetITStatus(EXTI_Line_GSM_RI) != RESET)
    {
        /* Clear the Key Button EXTI line pending bit */
        EXTI_ClearITPendingBit(EXTI_Line_GSM_RI);
        SetMsgRecFlag();
    }
}

void EXTI15_10_IRQHandler(void)
{
    SetLedFlashMode(MODE6);
    
#if CFG_POWER_MANAGEMENT > 0
    /*设置停机延时*/
    SetStopDelayTime(STOP_DELAY);
#endif

    if(EXTI_GetITStatus(EXTI_Line_RxPin) != RESET)
    {
        /* Clear the Key Button EXTI line pending bit */
        EXTI_ClearITPendingBit(EXTI_Line_RxPin);
//      RS232_RX_EXTI_Config(DISABLE);
    }
}
 
void USART3_IRQHandler(void)
{
    uint8_t tmpRx;
    uint8_t tmpTx;

#if CFG_POWER_MANAGEMENT > 0
    /*设置停机延时*/
    SetStopDelayTime(STOP_DELAY);
#endif
    /*接收中断处理*/
    if(USART_GetITStatus(RS232_USART, USART_IT_RXNE) != RESET)
    {
        /* Read one byte from the receive data register */
        /*持续接收数据，当队列满时，照常接收但不入队*/
        tmpRx = USART_ReceiveData(RS232_USART);
        EnQueue(&rs232RxQueue, tmpRx);
    }
    if(USART_GetITStatus(RS232_USART, USART_IT_IDLE) != RESET)
    {
        /*清除中断标志*/
        USART_GetITStatus(RS232_USART, USART_IT_IDLE);
        USART_ReceiveData(RS232_USART);

        RS232_SetUpdateBit();
    }
    if (USART_GetITStatus(RS232_USART, USART_IT_ORE) != RESET)
    {
        USART_GetITStatus(RS232_USART, USART_IT_ORE);
        USART_ReceiveData(RS232_USART);
    }
    /*发送中断处理*/
    if(USART_GetITStatus(RS232_USART, USART_IT_TXE) != RESET)
    {
        /* Write one byte to the transmit data register */
        if(DeQueue(&rs232TxQueue, &tmpTx))
            USART_SendData(RS232_USART, tmpTx);
        else
            /*为了保证发送完数据时不持续响应中断，关闭该中断*/
            USART_ITConfig(RS232_USART, USART_IT_TXE, DISABLE);   
    }
}
void USART2_IRQHandler(void)
{
    uint8_t tmpRx;
    uint8_t tmpTx;

//#if CFG_POWER_MANAGEMENT > 0
//  /*设置停机延时*/
////    SetStopDelayTime(STOP_DELAY);
//#endif    
    /*接收中断处理*/
    if(USART_GetITStatus(GSM_USART, USART_IT_RXNE) != RESET)
    {
        /*队列满时,去除队首数据*/
        if (IsFullQueue(&gsmRxQueue))
        {
            DeleteQueue(&gsmRxQueue);
        }
        /* Read one byte from the receive data register */
        tmpRx = USART_ReceiveData(GSM_USART);
        EnQueue(&gsmRxQueue, tmpRx);
    }

    if(USART_GetITStatus(GSM_USART, USART_IT_IDLE) != RESET)
    {
        /*清除中断标志*/
        USART_GetITStatus(GSM_USART, USART_IT_IDLE);
        USART_ReceiveData(GSM_USART);

        GSM_SetUpdateBit();
    }

    if (USART_GetITStatus(GSM_USART, USART_IT_ORE) != RESET)
    {
        USART_GetITStatus(GSM_USART, USART_IT_ORE);
        USART_ReceiveData(GSM_USART);
    }   
    if(USART_GetITStatus(GSM_USART, USART_IT_TXE) != RESET)
    {
        /* Write one byte to the transmit data register */
        if(DeQueue(&gsmTxQueue, &tmpTx))
            USART_SendData(GSM_USART, tmpTx);
        else
            /*为了保证发送完数据时不持续响应中断，关闭该中断*/
            USART_ITConfig(GSM_USART, USART_IT_TXE, DISABLE);   
    }
}

void USART1_IRQHandler(void)
{
    uint8_t tmpRx;
    uint8_t tmpTx;

    if(USART_GetITStatus(GPS_USART, USART_IT_RXNE) != RESET)
    {
        /*队列满时,去除队首数据*/
        if (IsFullQueue(&gpsRxQueue))
        {
            DeleteQueue(&gpsRxQueue);
        }
        /*接收数据,入队*/
        /* Read one byte from the receive data register */
        tmpRx = USART_ReceiveData(GPS_USART);
        EnQueue(&gpsRxQueue, tmpRx);
    }

    if(USART_GetITStatus(GPS_USART, USART_IT_IDLE) != RESET)
    {
        /*清除中断标志*/
        USART_GetITStatus(GPS_USART, USART_IT_IDLE);
        tmpRx=USART_ReceiveData(GPS_USART);
        GPS_SetUpdateBit();
    }

    if (USART_GetITStatus(GPS_USART, USART_IT_ORE) != RESET)
    {
        USART_GetITStatus(GPS_USART, USART_IT_ORE);
        USART_ReceiveData(GPS_USART);
    }

    if(USART_GetITStatus(GPS_USART, USART_IT_TXE) != RESET)
    {
        if(DeQueue(&gpsTxQueue, &tmpTx))
            /* Write one byte to the transmit data register */
            USART_SendData(GPS_USART, tmpTx);
        else
            /*为了保证发送完数据时不持续响应中断，关闭该中断*/
            USART_ITConfig(GPS_USART, USART_IT_TXE, DISABLE);   
    }
}


void RTC_IRQHandler(void)
{
#if CFG_SHORT_KEY > 0
    static uint8_t count = 0;
#endif
    if (RTC_GetITStatus(RTC_IT_SEC) != RESET)
    {
        /* Clear the RTC Second interrupt */
        RTC_ClearITPendingBit(RTC_IT_SEC);
        /* Wait until last write operation on RTC registers has finished */
        RTC_WaitForLastTask();
        g_secTicks++;
        DecreaseWireFsmTimeOut();
        DecreaseScreenDisplayTime();
        DecreaseAdcSampleTime();
#if CFG_POWER_MANAGEMENT > 0
        DecreaseStopDelayTime();
#endif
#if CFG_SHORT_KEY > 0
        count++;
        if (count == 3)
        {
            ClearLastKeyValue();
            count = 0;
        }
#endif
    }

    if(RTC_GetITStatus(RTC_IT_ALR) != RESET)
    {
        /* Check if the Wake-Up flag is set */
        if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET)
        {
          /* Clear Wake Up flag */
          PWR_ClearFlag(PWR_FLAG_WU);
        }
        EXTI_ClearITPendingBit(EXTI_Line17);
        /* Wait until last write operation on RTC registers has finished */
        RTC_WaitForLastTask();   
        /* Clear RTC Alarm interrupt pending bit */
        RTC_ClearITPendingBit(RTC_IT_ALR);
        /* Wait until last write operation on RTC registers has finished */
        RTC_WaitForLastTask();
    }
}

void WWDG_IRQHandler(void)
{
  /* Update WWDG counter */
  WWDG_SetCounter(0x7F);

  /* Clear EWI flag */
  WWDG_ClearFlag();
}

void ADC1_2_IRQHandler(void)
{
    /* Clear ADC1 AWD pending interrupt bit */
    ADC_ClearITPendingBit(POWER_ADC, ADC_IT_AWD);
}


void EXTI0_IRQHandler(void)
{
    KeyInterruptFlag = 1;
    SetLedFlashMode(MODE3);
#if CFG_POWER_MANAGEMENT > 0
    /*设置停机延时*/
    SetStopDelayTime(STOP_DELAY);
#endif
    SetScreenDisplayTime(SCREEN_DISPLAY_TIME);
    if(EXTI_GetITStatus(EXTI_Line_RightKey) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line_RightKey); 
    }
}

void EXTI1_IRQHandler(void)
{
    KeyInterruptFlag = 1;
    SetLedFlashMode(MODE3);
#if CFG_POWER_MANAGEMENT > 0
    /*设置停机延时*/
    SetStopDelayTime(STOP_DELAY);
#endif
    SetScreenDisplayTime(SCREEN_DISPLAY_TIME);
    if(EXTI_GetITStatus(EXTI_Line_MiddleKey) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line_MiddleKey);    
    }
}

void EXTI2_IRQHandler(void)
{
    KeyInterruptFlag = 1;
    SetLedFlashMode(MODE3);
#if CFG_POWER_MANAGEMENT > 0
    /*设置停机延时*/
    SetStopDelayTime(STOP_DELAY);
#endif
    SetScreenDisplayTime(SCREEN_DISPLAY_TIME);
    if(EXTI_GetITStatus(EXTI_Line_PowerKey) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line_PowerKey); 
    }
}

void EXTI3_IRQHandler(void)
{
    KeyInterruptFlag = 1;
    SetLedFlashMode(MODE3);
#if CFG_POWER_MANAGEMENT > 0
    /*设置停机延时*/
    SetStopDelayTime(STOP_DELAY);
#endif
    SetScreenDisplayTime(SCREEN_DISPLAY_TIME);
    if(EXTI_GetITStatus(EXTI_Line_LeftKey) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line_LeftKey);  
    }
}

void RTCAlarm_IRQHandler(void)
{
    SetLedFlashMode(MODE3);
#if CFG_POWER_MANAGEMENT > 0
    /*设置停机延时*/
    SetStopDelayTime(STOP_DELAY);
#endif
//  if(EXTI_GetITStatus(EXTI_Line17) != RESET)
//  {
//      EXTI_ClearITPendingBit(EXTI_Line17);    
//  }
    if(RTC_GetITStatus(RTC_IT_ALR) != RESET)
    {
        /* Clear EXTI line17 pending bit */
        EXTI_ClearITPendingBit(EXTI_Line17);
        
        /* Check if the Wake-Up flag is set */
        if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET)
        {
          /* Clear Wake Up flag */
          PWR_ClearFlag(PWR_FLAG_WU);
        }
        
        /* Wait until last write operation on RTC registers has finished */
        RTC_WaitForLastTask();   
        /* Clear RTC Alarm interrupt pending bit */
        RTC_ClearITPendingBit(RTC_IT_ALR);
        /* Wait until last write operation on RTC registers has finished */
        RTC_WaitForLastTask();
    }
}
/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
