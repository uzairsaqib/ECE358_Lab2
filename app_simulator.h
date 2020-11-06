/**
 *  @file   app_simulator.h
 *  @brief  API simulator application
 */

#ifndef APP_SIMULATOR_H
#define APP_SIMULATOR_H

/*************************************************************************
 *                           I N C L U D E S                             *
 *************************************************************************/

#include <stdint.h>

/*************************************************************************
 *                            D E F I N E S                              *
 *************************************************************************/

/*************************************************************************
 *                            T Y P E D E S                              *
 *************************************************************************/

typedef enum
{
    APP_SIMULATOR_RET_SIM_RUNNING,
    APP_SIMULATOR_RET_SIM_COMPLETE,
} app_simulator_retCode_E;

/*************************************************************************
 *          P U B L I C   F U N C T I O N   D E C L A R A T I O N S      *
 *************************************************************************/

/**
 *  @brief  Initialize the simulator application
 */
void app_simulator_init(double simulationTimeSec, double A, double L, double R, double N, double D, double S);

/**
 *  @brief  De-initialize the simulator application
 */
void app_simulator_deinit(void);

/**
 *  @brief  Run the simulation
 *  @return Current time value
 */
double app_simulator_run(void);

// /**
//  *  @brief  Output the results of the simulation
//  */
// void app_simulator_outputResults(void);

#endif /* APP_SIMULATOR_H */
