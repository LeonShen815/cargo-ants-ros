#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>
#include <stdlib.h>
#include <iostream>
#include "ros/ros.h"
#include "cargo_ants_msgs/ReferenceTrajectory.h"
#include <stdlib.h>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>

using namespace cargo_ants_msgs;
using namespace std;

int resolvehelper(const char* hostname, int family, const char* service, sockaddr_storage* pAddr)
{
    int result;
    addrinfo* result_list = NULL;
    addrinfo hints = {};
    hints.ai_family = family;
    hints.ai_socktype = SOCK_DGRAM; // without this flag, getaddrinfo will return 3x the number of addresses (one for each socket type).
    result = getaddrinfo(hostname, service, &hints, &result_list);
    if (result == 0)
    {
        //ASSERT(result_list->ai_addrlen <= sizeof(sockaddr_in));
        memcpy(pAddr, result_list->ai_addr, result_list->ai_addrlen);
        freeaddrinfo(result_list);
    }

    return result;
}

static int so_broadcast = 1;
static void reftrj_cb (ReferenceTrajectory::ConstPtr const & rosmsg)

{
    int result = 0;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if( sock == -1 ){
	    std::cout << "error: socket" << sock << std::endl;
    }
    //allow broadcasts
    result = setsockopt( sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast) );
    if( result == -1 ){
	    std::cout << "error: broadcast" << result << std::endl;
    }

    char szIP[100];
    sockaddr_in addrListen = {}; // zero-int, sin_port is 0, which picks a random port for bind.
    addrListen.sin_family = AF_INET;
    result = bind(sock, (sockaddr*)&addrListen, sizeof(addrListen));
    if (result == -1)
    {
       int lasterror = errno;
       std::cout << "error: " << lasterror;
       exit(1);
    }
    sockaddr_storage addrDest = {};
    result = resolvehelper(/*"10.10.172.255"*/ "192.168.196.255", AF_INET, "30000", &addrDest );//"127.0.0.1", AF_INET, "30000", &addrDest); //"169.254.145.80", AF_INET, "30000", &addrDest);
    if (result != 0)
    {
       int lasterror = errno;
       std::cout << "error: " << lasterror;
       exit(1);
    }

    double N = rosmsg->points.size();

    const int NF = 11;
    int x = N*NF;
    double array[x];

    for (int ii = 0; ii < rosmsg->points.size(); ii++){
      // 	array[ii * NF + 0] = rosmsg->start.sec;
	//std::cout << array[ii * NF + 0] << std::endl;
      	array[ii * NF + 1] = rosmsg->dt;
	//std::cout << array[ii * NF + 1] << std::endl;
        array[ii * NF + 2] = rosmsg->points[ii].xx;
	//std::cout << array[ii * NF + 2] << std::endl;
        array[ii * NF + 3] = rosmsg->points[ii].yy;
	//std::cout << array[ii * NF + 3] << std::endl;
	array[ii * NF + 4] = rosmsg->points[ii].th;
	//std::cout << array[ii * NF + 4] << std::endl;
	array[ii * NF + 5] = rosmsg->points[ii].xd;
	//std::cout << array[ii * NF + 5] << std::endl;
	array[ii * NF + 6] = rosmsg->points[ii].yd;
	//std::cout << array[ii * NF + 6] << std::endl;
	array[ii * NF + 7] = rosmsg->points[ii].thd;
	//std::cout << array[ii * NF + 7] << std::endl;
	array[ii * NF + 8] = rosmsg->points[ii].xdd;
	//std::cout << array[ii * NF + 8] << std::endl;
	array[ii * NF + 9] = rosmsg->points[ii].ydd;
	//std::cout << array[ii * NF + 9] << std::endl;
	array[ii * NF + 10] = rosmsg->points[ii].thdd;
	//std::cout << array[ii * NF + 10] << std::endl;
	//std::cout << ".............................." << std::endl;

    }
    
    //double array1[648] = {1,2,3,4,5,6,7,8,9,0,1,2,3,4,53,212,72,55,89,6,6,3,4,5,6,7,8,9,12,14,23}; 
    //  double const *msg = array;
    //  size_t msg_length = sizeof(array);

      //     for (int i=0; i<10000000; i++){
    //   result = sendto(sock, time, sizeof(time), 0, (sockaddr*)&addrDest, sizeof(addrDest));
    //std::cout << result << "  bytes of integer sent.. It has time date" << std::endl;
     result = sendto(sock, array, sizeof(array), 0, (sockaddr*)&addrDest, sizeof(addrDest));
 // }
    std::cout << N  << " is the size of rosmsg" << std::endl;
    std::cout << result << " bytes of doubles sent.. It has trajectory info" << std::endl;
    ROS_INFO("PING");
    
}

int main(int argc, char **argv)
{
   
    ros::init (argc, argv, "subscriber");
    ros::NodeHandle node;
    ros::Subscriber blah (node.subscribe ("trajectory", 1000, reftrj_cb));
  
    cout << "subscribed to udp, calling ros::spin\n";
    
    ros::spin();
 /*   while (ros::ok()) {
    ros::spinOnce();
    cout << "." << flush;
    usleep (2000000);
    }
   */    
    return 0;
}
