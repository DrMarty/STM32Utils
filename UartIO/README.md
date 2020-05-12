Utility functions for using the UART for stdio.

To use:
1. Ensure a UART with RX DMA is defined in CubeMX
2. Ensure UART global interrupts are enabled
3. Download this code and save it in your project source tree. By convention, the folder structure I use is: Core >> STM32Utils >> UartIO
4. In your main.c file:
* add the line #include "..\Utils\UartIO\UartIO.h"
* after all MX_xxx_init() functions, initalize the UartIO with the function UartIO_Init(&huart1);
5. Use printf() / scanf() freely.

Note:
If you find characters are being dropped:
* adjust TX_BUFF_SZ and/or RX_BUFF_SZ in UartIO.h.
* consider increasing the baud rate
