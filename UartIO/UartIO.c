/*
 * UartIO.c
 *
 *  Created on: Apr 26, 2020
 *      Author: Marty Hauff
 */

#include "UartIO.h"

static UART_HandleTypeDef *huartio;

#ifdef TX_MODE_INTERRUPT
static uint8_t TxBuff[TX_BUFF_SZ];
#endif
static uint8_t RxBuff[RX_BUFF_SZ];
static uint32_t RxIdx;		//Extract Index into RxBuff
static uint32_t TxInsertIdx;		//Insert Index into TxBuff
static uint32_t TxExtractIdx;		//Extract Index into TxBuff

/*
 * Receive Data ....
 *  Use the DMA Circular Buffer feature to continuously receive data.
 *  Need to ensure RX_BUFF_SZ is big enough to deal with maximum burst size.
 *  Need to ensure average processing time is less than average incoming data rate
 */
#define DMA_RX_INSERT_PTR	( (RX_BUFF_SZ - huartio->hdmarx->Instance->NDTR) & (RX_BUFF_SZ - 1) )

void UartIO_Init(UART_HandleTypeDef *huart)
{
	setbuf(stdout, NULL);	//Ensure the stdout buffer flushes immediately (rather than waiting for '\n')
	setbuf(stderr, NULL);
	setvbuf(stdin, NULL, _IONBF, 0);

	huartio = huart;
	HAL_UART_Receive_DMA(huartio, RxBuff, RX_BUFF_SZ);
	RxIdx = 0;
	TxInsertIdx = 0;
	TxExtractIdx = 0;
}

static int UartIO_RxEmpty(void)
{
	return (RxIdx == DMA_RX_INSERT_PTR) ? 1 : 0;
}

int __io_getchar(void)
{
	char ch;
	if (UartIO_RxEmpty())
		return (int)EOF;

	ch = RxBuff[RxIdx++];
	RxIdx &= (RX_BUFF_SZ - 1);
	return (int)ch;
}

/*
 * Transmit Data ....
 *  Hook into the atomic char output routine and send it to the UART
 *  Non-blocking and will utilize DMA to transmit out the UART
 *  but DMA routines do not update the TxExtractIdx / ptr so we need to
 *  do it manually as part of the TxComplete ISR
 *
 *  User is responsible for ensuring the TX Buffer is large enough to cater for
 *  their largest burst of data
 */

#ifdef TX_MODE_BLOCKING
//BLOCKING Implementation
/*
 * Processing will wait for the data to send out before moving on.
 * This method guarantees all data will be transmitted (if Blocking timout is high enough) but
 * will halt other processing in the system
 */

int _write(int file, char *ptr, int len)
{
	HAL_StatusTypeDef hres;
	hres = HAL_UART_Transmit(huartio, (uint8_t*)ptr, len, TX_BLOCKING_TIMEOUT);
	if (hres == HAL_OK)
		return len;
	else
		return -1;
}

//__io_putchar should only be required if not using the _write function above
void __io_putchar(uint8_t ch)
{
	HAL_UART_Transmit(huartio, &ch, 1, TX_BLOCKING_TIMEOUT);
	return;
}
#endif

#ifdef TX_MODE_INTERRUPT
//INTERRUPT Implementation
/*
 * Processing will transfer as much data as possible into the Tx Ring Buffer (TxBuff) and then use
 * interrupt processing to send it out the UART.
 * This method returns control back to the calling routine as soon as possible BUT if the ring buffer
 * is not large enough to accommodate the highest burst rate then data will be lost.
 */

int _write(int file, char *ptr, int len)
{
	int DataIdx = 0;
	int BlockAddr = (TxExtractIdx - 1) & (TX_BUFF_SZ - 1);

	while ( (DataIdx < len) && (TxInsertIdx != BlockAddr) )
	{
		TxBuff[TxInsertIdx++] = *ptr++;
		TxInsertIdx &= (TX_BUFF_SZ - 1);
		DataIdx++;
	}
	if (DataIdx != 0)
		HAL_UART_Transmit_IT(huartio, &TxBuff[TxExtractIdx], 1);

	return DataIdx;
}

//__io_putchar should only be required if not using the _write function above
void __io_putchar(uint8_t ch)
{
	TxBuff[TxInsertIdx++] = ch;
	TxInsertIdx &= (TX_BUFF_SZ - 1);
	HAL_UART_Transmit_IT(huartio, &TxBuff[TxExtractIdx], 1);
	return;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	uint16_t Size;

	TxExtractIdx = huart->pTxBuffPtr - TxBuff;
	TxExtractIdx &= (TX_BUFF_SZ - 1);
	if (TxExtractIdx == TxInsertIdx)
		return;		//Nothing more to do

	if (TxInsertIdx > TxExtractIdx)
		Size = TxInsertIdx - TxExtractIdx;
	else
		Size = TX_BUFF_SZ - TxExtractIdx;
	HAL_UART_Transmit_IT(huartio, &TxBuff[TxExtractIdx], Size);
	return;
}
#endif
