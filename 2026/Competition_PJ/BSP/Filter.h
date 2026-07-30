#ifndef FILTER_H
#define FILTER_H

float Kalman_Filter_x(float accel, float gyro);
float Complementary_Filter_x(float angle_measurement, float gyro);
float Kalman_Filter_y(float accel, float gyro);
float Complementary_Filter_y(float angle_measurement, float gyro);

#endif
