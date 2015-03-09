/****************************GroundWireMangement ******************************
* File Name          : filesystem.c
* Author             : Si Kedong
* Version            : V1.0
* Date               : 12/20/2011
* Description        : 一个仿制的字符文件系统，使一些外设使用同一接口访问
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "filesystem.h"
#include "config.h"
#include "driver_includes.h"
#include <string.h>
/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
struct file_operations
{
    char            *name;
    int             (*open)(void);
    int             (*close)(void);
    unsigned int    (*read)(void *buf, unsigned int nbytes);
    unsigned int    (*write)(const void *buf, unsigned int nbytes);
    unsigned int    (*block_read)(void *buf, unsigned int nbytes, unsigned int timeOut);
    unsigned int    (*block_write)(const void *buf, unsigned int nbytes, unsigned int timeOut);
};
/* Private define ------------------------------------------------------------*/
/*获取某个数组的大小*/
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/*文件操作表*/
static const struct file_operations dev[]   =
{
        {
                .name                   =   "RS232",
                .open                   =   RS232_Open,
                .close                  =   RS232_Close,
                .read                   =   RS232_Read,
                .write                  =   RS232_Write,
                .block_read             =   RS232_Block_Read,
                .block_write            =   RS232_Block_Write,
        },
        {
                .name                   =   "GSM",
                .open                   =   GSM_Open,
                .close                  =   GSM_Close,
                .read                   =   GSM_Read,
                .write                  =   GSM_Write,
                .block_read             =   GSM_Block_Read,
                .block_write            =   GSM_Block_Write,
        },
        {
                .name                   =   "GPS",
                .open                   =   GPS_Open,
                .close                  =   GPS_Close,
                .read                   =   GPS_Read,
                .write                  =   GPS_Write,
                .block_read             =   GPS_Block_Read,
                .block_write            =   GPS_Block_Write,
        },
        {
                .name                   =   "KEY",
                .open                   =   KEY_Open,
                .close                  =   KEY_Close,
                .read                   =   KEY_Read,
        },
        {
                .name                   =   "LED",
                .open                   =   LED_Open,
                .close                  =   LED_Close,
                .write                  =   LED_Write,
        },
};
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : open
* Description    : 打开文件
* Input          : - dev_name: 文件名
*                  - oflags: 文件读写权限
* Output         : None
* Return         : 文件描述符
*******************************************************************************/
int open(const char *dev_name, int oflags)
{
    int i       = 0;
    int index   = -1;
    int n       = 0;
    n = (int)(ARRAY_SIZE(dev));

    for (i=0; i<n; i++)
    {
        if (strcmp(dev_name, dev[i].name) == 0)
        {
            index = i;
            break;
        }
    }

    if (index != -1)
    {
        dev[index].open();
    }

    return index;
}

/*******************************************************************************
* Function Name  : close
* Description    : 关闭文件
* Input          : - fildes: 文件描述符
* Output         : None
* Return         : 是否关闭成功
*******************************************************************************/
int close(int fildes)
{
    return dev[fildes].close();
}

/*******************************************************************************
* Function Name  : read
* Description    : 非阻塞读取数据
* Input          : - fildes: 文件描述符
*                  - nbytes: 读取的字节数量
* Output         : - buf: 读取的数据存储位置
* Return         : 实际读取字节的数量
*******************************************************************************/
unsigned int read(int fildes, void *buf, unsigned int nbytes)
{
    return dev[fildes].read(buf, nbytes);
}

/*******************************************************************************
* Function Name  : block_read
* Description    : 阻塞读取数据
* Input          : - fildes: 文件描述符
*                  - nbytes: 读取的字节数量
*                  - timeOut: 超时
* Output         : - buf: 读取的数据存储位置
* Return         : 实际读取字节的数量
*******************************************************************************/
unsigned int block_read(int fildes, void *buf, unsigned int nbytes, unsigned int timeOut)
{
    return dev[fildes].block_read(buf, nbytes, timeOut);
}

/*******************************************************************************
* Function Name  : write
* Description    : 非阻塞写数据
* Input          : - fildes: 文件描述符
*                  - nbytes: 写入的字节数量
*                  - buf: 待写入的数据
* Output         : None
* Return         : 实际写入字节的数量
*******************************************************************************/
unsigned int write(int fildes, const void *buf, unsigned int nbytes)
{
    return dev[fildes].write(buf, nbytes);
}

/*******************************************************************************
* Function Name  : block_write
* Description    : 阻塞写数据
* Input          : - fildes: 文件描述符
*                  - nbytes: 写入的字节数量
*                  - buf: 待写入的数据
*                  - timeOut: 超时
* Output         : None
* Return         : 实际写入字节的数量
*******************************************************************************/
unsigned int block_write(int fildes, const void *buf, unsigned int nbytes, unsigned int timeOut)
{
    return dev[fildes].block_write(buf, nbytes, timeOut);
}

/*********************************END OF FILE***********************************/
