/**
  ******************************************************************************
  * @file    usbh_core.c 
  * @author  MCD Application Team
  * @version V2.1.0
  * @date    19-March-2012
  * @brief   This file implements the functions for the core state machine process
  *          the enumeration and the control transfer process
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/

#include "usbh_ioreq.h"
#include "usb_bsp_usr.h"
#include "usbh_hcs.h"
#include "usbh_stdreq.h"
#include "usbh_core.h"
#include "usb_hcd_int.h"


/** @addtogroup USBH_LIB
  * @{
  */

/** @addtogroup USBH_LIB_CORE
* @{
*/

/** @defgroup USBH_CORE 
  * @brief TThis file handles the basic enumaration when a device is connected 
  *          to the host.
  * @{
  */

/** @defgroup USBH_CORE_Private_TypesDefinitions
  * @{
  */
uint8_t USBH_Disconnected (USB_OTG_CORE_HANDLE *pdev);
uint8_t USBH_Connected (USB_OTG_CORE_HANDLE *pdev);
uint8_t USBH_SOF (USB_OTG_CORE_HANDLE *pdev);

USBH_HCD_INT_cb_TypeDef USBH_HCD_INT_cb =
		{
				USBH_SOF,
				USBH_Connected,
				USBH_Disconnected,
		};

USBH_HCD_INT_cb_TypeDef  *USBH_HCD_INT_fops = &USBH_HCD_INT_cb;
/**
  * @}
  */


/** @defgroup USBH_CORE_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup USBH_CORE_Private_Macros
  * @{
  */
/**
  * @}
  */


/** @defgroup USBH_CORE_Private_Variables
  * @{
  */
/**
  * @}
  */


/** @defgroup USBH_CORE_Private_FunctionPrototypes
  * @{
  */
static USBH_Status USBH_HandleEnum(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost);
USBH_Status USBH_HandleControl (USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost);

/**
  * @}
  */


/** @defgroup USBH_CORE_Private_Functions
  * @{
  */


/**
  * @brief  USBH_Connected
  *         USB Connect callback function from the Interrupt. 
  * @param  selected device
  * @retval Status
*/
uint8_t USBH_Connected (USB_OTG_CORE_HANDLE *pdev)
{
	pdev->host.ConnSts = 1;
	return 0;
}

/**
* @brief  USBH_Disconnected
*         USB Disconnect callback function from the Interrupt. 
* @param  selected device
* @retval Status
*/

uint8_t USBH_Disconnected (USB_OTG_CORE_HANDLE *pdev)
{
	pdev->host.ConnSts = 0;
	return 0;
}

/**
  * @brief  USBH_SOF
  *         USB SOF callback function from the Interrupt. 
  * @param  selected device
  * @retval Status
  */

uint8_t USBH_SOF (USB_OTG_CORE_HANDLE *pdev)
{
	/* This callback could be used to implement a scheduler process */
	return 0;
}

/**
  * @brief  USBH_DeInit   初始化主机  Re-Initialize Host
  * @param  None
  * @retval status: USBH_Status
  */
USBH_Status USBH_DeInit(USB_OTG_CORE_HANDLE *dev_p, USBH_HOST *host_p)//初始化主机
{
	/* Software Init */
	host_p->gState = HOST_IDLE; 		// 主机状态: 主机空闲
	host_p->gStateBkp = HOST_IDLE;	// 先前的主机状态备份: 主机空闲
	host_p->EnumState = ENUM_IDLE;	// 枚举状态: 枚举空闲
	host_p->RequestState = CMD_SEND;	// 请求状态: 请求发送

	/* 控制类型 */
	host_p->Control.state = CTRL_SETUP; // 设置
	host_p->Control.ep0size = USB_OTG_MAX_EP0_SIZE;	// 数据大小

	/* USB 设备类型定义  */
	host_p->device_prop.address = USBH_DEVICE_ADDRESS_DEFAULT;
	host_p->device_prop.speed = HPRT0_PRTSPD_FULL_SPEED;

	/* 释放 USB 主机通道 */
	USBH_Free_Channel  (dev_p, host_p->Control.hc_num_in);
	USBH_Free_Channel  (dev_p, host_p->Control.hc_num_out);
	return USBH_OK;
}

/**
  * @brief  USBH_Init
  *         Host hardware and stack initializations 
  * @param  class_cb: Class callback structure address
  * @param  usr_cb: User callback structure address
  * @retval None
 @brief USBH_Init USB主机硬件和堆栈初始化
 @param class_cb：类回调结构地址
 @param usr_cb：用户回调结构地址
 @retval无
  */
void USBH_Init(USB_OTG_CORE_HANDLE *pdev,
			   USB_OTG_CORE_ID_TypeDef coreID,
			   USBH_HOST *host_p,
			   USBH_Class_cb_TypeDef *class_cb, //类回调结构地址
			   USBH_Usr_cb_TypeDef *usr_cb) // 用户回调结构地址
{
	/* Hardware Init */
	//USB OTG 底层IO初始化
	//pdev:USB OTG内核结构体指针
	USB_OTG_BSP_Init(pdev);// USB OTG 底层IO初始化 GPIO

	/* configure GPIO pin used for switching VBUS power */
	USB_OTG_BSP_ConfigVBUS(NULL);//USB_OTG 端口供电IO配置(本例程未用到)

	/* Host de-initializations */
	USBH_DeInit(pdev, host_p);//初始化主机

	host_p->class_cb = class_cb;	/** 类回调结构地址 **/
	host_p->usr_cb = usr_cb;		/** 用户回调结构地址 **/

	/* 初始化驱动程序的HOST部分 */
	HCD_Init(pdev , coreID);

	/*USB HOST 用户回调函数: USB HOST 初始化*/
	host_p->usr_cb->Init();

	/* Enable Interrupts */
	USB_OTG_BSP_EnableInterrupt(pdev);//USB OTG 中断设置,开启USB FS中断
}

/**
* @brief  USBH_Process
*         USB Host core main state machine process   USB Host 核心主状态机过程
* @param  None
* @retval None
*/

void USBH_Process(USB_OTG_CORE_HANDLE *pdev , USBH_HOST *phost)
{
	volatile USBH_Status status = USBH_FAIL;

	/* check for Host port events 检测到设备插入、设备拔除 */
	if ((HCD_IsDeviceConnected(pdev) == 0) && (phost->gState != HOST_IDLE/*主机空闲*/))
	{
		_debug_log_info("设备已拔除")
//    if(phost->gState != HOST_DEV_DISCONNECTED) 
//    {
		phost->gState = HOST_DEV_DISCONNECTED;//设备已拔除
//    }
	}

	switch (phost->gState)/* Host State */
	{
		case HOST_IDLE :					//主机空闲
			/* check for Host port events 检测到设备插入、设备拔除 */
			if (HCD_IsDeviceConnected(pdev))
			{
				_debug_log_info("主机空闲 -> 设备已插入，复位总线100ms")
				USB_OTG_BSP_mDelay(90);//若检测到设备插入，复位总线100ms
				phost->gState = HOST_DEV_ATTACHED;//设备已插入
			}
			break;

		case HOST_DEV_ATTACHED :			//设备已插入
			/*USB HOST 用户回调函数: 检测到USB设备插入*/
			phost->usr_cb->DeviceAttached();

			phost->Control.hc_num_in = USBH_Alloc_Channel(pdev, 0x80);
			phost->Control.hc_num_out = USBH_Alloc_Channel(pdev, 0x00);

			/* Reset USB Device */ // 复位USB设备
			if ( HCD_ResetPort(pdev) == 0)
			{
				/*USB HOST 用户回调函数: 复位USB设备*/
				phost->usr_cb->ResetDevice();

				/*  Wait for USB USBH_ISR_PrtEnDisableChange()
				Host is Now ready to start the Enumeration       */

				phost->device_prop.speed = HCD_GetCurrentSpeed(pdev);//获取设备速度信息

				/*USB HOST 用户回调函数: 检测到从机速度*/
				phost->usr_cb->DeviceSpeedDetected(phost->device_prop.speed);

				_debug_log_info("分配并打开两个channel")
				/* Open Control pipes */
				USBH_Open_Channel (pdev,
								   phost->Control.hc_num_in,
								   phost->device_prop.address,
								   phost->device_prop.speed,
								   EP_TYPE_CTRL,
								   phost->Control.ep0size);
				/* Open Control pipes */
				USBH_Open_Channel (pdev,
								   phost->Control.hc_num_out,
								   phost->device_prop.address,
								   phost->device_prop.speed,
								   EP_TYPE_CTRL,
								   phost->Control.ep0size);

				phost->gState = HOST_ENUMERATION; // 枚举中
			}
			break;

		case HOST_ENUMERATION: // 枚举中
			/* Check for enumeration status */
			if ( USBH_HandleEnum(pdev , phost) == USBH_OK)
			{
				/* The function shall return USBH_OK when full enumeration is complete */

				/*USB HOST 用户回调函数: 设备枚举完成*/
				phost->usr_cb->EnumerationDone();
				phost->gState  = HOST_USR_INPUT; // 等待用户输入
			}
			break;

		case HOST_USR_INPUT: // 等待用户输入
			/*The function should return user response true to move to class state */
			/*USB HOST 用户回调函数: 等待用户输入*/
			if ( phost->usr_cb->UserInput() == USBH_USR_RESP_OK)
			{
				if((phost->class_cb->Init(pdev, phost)) == USBH_OK)
				{
					phost->gState = HOST_CLASS_REQUEST;//为发送设备所属类的请求做准备
				}
			}
			break;

		case HOST_CLASS_REQUEST: // 为发送设备所属类的请求做准备
			/* process class standard contol requests state machine */
			status = phost->class_cb->Requests(pdev, phost);
			if(status == USBH_OK)
			{
				phost->gState = HOST_CLASS;//处理各种类请求
			}
			else
			{
				USBH_ErrorHandle(phost, status);
			}
			break;

		case HOST_CLASS:    // 处理各种类请求
			/* process class state machine */
			status = phost->class_cb->Machine(pdev, phost);
			USBH_ErrorHandle(phost, status);
			break;

		case HOST_CTRL_XFER: // 过程控制转移状态机
			/* process control transfer state machine */
			USBH_HandleControl(pdev, phost);
			break;

		case HOST_SUSPENDED: // 挂起
			break;

		case HOST_ERROR_STATE: // 错误
			/* Re-Initilaize Host for new Enumeration */
			USBH_DeInit(pdev, phost);//初始化主机
			/*USB HOST 用户回调函数: 重新初始化*/
			phost->usr_cb->DeInit();//重新初始化
			phost->class_cb->DeInit(pdev, &phost->device_prop);
			break;

		case HOST_DEV_DISCONNECTED : // 设备已拔除
			/* Manage User disconnect operations*/
			/*USB HOST 用户回调函数: 管理用户断开连接操作*/
			phost->usr_cb->DeviceDisconnected();
			/* Re-Initilaize Host for new Enumeration */
			USBH_DeInit(pdev, phost);//初始化主机
			phost->usr_cb->DeInit();
			phost->class_cb->DeInit(pdev, &phost->device_prop);
			USBH_DeAllocate_AllChannel(pdev);
			phost->gState = HOST_IDLE;
			break;

		default : break;
	}
}

/**
  * @brief  USBH_ErrorHandle 
  *         This function handles the Error on Host side.
  * @param  errType : Type of Error or Busy/OK state
  * @retval None   */
void USBH_ErrorHandle(USBH_HOST *phost, USBH_Status errType)
{
	/* Error unrecovered or not supported device speed */
	if ( (errType == USBH_ERROR_SPEED_UNKNOWN) ||
		 (errType == USBH_UNRECOVERED_ERROR) )
	{
		/*USB HOST 用户回调函数: 无法恢复的错误*/
		phost->usr_cb->UnrecoveredError();
		phost->gState = HOST_ERROR_STATE;
	}
		/* USB host restart requested from application layer */
	else if(errType == USBH_APPLY_DEINIT)
	{
		phost->gState = HOST_ERROR_STATE;
		/*USB HOST 用户回调函数: USB HOST 初始化*/
		phost->usr_cb->Init();
	}
}

/**
  * @brief  USBH_HandleEnum 
  *         This function includes the complete enumeration process
  * @param  pdev: Selected device
  * @retval USBH_Status
  */
static USBH_Status USBH_HandleEnum(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{
	USBH_Status Status = USBH_BUSY;
	uint8_t buf_string[64];

	switch (phost->EnumState) // 枚举状态
	{
		case ENUM_IDLE:
			/* Get Device Desc for only 1st 8 bytes : To get EP0 MaxPacketSize */
			if ( USBH_Get_DevDesc(pdev , phost, 8) == USBH_OK)
			{
				phost->Control.ep0size = phost->device_prop.Dev_Desc.bMaxPacketSize;

				/* Issue Reset  */
				HCD_ResetPort(pdev);

				/* modify control channels configuration for MaxPacket size */
				USBH_Modify_Channel (pdev,
									 phost->Control.hc_num_out,
									 0,
									 0,
									 0,
									 phost->Control.ep0size);
				USBH_Modify_Channel (pdev,
									 phost->Control.hc_num_in,
									 0,
									 0,
									 0,
									 phost->Control.ep0size);

				phost->EnumState = ENUM_GET_FULL_DEV_DESC;
			}
			break;

		case ENUM_GET_FULL_DEV_DESC:
			/* Get FULL Device Desc  */
			if ( USBH_Get_DevDesc(pdev, phost, USB_DEVICE_DESC_SIZE) == USBH_OK)
			{
				/*USB HOST 用户回调函数: 设备描述符可用*/
				phost->usr_cb->DeviceDescAvailable(&phost->device_prop.Dev_Desc);

				phost->EnumState = ENUM_SET_ADDR;
			}
			break;

		case ENUM_SET_ADDR:
			/* set address */
			if ( USBH_SetAddress(pdev, phost, USBH_DEVICE_ADDRESS) == USBH_OK)
			{
				USB_OTG_BSP_uDelay(1900);
				phost->device_prop.address = USBH_DEVICE_ADDRESS;

				/*USB HOST 用户回调函数: 从机地址分配成功*/
				phost->usr_cb->DeviceAddressAssigned();

				/* modify control channels to update device address */
				USBH_Modify_Channel (pdev,
									 phost->Control.hc_num_in,
									 phost->device_prop.address,
									 0,
									 0,
									 0);
				USBH_Modify_Channel (pdev,
									 phost->Control.hc_num_out,
									 phost->device_prop.address,
									 0,
									 0,
									 0);

				phost->EnumState = ENUM_GET_CFG_DESC;
			}
			break;

		case ENUM_GET_CFG_DESC:
			/* get standard configuration descriptor */
			if ( USBH_Get_CfgDesc(pdev, phost,
								  USB_CONFIGURATION_DESC_SIZE) == USBH_OK)
			{
				phost->EnumState = ENUM_GET_FULL_CFG_DESC;
			}
			break;

		case ENUM_GET_FULL_CFG_DESC:
			/* get FULL config descriptor (config, interface, endpoints) */
			if (USBH_Get_CfgDesc(pdev, phost,
								 phost->device_prop.Cfg_Desc.wTotalLength) == USBH_OK)
			{
				/*USB HOST 用户回调函数: 设备配置描述符 */
				phost->usr_cb->ConfigurationDescAvailable(&phost->device_prop.Cfg_Desc,
														  phost->device_prop.Itf_Desc,
														  phost->device_prop.Ep_Desc[0]);

				phost->EnumState = ENUM_GET_MFC_STRING_DESC;
			}
			break;

		case ENUM_GET_MFC_STRING_DESC:	// 制造商字符串描述符
			if (phost->device_prop.Dev_Desc.iManufacturer != 0)
			{
				/* Check that Manufacturer String is available */
				if ( USBH_Get_StringDesc(pdev,
										 phost,
										 phost->device_prop.Dev_Desc.iManufacturer,
										 buf_string ,
										 0xff) == USBH_OK)
				{
					/*USB HOST 用户回调函数: 制造商字符串描述符 */
					phost->usr_cb->ManufacturerString(buf_string);
					phost->EnumState = ENUM_GET_PRODUCT_STRING_DESC;
				}
			}
			else
			{
				/*USB HOST 用户回调函数: 制造商字符串描述符 */
				phost->usr_cb->ManufacturerString("N/A");
				phost->EnumState = ENUM_GET_PRODUCT_STRING_DESC;
			}
			break;

		case ENUM_GET_PRODUCT_STRING_DESC: // 产品字符串索引
			if (phost->device_prop.Dev_Desc.iProduct != 0)
			{
				/* Check that Product string is available */
				if ( USBH_Get_StringDesc(pdev,
										 phost,
										 phost->device_prop.Dev_Desc.iProduct,
										 buf_string,
										 0xff) == USBH_OK)
				{
					/*USB HOST 用户回调函数: 产品字符串索引 */
					phost->usr_cb->ProductString(buf_string);
					phost->EnumState = ENUM_GET_SERIALNUM_STRING_DESC;
				}
			}
			else
			{
				/*USB HOST 用户回调函数: 产品字符串索引 */
				phost->usr_cb->ProductString("N/A");
				phost->EnumState = ENUM_GET_SERIALNUM_STRING_DESC;
			}
			break;

		case ENUM_GET_SERIALNUM_STRING_DESC: //设备序列号索引
			if (phost->device_prop.Dev_Desc.iSerialNumber != 0)
			{ /* Check that Serial number string is available */
				if ( USBH_Get_StringDesc(pdev,
										 phost,
										 phost->device_prop.Dev_Desc.iSerialNumber,
										 buf_string,
										 0xff) == USBH_OK)
				{
					/*USB HOST 用户回调函数: 设备序列号索引 */
					phost->usr_cb->SerialNumString(buf_string);
					phost->EnumState = ENUM_SET_CONFIGURATION;
				}
			}
			else
			{
				/*USB HOST 用户回调函数: 设备序列号索引 */
				phost->usr_cb->SerialNumString("N/A");
				phost->EnumState = ENUM_SET_CONFIGURATION;
			}
			break;

		case ENUM_SET_CONFIGURATION:
			/* set configuration  (default config) */
			if (USBH_SetCfg(pdev,
							phost,
							phost->device_prop.Cfg_Desc.bConfigurationValue) == USBH_OK)
			{
				phost->EnumState = ENUM_DEV_CONFIGURED;
			}
			break;

		case ENUM_DEV_CONFIGURED:
			/* user callback for enumeration done */
			Status = USBH_OK;
			break;

		default:
			break;
	}
	return Status;
}


/**
  * @brief  USBH_HandleControl
  *         Handles the USB control transfer state machine
  * @param  pdev: Selected device
  * @retval Status
  */
USBH_Status USBH_HandleControl (USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost)
{
	uint8_t direction;
	static uint16_t timeout = 0;
	USBH_Status status = USBH_OK;
	URB_STATE URB_Status = URB_IDLE;

	phost->Control.status = CTRL_START;

	switch (phost->Control.state)
	{
		case CTRL_SETUP:
			/* send a SETUP packet */
			USBH_CtlSendSetup     (pdev,
								   phost->Control.setup.d8 ,
								   phost->Control.hc_num_out);
			phost->Control.state = CTRL_SETUP_WAIT;
			break;

		case CTRL_SETUP_WAIT:
			URB_Status = HCD_GetURB_State(pdev , phost->Control.hc_num_out);
			/* case SETUP packet sent successfully */
			if(URB_Status == URB_DONE)
			{
				direction = (phost->Control.setup.b.bmRequestType & USB_REQ_DIR_MASK);

				/* check if there is a data stage */
				if (phost->Control.setup.b.wLength.w != 0 )
				{
					timeout = DATA_STAGE_TIMEOUT;
					if (direction == USB_D2H)
					{
						/* Data Direction is IN */
						phost->Control.state = CTRL_DATA_IN;
					}
					else
					{
						/* Data Direction is OUT */
						phost->Control.state = CTRL_DATA_OUT;
					}
				}
				else /* No DATA stage */
				{
					timeout = NODATA_STAGE_TIMEOUT;

					/* If there is No Data Transfer Stage */
					if (direction == USB_D2H)
					{
						/* Data Direction is IN */
						phost->Control.state = CTRL_STATUS_OUT;
					}
					else
					{
						/* Data Direction is OUT */
						phost->Control.state = CTRL_STATUS_IN;
					}
				}
				/* Set the delay timer to enable timeout for data stage completion */
				phost->Control.timer = HCD_GetCurrentFrame(pdev);
			}
			else if(URB_Status == URB_ERROR)
			{
				phost->Control.state = CTRL_ERROR;
				phost->Control.status = CTRL_XACTERR;
			}
			break;

		case CTRL_DATA_IN:
			/* Issue an IN token */
			USBH_CtlReceiveData(pdev,
								phost->Control.buff, // 数据
								phost->Control.length,
								phost->Control.hc_num_in);
			phost->Control.state = CTRL_DATA_IN_WAIT;
			break;

		case CTRL_DATA_IN_WAIT:
			URB_Status = HCD_GetURB_State(pdev , phost->Control.hc_num_in);
			/* check is DATA packet transfered successfully */
			if  (URB_Status == URB_DONE)
			{
				phost->Control.state = CTRL_STATUS_OUT;
			}
				/* manage error cases*/
			else if  (URB_Status == URB_STALL)
			{
				/* In stall case, return to previous machine state*/
				phost->gState = phost->gStateBkp;
			}
			else if (URB_Status == URB_ERROR ||
					 (HCD_GetCurrentFrame(pdev) - phost->Control.timer) > timeout )
			{
				/* Device error or timeout for IN transfer */
				phost->Control.state = CTRL_ERROR;
			}
			break;

		case CTRL_DATA_OUT:
			/* Start DATA out transfer (only one DATA packet)*/
			pdev->host.hc[phost->Control.hc_num_out].toggle_out = 1;
			USBH_CtlSendData (pdev,
							  phost->Control.buff, // 数据
							  phost->Control.length ,
							  phost->Control.hc_num_out);
			phost->Control.state = CTRL_DATA_OUT_WAIT;
			break;

		case CTRL_DATA_OUT_WAIT:
			URB_Status = HCD_GetURB_State(pdev , phost->Control.hc_num_out);
			if  (URB_Status == URB_DONE)
			{ /* If the Setup Pkt is sent successful, then change the state */
				phost->Control.state = CTRL_STATUS_IN;
			}
				/* handle error cases */
			else if  (URB_Status == URB_STALL)
			{
				/* In stall case, return to previous machine state*/
				phost->gState =   phost->gStateBkp;
				phost->Control.state = CTRL_STALLED;
			}
			else if  (URB_Status == URB_NOTREADY)
			{
				/* Nack received from device */
				phost->Control.state = CTRL_DATA_OUT;
			}
			else if (URB_Status == URB_ERROR)
			{
				/* device error */
				phost->Control.state = CTRL_ERROR;
			}
			break;

		case CTRL_STATUS_IN:
			/* Send 0 bytes out packet */
			USBH_CtlReceiveData (pdev, 0, 0,
								 phost->Control.hc_num_in);
			phost->Control.state = CTRL_STATUS_IN_WAIT;
			break;

		case CTRL_STATUS_IN_WAIT:
			URB_Status = HCD_GetURB_State(pdev , phost->Control.hc_num_in);
			if  ( URB_Status == URB_DONE)
			{ /* Control transfers completed, Exit the State Machine */
				phost->gState =   phost->gStateBkp;
				phost->Control.state = CTRL_COMPLETE;
			}
			else if (URB_Status == URB_ERROR ||
					 (HCD_GetCurrentFrame(pdev) - phost->Control.timer) > timeout)
			{
				phost->Control.state = CTRL_ERROR;
			}
			else if(URB_Status == URB_STALL)
			{
				/* Control transfers completed, Exit the State Machine */
				phost->gState =   phost->gStateBkp;
				phost->Control.status = CTRL_STALL;
				status = USBH_NOT_SUPPORTED;
			}
			break;

		case CTRL_STATUS_OUT:
			pdev->host.hc[phost->Control.hc_num_out].toggle_out ^= 1;
			USBH_CtlSendData (pdev, 0, 0,
							  phost->Control.hc_num_out);
			phost->Control.state = CTRL_STATUS_OUT_WAIT;
			break;

		case CTRL_STATUS_OUT_WAIT:
			URB_Status = HCD_GetURB_State(pdev , phost->Control.hc_num_out);
			if  (URB_Status == URB_DONE)
			{
				phost->gState =   phost->gStateBkp;
				phost->Control.state = CTRL_COMPLETE;
			}
			else if  (URB_Status == URB_NOTREADY)
			{
				phost->Control.state = CTRL_STATUS_OUT;
			}
			else if (URB_Status == URB_ERROR)
			{
				phost->Control.state = CTRL_ERROR;
			}
			break;

		case CTRL_ERROR:
			/* After a halt condition is encountered or an error is detected by the
			host, a control endpoint is allowed to recover by accepting the next Setup
			PID; i.e., recovery actions via some other pipe are not required for control
			endpoints. For the Default Control Pipe, a device reset will ultimately be
			required to clear the halt or error condition if the next Setup PID is not
			accepted.
			 遇到暂停条件或主机检测到错误后，通过接受下一个设置PID允许控制端点恢复；
			 即控制端点不需要通过其他管道进行恢复操作。对于默认控制管道，
			 如果不接受下一个设置PID，则最终将需要进行设备复位以清除暂停或错误情况。			*/
			if (++ phost->Control.errorcount <= USBH_MAX_ERROR_COUNT)
			{
				/* Do the transmission again, starting from SETUP Packet */
				phost->Control.state = CTRL_SETUP;
			}
			else
			{
				phost->Control.status = CTRL_FAIL;
				phost->gState = phost->gStateBkp;
				status = USBH_FAIL;
			}
			break;

		default: break;
	}
	return status;
}

