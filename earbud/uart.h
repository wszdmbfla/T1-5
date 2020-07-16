#include <panic.h>
#include <stddef.h>
#include <led.h>
#include <string.h>
#include <stream.h>
#include <message.h>

#ifdef INCLUDE_UART
#ifndef UARTDEMO_H
#define UARTDEMO_H
typedef struct
{
   TaskData task; 
   Sink uart_sink; 
   Source uart_source;
        
}UARTStreamTaskData;

//UARTStreamTaskData theUARTStreamTask;

enum
{   
        SEND_UART_DATA
};
 
void PilAppUARTInit(void);
 
void uart_send(const char *data, uint16 lenth);
 
void uart_handler(Task task, MessageId id, Message message);
 
void uart_rev(void);
#endif
#endif