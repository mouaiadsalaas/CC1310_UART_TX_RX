/* Standard C Libraries */
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
/* POSIX Header files */
#include <pthread.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* TI Drivers */
#include <ti/drivers/PIN.h>
#include <ti/sysbios/BIOS.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/UART.h>

#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>


/* Board Header file */
#include "Board.h"
#define delayMs(a)                Task_sleep(a*1000 / Clock_tickPeriod);
    char str[400] = {0};


/* Sytstem Tasks and clocks Structs */
Task_Struct task0Struct,
            task1Struct,
            task2Struct,
            task3Struct;

Clock_Struct clk2Struct;
Clock_Struct clk0Struct, clk1Struct;

/* System drivers handles */
Clock_Handle clk2Handle;
Clock_Handle clk2Handle;
UART_Handle uart;
#define THREADSTACKSIZE         1024

Char task0Stack[THREADSTACKSIZE],
     task1Stack[THREADSTACKSIZE],
     task2Stack[THREADSTACKSIZE],
     task3Stack[THREADSTACKSIZE];

/* Pin driver handles */
static PIN_Handle ledPinHandle;

/* Global memory storage for a PIN_Config table */
static PIN_State ledPinState;
uint32_t Rx_data_index = 0;
char Rx_data[100];
/* System Output Configuration */
PIN_Config ledPinTable[] = {

    IOID_6 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    IOID_7 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

unsigned char buf[128] = {0};
uint32_t GlobalCounter = 0;
bool tick = 0;
Void clk0Fxn(UArg arg0)
{
    GlobalCounter++;
    tick=true;
}

///*UART write function*/
//void GPRS_SendAtCommand( const char * data ){
//    //GPRS_BufferClear();
//   UART_write(uart, (unsigned char *)data, strlen(data));
//}


void uartReadCallback(UART_Handle handle, void *buf, size_t indexcount){
    //memset(Rx_data, '0', sizeof(Rx_data));
    strncpy(&Rx_data[Rx_data_index], (char *)buf, indexcount);
    Rx_data_index+=indexcount;
}


void uartThread(UArg arg0, UArg arg1)
{
    char rcvData;
    UART_Params uartParams;

    /* Create a UART with data processing off. */
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_TEXT;
    uartParams.readDataMode = UART_DATA_TEXT;
    uartParams.readMode = UART_MODE_CALLBACK;
    uartParams.readCallback = &uartReadCallback;
    uartParams.readTimeout = 10;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate = 9600;
    uart = UART_open(Board_UART0, &uartParams);

    if (uart == NULL) {
        /* UART_open() failed */
        while (1);
    }

    /* Loop forever echoing */
    while (1) {

        UART_read(uart, &rcvData, 1);
        delayMs(10);
    }
}

void clockThread(UArg arg0, UArg arg1)
{
    /* Construct BIOS Objects */
    Clock_Params clkParams;

    /* Call driver init functions */
    Clock_Params_init(&clkParams);
    clkParams.period = 1000;     //1 seconde
    clkParams.startFlag = TRUE;
    //startFlag = true; //starts immediatley
    //startFlag = false; //starts after timeout.

    /* Construct a periodic Clock Instance */
    Clock_construct(&clk0Struct, (Clock_FuncPtr)clk0Fxn,
                    1, &clkParams);


    clk2Handle = Clock_handle(&clk0Struct);

    Clock_start(clk2Handle);
    //Clock_stop(clk2Handle);

    while (1) {

        delayMs(10);
    }

}
/*UART write function*/
void SendCommand( const char * data ){
    //GPRS_BufferClear();
   UART_write(uart, (unsigned char *)data, strlen(data));
}

void setData( char * data){
    SendCommand(data);
}

void *mainThread(void *arg0)
{

    /* Loop forever echoing */
    while (1) {
        //GPRS_SendAtCommand("OMAR\r\n");
        delayMs(1000);
        /*UART write function*/
        SendCommand("Omar");
    }
}


int main(void){
    Task_Params taskParams;


    Board_init();

    GPIO_init();
    UART_init();


    /* Open LED pins */
    ledPinHandle = PIN_open(&ledPinState, ledPinTable);
    if(!ledPinHandle) {
        /* Error initializing board LED pins */
        while(1);
    }




    Task_Params_init(&taskParams);
    taskParams.stackSize = THREADSTACKSIZE;
    taskParams.stack = &task0Stack;
    taskParams.instance->name = "mainThread";
    Task_construct(&task0Struct, (Task_FuncPtr)mainThread, &taskParams, NULL);

    Task_Params_init(&taskParams);
    taskParams.stackSize = THREADSTACKSIZE;
    taskParams.stack = &task1Stack;
    taskParams.instance->name = "uartThread";
    Task_construct(&task1Struct, (Task_FuncPtr)uartThread, &taskParams, NULL);

    Task_Params_init(&taskParams);
    taskParams.stackSize = THREADSTACKSIZE;
    taskParams.stack = &task2Stack;
    taskParams.instance->name = "clockThread";
    Task_construct(&task2Struct, (Task_FuncPtr)clockThread, &taskParams, NULL);


    BIOS_start();

    return (0);
}

