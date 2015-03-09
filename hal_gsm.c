/****************************GroundWireMangement ******************************
* File Name          : hal_gsm.c
* Author             : Si Kedong
* Version            : V1.0
* Date               : 12/20/2011
* Description        : 包含短信模块的收、发、删除底层功能函数
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "includes.h"
/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define countof(a)      (sizeof(a) / sizeof(*(a)))
#define ResponseSize    (countof(g_responseString))
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/*该标志用于指示是否发生短信接收中断*/
static FlagStatus   g_msgRecFlag = RESET;
static const char *const g_responseString[] =
{
    [AT_OK]         =   "OK",
    [AT_CONNECT]    =   "CONNECT",
    [AT_RING]       =   "RING",
    [AT_NO_CARRIER] =   "NO CARRIER",
    [AT_ERROR]      =   "ERROR",
    [AT_NO_DIALTONE]=   "NO DIALTONE",
    [AT_BUSY]       =   "BUSY",
    [AT_ALERING]    =   "ALERING",
    [AT_DIALING]    =   "DIALING",
    [AT_MSG_ECHO]   =   "\r\n> ",
    [AT_CMTI]       =   "+CMTI:",
    [AT_CSQ]        =   "+CSQ:",
    [AT_CMGR]       =   "+CMGR:",
};
/* Private function prototypes -----------------------------------------------*/
static bool GSM_SetParam(const char *cmd,  ResponseStatus reply);
static int  WriteGSM(const void *pData, int nLength, int timeout);
static int  ReadGSM(void *pData, int nLength, int timeout);
static void ClearGsmBuffer(void);
static void ResetMsgRecFlag(void);
static FlagStatus GetMsgRecFlag(void);
static bool GSM_CheckResponses(const void *buf, ResponseStatus reply);
static bool GSM_DecodeMessage(const void *buf, Message *pMsg);
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : WriteGSM
* Description    : 往GSM模块中写数据
* Input          : - pData: 待写入的数据指针
*                  - nLength: 写入数据的长度
*                  - timeout: 超时
* Output         : None
* Return         : 写入数据的长度
*******************************************************************************/
static int WriteGSM(const void *pData, int nLength, int timeout)
{
    int numWrite = 0;
    int fd = 0;
    fd = open("GSM", 0);
    if (fd != -1)
    {
        numWrite = block_write(fd, pData, nLength, timeout);
        close(fd);      
    }
    return numWrite;
}

/*******************************************************************************
* Function Name  : ReadGSM
* Description    : 读取GSM中的数据
* Input          : - nLength: 写入数据的长度
*                  - timeout: 超时
* Output         : - pData: 读取的数据指针
* Return         : 读取数据的长度
*******************************************************************************/
static int ReadGSM(void *pData, int nLength, int timeout)
{
    int numRead = 0;
    int fd = 0;
    fd = open("GSM", 0);
    if (fd != -1)
    {
        if (timeout != 0)
            numRead = block_read(fd, pData, nLength, timeout);
        else
            numRead = read(fd, pData, nLength);
        
        close(fd);      
    }
    return numRead;
}

/*复位短信接收标志*/
static void ResetMsgRecFlag(void)
{
    ENTER_CRITICAL();
    g_msgRecFlag = RESET;
    EXIT_CRITICAL();
}

/*设置短信接收标志*/
void SetMsgRecFlag(void)
{
    ENTER_CRITICAL();
    g_msgRecFlag = SET;
    EXIT_CRITICAL();
}

/*获取短信接收标志*/
static FlagStatus GetMsgRecFlag(void)
{
    FlagStatus flag;

    ENTER_CRITICAL();
    flag = g_msgRecFlag;
    EXIT_CRITICAL();
    return flag;
}

/*清除短信接收缓冲区*/
static void ClearGsmBuffer(void)
{
    if (GetMsgRecFlag() != SET)
    {
        ClearQueue(&gsmRxQueue);
    }
    else
    {
        ResetMsgRecFlag();
        GSM_GetNewMessageIndex();
    }
}


/*******************************************************************************
* Function Name  : GSM_CheckResponses
* Description    : 检测响应字符串
* Input          : buf：输入字符串
                   reply：待检测字符串对应ID
* Output         : none
* Return         : 布尔值
*******************************************************************************/
static bool GSM_CheckResponses(const void *buf, ResponseStatus reply)
{
    assert_param(buf != NULL);
    if (strstr((const char *)buf, g_responseString[reply]) != NULL)
    {
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
* Function Name  : GSM_CheckSignalQuality
* Description    : 检测GSM模块天线质量
* Input          : quality：
* Output         : quality：信号质量(1-31,99)
* Return         : 布尔值
*******************************************************************************/
bool GSM_CheckSignalQuality(int *quality)
{
    char    cmd[16];            // 命令串
    uint8_t nLength;            // 串口收到的数据长度
    char    ans[32];            // 文件描述符;
    
    sprintf(cmd, "AT+CSQ?\r");
    /*清空接收缓冲区*/
    ClearGsmBuffer();
    WriteGSM(cmd, strlen(cmd), 100);
    nLength = ReadGSM(ans, 32, 100);

    if (nLength > 0 
        && GSM_CheckResponses(ans, AT_OK) 
        && GSM_CheckResponses(ans, AT_CSQ))
    {
        sscanf(ans, "+CSQ:%d,%*s", quality);
        return TRUE;
    }
    return FALSE;
}
/*******************************************************************************
* Function Name  : GSM_GetNewMessageIndex
* Description    : 检测新收到短信在SIM卡中的位置
* Input          : index：
* Output         : index：短信位置
* Return         : 布尔值
*******************************************************************************/
bool GSM_GetNewMessageIndex(void)
{
    int         nLength;            // 串口收到的数据长度
    char        ans[128];           // 文件描述符;
    char       *pIndex = NULL;      // 应答字符串关键字位置
    uint8_t     index = 0;

    do
    {
        nLength = ReadGSM(ans, 128, 100);
        if (nLength > 0                                      //是否接收到数据
            && ((pIndex = strstr(ans, "+CMTI")) != NULL))    //是否找到“+CMTI”
        {
            sscanf(pIndex, "%*[^,],%d[^\r]", (int *)&index);
            EnQueue(&msgIndexQueue, index);
            return TRUE;
        }
    }while(!IsEmptyQueue(&gsmRxQueue));
    return FALSE;   
}

/*******************************************************************************
* Function Name  : GSM_DecodeMessage
* Description    : 解析短信
* Input          : buf：短信字符串
                   pMsg：
* Output         : pMsg：保存短信的结构
* Return         : 布尔值
*******************************************************************************/
static bool GSM_DecodeMessage(const void *buf, Message *pMsg)
{
    char    nLength         = 0;
    char   *pBufIndex       = NULL;

    assert_param(buf != NULL && pMsg != NULL);
    /*判断是否是所需要的信息*/
    if ((pBufIndex = strstr(buf, "+CMGR")) == NULL)
    {
        return FALSE;
    }
    nLength = sscanf(pBufIndex, "%*[^\n]\n%[^\r]", pMsg->m_content);
    if (nLength != 1)
        return FALSE;
    return TRUE;
}

/*******************************************************************************
* Function Name  : GSM_SendMessage
* Description    : 发送短信
* Input          : pMsg
* Output         : None
* Return         : 布尔值
*******************************************************************************/
bool GSM_SendMessage(Message *pMsg)
{
    char    cmd[32];            // 命令串
    int     nLength;            // 串口收到的数据长度
    char    ans[128];           // 应答串

    /* Check the parameters */
    assert_param(pMsg != NULL);
    /*生成命令*/
    sprintf(cmd, "AT+CMGS=\"%.14s\"\r", (char *)&pMsg->m_phoneNumber);
    /*GSM从sleep模式唤醒要DUMP写*/
    GSM_SetParam("AT\r", AT_OK);
    /*清空接收缓冲区*/
    ClearGsmBuffer();
    /*先输出命令串*/
    WriteGSM(cmd, strlen(cmd), 100);
    /*读应答数据*/
    nLength = ReadGSM(ans, 128, 100);
    /*根据能否找到"\r\n> "决定成功与否 */
    if(nLength>0 && nLength<=128 && strstr(ans, "\r\n> ") != NULL)
    {
        /*加密TEXT*/
//      GSM_EncryptText(pMsg->m_content, strlen(pMsg->m_content));
        /*在信息结尾加 CTRL-Z*/
        strcat(pMsg->m_content, "\x1a");
        /*清空接收缓冲区*/
        ClearGsmBuffer();
        /*写信息*/
        WriteGSM(pMsg->m_content, strlen(pMsg->m_content), 1000);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

#if CFG_SEND_EXT_MESSAGE > 0
/*******************************************************************************
* Function Name  : GSM_SendMessage
* Description    : 发送短信
* Input          : pMsg
* Output         : None
* Return         : 布尔值
*******************************************************************************/
bool GSM_SendMessage1(Message *pMsg)
{
    char        cmd[32];            // 命令串
    int         nLength;            // 串口收到的数据长度
    char        ans[128];           // 应答串
    PhoneNumber tel;

#if   EXT_MSG_SIMCARD_2426 > 0  
    memcpy(tel, "+8613274112426", sizeof(PhoneNumber));
#elif EXT_MSG_SIMCARD_0642 > 0
    memcpy(tel, "+8613274110642", sizeof(PhoneNumber));
#elif EXT_MSG_SIMCARD_1667 > 0
    memcpy(tel, "+8615998521667", sizeof(PhoneNumber));
#elif EXT_MSG_SIMCARD_4016 > 0
    memcpy(tel, "+8613274114016", sizeof(PhoneNumber));
#endif
    /* Check the parameters */
    assert_param(pMsg != NULL);
    /*生成命令*/
    sprintf(cmd, "AT+CMGS=\"%.14s\"\r", tel);
    /*GSM从sleep模式唤醒要DUMP写*/
    GSM_SetParam("AT\r", AT_OK);
    /*清空接收缓冲区*/
    ClearGsmBuffer();
    /*先输出命令串*/
    WriteGSM(cmd, strlen(cmd), 100);
    /*读应答数据*/
    nLength = ReadGSM(ans, 128, 100);
    /*根据能否找到"\r\n> "决定成功与否 */
    if(nLength>0 && nLength<=128 && strstr(ans, "\r\n> ") != NULL)
    {
        /*加密TEXT*/
//      GSM_EncryptText(pMsg->m_content, strlen(pMsg->m_content));
        /*在信息结尾加 CTRL-Z*/
        strcat(pMsg->m_content, "\x1a");
        /*清空接收缓冲区*/
        ClearGsmBuffer();
        /*写信息*/
        WriteGSM(pMsg->m_content, strlen(pMsg->m_content), 1000);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
#endif
 
/*******************************************************************************
* Function Name  : GSM_ReadMessage
* Description    : 读短信
* Input          : pMsg：
                   index：短信位置
* Output         : pMsg：保存短信的结构
* Return         : 布尔值
*******************************************************************************/
bool GSM_ReadMessage(Message *pMsg, const int index)
{
    char        cmd[16];            // 命令串
    int         nLength;            // 串口收到的数据长度
    char        ans[256];           // 应答串

    /* Check the parameters */
    assert_param(pMsg != NULL);
    /*生成命令*/
    sprintf(cmd, "AT+CMGR=%d\r", index);
    /*清空接收缓冲区*/
    ClearGsmBuffer();
    /*先输出命令串*/
    WriteGSM(cmd, strlen(cmd), 100);
    /*读应答数据*/
    nLength = ReadGSM(ans, 256, 100);
    /*根据能否找到"+CMS ERROR"决定成功与否*/
    if(nLength > 0 && strncmp(ans, "+CMS ERROR", 10) != 0)
    {
        /*添加收到信息之后的处理*/
        if (GSM_DecodeMessage(ans,pMsg))
        {   
            /*解密*/
//          GSM_DecryptText(pMsg->m_content, strlen(pMsg->m_content));
            return TRUE;
        }
    }
    return FALSE;
}

/*******************************************************************************
* Function Name  : GSM_ReadMessage
* Description    : 读短信
* Input          : pMsg：
                   index：短信位置
* Output         : pMsg：保存短信的结构
* Return         : 布尔值
*******************************************************************************/
int GSM_ReadMessageList(void)
{
    char        cmd[32];            // 命令串
    int         nLength;            // 串口收到的数据长度
    /*生成命令*/
    /*GSM从sleep模式唤醒要DUMP写*/
    GSM_SetParam("AT\r", AT_OK);
    /*清空接收缓冲区*/
    ClearGsmBuffer();
//  sprintf(cmd, "AT+CMGL=\"ALL\"\r");
    sprintf(cmd, "AT+CMGL=\"REC UNREAD\"\r");
    nLength = WriteGSM(cmd, strlen(cmd), 100);
    return nLength; 
}
/*******************************************************************************
* Function Name  : GSM_DeleteMessage
* Description    : 删除短信
* Input          : index：短信位置
* Output         : None
* Return         : 布尔值
*******************************************************************************/
int GSM_DeleteMessage(const uint8_t index)
{
    int     nLength;          // 串口收到的数据长度
    char    cmd[16];         // 命令串

    /*GSM从sleep模式唤醒要DUMP写*/
    GSM_SetParam("AT\r", AT_OK);
    /*清空接收缓冲区*/
    ClearGsmBuffer();
    sprintf(cmd, "AT+CMGD=%d,0\r", index);      
    nLength = WriteGSM(cmd, strlen(cmd), 100);
    return nLength; 
} 

/*******************************************************************************
* Function Name  : GSM_GetResponse
* Description    : 获取模块的响应
* Input          : None
* Output         : - pBuff: 响应的数据
* Return         : 响应的状态
*                   - GSM_WAIT: 等待
*                   - GSM_OK:   成功
*                   - GSM_ERR:  失败
*******************************************************************************/
int GSM_GetResponse(SmBuffer* pBuff)
{
    int nLength = 0;        /*串口收到的数据长度*/
    int nState = 0;

    /*从串口读数据，追加到缓冲区尾部*/
    nLength = ReadGSM(&pBuff->m_data[pBuff->m_len], 512, 10);   
    pBuff->m_len += nLength;
    /*确定GSM MODEM的应答状态*/
    nState = GSM_WAIT;
    if ((nLength > 0) && (pBuff->m_len >= 4))
    {
//      if (strncmp(&pBuff->m_data[pBuff->m_len - 4], "OK\r\n", 4) == 0)
        if (strstr(pBuff->m_data, "OK\r\n") != NULL)  
            nState = GSM_OK;
        else if (strstr(pBuff->m_data, "+CMS ERROR") != NULL) 
            nState = GSM_ERR;
    }
    return nState;
}


/*******************************************************************************
* Function Name  : GSM_SetParam
* Description    : 设置GSM模块参数
* Input          : cmd：设置命令
                   reply：期待的响应
* Output         : None
* Return         : 布尔值
*******************************************************************************/
static bool GSM_SetParam(const char *cmd,  ResponseStatus reply)
{
    char    ans[128];            // 应答串
    uint8_t i;

    /*重复检测三次*/
    for (i=0; i<3; i++)
    {
        memset(ans, 0, sizeof(ans));
        ClearGsmBuffer();
        WriteGSM(cmd, strlen(cmd), 100);
        ReadGSM(ans, sizeof(ans), 500);
#ifdef _Debug
        {
            int rs232_fd;
            rs232_fd = open("RS232", 0);
            write(rs232_fd, cmd, strlen(cmd));
            write(rs232_fd, ans, nLength);
        }
#endif
        if (!GSM_CheckResponses(ans, reply))
        {
            continue;
        }
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
* Function Name  : GSM_ParseMessageList
* Description    : 从列表中解析出全部短消息
* Input          : - pBuff: 短消息列表缓冲区
* Output         : - pMsg: 短消息缓冲区
* Return         : 短消息条数
*******************************************************************************/
int GSM_ParseMessageList(Message* pMsg, const SmBuffer* pBuff)
{
    int         nMsg;                   // 短消息计数值
    const char *ptr;                    // 内部用的数据指针
    int         nLength = 0;

    nMsg = 0;
    ptr = pBuff->m_data;

    /*循环读取每一条短消息, 以"+CMGL:"开头*/
    while((ptr = strstr(ptr, "+CMGL:")) != NULL)
    {
        /*跳过"+CMGL:", 定位到序号*/
        ptr += 6;                                               
        /*读取序号*/
        nLength = sscanf(ptr, "%2d", (int *)&pMsg->m_index);
        /*找下一行*/     
        ptr = strchr(ptr, '\n');                            
        if (ptr != NULL)
        {
            /*跳过"\r\n", 定位到PDU*/
            ptr += 1;                                        
//          nLength = sscanf(ptr, "%*[^\n]\n%[^\r]", pMsg->m_content);
            nLength = sscanf(ptr, "%167[^\r]", pMsg->m_content);
            if (nLength == 1)
            {
//              GSM_DecryptText(pMsg->m_content, strlen(pMsg->m_content));
                pMsg++;         /*准备读下一条短消息*/
                nMsg++;         /*短消息计数加1*/
            }
        }
    }
    return nMsg;
}
/*******************************************************************************
* Function Name  : GSM_Init
* Description    : GSM初始化
* Input          : None
* Output         : None
* Return         : 布尔值
*******************************************************************************/
bool GSM_Config(void)
{
    int     i;
    uint8_t dump;

    /*把开机的所有信息全部清空*/
    while(DeQueue(&gsmRxQueue, &dump));
    for (i=0; i<1; i++)
    {
        /*同步*/
        if (!GSM_SetParam("AT\r", AT_OK))
            continue;
        /*恢复出厂设置*/
        if (!GSM_SetParam("AT&F0\r", AT_OK))
            continue;
        /*设置波特率*/
        if (!GSM_SetParam("AT+IPR=115200\r", AT_OK))
            continue;
        /*不回显*/
        if (!GSM_SetParam("ATE0\r", AT_OK))             // 0：不回显；1：回显
            continue;
        /*设置TEXT模式*/
        if (!GSM_SetParam("AT+CMGF=1\r", AT_OK))        // 0：PDU模式；1：TEXT模式
            continue;
        /*将读取、删除、写入、发送、接收存储短信的位置设置在sim卡中*/
        if (!GSM_SetParam("AT+CPMS=\"SM\",\"SM\",\"SM\"\r", AT_OK))     
            continue;
        /*设置新消息提示方式*/
        if (!GSM_SetParam("AT+CNMI=2,1,0,0,0\r", AT_OK))        
            continue;
        /*显示TEXT方式*/
        if (!GSM_SetParam("AT+CSDH=0\r", AT_OK))        
            continue;
        /*设置短消息TEXT方式参数*/
        if (!GSM_SetParam("AT+CSMP=17,167,0,241\r", AT_OK))     
            continue;
        /*删除所有短信*/
//      if (!GSM_SetParam("AT+CMGD=0,4\r", AT_OK))
//          continue;
        /*配置慢时钟*/
        if (!GSM_SetParam("AT+CSCLK=2\r", AT_OK))
            continue;
        return TRUE;
    }
    return FALSE;             
}

/*******************************************************************************
* Function Name  : SetSmsCenterTel
* Description    : 设置短信中心的电话号码
* Input          : - phone: 短信中心电话号码
* Output         : None
* Return         : 是否设置成功
*******************************************************************************/
bool SetSmsCenterTel(PhoneNumber phone)
{
    char buf[28];
    sprintf(buf, "AT+CSCA=\"%.14s\"\r", phone);
    return GSM_SetParam(buf, AT_OK);
}

/*******************************************************************************
* Function Name  : GSM_ShutDown
* Description    : 关闭GSM
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void GSM_ShutDown(void)
{
    GSM_PowerOff();
}

/*******************************************************************************
* Function Name  : GSM_BootStrap
* Description    : 按步骤启动GSM
* Input          : - step: 启动步骤
* Output         : None
* Return         : None
*******************************************************************************/
void GSM_BootStrap(uint8_t step)
{
    GSM_PowerOn(step);
}

/*********************************END OF FILE***********************************/
