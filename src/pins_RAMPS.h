/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

/**
 * Arduino Mega with RAMPS v1.4 (or v1.3) pin assignments
 * ATmega2560, ATmega1280
 *
 * Applies to the following boards:
 *
 *  RAMPS_14_EFB (Hotend, Fan, Bed)
 *  RAMPS_14_EEB (Hotend0, Hotend1, Bed)
 *  RAMPS_14_EFF (Hotend, Fan0, Fan1)
 *  RAMPS_14_EEF (Hotend0, Hotend1, Fan)
 *  RAMPS_14_SF  (Spindle, Controller Fan)
 *
 *  RAMPS_13_EFB (Hotend, Fan, Bed)
 *  RAMPS_13_EEB (Hotend0, Hotend1, Bed)
 *  RAMPS_13_EFF (Hotend, Fan0, Fan1)
 *  RAMPS_13_EEF (Hotend0, Hotend1, Fan)
 *  RAMPS_13_SF  (Spindle, Controller Fan)
 *
 *  Other pins_MYBOARD.h files may override these defaults
 *
 *  Differences between
 *  RAMPS_13 | RAMPS_14
 *         7 | 11
 */

/*
#if ENABLED(AZSMZ_12864) && DISABLED(ALLOW_SAM3X8E)
  #error "No pins defined for RAMPS with AZSMZ_12864."
#endif

#include "env_validate.h"
*/

// Custom flags and defines for the build
//#define BOARD_CUSTOM_BUILD_FLAGS -D__FOO__

#ifndef BOARD_INFO_NAME
  #define BOARD_INFO_NAME "RAMPS 1.4"
#endif

//
// Servos
//
#ifndef SERVO0_PIN
  #ifdef IS_RAMPS_13
    #define SERVO0_PIN                         7
  #else
    #define SERVO0_PIN                        11
  #endif
#endif
#ifndef SERVO1_PIN
  #define SERVO1_PIN                           6
#endif
#ifndef SERVO2_PIN
  #define SERVO2_PIN                           5
#endif
#ifndef SERVO3_PIN
  #define SERVO3_PIN                           4
#endif

//
// Foam Cutter requirements
//

/*
#if ENABLED(FOAMCUTTER_XYUV)
  #ifndef MOSFET_C_PIN
    #define MOSFET_C_PIN                      -1
  #endif
  #if HAS_CUTTER && !defined(SPINDLE_LASER_ENA_PIN) && NUM_SERVOS < 2
    #define SPINDLE_LASER_PWM_PIN              8  // Hardware PWM
  #endif
  #ifndef Z_MIN_PIN
    #define Z_MIN_PIN                         -1
  #endif
  #ifndef Z_MAX_PIN
    #define Z_MAX_PIN                         -1
  #endif
  #ifndef I_STOP_PIN
    #define I_STOP_PIN                        18  // Z-
  #endif
  #ifndef J_STOP_PIN
    #define J_STOP_PIN                        19  // Z+
  #endif
#endif
*/

//
// Limit Switches
//
#ifndef X_STOP_PIN
  #ifndef X_MIN_PIN
    #define X_MIN_PIN                          3  // X-
  #endif
  #ifndef X_MAX_PIN
    #define X_MAX_PIN                          2  // X+
  #endif
#endif
#ifndef Y_STOP_PIN
  #ifndef Y_MIN_PIN
    #define Y_MIN_PIN                         14  // Y-
  #endif
  #ifndef Y_MAX_PIN
    #define Y_MAX_PIN                         15  // Y+
  #endif
#endif
#ifndef Z_STOP_PIN
  #ifndef Z_MIN_PIN
    #define Z_MIN_PIN                         18  // Z-
  #endif
  #ifndef Z_MAX_PIN
    #define Z_MAX_PIN                         19  // Z+
  #endif
#endif

//
// Z Probe (when not Z_MIN_PIN)
//
#ifndef Z_MIN_PROBE_PIN
  #define Z_MIN_PROBE_PIN                     32
#endif

//
// Steppers
//
#define X_STEP_PIN                            54  // (A0)
#define X_DIR_PIN                             55  // (A1)
#define X_ENABLE_PIN                          38
#ifndef X_CS_PIN
  #define X_CS_PIN                       AUX3_06
#endif

#define Y_STEP_PIN                            60
#define Y_DIR_PIN                             61
#define Y_ENABLE_PIN                          56  // (A2)
#ifndef Y_CS_PIN
  #define Y_CS_PIN                       AUX3_02
#endif

#ifndef Z_STEP_PIN
  #define Z_STEP_PIN                          46
#endif
#ifndef Z_DIR_PIN
  #define Z_DIR_PIN                           48
#endif
#ifndef Z_ENABLE_PIN
  #define Z_ENABLE_PIN                        62
#endif
#ifndef Z_CS_PIN
  #define Z_CS_PIN                       AUX2_06
#endif

#ifndef E0_STEP_PIN
  #define E0_STEP_PIN                         26
#endif
#ifndef E0_DIR_PIN
  #define E0_DIR_PIN                          28
#endif
#ifndef E0_ENABLE_PIN
  #define E0_ENABLE_PIN                       24
#endif
#ifndef E0_CS_PIN
  #define E0_CS_PIN                      AUX2_08
#endif

#ifndef E1_STEP_PIN
  #define E1_STEP_PIN                         36
#endif
#ifndef E1_DIR_PIN
  #define E1_DIR_PIN                          34
#endif
#ifndef E1_ENABLE_PIN
  #define E1_ENABLE_PIN                       30
#endif
#ifndef E1_CS_PIN
  #define E1_CS_PIN                      AUX2_07
#endif

//
// Temperature Sensors
//
#ifndef TEMP_0_PIN
  #define TEMP_0_PIN                          13  // Analog Input
#endif
#ifndef TEMP_1_PIN
  #define TEMP_1_PIN                          15  // Analog Input
#endif
#ifndef TEMP_BED_PIN
  #define TEMP_BED_PIN                        14  // Analog Input
#endif

//
// SPI for MAX Thermocouple
//
#ifndef TEMP_0_CS_PIN
  #define TEMP_0_CS_PIN                  AUX2_09  // Don't use 53 if using Display/SD card (SDSS) or 49 (SD_DETECT_PIN)
#endif

//
// Heaters / Fans
//
#ifndef MOSFET_A_PIN
  #define MOSFET_A_PIN                        10
#endif
#ifndef MOSFET_B_PIN
  #define MOSFET_B_PIN                         9
#endif
#ifndef MOSFET_C_PIN
  #define MOSFET_C_PIN                         8
#endif
#ifndef MOSFET_D_PIN
  #define MOSFET_D_PIN                        -1
#endif

//
// AUX1    5V  GND D2  D1
//          2   4   6   8
//          1   3   5   7
//         5V  GND A3  A4
//
#define AUX1_05                               57  // (A3)
#define AUX1_06                                2
#define AUX1_07                               58  // (A4)
#define AUX1_08                                1

//
// AUX2    GND A9 D40 D42 A11
//          2   4   6   8  10
//          1   3   5   7   9
//         VCC A5 A10 D44 A12
//
#define AUX2_03                               59  // (A5)
#define AUX2_04                               63  // (A9)
#define AUX2_05                               64  // (A10)
#define AUX2_06                               40
#define AUX2_07                               44
#define AUX2_08                               42
#define AUX2_09                               66  // (A12)
#define AUX2_10                               65  // (A11)

//
// AUX3    GND D52 D50 5V
//          7   5   3   1
//          8   6   4   2
//         NC  D53 D51 D49
//
#define AUX3_02                               49
#define AUX3_03                               50
#define AUX3_04                               51
#define AUX3_05                               52
#define AUX3_06                               53

//
// AUX4    5V GND D32 D47 D45 D43 D41 D39 D37 D35 D33 D31 D29 D27 D25 D23 D17 D16
//
#define AUX4_03                               32
#define AUX4_04                               47
#define AUX4_05                               45
#define AUX4_06                               43
#define AUX4_07                               41
#define AUX4_08                               39
#define AUX4_09                               37
#define AUX4_10                               35
#define AUX4_11                               33
#define AUX4_12                               31
#define AUX4_13                               29
#define AUX4_14                               27
#define AUX4_15                               25
#define AUX4_16                               23
#define AUX4_17                               17
#define AUX4_18                               16

/**
 * LCD adapters come in different variants. The socket keys can be
 * on either side, and may be backwards on some boards / displays.
 */
#ifndef EXP1_08_PIN
  #define EXP1_01_PIN                    AUX4_09  // 37
  #define EXP1_02_PIN                    AUX4_10  // 35
  #define EXP1_03_PIN                    AUX4_17  // 17
  #define EXP1_04_PIN                    AUX4_18  // 16
  #define EXP1_05_PIN                    AUX4_16  // 23
  #define EXP1_06_PIN                    AUX4_15  // 25
  #define EXP1_07_PIN                    AUX4_14  // 27
  #define EXP1_08_PIN                    AUX4_13  // 29

  #define EXP2_01_PIN                    AUX3_03  // 50 (MISO)
  #define EXP2_02_PIN                    AUX3_05  // 52
  #define EXP2_04_PIN                    AUX3_06  // 53
  #define EXP2_06_PIN                    AUX3_04  // 51
  #define EXP2_07_PIN                    AUX3_02  // 49

  #define KILL_BTN_PIN                   AUX4_07

  #define LCD_PINS_CS                EXP1_04_PIN  // 7 CS make sure for zonestar zm3e4!
  #define LCD_PINS_MOSI              EXP1_03_PIN  // 6 DATA make sure for zonestar zm3e4!
  #define LCD_PINS_SCK               EXP1_05_PIN  // 8 SCK make sure for zonestar zm3e4!
  #define BEEPER_PIN                 EXP1_01_PIN
  #define BTN_EN1                    EXP2_05_PIN
  #define BTN_EN2                    EXP2_03_PIN
  #define BTN_ENC                    EXP1_02_PIN

#endif