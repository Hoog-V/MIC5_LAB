#include <MKL25Z4.h>
#include <math.h>

#include "FreeRTOS.h"
#include "task.h"

#include "mma8451.h"

#if (CLOCK_SETUP != 1)
  #warning This driver does not work as designed
#endif

#define M_PI (3.14159265f)

//extern TaskHandle_t xMma8451TaskHandler;

int16_t x_out_14_bit = 0,
        y_out_14_bit = 0,
        z_out_14_bit = 0;
float x_out_g = 0,
      y_out_g = 0,
      z_out_g = 0;
float roll = 0.0,
      pitch = 0.0;

float dt = 0;

// Local function prototypes

static void delay_us(uint32_t d)
{

#if (CLOCK_SETUP != 1)
#warning This delay function does not work as designed
#endif

    volatile uint32_t t;

    for(t=4*d; t>0; t--)
    {
        __asm("nop");
        __asm("nop");
    }
}

bool mma8451_init(void)
{
    i2c0_init();

    uint8_t value;

    // Read the WHO_AM_I_REG register
    if(!(i2c0_read_byte(MMA8451_ADDRESS, WHO_AM_I_REG, &value)))
    {
        return false;
    }

    // Check for device
    if(value != WHO_AM_I_VAL)
    {
        return false;
    }

    // Reset all register to POR values
    if(!(i2c0_write_byte(MMA8451_ADDRESS, CTRL_REG2, 0x40)))
    {
        return false;
    }

    // Wait for RST bit to clear
    value = 0;
    do
    {
        // Read the register
        if(!(i2c0_read_byte(MMA8451_ADDRESS, CTRL_REG2, &value)))
        {
            return false;
        }
    }
    while((value & 0x40) == 0);

    // +/-2g range -> 1g = 16384/4 = 4096 counts
    if(!(i2c0_write_byte(MMA8451_ADDRESS, XYZ_DATA_CFG_REG, 0x00)))
    {
        return false;
    }

    // High Resolution mode
    if(!(i2c0_write_byte(MMA8451_ADDRESS, CTRL_REG2, 0x02)))
    {
        return false;
    }

    // ODR = 100 Hz, Reduced noise, Active mode
    if(!(i2c0_write_byte(MMA8451_ADDRESS, CTRL_REG1, 0x1D)))
    {
        return false;
    }

	// Configure the PTA14, connected to the INT1 of the MMA8451Q, for
    // interrupts on falling edges
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
	PORTA->PCR[14] = PORT_PCR_ISF_MASK | PORT_PCR_MUX(0x1) | PORT_PCR_IRQC(0xA);

	// Enable interrupts
    NVIC_SetPriority(PORTA_IRQn, 64);
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    NVIC_EnableIRQ(PORTA_IRQn);

    // Make sure the time between I2C transfers is > t_BUF (1.3 us)
    delay_us(10);

    return true;
}

bool mma8451_calibrate(void)
{
    uint8_t value = 0;

    // Wait for data
    do
    {
        // Read the status register
        if(!(i2c0_read_byte(MMA8451_ADDRESS, STATUS_REG, &value)))
        {
            return false;
        }
    }
    while((value & 0x08) == 0);

    // Read values
    mma8451_read();

    // Calculate offsets as described in AN4069
    signed char x_offset = -1 * (x_out_14_bit >> 3);
    signed char y_offset = -1 * (y_out_14_bit >> 3);
    signed char z_offset = COUNTS_PER_G - (z_out_14_bit >> 3);

    // Standby mode
    if(!(i2c0_write_byte(MMA8451_ADDRESS, CTRL_REG1, 0x00)))
    {
        return false;
    }

    // Offsets
    if(!(i2c0_write_byte(MMA8451_ADDRESS, OFF_X_REG, x_offset)))
    {
        return false;
    }

    if(!(i2c0_write_byte(MMA8451_ADDRESS, OFF_Y_REG, y_offset)))
    {
        return false;
    }

    if(!(i2c0_write_byte(MMA8451_ADDRESS, OFF_Z_REG, z_offset)))
    {
        return false;
    }

    // Push-pull, active low interrupt
    if(!(i2c0_write_byte(MMA8451_ADDRESS, CTRL_REG3, 0x00)))
    {
        return false;
    }

    // Enable DRDY interrupt
    if(!(i2c0_write_byte(MMA8451_ADDRESS, CTRL_REG4, 0x01)))
    {
        return false;
    }

    // DRDY interrupt routed to INT1 - PTA14
    if(!(i2c0_write_byte(MMA8451_ADDRESS, CTRL_REG5, 0x01)))
    {
        return false;
    }

    // ODR = 100 Hz, Reduced noise, Active mode
    // Notice that delta dt is fixed, because it is set by ODR and DRDY
    // interrupts are enabled. This doesn't require a timer to measure delta t.
    dt = 0.010f;
    if(!(i2c0_write_byte(MMA8451_ADDRESS, CTRL_REG1, 0x1D)))
    {
        return false;
    }

    // Make sure the time between I2C transfers is > t_BUF (1.3 us)
    delay_us(10);

    return true;
}

void mma8451_read(void)
{
	int i;
	uint8_t data[6];

    i2c0_start();

	if(!(i2c0_read_setup(MMA8451_ADDRESS, OUT_X_MSB_REG)))
    {
        mma8451_init();
        return;
    }

	// Read five bytes in repeated mode
	for(i=0; i<5; i++)
    {
        if(!(i2c0_repeated_read(false, &data[i])))
        {
            mma8451_init();
            return;
        }
	}

	// Read last byte ending repeated mode
	if(!(i2c0_repeated_read(true, &data[i])))
    {
        mma8451_init();
        return;
    }

    // Combine the read bytes to 16-bit values
    x_out_14_bit = (int16_t)((data[0]<<8) | data[1]);
    y_out_14_bit = (int16_t)((data[2]<<8) | data[3]);
    z_out_14_bit = (int16_t)((data[4]<<8) | data[5]);

    // Compute 14-bit signed result
    x_out_14_bit >>= 2;
    y_out_14_bit >>= 2;
    z_out_14_bit >>= 2;

    // Compute result in g's
    x_out_g = (float)x_out_14_bit / COUNTS_PER_G;
    y_out_g = (float)y_out_14_bit / COUNTS_PER_G;
    z_out_g = (float)z_out_14_bit / COUNTS_PER_G;
}

void mma8451_rollpitch(void)
{
	roll = atan2(y_out_g, z_out_g)*180/M_PI;
	pitch = atan2(x_out_g, sqrt(y_out_g*y_out_g + z_out_g*z_out_g))*180/M_PI;
}

void PORTA_IRQHandler(void)
{
    NVIC_ClearPendingIRQ(PORTA_IRQn);

    // Clear the interrupt
    PORTA->PCR[14] |= PORT_PCR_ISF_MASK;

    // Notify the task
//    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//    vTaskNotifyGiveFromISR(xMma8451TaskHandler, &xHigherPriorityTaskWoken);
//    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
