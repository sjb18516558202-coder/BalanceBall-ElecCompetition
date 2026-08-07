#include "odometry.h"

#include <math.h>

#include "app_config.h"

#define CAR_PI_F (3.14159265358979323846f)

volatile Odometry_t g_carOdometry;

void Odometry_Reset(volatile Odometry_t *odom)
{
    if (odom == 0) {
        return;
    }

    odom->left_ticks = 0;
    odom->right_ticks = 0;
    odom->last_left_delta = 0;
    odom->last_right_delta = 0;
    odom->left_distance_m = 0.0f;
    odom->right_distance_m = 0.0f;
    odom->distance_m = 0.0f;
    odom->x_m = 0.0f;
    odom->y_m = 0.0f;
    odom->heading_rad = 0.0f;
}

void Odometry_Update(volatile Odometry_t *odom, int32_t left_delta,
                     int32_t right_delta)
{
    /*
     * 差速里程计计算：
     * 1. 左右 tick -> 左右轮走过的米数。
     * 2. 两轮平均值 -> 车体中心前进距离。
     * 3. 两轮距离差 / 轮距 -> 航向角变化。
     */
    const float meter_per_tick =
        (CAR_PI_F * CAR_WHEEL_DIAMETER_M) / CAR_ENCODER_TICKS_PER_REV;
    const float left_m = (float) left_delta * meter_per_tick;
    const float right_m = (float) right_delta * meter_per_tick;
    const float center_m = (left_m + right_m) * 0.5f;
    const float delta_heading = (right_m - left_m) / CAR_WHEEL_TRACK_M;
    float mid_heading;

    if (odom == 0) {
        return;
    }

    /* 使用中点航向积分，比直接用更新前/更新后的航向更平滑。 */
    mid_heading = odom->heading_rad + (delta_heading * 0.5f);

    odom->left_ticks += left_delta;
    odom->right_ticks += right_delta;
    odom->last_left_delta = left_delta;
    odom->last_right_delta = right_delta;
    odom->left_distance_m += left_m;
    odom->right_distance_m += right_m;
    odom->distance_m += center_m;
    odom->x_m += center_m * cosf(mid_heading);
    odom->y_m += center_m * sinf(mid_heading);
    odom->heading_rad += delta_heading;
}
