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
    int	        N;
    double      D;
    double      S;

    // NODES
    Queue** nodes;
    Queue* shared_bus;
    

    // METRICS
    double      transmitted_packets;
    double      successfully_transmitted_packets;

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
static double app_simulator_persistent_sensing(void);

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
    int returnCount = 0;
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
        if (wait_time >= app_simulator_data.simulationTimeSecs)
	{
	    Queue_Dequeue(node);
            Queue_Reset_Collision(node);
	}
	// If wait time is not greater than sim time, update all node values less than this wait time to be later than 
	// this wait time
	else
	{
		 returnCount = Queue_update_times(node, wait_time);
		 app_simulator_data.transmitted_packets += returnCount;
	}
    }

}

// Works on a per node basis
static void app_simulator_bus_busy(Queue* node, double localSendTime)
{
    // Dummy var so that I can use the same function
    int returnCount;
    if (Queue_PeekHead(node) < localSendTime)
    {
        returnCount =  Queue_update_times(node, localSendTime);
    }
}

/**
 *  @brief  Finds the earliest timestamp in all the nodes
 *  @return Index of node with earliest timestamp
 */
unsigned int app_simulator_private_findEarliestTimestamp(void)
{
    double minTimeStamp = DBL_MAX;

    for (unsigned int i = 0; i < app_simulator_data.N; i++)
    {
        double currentNodeEarliestTimestamp = Queue_PeekHead(app_simulator_data.nodes[i]);

        if ((currentNodeEarliestTimestamp != -1) && (currentNodeEarliestTimestamp < minTimeStamp))
        {
            minTimeStamp = currentNodeEarliestTimestamp;
        }
    }
}

/**
 *  @brief  Checks if a collision would occur due to failure to check
 *          if the bus is busy
 *  @return true if a collision was detected,
 *          false otherwise
 */
bool app_simulator_private_checkCollision(unsigned int transmittingNode)
{
    bool collisionDetected = false;
    double transmissionTimestamp = Queue_PeekHead(app_simulator_data.nodes[transmittingNode]);

    for (unsigned int i = 0; i < app_simulator_data.N; i++)
    {
        double curNodeNextTransmissionTimestamp = Queue_PeekHead(app_simulator_data.nodes[i]);
        if ((i == transmittingNode) || (curNodeNextTransmissionTimestamp == -1))
        {
            continue;
        }

        double firstBitArrivalTime = transmissionTimestamp + (app_simulator_data.T_prop*(abs(transmittingNode - i)));

        /** 
         *  If the first bit arrival time is after the earliest timestamp in the node, then the node will
         *  try to send its frame while the bus is busy and a collision will occur
         */
        if (firstBitArrivalTime > curNodeNextTransmissionTimestamp)
        {
            collisionDetected = true;

            // Increment collision counters for both nodes involved in the collision
            Queue_Increment_Collision(app_simulator_data.nodes[transmittingNode]);
            Queue_Increment_Collision(app_simulator_data.nodes[i]);

            /**
             *  If either of the nodes involved in the collision have experienced 10 consecutive
             *  then the nodes should drop their respective packet and reset their counters
             */
            if (Queue_Collision_Count(app_simulator_data.nodes[transmittingNode]) == 10)
            {
                Queue_Dequeue(app_simulator_data.nodes[transmittingNode]);
                Queue_Reset_Collision(app_simulator_data.nodes[transmittingNode]);
            }
            if (Queue_Collision_Count(app_simulator_data.nodes[i]) == 10)
            {
                Queue_Dequeue(app_simulator_data.nodes[i]);
                Queue_Reset_Collision(app_simulator_data.nodes[i]);
            }

            // Update the transmission times of the transmitting node
            double R = return_random(pow(2, Queue_Collision_Count(transmittingNode)) - 1);
            double T_waiting = R * 512 * ((double)1/(double)app_simulator_data.R);
            double unblockTimestamp = T_waiting + Queue_PeekHead(app_simulator_data.nodes[transmittingNode]);
            Queue_update_times(app_simulator_data.nodes[transmittingNode], unblockTimestamp);

            // Update the transmission times of the current node
            R = return_random(pow(2, Queue_Collision_Count(i)) - 1);
            T_waiting = R * 512 * ((double)1/(double)app_simulator_data.R);
            unblockTimestamp = T_waiting + Queue_PeekHead(app_simulator_data.nodes[i]);
            Queue_update_times(app_simulator_data.nodes[i], unblockTimestamp);
        }
    }

    return collisionDetected;
}

void app_simulator_persistent_sensing(void)
{
    unsigned int earliestNodeIndex = app_simulator_private_findEarliestTimestamp();
}

static double app_simulator_persistent_sensing(void)
{
    int i, minTimeNode, isCollisionDetected = 0;
    double minTimeStamp = DBL_MAX;
    double maxSendTime = 0, localSendTime = 0, ret = 0;
    double node_heads[app_simulator_data.N];

    // Check to see if bus is occupied. If occupied, Update the other node times to accomodate
    if(app_simulator_data.shared_bus->size != 0)
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
            if (localSendTime > app_simulator_data.simulationTimeSecs) 
            {
                continue;
            }
            app_simulator_bus_busy(app_simulator_data.nodes[i], localSendTime);
        }

        // Dequeue current packet from shared bus.
        ret = Queue_Dequeue(app_simulator_data.shared_bus);
        return ret;
    }
    else {

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

        if (minTimeStamp == -1)
        {
            return -1;
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
                isCollisionDetected = 0;
                app_simulator_collision_detected(app_simulator_data.nodes[i]);
                ret = minTimeStamp;
            }

        }

        if (!isCollisionDetected)
        {
            // Dequeue packet
            do{
                localSendTime = Queue_Dequeue(app_simulator_data.nodes[minTimeNode]);
                app_simulator_data.transmitted_packets++;
                app_simulator_data.successfully_transmitted_packets++;
            } while(Queue_PeekHead(app_simulator_data.nodes[minTimeNode]) == localSendTime);
            // next packet arrival time is less than current arrival time but if thats happening then I have a whole other butthole issue
            
            if (localSendTime == -1)
            {
                return -1;
            }

            // Enqueue packet onto shared bus and set the shared bus node to the transmitting node
            Queue_Enqueue(app_simulator_data.shared_bus, localSendTime);
            app_simulator_data.shared_bus_sending_node = minTimeNode;
            ret = minTimeStamp;

        }
        return ret;
    }
    
}


/*************************************************************************
 *                    P U B L I C   F U N C T I O N S                    *
 *************************************************************************/

void app_simulator_init(double simulationTimeSec, double A, double L, double R, double N, double D, double S)
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
        currentTime = 0;
	 do
        {
            // Generate random timestamps
            currentTime = timestamp_generate(app_simulator_data.A, currentTime);

            if (currentTime >= app_simulator_data.simulationTimeSecs) // Check this condition. It might mess things up in the logic.
            // Maybe should check nodes for this value? not sure
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




double app_simulator_run(void)
{

    return app_simulator_persistent_sensing();
    
}

void app_simulator_deinit(void)
{
    for (int i = 0; i < app_simulator_data.N; i++)
    {
        Queue_Delete(app_simulator_data.nodes[i]);
        app_simulator_data.nodes[i] = NULL;
    }
}

void app_simulator_print_results(void)
{
	printf("Transmitted packets %f\r\n", app_simulator_data.transmitted_packets);
	printf("Success packets %f\r\n", app_simulator_data.successfully_transmitted_packets);

}


