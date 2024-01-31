#ifndef MMA8451_H
#define MMA8451_H

#include "i2c0.h"

#define MMA8451_ADDRESS  (0x3A)

#define STATUS_REG       (0x00)

#define OUT_X_MSB_REG    (0x01)
#define OUT_X_LSB_REG    (0x02)
#define OUT_Y_MSB_REG    (0x03)
#define OUT_Y_LSB_REG    (0x04)
#define OUT_Z_MSB_REG    (0x05)
#define OUT_Z_LSB_REG    (0x06)

#define XYZ_DATA_CFG_REG (0x0E)
#define WHO_AM_I_REG     (0x0D)

#define CTRL_REG1        (0x2A)
#define CTRL_REG2        (0x2B)
#define CTRL_REG3        (0x2C)
#define CTRL_REG4        (0x2D)
#define CTRL_REG5        (0x2E)

#define OFF_X_REG        (0x2F)
#define OFF_Y_REG        (0x30)
#define OFF_Z_REG        (0x31)

#define WHO_AM_I_VAL     (0x1A)

#define COUNTS_PER_G     (4096)

extern int16_t x_out_14_bit, y_out_14_bit, z_out_14_bit;
extern float x_out_g, y_out_g, z_out_g;
extern float roll, pitch;
extern float dt;
extern bool mma8451_ready_flag;

bool mma8451_init(void);
bool mma8451_calibrate(void);
void mma8451_read(void);
void mma8451_rollpitch(void);

#endif
