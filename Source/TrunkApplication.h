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
#include <Service/Client.h>

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
    int app_ip_port_;

  private:
    NS_Service::Client<NS_ServiceType::ServiceMap>* map_cli;

    boost::thread map_generate_thread;

  private:
    void loadParameters();

    void mapGenerateLoop();

    bool sendMap(std::string map_path, std::string app_ip_addr, int app_ip_port);

  public:
    virtual void run();
    virtual void quit();
  };

} /* namespace NS_Trunk */

#endif /* TRUNKAPPLICATION_H_ */
