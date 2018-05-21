//------------------------------------------------------------
// SCU's Internet of Things Research Lab (SIOTLAB)
// Santa Clara University (SCU)
// Santa Clara, California
//------------------------------------------------------------
// This application is based on the Cypress WICED platform
//------------------------------------------------------------


#include "wiced.h"
#include "TemperatureMeter.h"
#include "TemperatureController.h"


/* Main application */
void application_start() {
    wiced_thread_t temperatureThreadHandle;
    temperature = 0;
    wiced_init();   /* Initialize the WICED device */

    /* Initialize and start a new thread */
    wiced_rtos_create_thread(&temperatureThreadHandle, TEMPERATURE_THREAD_PRIORITY, "temperatureThread", temperatureThread, TEMPERATURE_THREAD_STACK_SIZE, NULL);
    // TODO thread to get wifi data
    while(1) {
    	controlTemperature();
    }
}