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
    double simTime = 1000;
    double A = 7;
    double L = 1500;
    double R = 1;
    double N = 20;
    double D = 10;
    double S = (2.0/3.0)*3.0*100000000.0;
    

    double timeStamp = 0;
   
    while(timeStamp < simTime || timeStamp!= -1)
    {
    	app_simulator_init(simTime, A, L, R, N, D, S);
        timeStamp =  app_simulator_run();
	printf("The time is %f\r\n", timeStamp);
    }
    app_simulator_deinit();
    
}

/*
int main(void)
{
    app_simulator_init(100);

    while (app_simulator_run() != APP_SIMULATOR_RET_SIM_COMPLETE);

    app_simulator_outputResults();

    return 0;
}
*/
