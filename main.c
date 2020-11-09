/**
 *  @file   main.c
 *  @brief  main
 *  @author Haaris Ahmed 20648827
 *  @author Uzair Saqib 20659370
 */

#include "app_simulator.h"
#include "timestamp_generator.h"
#include "queue.h"
#include <stdio.h>

// QUESTION 
int main(void)
{
    double simTime = 1000.0;
    double A = 5.0;
    double L = 1500.0;
    double R = 1000000.0;
    double D = 10.0;
    double S = (2.0/3.0)*3.0*100000000.0;

    double timeStamp = 0;
    for (double N = 20; N < 120; N += 20)
    {
        app_simulator_init(simTime, A, L, R, N, D, S);

        while(timeStamp <= simTime)
        {
            timeStamp =  app_simulator_run();
        }

        app_simulator_print_results();
        app_simulator_deinit();
        timeStamp = 0;
    }
}
