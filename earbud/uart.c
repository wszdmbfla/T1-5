#include <panic.h>
#include <stddef.h>
#include <led.h>
#include <string.h>
#include <stream.h>
#include <message.h>
#include "uart.h"
#include "sink.h"
#include <vmal.h>
#include "av_headset.h"
#include "av_headset_log.h"

#ifdef INCLUDE_UART
#define BANK(pio) ((uint16)((pio) / 32))
#define OFFSET(pio) (1UL << ((pio) % 32))

static void uart_pio(void)
{
    PioSetMapPins32Bank(BANK(16), OFFSET(16), 0); // change   PIO16 for TX
    PioSetMapPins32Bank(BANK(17), OFFSET(17), 0); // change   PIO17 for RX
    
    PioSet32Bank(BANK(16), OFFSET(16), OFFSET(16));
    PioSet32Bank(BANK(17), OFFSET(17), OFFSET(17));
    
    PioSetFunction(0x10, UART_TX); // change PIO16 for TX
    PioSetFunction(0x11, UART_RX); // change PIO17 for RX

}

void PilAppUARTInit(void)
{
    UARTStreamTaskData *uart = appGetUart();
         
    uart->task.handler = uart_handler;
    StreamConfigure(VM_STREAM_UART_CONFIG, VM_STREAM_UART_THROUGHPUT);
    uart_pio();
    VmalMessageSinkTask(StreamUartSink(),&uart->task);
    StreamUartConfigure(VM_UART_RATE_115K2 ,VM_UART_STOP_ONE,VM_UART_PARITY_NONE);
}

void uart_send(const char *data, uint16 length)
{   
    Sink uart_sink = StreamUartSink();
    uint16 offset = 0;
    uint8 *dest = NULL;
    /* Claim space in the sink, getting the offset to it */
    offset = SinkClaim(uart_sink, length);
    if(offset == 0xFFFF) Panic();
    /* Map the sink into memory space */
    dest = SinkMap(uart_sink);
    PanicZero(dest);
    /* Copy data into the claimed space */
    memcpy(dest+offset, data, length);
    /* Flush the data out to the uart */
    PanicZero(SinkFlush(uart_sink, length));

}

void uart_rev(void) 
{
    DEBUG_LOG("uart_rev-----entry");

    Sink uart_sink = StreamUartSink();
    Source src = StreamSourceFromSink(uart_sink);
    uint16 length = 0;
    const uint8 *data = NULL;
    /* Get the number of bytes in the specified source before the next packet
    * boundary */
    if(!(length = SourceBoundary(src)))
    return;
    /* Maps the specified source into the address map */
    data = SourceMap(src);
    PanicNull((void*)data);
    //TODO: Copy data to local buffer.
    /* Discards the specified amount
    * of bytes from the front of the specified
    * source */
    SourceDrop(src, length);
    DEBUG_LOGF("%x%x%x%x%x",data[0],data[1],data[2],data[3],data[4]);
}
 
void uart_handler(Task task, MessageId id, Message message)
{
    UARTStreamTaskData *uart = (UARTStreamTaskData *) task;
	  
    PanicFalse(uart != NULL);
	  
    DEBUG_LOGF("uart_handler,id=%d,message=%d", id,message);
    switch(id)
    {
        case MESSAGE_MORE_SPACE:
    	  	DEBUG_LOG("uart_handler,MESSAGE_MORE_SPACE");
    	  	break;
    	  	
        case MESSAGE_MORE_DATA:
         uart_rev();
         break;

        default:
         break;
    }
}
#endif