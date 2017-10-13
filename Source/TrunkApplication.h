/*
 * TrunkApplication.h
 *
 *  Created on: Oct 11, 2017
 *      Author: root
 */

#ifndef _TRUNK_APPLICATION_H_
#define _TRUNK_APPLICATION_H_

#include <Application/Application.h>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <Service/ServiceType/ServiceMap.h>
#include <DataSet/DataType/PoseStamped.h>
#include <Service/Client.h>
#include <DataSet/Publisher.h>

#include <iostream>
#include <string>

namespace NS_Trunk
{

  class TrunkApplication: public Application
  {
  public:
    TrunkApplication();
    virtual ~TrunkApplication();

  private:
    std::string map_path_;
    double map_update_rate_;

    std::string app_ip_addr_;
    int app_ip_map_port_;
    int app_ip_goal_port_;

  private:
    NS_Service::Client< NS_ServiceType::ServiceMap >* map_cli;
    NS_DataSet::Publisher< NS_DataType::PoseStamped >* goal_pub;

    boost::thread map_generate_thread;

    boost::thread get_goal_thread;

  private:
    void loadParameters();

    void mapGenerateLoop();

    void getGoalLoop();

    bool sendMap(std::string map_path, std::string app_ip_addr, int app_ip_port);

  public:
    virtual void run();
    virtual void quit();
  };

} /* namespace NS_Trunk */

#endif /* TRUNKAPPLICATION_H_ */
