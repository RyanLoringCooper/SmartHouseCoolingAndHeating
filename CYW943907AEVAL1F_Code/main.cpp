//------------------------------------------------------------
// SCU's Internet of Things Research Lab (SIOTLAB)
// Santa Clara University (SCU)
// Santa Clara, California
//------------------------------------------------------------
// This application is based on the Cypress WICED platform
//------------------------------------------------------------


#include "wiced.h"
#define TEMPERATURE_THREAD_PRIORITY     (10)
#define TEMPERATURE_THREAD_STACK_SIZE   (1024)

#define I2C_ADDRESS (0x42)
#define RETRIES (1)
#define DISABLE_DMA (WICED_TRUE)
#define NUM_MESSAGES (1)
#define TEMPERATURE_REG 0x07
// in milliseconds
#define DELAY_BETWEEN_TEMP_SENSOR_PROBES 1000

volatile wiced_bool_t buttonPress = WICED_FALSE;

static wiced_mutex_t temperatureMut;
static volatile double temperature;

/* Rx buffer is used to get temperature, humidity, light, and POT data - 4 bytes each */
struct Rx_buffer {
    float temp;
    float humidity;
    float light;
    float pot;
};

void temperatureThread(wiced_thread_arg_t arg) {
    Rx_buffer rx_buffer;
    /* Setup I2C master */
    wiced_i2c_device_t i2cDevice;
    i2cDevice.port = WICED_I2C_2;
    i2cDevice.address = I2C_ADDRESS;
    i2cDevice.address_width = I2C_ADDRESS_WIDTH_7BIT;
    i2cDevice.speed_mode = I2C_STANDARD_SPEED_MODE;

    wiced_i2c_init(&i2cDevice);

    /* Tx buffer is used to set the offset */
    uint8_t tx_buffer = TEMPERATURE_REG;
    wiced_i2c_message_t setOffset;
    wiced_i2c_init_tx_message(&setOffset, &tx_buffer, sizeof(tx_buffer), RETRIES, DISABLE_DMA);

    /* Initialize offset */
    wiced_i2c_transfer(&i2cDevice, &setOffset, NUM_MESSAGES);

    wiced_i2c_message_t msg;
    wiced_i2c_init_rx_message(&msg, &rx_buffer, sizeof(rx_buffer), RETRIES, DISABLE_DMA);

    while ( 1 ) {
        wiced_i2c_transfer(&i2cDevice, &msg, NUM_MESSAGES); /* Get new data from I2C */
        //WPRINT_APP_INFO(("Temperature: %.1f\t Humidity: %.1f\t Light: %.1f\t POT: %.1f\n", rx_buffer.temp, rx_buffer.humidity, rx_buffer.light, rx_buffer.pot)); /* Print data to terminal */
        wiced_rtos_lock_mutex(&temperatureMut);
        temperature = rx_buffer.temp;
        wiced_rtos_unlock_mutex(&temperatureMut);
        wiced_rtos_delay_milliseconds(DELAY_BETWEEN_TEMP_SENSOR_PROBES);
    }
}
// TODO include the thing that has information about how many people are in the building

// these are in celcius
#define LOWER_TEMPERATURE_BOUNDARY 20
#define UPPER_TEMPERATURE_BOUNDARY 21.11111111

// pin 1 on the J6 header
#define HEATER_PIN WICED_GPIO_17
// pin 2 on the J6 header 
#define AC_PIN WICED_GPIO_18


#define NO_CONTROL 0
#define AC 1
#define HEATER 2

static volatile int people = 0; 
static int controlState = NO_CONTROL;

void turnOffAC() {
    wiced_gpio_output_high(AC_PIN);
}

void turnOffHeater() {
	wiced_gpio_output_high(HEATER_PIN);
}

void turnOnHeater() {
	turnOffAC();
	wiced_gpio_output_low(HEATER_PIN);
}

void turnOnAC() {
	turnOffHeater();
	wiced_gpio_output_low(AC_PIN);
}

void checkIfControlIsNeeded() {
	if(temperature < LOWER_TEMPERATURE_BOUNDARY) {
		controlState = HEATER;
		turnOnHeater();
	} else if(temperature > UPPER_TEMPERATURE_BOUNDARY) {
		controlState = AC;
		turnOnAC();
	}
}

void checkIfStillNeedAC() {
	if(temperature < LOWER_TEMPERATURE_BOUNDARY) {
		controlState = NO_CONTROL;
		turnOffAC();
	}
}

void checkIfStillNeedHeater() {
	if(temperature > UPPER_TEMPERATURE_BOUNDARY) {
		turnOffHeater();
		controlState = NO_CONTROL;
	}
}

void controlTemperature() {
	if(people < 1) {
		controlState = NO_CONTROL;
	}
	switch(controlState) {
		case NO_CONTROL:
			checkIfControlIsNeeded();
			break;
		case AC:
			checkIfStillNeedAC();
			break;
		case HEATER:
			checkIfStillNeedHeater();
			break;
	}
}
#define TCP_PACKET_MAX_DATA_LENGTH          (30)
#define TCP_SERVER_LISTEN_PORT              (12345)

/* Stack size should cater for printf calls */
#define TCP_SERVER_COMMAND_MAX_SIZE         (10)
#define TCP_PACKET_MAX_DATA_LENGTH          (30)

/* Enable this define to demonstrate tcp keep alive procedure */
#define TCP_KEEPALIVE_ENABLED

/* Keepalive will be sent every 2 seconds */
#define TCP_SERVER_KEEP_ALIVE_INTERVAL      (2)
/* Retry 10 times */
#define TCP_SERVER_KEEP_ALIVE_PROBES        (5)
/* Initiate keepalive check after 5 seconds of silence on a tcp socket */
#define TCP_SERVER_KEEP_ALIVE_TIME          (5)
#define TCP_SILENCE_DELAY                   (30)

/* Thread parameters */
#define WIFI_THREAD_PRIORITY     (10)
#define WIFI_THREAD_STACK_SIZE   (10000)

typedef struct
{
    wiced_bool_t quit;
    wiced_tcp_socket_t socket;
} tcp_server_handle_t;
static tcp_server_handle_t tcp_server_handle;

static wiced_result_t tcp_server_process(  tcp_server_handle_t* server, wiced_packet_t* rx_packet )
{
    char*           request;
    uint16_t        request_length;
    uint16_t        available_data_length;

    wiced_packet_get_data( rx_packet, 0, (uint8_t**) &request, &request_length, &available_data_length );

    if (request_length != available_data_length)
    {
        WPRINT_APP_INFO(("Fragmented packets not supported\n"));
        return WICED_ERROR;
    }

    /* Null terminate the received string */
    request[request_length] = '\x0';
    WPRINT_APP_INFO(("Received data: %s \n", request));


    people = (int) request[0];
    return WICED_SUCCESS;
}

static void tcp_server_thread_main(tcp_server_handle_t* server)
{

    while ( server->quit != WICED_TRUE )
    {
        wiced_packet_t* temp_packet = NULL;

        /* Wait for a connection */
        wiced_result_t result = wiced_tcp_accept( &server->socket );

#ifdef TCP_KEEPALIVE_ENABLED
        result = wiced_tcp_enable_keepalive(&server->socket, TCP_SERVER_KEEP_ALIVE_INTERVAL, TCP_SERVER_KEEP_ALIVE_PROBES, TCP_SERVER_KEEP_ALIVE_TIME );
        if( result != WICED_SUCCESS )
        {
            WPRINT_APP_INFO(("Keep alive initialization failed \n"));
        }
#endif /* TCP_KEEPALIVE_ENABLED */

        if ( result == WICED_SUCCESS )
        {
            /* Receive the query from the TCP client */
            if (wiced_tcp_receive( &server->socket, &temp_packet, WICED_WAIT_FOREVER ) == WICED_SUCCESS)
            {
                /* Process the client request */
                tcp_server_process( server, temp_packet );

                /* Delete the packet, we're done with it */
                wiced_packet_delete( temp_packet );

#ifdef TCP_KEEPALIVE_ENABLED
                WPRINT_APP_INFO(("Waiting for data on a socket\n"));
                /* Check keepalive: wait to see whether the keepalive protocol has commenced */
                /* This is achieved by waiting forever for a packet to be received on the TCP connection*/
                if (wiced_tcp_receive( &server->socket, &temp_packet, WICED_WAIT_FOREVER ) == WICED_SUCCESS)
                {
                    tcp_server_process( server, temp_packet );
                    /* Release the packet, we don't need it any more */
                    wiced_packet_delete( temp_packet );
                }
                else
                {
                    WPRINT_APP_INFO(("Connection has been dropped by networking stack\n\n"));
                }
#endif /* TCP_KEEPALIVE_ENABLED */

            }
            else
            {
                /* Send failed or connection has been lost, close the existing connection and */
                /* get ready to accept the next one */
                wiced_tcp_disconnect( &server->socket );
            }
        }
    }

}

void connectThread(wiced_thread_arg_t arg)
{
    wiced_result_t connectResult;
    wiced_bool_t connected = WICED_FALSE;

    while(connected == WICED_FALSE)
    {
        wiced_network_down(WICED_STA_INTERFACE);
        connectResult = wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);

        if (connectResult == WICED_SUCCESS) {

            WPRINT_APP_INFO (("Success!\n"));

            connected = WICED_TRUE; 
        }
        else
        {
            WPRINT_APP_INFO (("Failed!\n"));
            wiced_rtos_delay_milliseconds( 5000 );
        }
    }


    //TODO handle connection
    if (wiced_tcp_create_socket(&tcp_server_handle.socket, WICED_STA_INTERFACE) != WICED_SUCCESS)
        {
            WPRINT_APP_INFO(("TCP socket creation failed\n"));
        }

        if (wiced_tcp_listen( &tcp_server_handle.socket, TCP_SERVER_LISTEN_PORT ) != WICED_SUCCESS)
        {
            WPRINT_APP_INFO(("TCP server socket initialization failed\n"));
            wiced_tcp_delete_socket(&tcp_server_handle.socket);
            return;
        }
        tcp_server_thread_main(&tcp_server_handle);
}

/* Main application */
void application_start() {

        /* Initialize the WICED device */
	wiced_thread_t cnctThreadHandle;
	wiced_thread_t temperatureThreadHandle;
    temperature = 0;
    wiced_init();   /* Initialize the WICED device */

    /* Initialize and start a new thread */
    wiced_rtos_create_thread(&temperatureThreadHandle, TEMPERATURE_THREAD_PRIORITY, "temperatureThread", temperatureThread, TEMPERATURE_THREAD_STACK_SIZE, NULL);
    wiced_rtos_create_thread(&cnctThreadHandle, WIFI_THREAD_PRIORITY, "connectionThread", connectThread, WIFI_THREAD_STACK_SIZE, NULL);

    // TODO thread to get wifi data
    while(1) {
    	controlTemperature();
    }
}
} // there is no reason for this brace, but it does not compile without it
