/*
 * TrunkApplication.cpp
 *
 *  Created on: Oct 11, 2017
 *      Author: root
 */

#include "TrunkApplication.h"
#include <Parameter/Parameter.h>
#include "MapGenerator/MapGenerator.h"

namespace NS_Trunk
{

  TrunkApplication::TrunkApplication()
  {
    map_cli = new NS_Service::Client<NS_ServiceType::ServiceMap>("MAP");
  }

  TrunkApplication::~TrunkApplication()
  {
    if(running)
    {
      running = false;
      map_generate_thread.join ();
    }
    delete map_cli;
  }

  void TrunkApplication::loadParameters()
  {
    NS_NaviCommon::Parameter parameter;
    parameter.loadConfigurationFile("trunk.xml");

    map_path_ = parameter.getParameter("map_path", "/tmp/gmap.jpg");
    map_update_rate_ = parameter.getParameter("map_update_rate", 1.0f);
  }

  void TrunkApplication::mapGenerateLoop()
  {
    NS_NaviCommon::Rate rate(map_update_rate_);
    while(running)
    {
      NS_ServiceType::ServiceMap map;
      if(map_cli->call(map))
      {
        if(map.result)
        {
          //NS_NaviCommon::MapGenerator::saveMapInPGM(map.map.data, map.map.info.height, map.map.info.width, map_path_);
          std::vector< int > yuv_data;
          NS_NaviCommon::MapGenerator::pgmToYuv(map.map.data, yuv_data, map.map.info.height, map.map.info.width);
          NS_NaviCommon::MapGenerator::compressYuvToJpeg(yuv_data, map.map.info.height, map.map.info.width, map_path_);
        }
      }
      rate.sleep();
    }
  }

  void TrunkApplication::run()
  {
    console.message ("Trunk application is running!");

    loadParameters();

    running = true;

    map_generate_thread = boost::thread (boost::bind (&TrunkApplication::mapGenerateLoop, this));
  }

  void TrunkApplication::quit()
  {
    console.message ("Trunk application is quitting!");

    running = false;

    map_generate_thread.join ();
  }

} /* namespace NS_Trunk */
