#ifndef TEMPMETER_H
#define TEMPMETER_H

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


#endif
