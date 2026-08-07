/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     80000000



/* Defines for PWM_MOTOR */
#define PWM_MOTOR_INST                                                     TIMA0
#define PWM_MOTOR_INST_IRQHandler                               TIMA0_IRQHandler
#define PWM_MOTOR_INST_INT_IRQN                                 (TIMA0_INT_IRQn)
#define PWM_MOTOR_INST_CLK_FREQ                                         40000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_MOTOR_C0_PORT                                             GPIOB
#define GPIO_PWM_MOTOR_C0_PIN                                      DL_GPIO_PIN_8
#define GPIO_PWM_MOTOR_C0_IOMUX                                  (IOMUX_PINCM25)
#define GPIO_PWM_MOTOR_C0_IOMUX_FUNC                 IOMUX_PINCM25_PF_TIMA0_CCP0
#define GPIO_PWM_MOTOR_C0_IDX                                DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_MOTOR_C1_PORT                                             GPIOB
#define GPIO_PWM_MOTOR_C1_PIN                                     DL_GPIO_PIN_20
#define GPIO_PWM_MOTOR_C1_IOMUX                                  (IOMUX_PINCM48)
#define GPIO_PWM_MOTOR_C1_IOMUX_FUNC                 IOMUX_PINCM48_PF_TIMA0_CCP1
#define GPIO_PWM_MOTOR_C1_IDX                                DL_TIMER_CC_1_INDEX




/* Defines for QEI_LEFT */
#define QEI_LEFT_INST                                                      TIMG8
#define QEI_LEFT_INST_IRQHandler                                TIMG8_IRQHandler
#define QEI_LEFT_INST_INT_IRQN                                  (TIMG8_INT_IRQn)
/* Pin configuration defines for QEI_LEFT PHA Pin */
#define GPIO_QEI_LEFT_PHA_PORT                                             GPIOA
#define GPIO_QEI_LEFT_PHA_PIN                                     DL_GPIO_PIN_29
#define GPIO_QEI_LEFT_PHA_IOMUX                                   (IOMUX_PINCM4)
#define GPIO_QEI_LEFT_PHA_IOMUX_FUNC                  IOMUX_PINCM4_PF_TIMG8_CCP0
/* Pin configuration defines for QEI_LEFT PHB Pin */
#define GPIO_QEI_LEFT_PHB_PORT                                             GPIOA
#define GPIO_QEI_LEFT_PHB_PIN                                     DL_GPIO_PIN_30
#define GPIO_QEI_LEFT_PHB_IOMUX                                   (IOMUX_PINCM5)
#define GPIO_QEI_LEFT_PHB_IOMUX_FUNC                  IOMUX_PINCM5_PF_TIMG8_CCP1


/* Defines for ENCODER_RIGHT_COUNT */
#define ENCODER_RIGHT_COUNT_INST                                         (TIMG6)
#define ENCODER_RIGHT_COUNT_INST_IRQHandler                        TIMG6_IRQHandler
#define ENCODER_RIGHT_COUNT_INST_INT_IRQN                        (TIMG6_INT_IRQn)
/* GPIO defines for channel 0 */
#define GPIO_ENCODER_RIGHT_COUNT_C0_PORT                                   GPIOB
#define GPIO_ENCODER_RIGHT_COUNT_C0_PIN                            DL_GPIO_PIN_2
#define GPIO_ENCODER_RIGHT_COUNT_C0_IOMUX                         (IOMUX_PINCM15)
#define GPIO_ENCODER_RIGHT_COUNT_C0_IOMUX_FUNC             IOMUX_PINCM15_PF_TIMG6_CCP0




/* Defines for CONTROL_TIMER */
#define CONTROL_TIMER_INST                                               (TIMA1)
#define CONTROL_TIMER_INST_IRQHandler                           TIMA1_IRQHandler
#define CONTROL_TIMER_INST_INT_IRQN                             (TIMA1_INT_IRQn)
#define CONTROL_TIMER_INST_LOAD_VALUE                                    (2499U)



/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                           40000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_0_FBRD_40_MHZ_115200_BAUD                                      (45)
/* Defines for UART_1 */
#define UART_1_INST                                                        UART1
#define UART_1_INST_FREQUENCY                                           40000000
#define UART_1_INST_IRQHandler                                  UART1_IRQHandler
#define UART_1_INST_INT_IRQN                                      UART1_INT_IRQn
#define GPIO_UART_1_RX_PORT                                                GPIOB
#define GPIO_UART_1_TX_PORT                                                GPIOB
#define GPIO_UART_1_RX_PIN                                         DL_GPIO_PIN_7
#define GPIO_UART_1_TX_PIN                                         DL_GPIO_PIN_6
#define GPIO_UART_1_IOMUX_RX                                     (IOMUX_PINCM24)
#define GPIO_UART_1_IOMUX_TX                                     (IOMUX_PINCM23)
#define GPIO_UART_1_IOMUX_RX_FUNC                      IOMUX_PINCM24_PF_UART1_RX
#define GPIO_UART_1_IOMUX_TX_FUNC                      IOMUX_PINCM23_PF_UART1_TX
#define UART_1_BAUD_RATE                                                (115200)
#define UART_1_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_1_FBRD_40_MHZ_115200_BAUD                                      (45)
/* Defines for UART_2 */
#define UART_2_INST                                                        UART2
#define UART_2_INST_FREQUENCY                                           40000000
#define UART_2_INST_IRQHandler                                  UART2_IRQHandler
#define UART_2_INST_INT_IRQN                                      UART2_INT_IRQn
#define GPIO_UART_2_RX_PORT                                                GPIOB
#define GPIO_UART_2_TX_PORT                                                GPIOB
#define GPIO_UART_2_RX_PIN                                        DL_GPIO_PIN_16
#define GPIO_UART_2_TX_PIN                                        DL_GPIO_PIN_17
#define GPIO_UART_2_IOMUX_RX                                     (IOMUX_PINCM33)
#define GPIO_UART_2_IOMUX_TX                                     (IOMUX_PINCM43)
#define GPIO_UART_2_IOMUX_RX_FUNC                      IOMUX_PINCM33_PF_UART2_RX
#define GPIO_UART_2_IOMUX_TX_FUNC                      IOMUX_PINCM43_PF_UART2_TX
#define UART_2_BAUD_RATE                                                (115200)
#define UART_2_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_2_FBRD_40_MHZ_115200_BAUD                                      (45)




/* Defines for SPI_IMU */
#define SPI_IMU_INST                                                       SPI0
#define SPI_IMU_INST_IRQHandler                                 SPI0_IRQHandler
#define SPI_IMU_INST_INT_IRQN                                     SPI0_INT_IRQn
#define GPIO_SPI_IMU_PICO_PORT                                            GPIOA
#define GPIO_SPI_IMU_PICO_PIN                                     DL_GPIO_PIN_5
#define GPIO_SPI_IMU_IOMUX_PICO                                 (IOMUX_PINCM10)
#define GPIO_SPI_IMU_IOMUX_PICO_FUNC                 IOMUX_PINCM10_PF_SPI0_PICO
#define GPIO_SPI_IMU_POCI_PORT                                            GPIOA
#define GPIO_SPI_IMU_POCI_PIN                                     DL_GPIO_PIN_4
#define GPIO_SPI_IMU_IOMUX_POCI                                  (IOMUX_PINCM9)
#define GPIO_SPI_IMU_IOMUX_POCI_FUNC                  IOMUX_PINCM9_PF_SPI0_POCI
/* GPIO configuration for SPI_IMU */
#define GPIO_SPI_IMU_SCLK_PORT                                            GPIOA
#define GPIO_SPI_IMU_SCLK_PIN                                     DL_GPIO_PIN_6
#define GPIO_SPI_IMU_IOMUX_SCLK                                 (IOMUX_PINCM11)
#define GPIO_SPI_IMU_IOMUX_SCLK_FUNC                 IOMUX_PINCM11_PF_SPI0_SCLK



/* Port definition for Pin Group GPIO_LED */
#define GPIO_LED_PORT                                                    (GPIOA)

/* Defines for PIN_LED: GPIOA.0 with pinCMx 1 on package pin 33 */
#define GPIO_LED_PIN_LED_PIN                                     (DL_GPIO_PIN_0)
#define GPIO_LED_PIN_LED_IOMUX                                    (IOMUX_PINCM1)
/* Port definition for Pin Group GPIO_KEY */
#define GPIO_KEY_PORT                                                    (GPIOB)

/* Defines for PIN_KEY_MODE: GPIOB.21 with pinCMx 49 on package pin 20 */
#define GPIO_KEY_PIN_KEY_MODE_PIN                               (DL_GPIO_PIN_21)
#define GPIO_KEY_PIN_KEY_MODE_IOMUX                              (IOMUX_PINCM49)
/* Defines for PIN_KEY_START: GPIOB.15 with pinCMx 32 on package pin 3 */
#define GPIO_KEY_PIN_KEY_START_PIN                              (DL_GPIO_PIN_15)
#define GPIO_KEY_PIN_KEY_START_IOMUX                             (IOMUX_PINCM32)
/* Defines for PIN_KEY_STOP: GPIOB.19 with pinCMx 45 on package pin 16 */
#define GPIO_KEY_PIN_KEY_STOP_PIN                               (DL_GPIO_PIN_19)
#define GPIO_KEY_PIN_KEY_STOP_IOMUX                              (IOMUX_PINCM45)
/* Port definition for Pin Group GPIO_MOTOR_DIR */
#define GPIO_MOTOR_DIR_PORT                                              (GPIOA)

/* Defines for PIN_LEFT_IN1: GPIOA.22 with pinCMx 47 on package pin 18 */
#define GPIO_MOTOR_DIR_PIN_LEFT_IN1_PIN                         (DL_GPIO_PIN_22)
#define GPIO_MOTOR_DIR_PIN_LEFT_IN1_IOMUX                        (IOMUX_PINCM47)
/* Defines for PIN_LEFT_IN2: GPIOA.12 with pinCMx 34 on package pin 5 */
#define GPIO_MOTOR_DIR_PIN_LEFT_IN2_PIN                         (DL_GPIO_PIN_12)
#define GPIO_MOTOR_DIR_PIN_LEFT_IN2_IOMUX                        (IOMUX_PINCM34)
/* Defines for PIN_RIGHT_IN1: GPIOA.17 with pinCMx 39 on package pin 10 */
#define GPIO_MOTOR_DIR_PIN_RIGHT_IN1_PIN                        (DL_GPIO_PIN_17)
#define GPIO_MOTOR_DIR_PIN_RIGHT_IN1_IOMUX                       (IOMUX_PINCM39)
/* Defines for PIN_RIGHT_IN2: GPIOA.15 with pinCMx 37 on package pin 8 */
#define GPIO_MOTOR_DIR_PIN_RIGHT_IN2_PIN                        (DL_GPIO_PIN_15)
#define GPIO_MOTOR_DIR_PIN_RIGHT_IN2_IOMUX                       (IOMUX_PINCM37)
/* Port definition for Pin Group GPIO_ENCODER_RIGHT */
#define GPIO_ENCODER_RIGHT_PORT                                          (GPIOB)

/* Defines for PIN_RIGHT_ENCODER_B: GPIOB.3 with pinCMx 16 on package pin 51 */
#define GPIO_ENCODER_RIGHT_PIN_RIGHT_ENCODER_B_PIN               (DL_GPIO_PIN_3)
#define GPIO_ENCODER_RIGHT_PIN_RIGHT_ENCODER_B_IOMUX             (IOMUX_PINCM16)
/* Port definition for Pin Group GPIO_IMU */
#define GPIO_IMU_PORT                                                    (GPIOA)

/* Defines for PIN_IMU_CS: GPIOA.2 with pinCMx 7 on package pin 42 */
#define GPIO_IMU_PIN_IMU_CS_PIN                                  (DL_GPIO_PIN_2)
#define GPIO_IMU_PIN_IMU_CS_IOMUX                                 (IOMUX_PINCM7)
/* Port definition for Pin Group GPIO_OLED */
#define GPIO_OLED_PORT                                                   (GPIOB)

/* Defines for PIN_OLED_SCL: GPIOB.12 with pinCMx 29 on package pin 64 */
#define GPIO_OLED_PIN_OLED_SCL_PIN                              (DL_GPIO_PIN_12)
#define GPIO_OLED_PIN_OLED_SCL_IOMUX                             (IOMUX_PINCM29)
/* Defines for PIN_OLED_SDA: GPIOB.13 with pinCMx 30 on package pin 1 */
#define GPIO_OLED_PIN_OLED_SDA_PIN                              (DL_GPIO_PIN_13)
#define GPIO_OLED_PIN_OLED_SDA_IOMUX                             (IOMUX_PINCM30)

/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_MOTOR_init(void);
void SYSCFG_DL_QEI_LEFT_init(void);
void SYSCFG_DL_ENCODER_RIGHT_COUNT_init(void);
void SYSCFG_DL_CONTROL_TIMER_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_UART_1_init(void);
void SYSCFG_DL_UART_2_init(void);
void SYSCFG_DL_SPI_IMU_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
