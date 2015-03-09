/****************************GroundWireMangement ******************************
* File Name          : smsthread.c
* Author             : Si Kedong
* Version            : V1.0
* Date               : 12/20/2011
* Description        : 该文件是短信收发的核心处理过程，一个简单的状态机调度
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "includes.h"
#include "smsthread.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/*收发短信的类变量，这里的定义其实就是声明了一个类*/
static SmsTraffic   g_smsTraffic;               
// 接收短消息列表/响应的缓冲区   
static SmBuffer     buff;
// 发送/接收短消息缓冲区                          
static Message      param[8];                       
/* Private function prototypes -----------------------------------------------*/
static void PutSendMessage(Message* pparam);
static bool GetSendMessage(Message* pparam);
static void PutRecvMessage(Message* pparam, int nCount);
static bool GetRecvMessage(Message* pparam);
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : GetSmsTraffic
* Description    : 获取短信处理结构体指针，本身可以理解为一个类
* Input          : None
* Output         : None
* Return         : 结构体指针
*******************************************************************************/
SmsTraffic *GetSmsTraffic(void)
{
    return &g_smsTraffic;
}

/*******************************************************************************
* Function Name  : PutSendMessage
* Description    : 将一条短消息放入发送队列
* Input          : - pparam: 指向一条的短信指针
* Output         : None
* Return         : None
*******************************************************************************/
static void PutSendMessage(Message* pparam)
{
    memcpy(&g_smsTraffic.m_SmSend[g_smsTraffic.m_nSendIn], pparam, sizeof(Message));
    g_smsTraffic.m_nSendIn++;
    if (g_smsTraffic.m_nSendIn >= MAX_SM_SEND)  
        g_smsTraffic.m_nSendIn = 0;
}

/*******************************************************************************
* Function Name  : GetSendMessage
* Description    : 从发送队列中取一条短消息
* Input          : - pparam: 指向一条的短信指针
* Output         : None
* Return         : 是否获取成功
*******************************************************************************/
static bool GetSendMessage(Message* pparam)
{
    bool fSuccess = FALSE;

    if (g_smsTraffic.m_nSendOut != g_smsTraffic.m_nSendIn)
    {
        memcpy(pparam, &g_smsTraffic.m_SmSend[g_smsTraffic.m_nSendOut], sizeof(Message));
        
        g_smsTraffic.m_nSendOut++;
        if (g_smsTraffic.m_nSendOut >= MAX_SM_SEND)  
            g_smsTraffic.m_nSendOut = 0;
        
        fSuccess = TRUE;
    }

    return fSuccess;
}

/*******************************************************************************
* Function Name  : PutRecvMessage
* Description    : 将短消息放入接收队列
* Input          : - pparam: 指向一批的短信指针
*                  - nCount: 短信数量
* Output         : None
* Return         : None
*******************************************************************************/
static void PutRecvMessage(Message* pparam, int nCount)
{
    int i;

    for (i = 0; i < nCount; i++)
    {
        memcpy(&g_smsTraffic.m_SmRecv[g_smsTraffic.m_nRecvIn], pparam, sizeof(Message));
        
        g_smsTraffic.m_nRecvIn++;
        if (g_smsTraffic.m_nRecvIn >= MAX_SM_RECV)  
            g_smsTraffic.m_nRecvIn = 0;
        
        pparam++;
    }
}

/*******************************************************************************
* Function Name  : GetRecvMessage
* Description    : 从接收队列中取一条短消息
* Input          : None
* Output         : - pparam: 接收到的短信
* Return         : 是否接收到
*******************************************************************************/
static bool GetRecvMessage(Message* pparam)
{
    bool fSuccess = FALSE;

    if (g_smsTraffic.m_nRecvOut != g_smsTraffic.m_nRecvIn)
    {
        memcpy(pparam, &g_smsTraffic.m_SmRecv[g_smsTraffic.m_nRecvOut], sizeof(Message));

        g_smsTraffic.m_nRecvOut++;
        if (g_smsTraffic.m_nRecvOut >= MAX_SM_RECV)  
            g_smsTraffic.m_nRecvOut = 0;

        fSuccess = TRUE;
    }

    return fSuccess;
}

/*******************************************************************************
* Function Name  : SmThread
* Description    : 短信收、发、删除状态机
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SmThread(void)
{
    SmsTraffic  *p          = &g_smsTraffic;// this
    static int   nMsg       = 0;            // 收到短消息条数
    static int   nDelete    = 0;            // 目前正在删除的短消息编号
    static uint32_t tmOrg, tmNow;           // 上次和现在的时间，计算超时用

    enum State{
        stBeginRest,                        // 开始休息/延时
        stContinueRest,                     // 继续休息/延时
        stSendMessageRequest,               // 发送短消息
        stSendMessageResponse,              // 读取短消息列表到缓冲区
        stSendMessageWaitIdle,              // 发送不成功，等待GSM就绪
        stReadMessageRequest,               // 发送读取短消息列表的命令
        stReadMessageResponse,              // 读取短消息列表到缓冲区
        stDeleteMessageRequest,             // 删除短消息
        stDeleteMessageResponse,            // 删除短消息
        stDeleteMessageWaitIdle,            // 删除不成功，等待GSM就绪
        stExitThread                        // 退出
    };                                      // 处理过程的状态

    // 初始状态
    static enum State nState = stBeginRest;

    switch(nState)
    {
        case stBeginRest:
            tmOrg   = GetSecTicks();
            nState  = stContinueRest;
            break;

        case stContinueRest:
//          Sleep(300);
            tmNow = GetSecTicks();
            if (p->GetSendMessage(&param[0]))
            {
                nState = stSendMessageRequest;  // 有待发短消息，就不休息了
            }
            else if (tmNow - tmOrg >= 5)        // 待发短消息队列空，休息5秒
            {
                nState = stReadMessageRequest;  // 转到读取短消息状态
            }
            break;

        case stSendMessageRequest:
            if (!GSM_SendMessage(&param[0]))
            {
                ThrowOutException(SMSG_UNSUCCESS);  
            }
            memset(&buff, 0, sizeof(buff));
            tmOrg   = GetSecTicks();
            nState  = stSendMessageResponse;
            break;

        case stSendMessageResponse:
//          Sleep(100);
            tmNow = GetSecTicks();
            if (tmNow - tmOrg < 5)
            {
                break;  
            }
            switch (GSM_GetResponse(&buff))
            {
                case GSM_OK: 
                    nState = stBeginRest;
                    break;
                case GSM_ERR:
                    nState = stSendMessageWaitIdle;
                    break;
                default:
                    if (tmNow - tmOrg >= 10)        // 10秒超时
                    {
                        tmOrg = GetSecTicks();
                        nState = stSendMessageWaitIdle;
                    }
            }
            break;

        case stSendMessageWaitIdle:
//          Sleep(500);
            tmNow = GetSecTicks();
            if (tmNow - tmOrg < 2)
            {
                break;  
            }
            nState = stSendMessageRequest;      // 直到发送成功为止
            break;

        case stReadMessageRequest:
            GSM_ReadMessageList();
            memset(&buff, 0, sizeof(buff));
            tmOrg   = GetSecTicks();
            nState  = stReadMessageResponse;
            break;

        case stReadMessageResponse:
//          Sleep(100);
            tmNow = GetSecTicks();
            if (tmNow - tmOrg < 5)
            {
                break;  
            }
            switch (GSM_GetResponse(&buff))
            {
                case GSM_OK: 
                    nMsg = GSM_ParseMessageList(param, &buff);
                    if (nMsg > 0)
                    {
                        p->PutRecvMessage(param, nMsg);
                        nDelete = 0;
                        nState = stDeleteMessageRequest;
                    }
                    else
                    {
                        nState = stBeginRest;
                    }
                    break;
                case GSM_ERR:
                    nState = stBeginRest;
                    break;
                default:
                    if (tmNow - tmOrg >= 15)        // 15秒超时
                    {
                        nState = stBeginRest;
                    }
            }
            break;

        case stDeleteMessageRequest:
            if (nDelete < nMsg)
            {
                GSM_DeleteMessage(param[nDelete].m_index);
                memset(&buff, 0, sizeof(buff));
                tmOrg   = GetSecTicks();
                nState  = stDeleteMessageResponse;
            }
            else
            {
                nState = stBeginRest;
            }
            break;

        case stDeleteMessageResponse:
//          Sleep(100);
            tmNow = GetSecTicks();
            if (tmNow - tmOrg < 2)
            {
                break;  
            }
            switch (GSM_GetResponse(&buff))
            {
                case GSM_OK: 
                    nDelete++;
                    nState = stDeleteMessageRequest;
                    break;
                case GSM_ERR:
                    tmOrg = GetSecTicks();
                    nState = stDeleteMessageWaitIdle;
                    break;
                default:
                    if (tmNow - tmOrg >= (nMsg - nDelete) * 10)     // 10秒超时
                    {
                        nState = stBeginRest;
                    }
            }
            break;

        case stDeleteMessageWaitIdle:
//          Sleep(500);
            tmNow = GetSecTicks();
            if (tmNow - tmOrg < 2)
            {
                break;  
            }
            nState = stDeleteMessageRequest;        // 直到删除成功为止
            break;
    }
}

/*******************************************************************************
* Function Name  : InitCSmsTraffic
* Description    : 初始化短信处理的结构
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void InitCSmsTraffic(void)
{
    g_smsTraffic.m_nSendIn  = 0;
    g_smsTraffic.m_nSendOut = 0;
    g_smsTraffic.m_nRecvIn  = 0;
    g_smsTraffic.m_nRecvOut = 0;
    g_smsTraffic.PutSendMessage = PutSendMessage;
    g_smsTraffic.GetSendMessage = GetSendMessage;
    g_smsTraffic.PutRecvMessage = PutRecvMessage;
    g_smsTraffic.GetRecvMessage = GetRecvMessage;   
}

/*********************************END OF FILE***********************************/
