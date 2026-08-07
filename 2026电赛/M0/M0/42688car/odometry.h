#ifndef ODOMETRY_H_
#define ODOMETRY_H_

#include <stdint.h>

/*
 * Odometry_t 是全车公开的里程计变量类型。
 * g_carOdometry 会在控制周期中根据左右轮编码器增量持续更新。
 */
typedef struct {
    /* 左右轮累计编码器 tick。 */
    int64_t left_ticks;
    int64_t right_ticks;

    /* 最近一次更新使用的左右轮增量 tick。 */
    int32_t last_left_delta;
    int32_t last_right_delta;

    /* 左右轮累计行驶距离，单位：米。 */
    float left_distance_m;
    float right_distance_m;

    /* 车体中心累计路程，单位：米。 */
    float distance_m;

    /* 平面坐标，单位：米；heading_rad 为车头朝向弧度。 */
    float x_m;
    float y_m;
    float heading_rad;
} Odometry_t;

/* 全局里程计变量，外部可直接读取，不建议外部随意写入。 */
extern volatile Odometry_t g_carOdometry;

/* 清零里程计。 */
void Odometry_Reset(volatile Odometry_t *odom);

/* 根据一个控制周期的左右轮增量更新里程计。 */
void Odometry_Update(volatile Odometry_t *odom, int32_t left_delta,
                     int32_t right_delta);

#endif /* ODOMETRY_H_ */
