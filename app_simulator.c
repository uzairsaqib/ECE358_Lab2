/**
 *  @file   app_simulator.c
 *  @brief  Simulator implementation
 */

/*************************************************************************
 *                           I N C L U D E S                             *
 *************************************************************************/

#include "app_simulator.h"
#include "timestamp_generator.h"

#include <string.h>
#include <stdio.h>
#include "queue.h"
#include <float.h>

/*************************************************************************
 *                            D E F I N E S                              *
 *************************************************************************/

#define APP_SIMULATOR_PROGRESS           (0U)
#define APP_SIMULATOR_QUEUE_DEFAULT_SIZE (1000000000)

/*************************************************************************
 *                            T Y P E D E S                              *
 *************************************************************************/

typedef enum
{
    APP_SIMULATOR_QUEUE_NONE,
    APP_SIMULATOR_NODE
} app_simulator_queueType_E;

typedef struct
{
    // SIM PARAMETERS
    double      simulationTimeSecs;
    double      A;
    double      L;
    double      R;
    double      N;
    double      D;
    double      S;
    // double      rho;
    int64_t     maxBufferSize;

    // NODES
    Queue** nodes;
    Queue* shared_bus;
    

    // METRICS
    double      transmitted_packets;
    double      successfully_transmitted_packets;
    int64_t     departurePackets;
    int64_t     droppedPackets;
    long int    totalObservations;
    long int    totalArrivals;
    double      idleTime;
    long int    averagePacketsInBuffer;

    // HELPERS
    double      T_prop;
    double      T_trans;
    int         shared_bus_sending_node;
} app_simulator_data_S;


/*************************************************************************
 *        P R I V A T E   F U N C T I O N   D E C L A R A T I O N S      *
 *************************************************************************/

/**
 * @brief Persistent carrier sensing
 */
static void app_simulator_persistent_sensing(void);

/**
 * @brief Perform operations on a node when collision is detected
 */
static void app_simulator_collision_detected(Queue* node);

/**
 * @brief Check to see if current node head is scheduled to arrive before bus send is over. If so, update node values
 */
static void app_simulator_bus_busy(Queue* node, double localSendTime);

/*************************************************************************
 *            P R I V A T E   D A T A   D E C L A R A T I O N S          *
 *************************************************************************/

app_simulator_data_S app_simulator_data;

/*************************************************************************
 *                   P R I V A T E   F U N C T I O N S                   *
 *************************************************************************/

// Works on a per node basis
static void app_simulator_collision_detected(Queue* node)
{
    // Increment the Queue collision counter
    Queue_Increment_Collision(node);

    // Choose a random var
    int K_pick = return_random(Queue_Collision_Count(node));

    if(K_pick > 10)
    {
        Queue_Dequeue(node);
        Queue_Reset_Collision(node);
    }

    else
    {
        // Calculate exponential backoff time and update all Queue values to correspond to this
        double wait_time = (double)K_pick*512.0 + Queue_PeekHead(node);
        Queue_update_times(node, wait_time);
    }

}

// Works on a per node basis
static void app_simulator_bus_busy(Queue* node, double localSendTime)
{
    if (Queue_PeekHead(node) < localSendTime)
    {
        Queue_update_times(node, localSendTime);
    }
}

static void app_simulator_persistent_sensing(void)
{
    int i, minTimeNode, isCollisionDetected = 0;
    double minTimeStamp = DBL_MAX;
    double maxSendTime = 0, localSendTime = 0;
    double node_heads[app_simulator_data.N];

    // Check to see if bus is occupied. If occupied, Update the other node times to accomodate
    if(!Queue_IsEmpty(app_simulator_data.shared_bus))
    {
        for (i = 0; i < app_simulator_data.N; i++)
        {
            // Skip if current node transmitting
            if(i == app_simulator_data.shared_bus_sending_node)
            {
                continue;
            }

        // Calculate time to send to each node and them update the queues if needed
        localSendTime = app_simulator_data.T_trans + (app_simulator_data.T_prop * abs(app_simulator_data.shared_bus_sending_node-i));
        app_simulator_bus_busy(app_simulator_data.nodes[i], localSendTime);
        }

        // Dequeue current packet from shared bus.
        Queue_Dequeue(app_simulator_data.shared_bus);
    }
    
    localSendTime = 0;

    // Bus is empty. Can send packet
    for (i = 0; i < app_simulator_data.N; i++)
    {
        // Check for lowest timestamp
        // TODO: Confirm lowest timestamp against waiting value for exponential backoff
        node_heads[i] = Queue_PeekHead(app_simulator_data.nodes[i]);
        if (node_heads[i] < minTimeStamp)
        {
            minTimeStamp = node_heads[i];
            minTimeNode = i;
        }

    }

    // Scan through heads of all the nodes to determine which nodes will experience a collision
    for (i = 0; i < app_simulator_data.N; i++)
    {
        // Skip the node with the lowest timestep aka the node we're checking against for collisions
        if (i == minTimeNode)
        {
            continue;
        }

        // Check how long it will take for first bit of current packet to reach selected node 
        localSendTime = minTimeStamp + (app_simulator_data.T_prop*(abs(minTimeNode - i)));

        if (node_heads[i] < localSendTime)
        {
            isCollisionDetected = 1;
            app_simulator_data.transmitted_packets++;
            app_simulator_collision_detected(app_simulator_data.nodes[i]);
        }

    }

    if (!isCollisionDetected)
    {
        // Dequeue packet
        do {
            localSendTime = Queue_Dequeue(app_simulator_data.nodes[minTimeNode]);
            app_simulator_data.transmitted_packets++;
            app_simulator_data.successfully_transmitted_packets++;
        } while(Queue_PeekHead(app_simulator_data.nodes[minTimeNode]) == localSendTime); // Maybe change this to <= in case scenario of
        // next packet arrival time is less than current arrival time but if thats happening then I have a whole other butthole issue
        
        // Enqueue packet onto shared bus and set the shared bus node to the transmitting node
        Queue_Enqueue(app_simulator_data.shared_bus, localSendTime);
        app_simulator_data.shared_bus_sending_node = minTimeNode;

    }
    
}


/*************************************************************************
 *                    P U B L I C   F U N C T I O N S                    *
 *************************************************************************/

void app_simulator_init(double simulationTimeSec, double A, double L, double R, double N, double D, double S, int64_t max_buffer_size)
{
    memset(&app_simulator_data, 0, sizeof(app_simulator_data));

    //Store passed in sim variables
    app_simulator_data.simulationTimeSecs = simulationTimeSec;
    app_simulator_data.A = A;
    app_simulator_data.L = L;
    app_simulator_data.R = R;
    app_simulator_data.N = N;
    app_simulator_data.D = D;
    app_simulator_data.S = S;
    app_simulator_data.T_prop = D/S;
    app_simulator_data.T_trans = L/R;
    app_simulator_data.maxBufferSize = max_buffer_size;
    app_simulator_data.nodes = malloc(N*sizeof(Queue*));
    app_simulator_data.shared_bus = Queue_Init(1, -1);


    // Calculate lambda
    // app_simulator_data.lambda = ((double)rho*C)/((double)L);
    // printf("rho: %f\r\n", app_simulator_data.rho);


    // Populate nodes
    double currentTime = 0;
    for(int i = 0; i < N; i++)
    {
        //node_ptr = malloc(app_simulator_data.N = N, sizeof(Queue *));
        Queue* node_ptr = Queue_Init(APP_SIMULATOR_QUEUE_DEFAULT_SIZE, i);
        do
        {
            // Generate random timestamps
            currentTime = timestamp_generate(app_simulator_data.A, currentTime);

            if (currentTime >= app_simulator_data.simulationTimeSecs)
            {
                currentTime = -1;
            }

            // Add 
            Queue_Enqueue(node_ptr, currentTime);

            if (currentTime == -1)
            {
                break;
            }
        } while (Queue_IsFull(node_ptr) != true);

        app_simulator_data.nodes[i] = node_ptr;
    } 

    /*
    // Make sure we didn't run out of space filling up the event queues
    if ((Queue_PeekTail(app_simulator_data.observerEvents) != -1) ||
        Queue_PeekTail(app_simulator_data.arrivalEvents) != -1)
    {
        printf("ERROR: Queue overflow\r\n");
        printf("ObserverQueueTail: %f\r\n", Queue_PeekHead(app_simulator_data.observerEvents));
        printf("ArrivalQueueTail: %f\r\n", Queue_PeekHead(app_simulator_data.arrivalEvents));
    }
    */
}

























































// BELOW THIS IS A TODO THAT I HAVE NOT TOUCHED YET I JUST COPIED IT

void app_simulator_deinit(void)
{
    Queue_Delete(app_simulator_data.arrivalEvents);
    app_simulator_data.arrivalEvents = NULL;

    Queue_Delete(app_simulator_data.observerEvents);
    app_simulator_data.observerEvents = NULL;

    Queue_Delete(app_simulator_data.departureEvents);
    app_simulator_data.departureEvents = NULL;
}

app_simulator_retCode_E app_simulator_run(void)
{
    app_simulator_retCode_E ret = APP_SIMULATOR_RET_SIM_RUNNING;

    app_simulator_queueType_E nextQueue = app_simulator_private_getNextQueue();
    switch(nextQueue)
    {
        case APP_SIMULATOR_QUEUE_ARRIVAL:
        {
            app_simulator_private_handleArrival();
            break;
        }

        case APP_SIMULATOR_QUEUE_OBSERVER:
        {
            app_simulator_private_handleObserver();
            break;
        }

        case APP_SIMULATOR_QUEUE_DEPARTURE:
        {
            app_simulator_private_handleDeparture();
            break;
        }

        case APP_SIMULATOR_QUEUE_NONE:
        default:
        {
            ret = APP_SIMULATOR_RET_SIM_COMPLETE;
            break;
        }
    }
    
    return ret;
}

void app_simulator_outputResults(void)
{
    printf("SIMULATION COMPLETE:\r\n");
    
    // Average number of packets in queue
    #if (APP_SIMULATOR_PROGRESS)
    printf("TotalPackets: %ld\r\n", app_simulator_data.averagePacketsInBuffer);
    printf("Observations: %ld\r\n", app_simulator_data.totalObservations);
    #endif

    double E_n = ((double)app_simulator_data.averagePacketsInBuffer)/((double)app_simulator_data.totalObservations);
    printf("E[N] = %f\r\n", E_n);

    // P_loss = (lostPackets)/(arrivalPackets + lostPackets) * 100
    double P_loss = ((double)app_simulator_data.droppedPackets/(double)(app_simulator_data.totalArrivals + app_simulator_data.droppedPackets))*100;
    printf("P_loss = %f\r\n", P_loss);

    double P_idle = (app_simulator_data.idleTime/app_simulator_data.simulationTimeSecs)*100;
    printf("P_Idle = %f\r\n\r\n", P_idle);
}
