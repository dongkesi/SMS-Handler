/****************************GroundWireMangement ******************************
* File Name          : ring_buffer.c
* Author             : Si Kedong
* Version            : V1.0
* Date               : 12/20/2011
* Description        : 队列的操作函数
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ring_buffer.h"
#include "def.h"
#include <string.h>
/* Global variables ----------------------------------------------------------*/
uint8_t         gsmRxBuffer     [GSM_RX_BUFFER_SIZE];
uint8_t         gsmTxBuffer     [GSM_TX_BUFFER_SIZE];
uint8_t         rs232RxBuffer   [RS232_RX_BUFFER_SIZE];
uint8_t         rs232TxBuffer   [RS232_TX_BUFFER_SIZE];
uint8_t         gpsRxBuffer     [GPS_RX_BUFFER_SIZE];
uint8_t         gpsTxBuffer     [GPS_TX_BUFFER_SIZE];
uint8_t         eventBuffer     [EVENT_BUFFER_SIZE];
uint8_t         exceptionBuffer [EXCEPTION_BUFFER_SIZE];
uint8_t         wireIndexBuffer [WIRE_INDEX_BUFFER_SIZE];
uint8_t         msgIndexBuffer  [MSG_INDEX_BUFFER_SIZE];
CircularQueue   rs232RxQueue,   rs232TxQueue;
CircularQueue   gpsRxQueue,     gpsTxQueue;
CircularQueue   gsmRxQueue,     gsmTxQueue;
CircularQueue   eventQueue,     exceptionQueue;
CircularQueue   wireIndexQueue;
CircularQueue   msgIndexQueue;
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*初始化队列*/
void InitQueue(CircularQueue *Q, uint8_t *buf, uint16_t size) 
{ 
    assert_param(Q != NULL && buf != NULL);
    ENTER_CRITICAL();
    Q->front    = Q->rear = 0; 
    Q->count    = 0;
    Q->buffer   = buf;
    Q->size     = size;
    EXIT_CRITICAL();
} 

/*队列判空*/
bool IsEmptyQueue(CircularQueue *Q) 
{ 
    assert_param(Q != NULL);
    return (Q->count == 0)? TRUE : FALSE; 
}

/*队列判满*/
bool IsFullQueue(CircularQueue *Q)
{
    assert_param(Q != NULL);
    
    ENTER_CRITICAL();
    if (Q->count == Q->size)
    {
        EXIT_CRITICAL();
        return TRUE;
    }
    else
    {
        EXIT_CRITICAL();
        return FALSE;
    }
}

/*入队*/
bool EnQueue(CircularQueue *Q, const uint8_t data) 
{    
    assert_param(Q != NULL);
    if(IsFullQueue(Q))
        return FALSE;

    ENTER_CRITICAL();
    Q->buffer[Q->rear]  = data;
    Q->rear             = (Q->rear+1)%Q->size;
    Q->count++;
    EXIT_CRITICAL();

    return TRUE; 
} 

/*删除队列*/
void DeleteQueue(CircularQueue *Q) 
{ 
    assert_param(Q != NULL);
    if(!IsEmptyQueue(Q))
    { 
        ENTER_CRITICAL();
        Q->front = (Q->front+1)%Q->size;
        Q->count--;
        EXIT_CRITICAL();
    }
} 

/*出队*/
bool DeQueue(CircularQueue *Q, uint8_t *data) 
{ 
    assert_param(Q != NULL);
    if(!IsEmptyQueue(Q))
    {
        ENTER_CRITICAL();
        *data       = Q->buffer[Q->front];
        Q->front    = (Q->front+1)%Q->size;
        Q->count--;
        EXIT_CRITICAL();
        return TRUE;
    }
    return FALSE;
}

/*从队尾出队*/
bool ReverseDeQueue(CircularQueue *Q, uint8_t *data)
{
    assert_param(Q != NULL);
    if(!IsEmptyQueue(Q))
    {
        ENTER_CRITICAL();
        *data       = Q->buffer[Q->rear-1];
        Q->rear     = (Q->rear-1)%Q->size;
        Q->count--;
        EXIT_CRITICAL();
        return TRUE;
    }
    return FALSE;   
}
/*清空队列*/
void ClearQueue(CircularQueue *Q)
{
    assert_param(Q != NULL);
    ENTER_CRITICAL();
    Q->front        = Q->rear = 0; 
    Q->count        = 0;
    EXIT_CRITICAL(); 
}

/*获取队列当前有效数据长度*/
uint16_t GetQueueLength(CircularQueue *Q)
{
    assert_param(Q != NULL);
    return Q->count;
}

/*成批出队：一次出一大块数据*/
bool DeQueueBlock(CircularQueue *Q, uint8_t *data, uint16_t size) 
{ 
    assert_param(Q != NULL && data != NULL);
    if(GetQueueLength(Q)>=size)
    {
        ENTER_CRITICAL();
        memcpy(data, &Q->buffer[Q->front], size);
        Q->front    = (Q->front+size)%Q->size;
        Q->count   -= size;
        EXIT_CRITICAL();
        return TRUE;
    }
    return FALSE;
}

/*成批入队：一次入一大块数据*/
bool EnQueueBlock(CircularQueue *Q, const uint8_t *data, uint16_t size) 
{    
    assert_param(Q != NULL && data != NULL);
    if(IsFullQueue(Q))
        return FALSE;

    ENTER_CRITICAL();
    if (Q->size - Q->count >= size)
    {
        memcpy(&Q->buffer[Q->rear], data, size);
        Q->rear     = (Q->rear+size)%Q->size;
        Q->count   += size;
        EXIT_CRITICAL();
        return TRUE;
    }
    else
    {
        EXIT_CRITICAL();
        return FALSE; 
    }
}

/*********************************END OF FILE***********************************/
