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
    double      currentTime;
    int         isBusy;
    int         sendingNode;
} shared_bus_S;


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
    Queue**         nodes;
    shared_bus_S    shared_bus;
    

    // METRICS
    double      transmitted_packets;
    double      successfully_transmitted_packets;

    // HELPERS
    double      T_prop;
    double      T_trans;
    int         shared_bus_sending_node;
    double      current_time;
    int         num_packets_to_send;
} app_simulator_data_S;


/*************************************************************************
 *        P R I V A T E   F U N C T I O N   D E C L A R A T I O N S      *
 *************************************************************************/

/**
 * @brief Persistent carrier sensing
 */
static double app_simulator_persistent_sensing(void);

/**
 * @brief Handle updating times if bus is busy
 */
static void app_simulator_bus_busy(void);

static int app_simulator_find_earliest_timestamp(void);

static int app_simulator_check_collision(int minTimeNode);

static double app_simulator_no_collision(int minTimeNode);
/*************************************************************************
 *            P R I V A T E   D A T A   D E C L A R A T I O N S          *
 *************************************************************************/

app_simulator_data_S app_simulator_data;

/*************************************************************************
 *                   P R I V A T E   F U N C T I O N S                   *
 *************************************************************************/




static int app_simulator_find_earliest_timestamp(void)
{
    int minTimeNode;
    double minTimeStamp = DBL_MAX;
    double currentNodeEarliestTimestamp;


    for (int i = 0; i < app_simulator_data.N; i++)
        {
            // Check for lowest timestamp
            currentNodeEarliestTimestamp = Queue_PeekHead(app_simulator_data.nodes[i]);
            if ((currentNodeEarliestTimestamp != -1) && (currentNodeEarliestTimestamp < minTimeStamp))
            {
                minTimeNode = i;
                minTimeStamp = currentNodeEarliestTimestamp;
            }
        }

    return minTimeNode;
}


static int app_simulator_check_collision(int minTimeNode)
{
    int isCollisionDetected = 0;
    double localSendTime = 0, currentNodeTimestamp, R, T_waiting, unblockTimestamp;
    double currTransmissionTime = Queue_PeekHead(app_simulator_data.nodes[minTimeNode]);
    double farthestCollisionTime = 0;
    for (int i = 0; i < app_simulator_data.N; i++)
    {
        currentNodeTimestamp = Queue_PeekHead(app_simulator_data.nodes[i]);
        // Skip the node with the lowest timestep aka the node we're checking against for collisions
        if (i == minTimeNode || currentNodeTimestamp == -1)
        {
            continue;
        }

        // Check how long it will take for first bit of current packet to reach selected node 
        localSendTime = currTransmissionTime + (app_simulator_data.T_prop*(abs(minTimeNode - i)));

        if(localSendTime > farthestCollisionTime)
        {
            farthestCollisionTime = localSendTime;
        }

        if (currentNodeTimestamp <= localSendTime)
        {
            

            if(!isCollisionDetected)
            {
                Queue_Increment_Collision(app_simulator_data.nodes[i]);
            }
            Queue_Increment_Collision(app_simulator_data.nodes[minTimeNode]);

            isCollisionDetected = 1;
            // Reset if greater than 10
            if (Queue_Collision_Count(app_simulator_data.nodes[minTimeNode]) == 10)
            {
                Queue_Dequeue(app_simulator_data.nodes[minTimeNode]);
                Queue_Reset_Collision(app_simulator_data.nodes[minTimeNode]);
            }

            if (Queue_Collision_Count(app_simulator_data.nodes[i]) == 10)
            {
                Queue_Dequeue(app_simulator_data.nodes[i]);
                Queue_Reset_Collision(app_simulator_data.nodes[i]);
            }

            
            if(Queue_Collision_Count(app_simulator_data.nodes[i]) > 0)
            {
                R = return_random(pow(2, Queue_Collision_Count(app_simulator_data.nodes[i])) - 1);
                T_waiting = R * 512 * ((double)1/(double)app_simulator_data.R);
                unblockTimestamp = (T_waiting) + localSendTime + app_simulator_data.T_trans;
                Queue_update_times(app_simulator_data.nodes[i], unblockTimestamp);
                app_simulator_data.transmitted_packets++;
            }

        }
        
    }

    if(isCollisionDetected)
    {
        if(Queue_Collision_Count(app_simulator_data.nodes[minTimeNode])>0)
        {
            R = return_random(pow(2, Queue_Collision_Count(app_simulator_data.nodes[minTimeNode])) - 1); // Problem wherein we could reset to 0 and still be doing a backoff
            T_waiting = R * 512 * ((double)1/(double)app_simulator_data.R);
            unblockTimestamp = (T_waiting) + farthestCollisionTime;
            Queue_update_times(app_simulator_data.nodes[minTimeNode], unblockTimestamp);
        }
    }
    
        // Due to the transmitting node
        // app_simulator_data.transmitted_packets++;
    

    return isCollisionDetected;
}


static double app_simulator_no_collision(int minTimeNode)
{

    // Dequeue the node to be transmitted
    double localSendTime = Queue_Dequeue(app_simulator_data.nodes[minTimeNode]);

    if (localSendTime < app_simulator_data.simulationTimeSecs)
    {
        app_simulator_data.transmitted_packets++;
        app_simulator_data.successfully_transmitted_packets++;
    }

    // Reset the collision counter for this node
    Queue_Reset_Collision(app_simulator_data.nodes[minTimeNode]);
    app_simulator_data.current_time = localSendTime;

    if (localSendTime == -1)
    {
        return -1;
    }

    // Set status of the shared bus 
    app_simulator_data.shared_bus.isBusy = 1;
    app_simulator_data.shared_bus.currentTime = localSendTime;
    app_simulator_data.shared_bus.sendingNode = minTimeNode;

    return localSendTime;

}

static void app_simulator_non_persistant_backoff_calculation(Queue* node, double timeTotalPacketSend)
{
    double R, T_waiting, T_backoff; 

    // Calculate our backoff value. While backoff value < bus transmission time, keep doing this
    while(T_backoff < timeTotalPacketSend)
    {
        Queue_non_persistant_increment(node);
        
        //Drop if over 10
        if(Queue_non_persistant_count(node) > 10)
        {
            Queue_Dequeue(node);
            return;
        }

        // Calculate out other values
        R = return_random(pow(2, Queue_non_persistant_count(node)) - 1);
        T_waiting = R * 512 * ((double)1/(double)app_simulator_data.R);
        T_backoff = T_waiting + Queue_PeekHead(node);

    }

    Queue_update_times(node, T_backoff);
}


static void app_simulator_bus_busy(void)
{
    int max_nodal_distance;

    if(app_simulator_data.N - app_simulator_data.shared_bus.sendingNode > app_simulator_data.shared_bus.sendingNode)
    {
        max_nodal_distance = app_simulator_data.N - app_simulator_data.shared_bus.sendingNode;
    }
    else
    {
        max_nodal_distance = app_simulator_data.shared_bus.sendingNode;
    }
    

    for (int i = 0; i < app_simulator_data.N; i++)
    {
        // If current node, skip
        if (i == app_simulator_data.shared_bus.sendingNode)
        {
            continue;
        }

        // Calculate time to receive total packet for each node
        double timeTotalPacketSend = app_simulator_data.shared_bus.currentTime + app_simulator_data.T_trans + (app_simulator_data.T_prop*((double)max_nodal_distance));
        double curNodeTime = Queue_PeekHead(app_simulator_data.nodes[i]);

        // If arrival packet less than time for bus to be unbusy, update times
        if (curNodeTime <= timeTotalPacketSend)
        {
            Queue_update_times(app_simulator_data.nodes[i], timeTotalPacketSend);
        }
        
        
    }

    app_simulator_data.shared_bus.isBusy = 0;
    app_simulator_data.shared_bus.currentTime = 0;
    app_simulator_data.shared_bus.sendingNode = -1;
}

static void app_simulator_bus_busy_non_persistent(void)
{
    for (int i = 0; i < app_simulator_data.N; i++)
    {
        // If current node, skip
        if (i == app_simulator_data.shared_bus.sendingNode)
        {
            continue;
        }

        // Calculate time to receive total packet for each node
        double timeTotalPacketSend = app_simulator_data.shared_bus.currentTime + app_simulator_data.T_trans + (app_simulator_data.T_prop*(abs(app_simulator_data.shared_bus.sendingNode - i)));
        double curNodeTime = Queue_PeekHead(app_simulator_data.nodes[i]);

        // If arrival packet less than time for bus to be unbusy, update times
        if (curNodeTime <= timeTotalPacketSend)
        {
            app_simulator_non_persistant_backoff_calculation(app_simulator_data.nodes[i], timeTotalPacketSend);
        }
        
        
    }

    app_simulator_data.shared_bus.isBusy = 0;
    app_simulator_data.shared_bus.currentTime = -1;
    app_simulator_data.shared_bus.sendingNode = -1;
}

static double app_simulator_persistent_sensing(void)
{
    int earliestTransmissionNode, isCollision;
    
    if(app_simulator_data.shared_bus.isBusy)
    {
        app_simulator_bus_busy();
    }
    
    earliestTransmissionNode = app_simulator_find_earliest_timestamp();
    isCollision = app_simulator_check_collision(earliestTransmissionNode);
    double ret;

    if (isCollision)
    {
        ret = Queue_PeekHead(app_simulator_data.nodes[earliestTransmissionNode]);
    }
    else
    {
        ret = app_simulator_no_collision(earliestTransmissionNode);
    }
    
    return ret;
}

static double app_simulator_non_persistent_sensing(void)
{
    int earliestTransmissionNode, isCollision;
    
    if(app_simulator_data.shared_bus.isBusy)
    {
        app_simulator_bus_busy_non_persistent();
    }
    
    earliestTransmissionNode = app_simulator_find_earliest_timestamp();
    isCollision = app_simulator_check_collision(earliestTransmissionNode);
    double ret;

    if (isCollision)
    {
        ret = Queue_PeekHead(app_simulator_data.nodes[earliestTransmissionNode]);
    }
    else
    {
        ret = app_simulator_no_collision(earliestTransmissionNode);
    }
    
    return ret;
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
    app_simulator_data.current_time = 0.0;
    app_simulator_data.T_prop = D/S;
    app_simulator_data.T_trans = L/R;
    app_simulator_data.nodes = malloc(N*sizeof(Queue*));
    app_simulator_data.shared_bus.isBusy = 0;
    app_simulator_data.shared_bus.currentTime = 0;
    app_simulator_data.shared_bus.sendingNode = -1;


    // Populate nodes
    double currentTime = 0;
    for(int i = 0; i < N; i++)
    {
        Queue* node_ptr = Queue_Init(APP_SIMULATOR_QUEUE_DEFAULT_SIZE, i);
        currentTime = 0;
	 do
        {
            // Generate random timestamps
            currentTime = timestamp_generate(app_simulator_data.A, currentTime);

            if (currentTime >= (app_simulator_data.simulationTimeSecs+10.0))
            {
                 break;
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
    double efficiency = ((double)app_simulator_data.successfully_transmitted_packets/(double)app_simulator_data.transmitted_packets);
    double throughput = (app_simulator_data.successfully_transmitted_packets * app_simulator_data.L)/(app_simulator_data.simulationTimeSecs * 1000000);

	printf("Transmitted packets: %f\r\n", app_simulator_data.transmitted_packets);
	printf("Successfully transmitted packets: %f\r\n", app_simulator_data.successfully_transmitted_packets);
    printf("Efficiency rate: %f\r\n", efficiency);
    printf("Throughput: %f Mbps\r\n\r\n", throughput);
}

double app_simulator_totalTransmittedPackets(void)
{
    return app_simulator_data.transmitted_packets;
}
