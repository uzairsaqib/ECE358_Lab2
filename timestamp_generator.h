/**
 *  @file   timestamp_generator.h
 *  @brief  Generate new timestamp API
 */

#ifndef TIMESTAMP_GENERATOR_H
#define TIMESTAMP_GENERATOR_H

/*************************************************************************
 *                           I N C L U D E S                             *
 *************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

/*************************************************************************
 *                            D E F I N E S                              *
 *************************************************************************/

/*************************************************************************
 *                            T Y P E D E S                              *
 *************************************************************************/

/*************************************************************************
 *          P U B L I C   F U N C T I O N   D E C L A R A T I O N S      *
 *************************************************************************/

/**
 *  @brief  Generate times based on exponential distribution
 */
double exp_generate(double lambda);

/**
 *  @brief  Generate a new timestamp based on an input lambda value and the current time
 */
double timestamp_generate(double lambda, double current_time);

#endif /* TIMESTAMP_GENERATOR_H */