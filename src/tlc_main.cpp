#include <config.h>

#include <iostream>
#include <string>
#include <cstdlib>
#include "sumo_client.hpp"

#include <unistd.h>

SUMO_CLIENT client;

const std::string NSGREEN = "Grr";
const std::string NSYELLOW = "yrr";
const std::string WEGREEN = "rGG";
const std::string WEYELLOW = "ryy";

const std::string NSGREEN1 = "GGrr";
const std::string NSYELLOW1 = "yyrr";
const std::string WEGREEN1 = "rrGG";
const std::string WEYELLOW1 = "rryy";

const int Light_Min_IK=30; 
const int Light_Max_IK=120; //the constrains of the length of traffic lights for intersection IK

const int Light_Min_LJ=30;  
const int Light_Max_LJ=120; //the constrains of the length of traffic lights for intersection LJ

const int Light_Min_GD=30;  
const int Light_Max_GD=120; //the constrains of the length of traffic lights for intersection GD

const int Light_Min_FH=30;  
const int Light_Max_FH=120; //the constrains of the length of traffic lights for intersection FH

const int Light_Min_AC=30;  
const int Light_Max_AC=120; //the constrains of the length of traffic lights for intersection AC

const int s_NS_IK=4; 
const int s_WE_IK=15; //the threshold of North-South and West-East direction for intersection IK

const int s_NS_LJ=4; 
const int s_WE_LJ=15; //the threshold of North-South and West-East direction for intersection LJ

const int s_NS_GD=4; 
const int s_WE_GD=15; //the threshold of North-South and West-East direction for intersection GD

const int s_NS_FH=4; 
const int s_WE_FH=15; //the threshold of North-South and West-East direction for intersection FH

const int s_NS_AC=4; 
const int s_WE_AC=15; //the threshold of North-South and West-East direction for intersection IK


void vehicle_number(std::string sensor1,
		    std::string sensor2,
		    std::string& id_T1,
		    std::string& id_S1,
		    int& sum_sensor1,
		    int& sum_sensor2,
		    int& queue_length)
{
  int numVehicles = client.inductionloop.getLastStepVehicleNumber(sensor1);
  if (numVehicles == 0)
    id_T1="";
  else
    {
      std::vector<std::string> list_T1 = client.inductionloop.getLastStepVehicleIDs(sensor1);
      for ( std::vector<std::string>::iterator it = list_T1.begin(); it != list_T1.end(); ++it)
	{
	  if ( *it != id_T1 )
	    {
	      id_T1 = *it;
	      sum_sensor1 += 1;
	    }
	}
    }
  numVehicles = client.inductionloop.getLastStepVehicleNumber(sensor2);
  if (numVehicles == 0)
    id_S1="";
  else
    {
      std::vector<std::string> list_S1 = client.inductionloop.getLastStepVehicleIDs(sensor2);
      for ( std::vector<std::string>::iterator it = list_S1.begin(); it != list_S1.end(); ++it)
	{
	  if ( *it != id_S1 )
	    {
	      id_S1 = *it;
	      sum_sensor2 += 1;
	    }
	}
    }
  queue_length = sum_sensor1-sum_sensor2 + 1 ;
}

void clock_value(std::string intersection,
		 int& clock_WE,
		 int& clock_NS,
		 std::string& tl_state)
{
  tl_state = client.trafficlights.getRedYellowGreenState(intersection);
  if (!tl_state.compare(NSGREEN))
    {
      clock_NS = clock_NS + 1;
      clock_WE = 0;
    }
  else
    {
      clock_WE = clock_WE + 1;
      clock_NS = 0;
    }
}

void clock_value1(std::string intersection,
		  int& clock_WE,
		  int& clock_NS,
		  std::string& tl_state)
{
  tl_state = client.trafficlights.getRedYellowGreenState(intersection);
  if (!tl_state.compare(NSGREEN1))
    {
      clock_NS = clock_NS + 1;
      clock_WE = 0;
    }
  else
    {
      clock_WE = clock_WE + 1;
      clock_NS = 0;
    }
}

void controller(std::string intersection,
		std::string& tl_state,
		int queue_WE,
		int queue_NS,
		int& clock_WE,
		int& clock_NS,
		int Light_Min,
		int Light_Max,
		int s_WE,
		int s_NS)
{
  if ((queue_WE < s_WE && queue_NS < s_NS) || (queue_WE >= s_WE && queue_NS >= s_NS))
    {
      if ( !tl_state.compare(WEGREEN) && clock_WE > Light_Max )
	{
	  client.trafficlights.setRedYellowGreenState(intersection, NSGREEN);
	  clock_WE = 0;
	}
      if ( !tl_state.compare(NSGREEN) && clock_NS > Light_Max )
	{
	  client.trafficlights.setRedYellowGreenState(intersection, WEGREEN);
	  clock_NS = 0;
	}
    }
  else if (queue_WE >= s_WE && queue_NS <s_NS)
    {
      if ( !tl_state.compare(NSGREEN) && clock_NS > Light_Min )
	{
	  client.trafficlights.setRedYellowGreenState(intersection, WEGREEN);
	  clock_NS = 0;
	}
      if ( !tl_state.compare(WEGREEN) && clock_WE > Light_Max )
	{
	  client.trafficlights.setRedYellowGreenState(intersection, NSGREEN);
	  clock_WE = 0;
	}
    }
  else
    {
      if ( !tl_state.compare(WEGREEN) && clock_WE > Light_Min )
	{
	  client.trafficlights.setRedYellowGreenState(intersection, NSGREEN);
	  clock_WE = 0;
	}
      if ( !tl_state.compare(NSGREEN) && clock_NS > Light_Max )
	{
	  client.trafficlights.setRedYellowGreenState(intersection, WEGREEN);
	  clock_NS = 0;
	}
    }
}

void controller1(std::string intersection,
		 std::string& tl_state,
		 int queue_WE,
		 int queue_NS,
		 int& clock_WE,
		 int& clock_NS,
		 int Light_Min,
		 int Light_Max,
		 int s_WE,
		 int s_NS)
{
  if ((queue_WE < s_WE && queue_NS < s_NS) || (queue_WE >= s_WE && queue_NS >= s_NS))
    {
      if ( !tl_state.compare(WEGREEN1) && clock_WE > Light_Max )
	{
	  client.trafficlights.setRedYellowGreenState(intersection, NSGREEN1);
	  clock_WE = 0;
	}
      if ( !tl_state.compare(NSGREEN1) && clock_NS > Light_Max )
	{
	  client.trafficlights.setRedYellowGreenState(intersection, WEGREEN1);
	  clock_NS = 0;
	}
    }
  else if (queue_WE >= s_WE && queue_NS <s_NS)
    {
      if ( !tl_state.compare(NSGREEN1) && clock_NS > Light_Min )
	{
	  client.trafficlights.setRedYellowGreenState(intersection, WEGREEN1);
	  clock_NS = 0;
	}
      if ( !tl_state.compare(WEGREEN1) && clock_WE > Light_Max )
	{
	  client.trafficlights.setRedYellowGreenState(intersection, NSGREEN1);
	  clock_WE = 0;
	}
    }
  else
    {
      if ( !tl_state.compare(WEGREEN1) && clock_WE > Light_Min )
	{
	  client.trafficlights.setRedYellowGreenState(intersection, NSGREEN1);
	  clock_WE = 0;
	}
      if ( !tl_state.compare(NSGREEN1) && clock_NS > Light_Max )
	{
	  client.trafficlights.setRedYellowGreenState(intersection, WEGREEN1);
	  clock_NS = 0;
	}
    }
}

int main(int argc, char* argv[]) {
    int port = -1;
    std::string host = "localhost";
    int sleep_us = -1;

    if (argc < 5) {
        std::cout << "Usage: tlc -p <remote port> -s <sleep time in us>"
                  << " [-h <remote host>]" << std::endl;
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
	if (arg.compare("-p") == 0) {
            port = atoi(argv[i + 1]);
            i++;
	} else if (arg.compare("-s") == 0) {
            sleep_us = atoi(argv[i + 1]);
            i++;
        } else if (arg.compare("-h") == 0) {
            host = argv[i + 1];
            i++;
        } else {
            std::cout << "unknown parameter: " << argv[i] << std::endl;
            return 1;
        }
    }

    if (port == -1) {
        std::cout << "Missing port" << std::endl;
        return 1;
    }

    client.create_connection(port,host);

    // IMPLEMENT TRAFFIC LIGHT CONTROLLER HERE
    
    int clock_I_IK=0;
    int clock_K_IK=0;
    int clock_IK_LJ=0;
    int clock_L_LJ=0;
    int clock_G_GD=0;
    int clock_IK_GD=0;
    int clock_GD_FH=0;
    int clock_LJ_FH=0;
    int clock_A_AC=0;
    int clock_E_AC=0;

    int queue_I_IK=0;
    int queue_K_IK=0;
    int queue_IK_LJ=0;
    int queue_LK_LJ=0;
    int queue_L_LJ=0;
    int queue_G_GD=0;
    int queue_IK_GD=0;
    int queue_GD_FH=0;
    int queue_LJ_FH=0;
    int queue_A_AC=0;
    int queue_E_AC=0;

    int sum1_I_IK_0 = 0;
    int sum2_I_IK_0 = 0;
    int sum1_I_IK_1 = 0;
    int sum2_I_IK_1 = 0;
    int sum1_K_IK=0;
    int sum2_K_IK=0;
    int sum1_IK_LJ_0 = 0;
    int sum2_IK_LJ_0 = 0;
    int sum1_IK_LJ_1 = 0;
    int sum2_IK_LJ_1 = 0;
    int sum1_L_LJ = 0;
    int sum2_L_LJ = 0;
    int sum1_G_GD_0 = 0;
    int sum2_G_GD_0 = 0;
    int sum1_G_GD_1 = 0;
    int sum2_G_GD_1 = 0;
    int sum1_IK_GD=0;
    int sum2_IK_GD=0;
    int sum1_GD_FH_0 = 0;
    int sum2_GD_FH_0 = 0;
    int sum1_GD_FH_1 = 0;
    int sum2_GD_FH_1 = 0;
    int sum1_LJ_FH = 0;
    int sum2_LJ_FH = 0;
    int sum1_A_AC_0 = 0;
    int sum2_A_AC_0 = 0;
    int sum1_A_AC_1 = 0;
    int sum2_A_AC_1 = 0;
    int sum1_E_AC_0 = 0;
    int sum2_E_AC_0 = 0;
    int sum1_E_AC_1 = 0;
    int sum2_E_AC_1 = 0;

    std::string id_V1="";
    std::string id_V2="";
    std::string id_V3="";
    std::string id_V4="";
    std::string id_V5="";
    std::string id_V6="";
    std::string id_V7="";
    std::string id_V8="";
    std::string id_V9="";
    std::string id_V10="";
    
    std::string id_U1="";
    std::string id_U2="";
    std::string id_U3="";
    std::string id_U4="";
    std::string id_U5="";
    std::string id_U6="";
    std::string id_U7="";
    std::string id_U8="";
    std::string id_U9="";
    std::string id_U10="";

    std::string id_T1="";
    std::string id_T2="";
    std::string id_T3="";
    std::string id_T4="";
    std::string id_T5="";
    std::string id_T6="";
    std::string id_S1="";
    std::string id_S2="";
    std::string id_S3="";
    std::string id_S4="";
    std::string id_S5="";
    std::string id_S6="";
   
    std::string state_IK = "";
    std::string state_LJ = "";
    std::string state_GD = "";
    std::string state_FH = "";
    std::string state_AC = "";

    int step = 0;

    int total_latency=0;
    int car_number=0;
    int car_latency=0;
    int truck_number=0;
    int truck_latency=0;

    int minExpectedNumber = client.simulation.getMinExpectedNumber();
    while (true)
      {
        //The first controller IK ~~~~~~~~~~~~~~~~~~~~~~~~
        //First we compute the queue length of West-East direction
	int queue_I_IK_0, queue_I_IK_1;
        vehicle_number( "V1", "U1", id_V1, id_U1, sum1_I_IK_0, sum2_I_IK_0, queue_I_IK_0 );
        vehicle_number( "V2", "U2", id_V2, id_U2, sum1_I_IK_1, sum2_I_IK_1, queue_I_IK_1 );
        queue_I_IK = queue_I_IK_0 + queue_I_IK_1;
	std::cout << "EW Q len: " << queue_I_IK << std::endl;
        //Then we compute the length in North-South direction
        vehicle_number( "T1", "S1", id_T1, id_S1, sum1_K_IK, sum2_K_IK, queue_K_IK );
        //Now we compute the clock value of the traffic lights(value k in the paper)
	std::string tl_state_IK;
        clock_value ("IK", clock_I_IK, clock_K_IK, tl_state_IK);
        //Now we need to design the traffic light control logic
	controller( "IK", tl_state_IK, queue_I_IK, queue_K_IK, clock_I_IK, clock_K_IK,
		    Light_Min_IK, Light_Max_IK, s_WE_IK, s_NS_IK );
        

        //The second controller LJ ~~~~~~~~~~~~~~~~~~~~~~~~~~
        //First we compute the queue length of West-East direction
	int queue_IK_LJ_0, queue_IK_LJ_1;
        vehicle_number( "V3", "U3", id_V3, id_U3, sum1_IK_LJ_0, sum2_IK_LJ_0, queue_IK_LJ_0 );
        vehicle_number("V4", "U4", id_V4, id_U4, sum1_IK_LJ_1, sum2_IK_LJ_1, queue_IK_LJ_1 );
        queue_IK_LJ = queue_IK_LJ_0 + queue_IK_LJ_1;
        //Then we compute the length in North-South direction
        vehicle_number( "T3", "S3", id_T3, id_S3, sum1_L_LJ, sum2_L_LJ, queue_L_LJ );
        //Now we compute the clock value of the traffic lights(value k in the paper)
	std::string tl_state_LJ;
        clock_value( "LJ", clock_IK_LJ, clock_L_LJ, tl_state_LJ );
        //Now we need to design the traffic light control logic
        controller( "LJ", tl_state_LJ, queue_IK_LJ, queue_L_LJ, clock_IK_LJ, clock_L_LJ,
		    Light_Min_LJ, Light_Max_LJ, s_WE_LJ, s_NS_LJ );
      

        //The 3rd controller GD ~~~~~~~~~~~~~~~~~~~~~~~~
        //First we compute the queue length of West-East direction
	int queue_G_GD_0, queue_G_GD_1;
        vehicle_number("V5", "U5", id_V5, id_U5, sum1_G_GD_0, sum2_G_GD_0, queue_G_GD_0 );
        vehicle_number("V6", "U6", id_V6, id_U6, sum1_G_GD_1, sum2_G_GD_1, queue_G_GD_1 );
	queue_G_GD = queue_G_GD_0 + queue_G_GD_1;
        //Then we compute the length in North-South direction
	vehicle_number("T2", "S2", id_T2, id_S2, sum1_IK_GD, sum2_IK_GD, queue_IK_GD );
        //Now we compute the clock value of the traffic lights(value k in the paper)
	std::string tl_state_GD;
        clock_value ("GD", clock_G_GD, clock_IK_GD, tl_state_GD);
        //Now we need to design the traffic light control logic
        controller( "GD", tl_state_GD, queue_G_GD, queue_IK_GD, clock_G_GD, clock_IK_GD,
		    Light_Min_GD, Light_Max_GD, s_WE_GD, s_NS_GD );


        //The 4th controller FH ~~~~~~~~~~~~~~~~~~~~~~~~
        //First we compute the queue length of West-East direction
	int queue_GD_FH_0, queue_GD_FH_1;
        vehicle_number("V7", "U7", id_V7, id_U7, sum1_GD_FH_0, sum2_GD_FH_0, queue_GD_FH_0 );
	vehicle_number("V8", "U8", id_V8, id_U8, sum1_GD_FH_1, sum2_GD_FH_1, queue_GD_FH_1 );
        queue_GD_FH = queue_GD_FH_0 + queue_GD_FH_1;
        //Then we compute the length in North-South direction
        vehicle_number("T4", "S4", id_T4, id_S4, sum1_LJ_FH, sum2_LJ_FH, queue_LJ_FH );
        //Now we compute the clock value of the traffic lights(value k in the paper)
	std::string tl_state_FH;
        clock_value ("FH", clock_GD_FH, clock_LJ_FH, tl_state_FH);
        //Now we need to design the traffic light control logic
        controller ("FH", tl_state_FH, queue_GD_FH, queue_LJ_FH, clock_GD_FH, clock_LJ_FH,
		    Light_Min_FH, Light_Max_FH, s_WE_FH, s_NS_FH );

        //The 5th controller AC ~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //First we compute the queue length of West-East direction
	int queue_A_AC_0, queue_A_AC_1;
        vehicle_number("V9", "U9", id_V9, id_U9, sum1_A_AC_0, sum2_A_AC_0, queue_A_AC_0 );
        vehicle_number("V10", "U10", id_V10, id_U10, sum1_A_AC_1, sum2_A_AC_1, queue_A_AC_1 );
        queue_A_AC = queue_A_AC_0 + queue_A_AC_1;
        //Then we compute the length in North-South direction
	int queue_E_AC_0, queue_E_AC_1;
        vehicle_number("T5", "S5", id_T5, id_S5, sum1_E_AC_0, sum2_E_AC_0, queue_E_AC_0 );
        vehicle_number("T6", "S6", id_T6, id_S6, sum1_E_AC_1, sum2_E_AC_1, queue_E_AC_1 );
        queue_E_AC = queue_E_AC_0 + queue_E_AC_1;
        //Now we compute the clock value of the traffic lights(value k in the paper)
	std::string tl_state_AC;
        clock_value1 ("AC", clock_A_AC, clock_E_AC, tl_state_AC );
        //Now we need to design the traffic light control logic
        controller1 ("AC", tl_state_IK, queue_A_AC, queue_E_AC, clock_A_AC, clock_E_AC,
		     Light_Min_AC, Light_Max_AC, s_WE_AC, s_NS_AC );

	int total_number = client.simulation.getArrivedNumber();
        if (total_number>=1)
	  {
	    std::vector<std::string> current_list = client.simulation.getArrivedIDList();
	    for (std::vector<std::string>::iterator it = current_list.begin(); it != current_list.end(); ++it)
	      {
		std::string v_id = it->substr(0,8);
		if (!v_id.compare("flowsI2J") || !v_id.compare("flowsG2H") || v_id.compare("flowsA2B"))
		  {
		    car_number++;
		    car_latency += step;
		  }
		else
		  {
		    truck_number++;
		    truck_latency += step;
		  }
	      }
	  }
        step += 1;
	minExpectedNumber = client.simulation.getMinExpectedNumber();
	usleep(sleep_us);
      }
    float average_car_latency = 1.0 * float(car_latency) / float(car_number);
    float average_truck_latency = 1.0 * float(truck_latency) / float(truck_number);
    std::cout << "Step: " << step << std::endl;
    std::cout << "Car number, Car latency: " << car_number << ", " << average_car_latency << std::endl;
    std::cout << "Truck number, Truck latency: " << truck_number << ", " << average_truck_latency << std::endl;
    for (int i=1; i<11; i++)
      {
	float average_latency = (float(car_latency) + float(i)*float(truck_latency)) /
	  (float(car_number) + float(i)*float(truck_number));
	std::cout << "Average Latency: " << average_latency << std::endl;
      }
    client.close_connection();
    return 0;
}
