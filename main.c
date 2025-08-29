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
* Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
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
        while (!Cy_SCB_UART_IsTxComplete(UART_HW));
        Cy_SCB_UART_Put(UART_HW, srcdata1[i]);
    }
    /* Validate the transferred data */
    Cy_SCB_UART_PutString(UART_HW, "\r\n");
    Cy_SCB_UART_PutString(UART_HW, "PING destination = ");

    for(uint8_t i=0; i<DMAC_TRANSFER_SIZE; i++)
    {
        dstdata1[i] = g_region1Dst[i];
        while (!Cy_SCB_UART_IsTxComplete(UART_HW));
        Cy_SCB_UART_Put(UART_HW, dstdata1[i]);
    }
    Cy_SCB_UART_PutString(UART_HW, "\r\n");

    /* Validate the transferred data */
    Cy_SCB_UART_PutString(UART_HW, "PONG source = ");

    for(uint32_t i=15; i<DMAC_TRANSFER_SIZE; i--)
    {
        srcdata2[i] = g_region2Src[i];
        while (!Cy_SCB_UART_IsTxComplete(UART_HW));
        Cy_SCB_UART_Put(UART_HW, srcdata2[i]);
    }
    /* Validate the transferred data */
    Cy_SCB_UART_PutString(UART_HW, "\r\n");
    Cy_SCB_UART_PutString(UART_HW, "PONG destination = ");

    for(uint32_t i=15; i<DMAC_TRANSFER_SIZE; i--)
    {
        dstdata2[i] = g_region2Dst[i];
        while (!Cy_SCB_UART_IsTxComplete(UART_HW));
        Cy_SCB_UART_Put(UART_HW, dstdata2[i]);
   }

    Cy_SCB_UART_PutString(UART_HW, "\r\n");


    Cy_SCB_UART_PutString(UART_HW, "- DMA transfer is completed. \r\n");

    for(;;)
    {
    }
}

/* [] END OF FILE */
