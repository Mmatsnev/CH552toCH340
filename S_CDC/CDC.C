/********************************** (C) COPYRIGHT *******************************
* File Name          : CDC.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/03/01
* Description        : CH554做CDC设备转串口，选择串口1
*******************************************************************************/
#include "CH554.H"
#include "DEBUG.H"
#include <stdio.h>
#include <string.h>
#define THIS_ENDP0_SIZE         DEFAULT_ENDP0_SIZE 
UINT8X	Ep0Buffer[THIS_ENDP0_SIZE] _at_ 0x0000;                                //端点0 OUT&IN缓冲区，必须是偶地址
UINT8X	Ep2Buffer[2*MAX_PACKET_SIZE] _at_ 0x0008;                              //端点2 IN&OUT缓冲区,必须是偶地址
UINT8X  Ep1Buffer[MAX_PACKET_SIZE] _at_ 0x00a0;

UINT16 SetupLen,ControlData;
UINT8   SetupReq,Count,UsbConfig,down=0;
UINT8   RTS_DTR=0;
UINT8   baudFlag0,baudFlag1,baudFlag2=0;
UINT8   baud0,baud1;
UINT8   num = 0;
PUINT8  pDescr;                                                                //USB配置标志
USB_SETUP_REQ   SetupReqBuf;                                                   //暂存Setup包
#define UsbSetupBuf     ((PUSB_SETUP_REQ)Ep0Buffer)

#define  SET_LINE_CODING                0X20            // Configures DTE rate, stop-bits, parity, and number-of-character
#define  GET_LINE_CODING                0X21            // This request allows the host to find out the currently configured line coding.
#define  SET_CONTROL_LINE_STATE         0X22            // This request generates RS-232/V.24 style control signals.


/*设备描述符*/
/*
UINT8C DevDesc[] = {0x12,0x01,0x10,0x01,0x02,0x00,0x00,DEFAULT_ENDP0_SIZE,
                    0x86,0x1a,0x22,0x57,0x00,0x01,0x01,0x02,
                    0x03,0x01
                   };
UINT8C CfgDesc[] ={
    0x09,0x02,0x43,0x00,0x02,0x01,0x00,0xa0,0x32,             //配置描述符（两个接口）
	//以下为接口0（CDC接口）描述符	
    0x09,0x04,0x00,0x00,0x01,0x02,0x02,0x01,0x00,             //CDC接口描述符(一个端点)
	//以下为功能描述符
    0x05,0x24,0x00,0x10,0x01,                                 //功能描述符(头)
	0x05,0x24,0x01,0x00,0x00,                                 //管理描述符(没有数据类接口) 03 01
	0x04,0x24,0x02,0x02,                                      //支持Set_Line_Coding、Set_Control_Line_State、Get_Line_Coding、Serial_State	
	0x05,0x24,0x06,0x00,0x01,                                 //编号为0的CDC接口;编号1的数据类接口
	0x07,0x05,0x81,0x03,0x08,0x00,0xFF,                       //中断上传端点描述符
	//以下为接口1（数据接口）描述符
	0x09,0x04,0x01,0x00,0x02,0x0a,0x00,0x00,0x00,             //数据接口描述符
    0x07,0x05,0x02,0x02,0x40,0x00,0x00,                       //端点描述符	
	0x07,0x05,0x82,0x02,0x40,0x00,0x00,                       //端点描述符
};*/


/*
UINT8C DevDesc[18]={0x12,0x01,0x10,0x01,0xff,0x00,0x02,0x08,                   //设备描述符
                    0x86,0x1a,0x23,0x55,0x04,0x03,0x00,0x00,
                    0x00,0x01};*/
UINT8C DevDesc[18]={0x12,0x01,0x10,0x01,0xff,0x00,0x00,0x08,                   //设备描述符
                    0x86,0x1a,0x23,0x75,0x63,0x02,0x00,0x02,
                    0x00,0x01};
/*
UINT8C CfgDesc[39]={0x09,0x02,0x27,0x00,0x01,0x01,0x00,0x80,0xf0,              //配置描述符，接口描述符,端点描述符
	                  0x09,0x04,0x00,0x00,0x03,0xff,0x01,0x02,0x00,           
                    0x07,0x05,0x82,0x02,0x20,0x00,0x00,                        //批量上传端点
		                0x07,0x05,0x02,0x02,0x20,0x00,0x00,                        //批量下传端点      
			              0x07,0x05,0x81,0x03,0x08,0x00,0x01};                       //中断上传端点*/
										
UINT8C CfgDesc[39]={0x09,0x02,0x27,0x00,0x01,0x01,0x00,0x80,0xf0,              //配置描述符，接口描述符,端点描述符
	                  0x09,0x04,0x00,0x00,0x03,0xff,0x01,0x02,0x00,           
                    0x07,0x05,0x82,0x02,0x20,0x00,0x00,                        //批量上传端点
		                0x07,0x05,0x02,0x02,0x20,0x00,0x00,                        //批量下传端点      
			              0x07,0x05,0x81,0x03,0x08,0x00,0x01};                       //中断上传端

UINT8C DataBuf[26]={0x30,0x00,0xc3,0x00,0xff,0xec,0x9f,0xec,0xff,0xec,0xdf,0xec,
                    0xdf,0xec,0xdf,0xec,0x9f,0xec,0x9f,0xec,0x9f,0xec,0x9f,0xec,
                    0xff,0xec};


UINT8X UserEp2Buf[64];                                            //用户数据定义

/*字符串描述符*/
										/*
 unsigned char  code LangDes[]={0x04,0x03,0x09,0x04};           //语言描述符
 unsigned char  code SerDes[]={                                 //序列号字符串描述符
                0x14,0x03,
				0x32,0x00,0x30,0x00,0x31,0x00,0x37,0x00,0x2D,0x00,
				0x32,0x00,0x2D,0x00,
				0x32,0x00,0x35,0x00
                };     
 unsigned char  code Prod_Des[]={                                //产品字符串描述符
				0x14,0x03,
				0x43,0x00,0x48,0x00,0x35,0x00,0x35,0x00,0x34,0x00,0x5F,0x00,
				0x43,0x00,0x44,0x00,0x43,0x00,
 };
 unsigned char  code Manuf_Des[]={  
				0x0A,0x03,
				0x5F,0x6c,0xCF,0x82,0x81,0x6c,0x52,0x60,
 };*/

//cdc参数
UINT8X LineCoding[7]={0x00,0xe1,0x00,0x00,0x00,0x00,0x08};   //初始化波特率为57600，1停止位，无校验，8数据位。

#define UART_REV_LEN  64                 //串口接收缓冲区大小
UINT8I Receive_Uart_Buf[UART_REV_LEN];   //串口接收缓冲区
volatile UINT8I Uart_Input_Point = 0;   //循环缓冲区写入指针，总线复位需要初始化为0
volatile UINT8I Uart_Output_Point = 0;  //循环缓冲区取出指针，总线复位需要初始化为0
volatile UINT8I UartByteCount = 0;      //当前缓冲区剩余待取字节数


volatile UINT8I USBByteCount = 0;      //代表USB端点接收到的数据
volatile UINT8I USBBufOutPoint = 0;    //取数据指针

volatile UINT8I UpPoint2_Busy  = 0;   //上传端点是否忙标志


/*******************************************************************************
* Function Name  : USBDeviceCfg()
* Description    : USB设备模式配置
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceCfg()
{
    USB_CTRL = 0x00;                                                           //清空USB控制寄存器
    USB_CTRL &= ~bUC_HOST_MODE;                                                //该位为选择设备模式
    USB_CTRL |=  bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;                    //USB设备和内部上拉使能,在中断期间中断标志未清除前自动返回NAK
    USB_DEV_AD = 0x00;                                                         //设备地址初始化
//     USB_CTRL |= bUC_LOW_SPEED;
//     UDEV_CTRL |= bUD_LOW_SPEED;                                                //选择低速1.5M模式
    USB_CTRL &= ~bUC_LOW_SPEED;
    UDEV_CTRL &= ~bUD_LOW_SPEED;                                             //选择全速12M模式，默认方式
	  UDEV_CTRL = bUD_PD_DIS;  // 禁止DP/DM下拉电阻
    UDEV_CTRL |= bUD_PORT_EN;                                                  //使能物理端口
}
/*******************************************************************************
* Function Name  : USBDeviceIntCfg()
* Description    : USB设备模式中断初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceIntCfg()
{
    USB_INT_EN |= bUIE_SUSPEND;                                               //使能设备挂起中断
    USB_INT_EN |= bUIE_TRANSFER;                                              //使能USB传输完成中断
    USB_INT_EN |= bUIE_BUS_RST;                                               //使能设备模式USB总线复位中断
    USB_INT_FG |= 0x1F;                                                       //清中断标志
    IE_USB = 1;                                                               //使能USB中断
    EA = 1;                                                                   //允许单片机中断
}
/*******************************************************************************
* Function Name  : USBDeviceEndPointCfg()
* Description    : USB设备模式端点配置，模拟兼容HID设备，除了端点0的控制传输，还包括端点2批量上下传
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceEndPointCfg()
{
	UEP1_DMA = Ep1Buffer;                                                      //端点1 发送数据传输地址
    UEP2_DMA = Ep2Buffer;                                                      //端点2 IN数据传输地址	
    UEP2_3_MOD = 0xCC;                                                         //端点2/3 单缓冲收发使能
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;                 //端点2自动翻转同步标志位，IN事务返回NAK，OUT返回ACK

    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;                                 //端点1自动翻转同步标志位，IN事务返回NAK	
	UEP0_DMA = Ep0Buffer;                                                      //端点0数据传输地址
    UEP4_1_MOD = 0X40;                                                         //端点1上传缓冲区；端点0单64字节收发缓冲区
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                                 //手动翻转，OUT事务返回ACK，IN事务返回NAK
}
/*******************************************************************************
* Function Name  : Config_Uart1(UINT8 *cfg_uart)
* Description    : 配置串口1参数
* Input          : 串口配置参数 四位波特率、停止位、校验、数据位
* Output         : None
* Return         : None
*******************************************************************************/
void Config_Uart1(UINT8 *cfg_uart)
{
	UINT32 uart1_buad = 0;
	*((UINT8 *)&uart1_buad) = cfg_uart[3];
	*((UINT8 *)&uart1_buad+1) = cfg_uart[2];
	*((UINT8 *)&uart1_buad+2) = cfg_uart[1];
	*((UINT8 *)&uart1_buad+3) = cfg_uart[0];
	IE_UART1 = 0;
	SBAUD1 = 0 - FREQ_SYS/16/uart1_buad;
	IE_UART1 = 1;
}
/*******************************************************************************
* Function Name  : DeviceInterrupt()
* Description    : CH559USB中断处理函数
*******************************************************************************/
void    DeviceInterrupt( void ) interrupt INT_NO_USB using 1                    //USB中断服务程序,使用寄存器组1
{
		
    UINT8 len,i;
    if(UIF_TRANSFER)                                                            //USB传输完成标志
    {
        switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
        {
        case UIS_TOKEN_IN | 2:                                                  //endpoint 2# 端点批量上传
             UEP2_T_LEN = 0;                                                    //预使用发送长度一定要清空
//            UEP1_CTRL ^= bUEP_T_TOG;                                          //如果不设置自动翻转则需要手动翻转
            UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;           //默认应答NAK
						UpPoint2_Busy = 0;// not  busy
						break;
        case UIS_TOKEN_OUT | 2:                                                 //endpoint 2# 端点批量下传
            if ( U_TOG_OK )                                                     // 不同步的数据包将丢弃
            {
                len = USB_RX_LEN;
							  USBByteCount = USB_RX_LEN;
							  USBBufOutPoint = 0;
							  UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_NAK;
							  
            }
            break;
        case UIS_TOKEN_SETUP | 0:                                               //SETUP事务
            len = USB_RX_LEN;
            if(len == (sizeof(USB_SETUP_REQ)))
            {
                SetupLen = UsbSetupBuf->wLengthL;
								RTS_DTR = UsbSetupBuf->wValueL;
                if(UsbSetupBuf->wLengthH || SetupLen > 0x7F )
                {
                    SetupLen = 0x7F;                                             // 限制总长度
                }
                len = 0;                                                         // 默认为成功并且上传0长度
                SetupReq = UsbSetupBuf->bRequest;							
                if ( ( UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD ){/*HID类命令*/
										switch( SetupReq )                                             
										{
												case 0xC0:                                                  
														pDescr = &DataBuf[num];
														len = 2;
														if(num<24){	
																num += 2;
														}else{
																num = 24;
														}											
														break;
												case 0x40:
														len = 9;   //保证状态阶段，这里只要比8大，且不等于0xff即可
														break;
												case 0xa4:
													  switch(RTS_DTR){
																case 0x9f://RTS->LOW, DTR->LOW
																	P1 &= ~0x10; //RTS LOW
																	P1 &= ~0x20; //DTR LOW
																  break;
																case 0xdf://RTS->HIGH,DTR->LOW
																	P1 |=  0x10; //RTS HIGH
																	P1 &= ~0x20; //DTR LOW
																  break;
																case 0xff://RTS->HIGH,DTR->HIGH
																	P1 |=  0x10; //RTS HIGH
																  P1 |=  0x20; //DTR HIGH
																  break;
																case 0xbf://RTS->LOW,DTR->HIGH
																	P1 &= ~0x10; //RTS LOW
																	P1 |=  0x20; //DTR HIGH
																  break;
																default:
																	break;
														}
												    break;
												case 0x9a://set baud
														baudFlag0 = UsbSetupBuf->wValueL;
														baudFlag1 = UsbSetupBuf->wValueH;
														if((baudFlag0 == 0x12) && (baudFlag1 == 0x13)){
																baud0 = UsbSetupBuf->wIndexL;
																baud1 = UsbSetupBuf->wIndexH;
																switch (baud0)
																{
																		case 0x80:
																		{
																				switch(baud1)
																				{
																						case 0x96://110=0x6e
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x00;
																							 LineCoding[1] = 0x00;
																						   LineCoding[0] = 0x6E;
																							 break;
																						 case 0xd9://300=0x12c
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x00;
																							 LineCoding[1] = 0x01;
																						   LineCoding[0] = 0x2c;
																							 break;
																						 default:
																							 break;
																				}
																		}
																				break;
																		case 0x81:
																		{
																				switch(baud1)
																				{
																						case 0x64://600=0x258
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x00;
																							 LineCoding[1] = 0x02;
																						   LineCoding[0] = 0x58;
																							 break;
																						 case 0xb2://1200=0x4b0
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x00;
																							 LineCoding[1] = 0x02;
																						   LineCoding[0] = 0xb0;
																							 break;
																						 case 0xd9://2400=0x960
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x00;
																							 LineCoding[1] = 0x09;
																						   LineCoding[0] = 0x60;
																							 break;
																						 default:
																							 break;
																				}
																		}
																				break;
																		case 0x82:
																		{
																				switch(baud1)
																					{
																						 case 0x64://4800 = 0x12c0
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x00;
																							 LineCoding[1] = 0x12;
																						   LineCoding[0] = 0xc0;
																							 break;
																						 case 0xb2://9600 = 0x2580
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x00;
																							 LineCoding[1] = 0x25;
																						   LineCoding[0] = 0x80;
																							 break;
																						 case 0xcc://14400 = 0x3840
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x00;
																							 LineCoding[1] = 0x38;
																						   LineCoding[0] = 0x40;
																						   break;
																						 case 0xd9://19200=0x4B00
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x00;
																							 LineCoding[1] = 0x4b;
																						   LineCoding[0] = 0x00;
																							 break;
																						 default:
																							 break;
																					}
																		}
																				break;
																		case 0x83:
																		{
																				switch(baud1)
																				{
																						 case 0x64://38400 = 0x9600
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x00;
																							 LineCoding[1] = 0x96;
																						   LineCoding[0] = 0x00;
																							 break;
																						 case 0x95://56000 = 0xDAC0
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x00;
																							 LineCoding[1] = 0xDA;
																						   LineCoding[0] = 0xC0;
																							 break;
																						 case 0x98://57600 = 0xE100
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x00;
																							 LineCoding[1] = 0xE1;
																						   LineCoding[0] = 0x00;
																						   break;
																						 case 0xcc://115200=0x1c200
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x01;
																							 LineCoding[1] = 0xc2;
																						   LineCoding[0] = 0x00;
																							 break;
																						 case 0xd1://128000=0x1f400
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x01;
																							 LineCoding[1] = 0xf4;
																						   LineCoding[0] = 0x00;
																							 break;
																						 case 0xe6://230400=0x38400
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x03;
																							 LineCoding[1] = 0x84;
																						   LineCoding[0] = 0x00;
																							 break;
																						 case 0xe9://256000=0x3e800
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x03;
																							 LineCoding[1] = 0xe8;
																						   LineCoding[0] = 0x00;
																							 break;
																						 case 0xf3://460800=0x70800
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x07;
																							 LineCoding[1] = 0x08;
																						   LineCoding[0] = 0x00;
																							 break;
																						 case 0xf4://512000=0x7D000
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x07;
																							 LineCoding[1] = 0xD0;
																						   LineCoding[0] = 0x00;
																							 break;
																						 case 0xf6://600000=0x927C0
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x09;
																							 LineCoding[1] = 0x27;
																						   LineCoding[0] = 0xC0;
																						 case 0xf8://750000=0xB71B0
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x0B;
																							 LineCoding[1] = 0x71;
																						   LineCoding[0] = 0xB0;
																							 break;
																						 default:
																							 break;
																				}
																			}
																				break;
																		case 0x87:
																		{
																				switch(baud1)
																				{
																						case 0xf3://921600 = 0xE1000 
																							 LineCoding[3] = 0x00;
																						   LineCoding[2] = 0x0E;
																							 LineCoding[1] = 0x10;
																						   LineCoding[0] = 0x00;
																							break;
																						default:
																							break;
																				}
																		}
																				break;
																			default:
																				break;
																}
																		Config_Uart1(LineCoding);
														}
													break;
												default:
														len = 0xFF;  				                                   /*命令不支持*/					
														break;
										}
										if ( SetupLen > len )
										{
												SetupLen = len;    //限制总长度
										}
										len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;//本次传输长度
										memcpy(Ep0Buffer,pDescr,len);                            //加载上传数据
										SetupLen -= len;
										pDescr += len;
						
                }else                                                             //标准请求
                {
                    switch(SetupReq)                                             //请求码
                    {
                    case USB_GET_DESCRIPTOR:
                        switch(UsbSetupBuf->wValueH)
                        {
                        case 1:                                                  //设备描述符
                            pDescr = DevDesc;                                    //把设备描述符送到要发送的缓冲区
                            len = sizeof(DevDesc);
                            break;
                        case 2:                                                  //配置描述符
                            pDescr = CfgDesc;                                    //把设备描述符送到要发送的缓冲区
                            len = sizeof(CfgDesc);
                            break;
                        default:
                            len = 0xff;                                          //不支持的命令或者出错
                            break;
                        }
                        if ( SetupLen > len )
                        {
                            SetupLen = len;    //限制总长度
                        }
                        len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;//本次传输长度
                        memcpy(Ep0Buffer,pDescr,len);                            //加载上传数据
                        SetupLen -= len;
                        pDescr += len;
                        break;
                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL;                         //暂存USB设备地址
                        break;
                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;
                        if ( SetupLen >= 1 )
                        {
                            len = 1;
                        }
                        break;
                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;
                        break;
                    case 0x0A:
                        break;
                    case USB_CLEAR_FEATURE:                                      //Clear Feature
                        if ( ( UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )// 端点
                        {
                            switch( UsbSetupBuf->wIndexL )
                            {
                            case 0x82:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x02:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
						
                            default:
                                len = 0xFF;                                       // 不支持的端点
                                break;
                            }
                        }
                        else
                        {
                            len = 0xFF;                                           // 不是端点不支持
                        }
                        break;
                    case USB_SET_FEATURE:                                         /* Set Feature */
                        if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x00 )        /* 设置设备 */
                        {
                            if( ( ( ( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x01 )
                            {
                                if( CfgDesc[ 7 ] & 0x20 )
                                {
                                    /* 设置唤醒使能标志 */
                                }
                                else
                                {
                                    len = 0xFF;                                    /* 操作失败 */
                                }
                            }
                            else
                            {
                                len = 0xFF;                                        /* 操作失败 */
                            }
                        }
                        else if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x02 )    /* 设置端点 */
                        {
                            if( ( ( ( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x00 )
                            {
                                switch( ( ( UINT16 )UsbSetupBuf->wIndexH << 8 ) | UsbSetupBuf->wIndexL )
                                {
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点2 IN STALL */
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* 设置端点2 OUT Stall */
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点1 IN STALL */
                                    break;
                                default:
                                    len = 0xFF;                                     /* 操作失败 */
                                    break;
                                }
                            }
                            else
                            {
                                len = 0xFF;                                         /* 操作失败 */
                            }
                        }
                        else
                        {
                            len = 0xFF;                                             /* 操作失败 */
                        } 
                        break;
                    case USB_GET_STATUS:
                        Ep0Buffer[0] = 0x00;
                        Ep0Buffer[1] = 0x00;
                        if ( SetupLen >= 2 )
                        {
                            len = 2;
                        }
                        else
                        {
                            len = SetupLen;
                        }
                        break;
                    default:
                        len = 0xff;                                                  //操作失败
                        break;
                    }
                }
            }
            else
            {
                len = 0xff;                                                          //包长度错误
            }
            if(len == 0xff)
            {
                SetupReq = 0xFF;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;//STALL
            }
            else if(len <= THIS_ENDP0_SIZE)                                         //上传数据或者状态阶段返回0长度包
            {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//默认数据包是DATA1，返回应答ACK
            }
            else
            {
                UEP0_T_LEN = 0;  //虽然尚未到状态阶段，但是提前预置上传0长度数据包以防主机提前进入状态阶段
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//默认数据包是DATA1,返回应答ACK
            }
            break;
        case UIS_TOKEN_IN | 0:                                                      //endpoint0 IN
            switch(SetupReq)
            {
            case USB_GET_DESCRIPTOR:
                len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;     //本次传输长度
                memcpy( Ep0Buffer, pDescr, len );                                   //加载上传数据
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;                                            //同步标志位翻转
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0;                                                      //状态阶段完成中断或者是强制上传0长度数据包结束控制传输
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;
        case UIS_TOKEN_OUT | 0:  // endpoint0 OUT
            len = USB_RX_LEN;
            if(SetupReq == 0x09)
            {
                if(Ep0Buffer[0])
                {
                    printf("Light on Num Lock LED!\n");
                }
                else if(Ep0Buffer[0] == 0)
                {
                    printf("Light off Num Lock LED!\n");
                }
            }
            UEP0_T_LEN = 0;  //虽然尚未到状态阶段，但是提前预置上传0长度数据包以防主机提前进入状态阶段
            UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_ACK;//默认数据包是DATA0,返回应答ACK
            break;
        default:
            break;
        }
        UIF_TRANSFER = 0;                                                           //写0清空中断
    }
    if(UIF_BUS_RST)                                                                 //设备模式USB总线复位中断
    {
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        UIF_BUS_RST = 0;                                                             //清中断标志
		    Uart_Input_Point = 0;   //循环缓冲区输入指针
				Uart_Output_Point = 0;  //循环缓冲区读出指针
				UartByteCount = 0;      //当前缓冲区剩余待取字节数
				USBByteCount = 0;       //USB端点收到的长度
				UsbConfig = 0;          //清除配置值
				UpPoint2_Busy = 0;
    }
    if (UIF_SUSPEND)                                                                 //USB总线挂起/唤醒完成
    {
        UIF_SUSPEND = 0;
        if ( USB_MIS_ST & bUMS_SUSPEND )                                             //挂起
        {
#ifdef DE_PRINTF
            printf( "zz" );                                                          //睡眠状态
#endif
            while ( XBUS_AUX & bUART0_TX )
            {
                ;    //等待发送完成
            }
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO | bWAK_RXD1_LO;                      //USB或者RXD0/1有信号时可被唤醒
            PCON |= PD;                                                                 //睡眠
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = 0x00;
        }
    }
    else {                                                                             //意外的中断,不可能发生的情况
        USB_INT_FG = 0xFF;                                                             //清中断标志
//      printf("UnknownInt  N");
    }
}
/*******************************************************************************
* Function Name  : Uart1_ISR()
* Description    : 串口接收中断函数，实现循环缓冲接收
*******************************************************************************/
void Uart1_ISR(void) interrupt INT_NO_UART1
{
	if(U1RI)   //收到数据
	{
		Receive_Uart_Buf[Uart_Input_Point++] = SBUF1;
		UartByteCount++;                    //当前缓冲区剩余待取字节数
		if(Uart_Input_Point>=UART_REV_LEN)
			Uart_Input_Point = 0;           //写入指针
		U1RI =0;		
	}
	
}
//主函数
main()
{
	UINT8 lenth;
	UINT8 j = 0;
	UINT8 Uart_Timeout = 0;
	UINT8 nowStatus = 0;
	//P3_DIR_PU=0xFF;
	P1_DIR_PU=0xFF;
	P1 &= ~0x10; //RTS LOW
	P1 &= ~0x20; //DTR LOW
	//P1 &= ~0x10; //RTS LOW
	//P1 |=  0x10; //RTS HIGH
	//P1 &= ~0x20; //DTR LOW
	//P1 |=  0x20; //DTR HIGH
	//p1.4RTS
	//p1.5DTR
    CfgFsys( );                                                           //CH559时钟选择配置
    mDelaymS(5);                                                          //修改主频等待内部晶振稳定,必加	
    mInitSTDIO( );                                                        //串口0,可以用于调试
	UART1Setup( );                                                        //用于CDC
#ifdef DE_PRINTF
    printf("start ...\n");
#endif	

    USBDeviceCfg();                                                    
    USBDeviceEndPointCfg();                                               //端点配置
    USBDeviceIntCfg();                                                    //中断初始化
	  UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;                                                       //预使用发送长度一定要清空
    UEP2_T_LEN = 0;                                                       //预使用发送长度一定要清空

    while(1)
    {
		if(UsbConfig)
		{
			if(USBByteCount)   //USB接收端点有数据
			{
				CH554UART1SendByte(Ep2Buffer[USBBufOutPoint++]);
				USBByteCount--;
				if(USBByteCount==0) 
					UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;
			}
			if(UartByteCount)
				Uart_Timeout++;
			if(!UpPoint2_Busy)   //端点不繁忙（空闲后的第一包数据，只用作触发上传）
			{
				lenth = UartByteCount;
				if(lenth>0)
				{
					if(lenth>31 || Uart_Timeout>100)
					{		
						Uart_Timeout = 0;
						if(Uart_Output_Point+lenth>UART_REV_LEN)
							lenth = UART_REV_LEN-Uart_Output_Point;
						UartByteCount -= lenth;			
						//写上传端点
						memcpy(Ep2Buffer+MAX_PACKET_SIZE,&Receive_Uart_Buf[Uart_Output_Point],lenth);
						Uart_Output_Point+=lenth;
						if(Uart_Output_Point>=UART_REV_LEN)
							Uart_Output_Point = 0;						
						UEP2_T_LEN = lenth;                                                    //预使用发送长度一定要清空
						UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;            //应答ACK
						UpPoint2_Busy = 1;
					}
				}
			}
		}		
    }
}
