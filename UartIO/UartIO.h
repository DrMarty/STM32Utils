/*
 * UartIO.h
 *
 *  Created on: Apr 27, 2020
 *      Author: user
 */

#ifndef INC_UARTIO_H_
#define INC_UARTIO_H_

#include "main.h"
#include <stdio.h>

#ifndef EOF
#define EOF (-1)
#endif

//#define TX_MODE_BLOCKING
//#define TX_BLOCKING_TIMEOUT	(100)		//Milliseconds until Transmitter will timeout when blocked
#define TX_MODE_INTERRUPT				//Need to ALSO ensure Global USART interrup is enabled in CubeMX

#define TX_BUFF_SZ	32		//Must be a power of 2
#define RX_BUFF_SZ	32		//Must be a power of 2

void UartIO_Init(UART_HandleTypeDef *huart);
char UartIO_getch(void);

#endif /* INC_UARTIO_H_ */
