/**
 *  @file   timestamp_generator.c
 *  @brief  Implementation for timestamp generation API
 */

/*************************************************************************
 *                           I N C L U D E S                             *
 *************************************************************************/

#include "timestamp_generator.h"
#include <stdio.h>

/*************************************************************************
 *                            D E F I N E S                              *
 *************************************************************************/

/*************************************************************************
 *                            T Y P E D E S                              *
 *************************************************************************/

/*************************************************************************
 *        P R I V A T E   F U N C T I O N   D E C L A R A T I O N S      *
 *************************************************************************/

/*************************************************************************
 *            P R I V A T E   D A T A   D E C L A R A T I O N S          *
 *************************************************************************/

/*************************************************************************
 *                   P R I V A T E   F U N C T I O N S                   *
 *************************************************************************/

/*************************************************************************
 *                    P U B L I C   F U N C T I O N S                    *
 *************************************************************************/

double exp_generate(double lambda)
{
    double u_rand = (double)rand() / (double)RAND_MAX;
    double exp_rand = (-1/(double)lambda) * logf( (1 - u_rand));

    return (exp_rand);
}

double timestamp_generate(double lambda, double current_time) 
{
    double u_rand = (double)rand() / (double)RAND_MAX;
    double exp_rand = (-1/(double)lambda) * logf( (1 - u_rand));
    
    return (exp_rand + current_time);
}

int return_random(int upper)
{
    return ( rand() % (upper+1) );
}
