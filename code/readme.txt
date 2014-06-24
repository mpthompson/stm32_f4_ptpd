/**
  @page PTPD client demonstration Readme file
 
  @verbatim
  ***************** Portions COPYRIGHT 2011 STMicroelectronics *****************
  * @file    httpserver socket/readme.txt 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    31-October-2011
  * @brief   Description of the STM32F4x7 http server socket demonstration.
  ******************************************************************************
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  ******************************************************************************
  @endverbatim
  */
/**
  @page http server socket demonstration Readme file
 
  @verbatim
  ******************* Portions COPYRIGHT 2012 Embest Tech. Co., Ltd.** *********
  * @file    readme.txt 
  * @author  CMP Team
  * @version V1.0.0
  * @date    28-December-2012
  * @brief   Description of the STM32F4x7 http server socket demonstration.
  *          Modified to support the STM32F4DISCOVERY, STM32F4DIS-BB and
  *          STM32F4DIS-LCD modules. 
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, Embest SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT
  * OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
  * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
  * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  ******************************************************************************
  @endverbatim
  */
   
@par Description

This directory contains a set of sources files that implement a PTPD client
application, based on the socket API, for STM32F4x7 devices.

Please note that for http server socket demonstration, LwIP v1.4.1 is used a 
the TCP/IP stack and Keil RTX v4.73 is used as the Real Time Kernel.  The
libraries have been modified as needed to support PTPD functionality.

@par Hardware and Software environment
  
  - This example has been tested with the following hardware:
    - STM32F4DISCOVERY board
    - STM32F4DIS-BB for the Base Board
    - STM32F4DIS-LCD for the LCD module

  - Software development tools
    - MDK-ARM V5.10

  - Hardware Set-up
    - Mount STM32F4DISCOVERY board onto STM32F4DIS-BB board through CON1 and CON2
    - Mount STM32F4DIS-LCD module onto STM32F4DIS-BB board through CON3
    - Connect the STM32F4DIS-BB board to a PC with a crossover Ethernet cable through RJ45 connector J1
    - Connect the STM32F4DISCOVERY board to a PC with a 'USB type A to Mini-B' cable 
      through USB connector CN1 to power the board.

  In order to load the Project code, you have do the following:

   - MDK-ARM
      - Open the Discovery.uvproj project
      - Rebuild all files: Project->Rebuild all target files
      - Load project image: Debug->Start/Stop Debug Session
      - Run program: Debug->Run (F5)

/*****END OF FILE****/
