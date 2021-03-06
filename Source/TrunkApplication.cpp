/*
 * TrunkApplication.cpp
 *
 *  Created on: Oct 11, 2017
 *      Author: root
 */

#include "TrunkApplication.h"
#include <Parameter/Parameter.h>
#include <Transform/DataTypes.h>
#include "MapGenerator/MapGenerator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <boost/filesystem.hpp>
#include <iostream>
#include <string>


namespace NS_Trunk
{
  TrunkApplication::TrunkApplication()
  {
    map_cli = new NS_Service::Client< NS_ServiceType::ServiceMap >("MAP");

    goal_pub = new NS_DataSet::Publisher< NS_DataType::PoseStamped >("GOAL");

    goal_sub = new NS_DataSet::Subscriber< NS_DataType::Pose >("GOAL_FROM_APP",
                                                               boost::bind(&TrunkApplication::goalCallback, this, _1));

    map_file_srv = new NS_Service::Server< NS_ServiceType::ServiceString >("MAP_FILE_FOR_APP",
                                                                           boost::bind(&TrunkApplication::mapFileService, this, _1));

    global_target_cli = new NS_Service::Client<NS_DataType::PoseStamped>("GLOBAL_TARGET");

    plan_cli = new NS_Service::Client<std::vector<NS_DataType::PoseStamped> >("PLAN");

    current_pose_cli = new NS_Service::Client<NS_DataType::PoseStamped>("CURRENT_POSE");
  }

  TrunkApplication::~TrunkApplication()
  {
    if(running)
    {
      running = false;
      map_generate_thread.join ();
    }

    delete map_cli;
    delete goal_pub;
    delete goal_sub;
    delete map_file_srv;
    delete global_target_cli;
    delete plan_cli;
    delete current_pose_cli;
  }

  void TrunkApplication::loadParameters()
  {
    NS_NaviCommon::Parameter parameter;
    parameter.loadConfigurationFile("trunk.xml");

    map_path_ = parameter.getParameter("map_path", "/tmp/gmap.jpg");
    map_update_rate_ = parameter.getParameter("map_update_rate", 1.0f);
  }

  void TrunkApplication::goalCallback(NS_DataType::Pose& goal)
  {
    console.message("Get goal from service! x=%f, y=%f, z=%f, w=%f.",
                    goal.position.x, goal.position.y,
                    goal.orientation.z, goal.orientation.w);
    /*
    NS_DataType::PoseStamped set_goal;
    set_goal.pose = goal;
    goal_pub->publish(set_goal);
    */
    goal_iss.runActionProcess(goal.position.x, goal.position.y, NS_Transform::getYaw(goal.orientation));
  }

  void TrunkApplication::mapFileService(NS_ServiceType::ServiceString& filename)
  {
    console.message("Map file %s ready to be download.", map_file_name.c_str());
    boost::mutex::scoped_lock locker(map_file_lock);
    if(boost::filesystem::exists(boost::filesystem::path(map_file_name)))
    {
      filename.text = map_file_name;
      filename.result = true;
    }
    else
    {
      filename.result = false;
    }
  }

  void TrunkApplication::mapGenerateLoop()
  {
    NS_NaviCommon::Rate rate(map_update_rate_);
    int save_index = 0;
    while(running)
    {
      NS_ServiceType::ServiceMap map;
      if(map_cli->call(map))
      {
        if(map.result)
        {
          NS_NaviCommon::MapGenerator::saveMapInPGM(map.map.data, map.map.info.height, map.map.info.width, map_path_);
          /*
          std::vector< char > yuv_data;
          int yuv_height, yuv_width;
          NS_NaviCommon::MapGenerator::mapToYuv(map.map.data, yuv_data, map.map.info.height, map.map.info.width, yuv_height, yuv_width);
          NS_NaviCommon::MapGenerator::compressYuvToJpeg(yuv_data, map.map.info.height, map.map.info.width, map_path_);

          boost::mutex::scoped_lock locker(map_file_lock);

          std::stringstream map_file_name_str;
          map_file_name_str << "/tmp/gmap_";
          map_file_name_str << save_index++;
          map_file_name_str << ".jpg";
          map_file_name = map_file_name_str.str();

          boost::filesystem::copy_file(boost::filesystem::path(map_path_), boost::filesystem::path(map_file_name), boost::filesystem::copy_option::fail_if_exists);
          */

          console.debug("Map got, Begin to get plan!\n");
          std::vector<NS_DataType::PoseStamped> plan;
          plan_cli->call(plan);
          if(plan.size() != 0)
          {
            for(int i = 0; i < plan.size(); ++i)
            {
                double x = (plan[i].pose.position.x + 25) / 0.1 - 0.5;
                double y = (plan[i].pose.position.y + 25) / 0.1 - 0.5;
                NS_NaviCommon::MapGenerator::addPathPointInMap(
                        map.map.data, map.map.info.height,
                        map.map.info.width, x, y);
            }
          }
          else
          {
            console.debug("Plan is null\n");
          }

          NS_DataType::PoseStamped current_pose;
          current_pose_cli->call(current_pose);
          if(current_pose.pose.position.x != 0)
          {
            double x = (current_pose.pose.position.x + 25) / 0.1 - 0.5;
            double y = (current_pose.pose.position.y + 25) / 0.1 - 0.5;

            NS_NaviCommon::MapGenerator::addRobotPoseInMap(map.map.data,
                    map.map.info.height, map.map.info.width, x, y);
          }
          else
          {
            console.debug("current_pose is null\n");
          }

          NS_DataType::PoseStamped target;
          global_target_cli->call(target);
          if(target.pose.position.x != 0)
          {
            double x = (target.pose.position.x + 25) / 0.1 - 0.5;
            double y = (target.pose.position.y + 25) / 0.1 - 0.5;
            NS_NaviCommon::MapGenerator::addTargetInMap(map.map.data,
                    map.map.info.height, map.map.info.width, x, y);
          }
          else
          {
            console.debug("global_target is null\n");
          }

          //NS_NaviCommon::MapGenerator::saveMapInPGM(map.map.data, map.map.info.height, map.map.info.width, map_path_);
          std::vector<char> yuv_data;
          int yuv_height, yuv_width;
          NS_NaviCommon::MapGenerator::mapToYuv(map.map.data, yuv_data,
                  map.map.info.height, map.map.info.width, yuv_height,
                  yuv_width);
          NS_NaviCommon::MapGenerator::compressYuvToJpeg(yuv_data,
                  map.map.info.height, map.map.info.width, map_path_);

          boost::mutex::scoped_lock locker(map_file_lock);

          std::stringstream map_file_name_str;
          map_file_name_str << "/tmp/gmap_";
          map_file_name_str << save_index++;
          map_file_name_str << ".jpg";
          map_file_name = map_file_name_str.str();

          boost::filesystem::copy_file(boost::filesystem::path(map_path_),
                  boost::filesystem::path(map_file_name),
                  boost::filesystem::copy_option::fail_if_exists);

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
