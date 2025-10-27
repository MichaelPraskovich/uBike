/*
 * Universal Bike Controller
 * Copyright (C) 2022-2023, Greco Engineering Solutions LLC
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bikeControl.h"

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "asciiModbus.h"

LOG_MODULE_REGISTER ( bike );
static send_msg_callback_t sendMsgCbFunc = NULL;

// Global variables
static cmd_msg_data_t CFG_CMD_1 = { RES_NODE, WRITE_HOLD, 0x0007, 0x000F };
static cmd_msg_data_t CFG_CMD_2 = { RES_NODE, WRITE_HOLD, 0x0008, 0x00BE };
static cmd_msg_data_t CFG_CMD_3 = { INC_NODE, WRITE_HOLD, 0x0006, 0x0000 };
static cmd_msg_data_t CFG_CMD_4 = { INC_NODE, WRITE_HOLD, 0x0007, 0x003C };
static cmd_msg_data_t CFG_CMD_5 = { INC_NODE, WRITE_HOLD, 0x0009, 0x0014 };
static cmd_msg_data_t CFG_CMD_6 = { INC_NODE, WRITE_HOLD, 0x0008, 0x003C };
static cmd_msg_data_t RPM_REQ = { RPM_NODE, READ_MULTI_HOLD, 0x0002, 0x0000 };
static cmd_msg_data_t INC_REQ = { INC_NODE, READ_MULTI_HOLD, 0x0002, 0x0000 };
static cmd_msg_data_t SET_RES = { RES_NODE, WRITE_HOLD, 0x0005, INIT_RES };
static cmd_msg_data_t SET_INC = { INC_NODE, WRITE_HOLD, 0X0001, INIT_INC };
static cmd_msg_data_t ZERO_RPM
    = { INC_NODE, WRITE_HOLD, 0X0004, 0x0000 };  // TODO - Send on stop

// Control parameters
static uint16_t act_rpm = 0;
static uint16_t act_inc = INIT_INC;
static uint16_t disp_res = 1;
static bool firstRead = false;

void setSendMsgCb ( send_msg_callback_t func )
{
    sendMsgCbFunc = func;
}

void sendWithRetries ( cmd_msg_data_t cmd, uint16_t retries, int32_t delay_ms )
{
    int res;
    for ( int i = 0; i < retries + 1; i++ ) {
        res = sendMsgCbFunc ( cmd );
        if ( !res ) {
            return;
        } else {
            LOG_ERR ( "Failed to send on attempt %d out of %u.  Returned: %d",
                      i + 1,
                      retries + 1,
                      res );
        }
        k_msleep ( delay_ms );
    }
}

// Evalute user inputs
buttonStatus_t evaluateButton ( int up, int down )
{
    if ( up && !down ) {
        return INCREASE;
    } else if ( down && !up ) {
        return DECREASE;
    }
    return NOTHING;
}

static uint16_t calc_res()
{
    int16_t res = 15;

    // +1 display resistance is worth 5 counts
    res += 5 * ( disp_res - 1 );

    // 1% grade is worth 4 counts
    res += 4 * ( ( act_inc - 20 ) / 2 );
    
    // Clip
    if ( res < 15 ) {
        return 15;
    } else if ( res > 190 ) {
        return 190;
    }
    return res;
}

void adjustIncline ( buttonStatus_t adj )
{
    if ( ( adj == INCREASE ) && ( SET_INC.value < 60 ) ) {
        SET_INC.value++;
        LOG_INF ( "Increasing incline to: %d", SET_INC.value );
    } else if ( ( adj == DECREASE ) && ( SET_INC.value > 0 ) ) {
        SET_INC.value--;
        LOG_INF ( "Decreasing incline to: %d", SET_INC.value );
    }
}

void adjustResistance ( buttonStatus_t adj )
{
    if ( ( adj == INCREASE ) && ( disp_res < 22 ) ) {
        disp_res++;
        LOG_INF ( "Increasing resistance to: %d", disp_res );
    } else if ( ( adj == DECREASE ) && ( disp_res > 1 ) ) {
        disp_res--;
        LOG_INF ( "Decreasing resistance to: %d", disp_res );
    }
}

static void setIncline ( uint16_t tgt )
{
    LOG_INF ( "Setting incline to: %u", tgt );
    if ( tgt > 60 ) {
        SET_INC.value = 60;
    } else {
        SET_INC.value = tgt;
    }
}

static void setResistance ( uint16_t tgt )
{
    LOG_INF ( "Setting resistance to: %u", tgt );
    if ( tgt > 22 ) {
        SET_INC.value = 22;
    } else if ( tgt <= 1 ) {
        SET_INC.value = 1;
    } else {
        SET_INC.value = tgt;
    }
}

// Update bike targets
void updateBikeTgts ( const bike_tgts_t tgts )
{
    // Incline
    if ( tgts.incline != 0x7FFF ) {
        if ( tgts.incline >= 2000 ) {
            setIncline ( 60 );
        } else if ( tgts.incline <= -1000 ) {
            setIncline ( 0 );
        } else {
            uint16_t roundUp = ( tgts.incline + 1000 ) % 50 > 25 ? 1 : 0;
            setIncline ( ( tgts.incline + 1000 ) / 50 + roundUp );
        }
    }

    // Resistance
    if ( tgts.resistance != 0xFF ) {
        if ( tgts.resistance >= 200 ) {
            setResistance ( 22 );
        } else if ( tgts.resistance == 0 ) {
            setResistance ( 1 );
        } else {
            uint16_t roundUp = ( tgts.resistance * 21 ) % 200 > 25 ? 1 : 0;
            setResistance ( 1 + ( tgts.resistance * 21 ) / 200 + roundUp );
        }
    }
}

void initBike()
{
    if ( !sendMsgCbFunc ) {
        LOG_ERR ( "Send message callback not registered!" );
        return;
    }
    k_msleep ( CFG_DELAY_MS );
    sendWithRetries ( CFG_CMD_1, 50, 200 );
    sendWithRetries ( CFG_CMD_2, 5, 50 );
    sendWithRetries ( CFG_CMD_3, 5, 50 );
    sendWithRetries ( CFG_CMD_4, 5, 50 );
    sendWithRetries ( CFG_CMD_5, 5, 50 );
    sendWithRetries ( CFG_CMD_6, 5, 50 );
    sendWithRetries ( SET_RES, 5, 50 );
    sendWithRetries ( INC_REQ, 5, 50 );
}

int new_msg ( uint8_t *buff, size_t len )
{
    convert_8N1to_7N2 ( buff, len );
    // Remove start
    if ( buff [0] != ':' ) {
        return -1;
    }
    buff++;
    // Remove termination characters
    if ( ( buff [len - 3] == '\r' ) && ( buff [len - 2] == '\n' ) ) {
        return -2;
    }
    len -= 3;

    // Check Checksum
    if ( calc_checksum ( buff, len ) ) {
        return -3;
    }

    // Get params
    uint8_t nodeId = ascii_to_int_2 ( buff );
    uint8_t funcCode = ascii_to_int_2 ( buff + 2 );
    if ( funcCode == WRITE_HOLD ) {
        // Assume a succesful write
        return 0;
    } else if ( funcCode == READ_MULTI_HOLD ) {
        //  This is a reply, we need to extract data
        uint16_t value = ascii_to_int_4 ( buff + 10 );
        if ( nodeId == RPM_NODE ) {
            act_rpm = value;
            return 0;
        } else if ( nodeId == INC_NODE ) {
            if ( !firstRead ) {
                SET_INC.value = value;
                firstRead = true;
            }
            act_inc = value;
            return 0;
        } else {
            return -4;  // Unhandled node id
        }
    }

    return -5;  // Unhandled func code
}

static float power ( float base, int pwr )
{
    return pwr == 0 ? 1.0 : base * power ( base, pwr - 1 );
}

// // Power curve fitting
// #define A -8.06357866e+06
// #define B -2.45457703e+05
// #define C 1.49961556e+01
// #define D -6.48778450e-02
// #define E 1.09741619e-04
// #define F 3.16050229e-09
// #define G -1.00397536e-07
// #define H 1.01525992e-08
// #define I -2.94388976e-10
// #define J 3.48657332e-12
// #define K -1.97940519e-14
// #define L 4.09271382e-17
// #define M 5.62850051e-02

// static uint16_t calc_watts()
// {
//     // If no motion return 0
//     if ( act_rpm == 0 ) {
//         return 0;
//     }

//     // Caculate wats in N*m
//     float pwr = ( A + B * act_rpm + C * power ( act_rpm, 2 )
//                   + D * power ( act_rpm, 3 ) + E * power ( act_rpm, 4 )
//                   + F * power ( act_rpm, 5 ) )
//                     * ( G + H * SET_RES.value + I * power ( SET_RES.value, 2 )
//                         + J * power ( SET_RES.value, 3 )
//                         + K * power ( SET_RES.value, 4 )
//                         + L * power ( SET_RES.value, 5 ) )
//                 + M;

//     // Calculate watts from torque
//     pwr *= act_rpm;

//     // If motion don't return less than one
//     if ( pwr < 1.0 ) {
//         return 1;
//     }
//     return pwr;
// }

// The following "Phyics-Based" model is based on analysis of coast-down data
// collected from an s22i bike. It estimates the power required to maintain a
// given flywheel speed (in RPM) at a given eddy-current brake level (1..22).
// The model accounts for both eddy-current drag and Coulomb friction losses,   
// and can be adapted to either crank or flywheel speed sensors.
//
// Coast Down data used:
// L=1.1
// time_s, rpm
// 0, 60
// 0.5, 59
// 1.1, 58
// 1.5, 56
// 2.1, 53
// 3.1, 49
// 3.5, 46
// 4.5, 42
// 5.5, 38
// 6.5, 34
// 8, 30
// 9.5, 25
// 11.5, 0

// L=1.2
// time_s, rpm
// 0, 59
// 1, 58
// 1.5, 55
// 2, 52
// 2.5, 49
// 3.5, 45
// 4.5, 41
// 5.5, 37
// 6.5, 33
// 7.5, 29
// 9.5, 25
// 11.5, 0

// L=4.1
// time_s,rpm
// 0, 60
// 0.5, 59
// 1, 58
// 2, 57
// 2.5, 54
// 3, 51
// 4, 48
// 4.5, 44
// 5.5, 40
// 6.5, 36
// 7.5, 32
// 9, 27
// 11, 23
// 13, 0

// L=4.2
// time_s, rpm
// 0, 60
// 0.5, 59
// 1.5, 57
// 2, 55
// 2.5, 51
// 3.5, 47
// 4, 44
// 5, 40
// 6, 36
// 7, 31
// 8.5, 27
// 10.5, 22
// 12.5, 0

// L=10.1
// time_s, rpm
// 0, 61
// 0.3, 60
// 0.8, 59
// 1.3, 56
// 2.3, 51
// 2.8, 44
// 4.3, 36
// 6.3, 28
// 7.8, 0

// L=10.2
// time_s, rpm
// 0, 60
// 0.1, 59
// 1, 57
// 2, 54
// 2.5, 49
// 3.5, 42
// 5, 34
// 7, 26
// 9, 0

// L=13.1
// time_s, rpm
// 0, 60
// 1, 58
// 1.5, 52
// 3, 43
// 5, 32
// 7, 0

// L=13.2
// time_s, rpm
// 0, 59
// 0.2, 57
// 0.7, 54
// 1.7, 47
// 3.2, 38
// 5.2, 0

// L=16.1
// time_s, rpm
// 0, 61
// 0.5, 59
// 1.5, 52
// 3, 41
// 5, 0

// L=16.2
// time_s, rpm
// 0, 59
// 0.1, 58
// 0.5, 56
// 1.5, 50
// 3, 40
// 5, 0

// L=18.1
// time_s, rpm
// 0, 61
// 0.9, 59
// 1.9, 52
// 3.4, 0

// L=18.2
// time_s, rpm
// 0, 61
// 0.1, 59
// 1.1, 51
// 3.1, 0

// L=20.1
// time_s, rpm
// 0, 60
// 0.5, 58
// 1.5, 48
// 3.5, 0

// L=20.2
// time_s, rpm
// 0, 60
// 0.7, 56
// 2.3, 46
// 3.7, 0

// L=22.1
// time_s, rpm
// 0, 60
// 0.6, 56
// 2.6, 0

// L=22.2
// time_s, rpm
// 0, 59
// 0.5, 55
// 2, 0

/*** BIKE CONSTANTS ***/
#define SENSOR_IS_CRANK            1          // 1 = rpm is crank cadence; 0 = flywheel
#define R_FLYWHEEL_PER_CRANK       5.317f
#define DRIVETRAIN_ETA             0.95f
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline double rpm_to_rads(double rpm){ return rpm * (2.0 * M_PI / 60.0); }

/*** Eddy-current k(L): log-linear interpolation over anchors ***/
static const int    K_LVLS[] = {  1,   4,   10,   13,   16,    18,    20,    22 };
static const double K_VALS[] = { 0.0308857954, 0.0308857954, 0.0458868358, 0.0619309863,
                                 0.0753609278, 0.1123894586, 0.1123894586, 0.1635043007 };
static const int K_COUNT = sizeof(K_LVLS)/sizeof(K_LVLS[0]);

static double k_for_level(int L){
    if (L <= K_LVLS[0]) return K_VALS[0];
    if (L >= K_LVLS[K_COUNT-1]) return K_VALS[K_COUNT-1];
    for (int i=0;i<K_COUNT-1;i++){
        int L0 = K_LVLS[i], L1 = K_LVLS[i+1];
        if (L >= L0 && L <= L1){
            double y0 = log(K_VALS[i]), y1 = log(K_VALS[i+1]);
            double t  = (double)(L - L0) / (double)(L1 - L0);
            return exp(y0 + t*(y1 - y0));
        }
    }
    return K_VALS[K_COUNT-1];
}

/*** Coulomb friction torque vs level (N·m) ***/
static inline double Tc_for_level(int L){
    const double a = 0.0166779771;   // N·m per level
    const double b = 0.4338270944;   // N·m base
    double Tc = b + a * (double)L;
    return (Tc > 0.0) ? Tc : 0.0;
}

/*** CORE API: pass crank rpm and resistance level ***/
uint16_t calc_watts_physics_params(double rpm_crank, int level)
{
    if (rpm_crank <= 0.0) return 0;
    if (level < 1) level = 1; else if (level > 22) level = 22;

    // flywheel angular speed
    double rpm_fly = SENSOR_IS_CRANK ? rpm_crank * (double)R_FLYWHEEL_PER_CRANK : rpm_crank;
    double omega   = rpm_to_rads(rpm_fly); // rad/s

    // model
    double k  = k_for_level(level);   // W / (rad/s)^2
    double Tc = Tc_for_level(level);  // N·m
    double Pf = k*(omega*omega) + Tc*omega;      // flywheel watts
    if (!(Pf > 0.0)) Pf = 0.0;

    double W  = Pf / (double)DRIVETRAIN_ETA;     // pedal watts
    if (W < 1.0) W = 1.0;
    if (W > 65535.0) W = 65535.0;
    return (uint16_t)(W + 0.5);
}

static void updateResistance()
{
    const uint16_t new_res = calc_res();
    if ( SET_RES.value != new_res ) {
        SET_RES.value = new_res;
        LOG_INF ( "Changing resistance magnitude to: %d", new_res );
        sendWithRetries ( SET_RES, 3, 50 );
    }
}

void updateBike()
{
    sendWithRetries ( RPM_REQ, 0, 0 );
    if ( firstRead && ( act_inc != SET_INC.value ) ) {
        sendWithRetries ( SET_INC, 1, 50 );
        sendWithRetries ( INC_REQ, 0, 50 );
    }
    updateResistance();
}

bike_data_t getBikeData()
{
    bike_data_t data;
    data.act_rpm = act_rpm;
    data.disp_res = disp_res;
    data.tgt_inc = SET_INC.value;
    data.watts = calc_watts_physics_params((double)act_rpm, calc_res());
    return data;
}