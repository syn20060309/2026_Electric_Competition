#include "Filter.h"

#define FILTER_DT_SECONDS (0.060f)

typedef struct {
    float angle;
    float bias;
    float covariance[2][2];
} KalmanState;

static KalmanState x_state = {
    .covariance = {{1.0f, 0.0f}, {0.0f, 1.0f}},
};
static KalmanState y_state = {
    .covariance = {{1.0f, 0.0f}, {0.0f, 1.0f}},
};

static float kalman_filter(KalmanState *state, float accel, float gyro)
{
    const float q_angle = 0.001f;
    const float q_gyro = 0.003f;
    const float r_angle = 0.5f;
    float p_dot[4];
    float innovation;
    float denominator;
    float gain_0;
    float gain_1;
    float p_ct_0;
    float p_ct_1;
    float temp_0;
    float temp_1;

    state->angle += (gyro - state->bias) * FILTER_DT_SECONDS;

    p_dot[0] = q_angle - state->covariance[0][1] -
        state->covariance[1][0];
    p_dot[1] = -state->covariance[1][1];
    p_dot[2] = -state->covariance[1][1];
    p_dot[3] = q_gyro;

    state->covariance[0][0] += p_dot[0] * FILTER_DT_SECONDS;
    state->covariance[0][1] += p_dot[1] * FILTER_DT_SECONDS;
    state->covariance[1][0] += p_dot[2] * FILTER_DT_SECONDS;
    state->covariance[1][1] += p_dot[3] * FILTER_DT_SECONDS;

    innovation = accel - state->angle;
    p_ct_0 = state->covariance[0][0];
    p_ct_1 = state->covariance[1][0];
    denominator = r_angle + p_ct_0;
    gain_0 = p_ct_0 / denominator;
    gain_1 = p_ct_1 / denominator;
    temp_0 = p_ct_0;
    temp_1 = state->covariance[0][1];

    state->covariance[0][0] -= gain_0 * temp_0;
    state->covariance[0][1] -= gain_0 * temp_1;
    state->covariance[1][0] -= gain_1 * temp_0;
    state->covariance[1][1] -= gain_1 * temp_1;

    state->angle += gain_0 * innovation;
    state->bias += gain_1 * innovation;
    return state->angle;
}

float Kalman_Filter_x(float accel, float gyro)
{
    return kalman_filter(&x_state, accel, gyro);
}

float Kalman_Filter_y(float accel, float gyro)
{
    return kalman_filter(&y_state, accel, gyro);
}

float Complementary_Filter_x(float angle_measurement, float gyro)
{
    static float angle;
    const float measurement_weight = 0.02f;

    angle = measurement_weight * angle_measurement +
        (1.0f - measurement_weight) *
            (angle + gyro * FILTER_DT_SECONDS);
    return angle;
}

float Complementary_Filter_y(float angle_measurement, float gyro)
{
    static float angle;
    const float measurement_weight = 0.02f;

    angle = measurement_weight * angle_measurement +
        (1.0f - measurement_weight) *
            (angle + gyro * FILTER_DT_SECONDS);
    return angle;
}
