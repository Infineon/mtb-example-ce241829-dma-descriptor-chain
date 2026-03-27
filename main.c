/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the PSoC 4 DMA Data Transfer with 
*              Descriptor Chain example for ModusToolbox.
*
* Related Document: See README.md 
*
*
*******************************************************************************
 * (c) 2024-2025, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
 * Include Header files
 *******************************************************************************/

#include "cy_pdl.h"
#include "cybsp.h"

/*******************************************************************************
* Macros
********************************************************************************/

/* Macro for DMA channel interrupt */
#define DMAC_CHANNEL_INTR               0x00000001u

/* Macro for DMA transfer size */
#define DMAC_TRANSFER_SIZE              16UL

/* DMA channel rrigger select line */
#define DMA_TRIGGER_SELECT              TRIG0_OUT_CPUSS_DMAC_TR_IN0

/* DMA Channel Trigger Group */
#define DMA_TRIGGER_ASSERT_CYCLES       CY_DMAC_RETRIG_4CYC

/* DELAY time */
#define DELAY                           1u

/* DEALY loop */
#define DELAY_LOOP                      100UL

/*******************************************************************************
* Global Variables
********************************************************************************/

/* Memory region. Data located in SRAM. */
const uint8_t g_region1Src[DMAC_TRANSFER_SIZE] = "PSoC4_HVMS-DMADC";
uint8_t g_region1Dst[DMAC_TRANSFER_SIZE] = {0UL};
const uint8_t g_region2Src[DMAC_TRANSFER_SIZE] = "PSoC4_HVMS-DMADC";
uint8_t g_region2Dst[DMAC_TRANSFER_SIZE] = {0UL};


/********************************************************************************
* Function Name: main
*********************************************************************************
* Summary:
* The main function performs the following actions:
*  1. Initializes the BSP
*  2. Located transfer data (PING data: Source 1 / Destination 1, and PONG data 
*     : Source 2 / Destination 2) in SRAM
*  3. Initialize and enable GPIO interrupt
*  4. Enable DMA Controller (DMAC) and initialize DMAC channel and descriptor 
*     configurations
*  5. Enable using DMAC channel
*  6. Set Trigger to initialize transfer
*  7. Confirm results on the display of terminal software
*
********************************************************************************/
int main(void)
{
    cy_rslt_t result;
    char srcdata1[16];
    char dstdata1[16];
    char srcdata2[16];
    char dstdata2[16];

    /* Initialize system */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
    
    /* Enable global interrupts */
    __enable_irq();

    /* Allocate channel number to use with DMA functions. */
    Cy_DMAC_Channel_Init(USER_DMA_HW, USER_DMA_CHANNEL, &USER_DMA_channel_config);

    /* Configure descriptor 0 for memory region transfer. */
    Cy_DMAC_Descriptor_Init(USER_DMA_HW, USER_DMA_CHANNEL, CY_DMAC_DESCRIPTOR_PING, &USER_DMA_ping_config);
    Cy_DMAC_Descriptor_SetSrcAddress(USER_DMA_HW, USER_DMA_CHANNEL, CY_DMAC_DESCRIPTOR_PING, &g_region1Src);
    Cy_DMAC_Descriptor_SetDstAddress(USER_DMA_HW, USER_DMA_CHANNEL, CY_DMAC_DESCRIPTOR_PING, &g_region1Dst);

    /* Configure descriptor 1 for memory region transfer. */
    Cy_DMAC_Descriptor_Init(USER_DMA_HW, USER_DMA_CHANNEL, CY_DMAC_DESCRIPTOR_PONG, &USER_DMA_pong_config);
    Cy_DMAC_Descriptor_SetSrcAddress(USER_DMA_HW, USER_DMA_CHANNEL, CY_DMAC_DESCRIPTOR_PONG, &g_region2Src);
    Cy_DMAC_Descriptor_SetDstAddress(USER_DMA_HW, USER_DMA_CHANNEL, CY_DMAC_DESCRIPTOR_PONG, &g_region2Dst);

    /* Enable the DMA channel */
    Cy_DMAC_Channel_Enable(USER_DMA_HW, USER_DMA_CHANNEL);

    /* Enable DMA engine. */
    Cy_DMAC_Enable(USER_DMA_HW);

    /* Initialize with config set in peripheral and enable the UART to display the result*/
    Cy_SCB_UART_Init(UART_HW, &UART_config, NULL);
    Cy_SCB_UART_Enable(UART_HW);

    Cy_SCB_UART_PutString(UART_HW, "\x1b[2J\x1b[;H");
    Cy_SCB_UART_PutString(UART_HW, "************************************************************\r\n");
    Cy_SCB_UART_PutString(UART_HW, "DMA Data Transfer with Descriptor Chain \r\n");
    Cy_SCB_UART_PutString(UART_HW, "************************************************************\r\n\n");

    /* At this point both transfer descriptors are configured, the DMA channel
    * is enabled and waiting for a trigger.
    * 
    * Generate an SW trigger to initiate a transfer.
    */
    Cy_TrigMux_SwTrigger(DMA_TRIGGER_SELECT, DMA_TRIGGER_ASSERT_CYCLES);

    /* Wait until transfer is over */
    while (CY_DMAC_DONE == Cy_DMAC_Descriptor_GetResponse(USER_DMA_HW, USER_DMA_CHANNEL, CY_DMAC_DESCRIPTOR_PONG));

    /* Validate the transferred data */
    Cy_SCB_UART_PutString(UART_HW, "PING source = ");

    for(uint8_t i=0; i<DMAC_TRANSFER_SIZE; i++)
    {
        srcdata1[i] = g_region1Src[i];
        for(uint8_t j=0; j<DELAY_LOOP; j++)
        {
            if(Cy_SCB_UART_IsTxComplete(UART_HW))
            {
                Cy_SCB_UART_Put(UART_HW, srcdata1[i]);
                break;
            }
            Cy_SysLib_Delay(DELAY);
        }
    }
    /* Validate the transferred data */
    Cy_SCB_UART_PutString(UART_HW, "\r\n");
    Cy_SCB_UART_PutString(UART_HW, "PING destination = ");

    for(uint8_t i=0; i<DMAC_TRANSFER_SIZE; i++)
    {
        dstdata1[i] = g_region1Dst[i];
        for(uint8_t j=0; j<DELAY_LOOP; j++)
        {
            if(Cy_SCB_UART_IsTxComplete(UART_HW))
            {
                Cy_SCB_UART_Put(UART_HW, dstdata1[i]);
                break;
            }
            Cy_SysLib_Delay(DELAY);
        }
    }
    Cy_SCB_UART_PutString(UART_HW, "\r\n");

    /* Validate the transferred data */
    Cy_SCB_UART_PutString(UART_HW, "PONG source = ");

    for(uint32_t i=15; i<DMAC_TRANSFER_SIZE; i--)
    {
        srcdata2[i] = g_region2Src[i];
        for(uint8_t j=0; j<DELAY_LOOP; j++)
        {
            if(Cy_SCB_UART_IsTxComplete(UART_HW))
            {
                Cy_SCB_UART_Put(UART_HW, srcdata2[i]);
                break;
            }
            Cy_SysLib_Delay(DELAY);
        }
    }
    /* Validate the transferred data */
    Cy_SCB_UART_PutString(UART_HW, "\r\n");
    Cy_SCB_UART_PutString(UART_HW, "PONG destination = ");

    for(uint32_t i=15; i<DMAC_TRANSFER_SIZE; i--)
    {
        dstdata2[i] = g_region2Dst[i];
        for(uint8_t j=0; j<DELAY_LOOP; j++)
        {
            if(Cy_SCB_UART_IsTxComplete(UART_HW))
            {
                Cy_SCB_UART_Put(UART_HW, dstdata2[i]);
                break;
            }
            Cy_SysLib_Delay(DELAY);
        }
   }

    Cy_SCB_UART_PutString(UART_HW, "\r\n");


    Cy_SCB_UART_PutString(UART_HW, "- DMA transfer is completed. \r\n");

    for(;;)
    {
    }
}

/* [] END OF FILE */
