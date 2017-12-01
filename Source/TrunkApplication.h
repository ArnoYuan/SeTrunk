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
#include <Service/ServiceType/ServiceString.h>
#include <DataSet/DataType/PoseStamped.h>
#include <Service/Server.h>
#include <Service/Client.h>
#include <DataSet/Publisher.h>
#include <DataSet/Subscriber.h>
#include <Mission/Issuer.h>

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

  private:
    NS_Service::Client< NS_ServiceType::ServiceMap >* map_cli;
    NS_DataSet::Publisher< NS_DataType::PoseStamped >* goal_pub;
    NS_DataSet::Subscriber< NS_DataType::Pose >* goal_sub;
    NS_Service::Server< NS_ServiceType::ServiceString >* map_file_srv;
    NS_Service::Client<NS_DataType::PoseStamped>* global_target_cli;
    NS_Service::Client<std::vector< NS_DataType::PoseStamped> >* plan_cli;
    NS_Service::Client< NS_DataType::PoseStamped >* current_pose_cli;

    NS_Mission::Issuer goal_iss;

    boost::thread map_generate_thread;

    boost::mutex map_file_lock;
    std::string map_file_name;

  private:
    void loadParameters();

    void mapGenerateLoop();

    void goalCallback(NS_DataType::Pose& goal);

    void mapFileService(NS_ServiceType::ServiceString& filename);

  public:
    virtual void run();
    virtual void quit();
  };

} /* namespace NS_Trunk */

#endif /* TRUNKAPPLICATION_H_ */
