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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

namespace NS_Trunk
{
  typedef struct
  {
    float x;
    float y;
    float theta;
  }GoalType;

  TrunkApplication::TrunkApplication()
  {
    map_cli = new NS_Service::Client< NS_ServiceType::ServiceMap >("MAP");
    goal_pub = new NS_DataSet::Publisher< NS_DataType::PoseStamped >("GOAL");
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
  }

  void TrunkApplication::loadParameters()
  {
    NS_NaviCommon::Parameter parameter;
    parameter.loadConfigurationFile("trunk.xml");

    map_path_ = parameter.getParameter("map_path", "/tmp/gmap.jpg");
    map_update_rate_ = parameter.getParameter("map_update_rate", 1.0f);
    app_ip_addr_ = parameter.getParameter("app_ip_addr", "127.0.0.1");
    app_ip_map_port_ = parameter.getParameter("app_map_port", 12345);
    app_ip_goal_port_ = parameter.getParameter("app_goal_port", 12346);
  }

  bool TrunkApplication::sendMap(std::string map_path, std::string app_ip_addr,
                                 int app_ip_port)
  {
    int sock_id;
    FILE* fp;
    char buf[2048];
    int read_len;
    int send_len;
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(app_ip_port);
    inet_pton(AF_INET, app_ip_addr.c_str(), &serv_addr.sin_addr);

    //connect socket
    if((sock_id = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      console.warning("Create socket failed %d\n", sock_id);
      return false;
    }

    int i_ret;
    int reuseflag = 1;
    setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseflag,
               sizeof(int)); //set reused

    struct linger clsflag;
    clsflag.l_onoff = 1;
    clsflag.l_linger = 2000;
    setsockopt(sock_id, SOL_SOCKET, SO_LINGER, (const struct linger*)&clsflag,
               sizeof(struct linger)); //set closed

    i_ret = connect(sock_id, (struct sockaddr *)&serv_addr,
                    sizeof(struct sockaddr));

    if(i_ret == -1)
    {
      console.warning("Connect socket failed\n");
      close(sock_id);
      return false;
    }

    if((fp = fopen(map_path.c_str(), "r")) == NULL)
    {
      console.warning("Open file failed\n");
      close(sock_id);
      return false;
    }

    bzero(buf, sizeof(buf));

    while((read_len = fread(buf, sizeof(char), sizeof(buf), fp)) > 0)
    {
      send_len = send(sock_id, buf, read_len, 0);

      if(send_len < 0)
      {
        console.warning("Send file failed\n");
        fclose(fp);
        close(sock_id);
        return false;
      }

      bzero(buf, sizeof(buf));
    }
    fclose(fp);
    close(sock_id);
    return true;
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
          std::vector< char > yuv_data;
          int yuv_height, yuv_width;
          NS_NaviCommon::MapGenerator::mapToYuv(map.map.data, yuv_data, map.map.info.height, map.map.info.width, yuv_height, yuv_width);
          NS_NaviCommon::MapGenerator::compressYuvToJpeg(yuv_data, map.map.info.height, map.map.info.width, map_path_);
          sendMap(map_path_, app_ip_addr_, app_ip_map_port_);
        }
      }
      rate.sleep();
    }
  }

  void TrunkApplication::getGoalLoop()
  {
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(app_ip_goal_port_);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(server_addr.sin_zero), 8);

    int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock_fd == -1)
    {
      console.warning("Create goal socket failure!");
      return;
    }
    //绑定socket
    int bind_result = bind(server_sock_fd, (struct sockaddr *)&server_addr,
                           sizeof(server_addr));
    if(bind_result == -1)
    {
      console.warning("Set goal port failure!");
      return;
    }
    //listen
    if(listen(server_sock_fd, 3) == -1)
    {
      console.warning("Listen goal port error!");
      return;
    }

    fd_set server_fd_set;

    struct timeval tv;

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while(running)
    {
      FD_ZERO(&server_fd_set);
      FD_SET(server_sock_fd, &server_fd_set);
      int ret = select(server_sock_fd + 1, &server_fd_set, NULL, NULL, &tv);
      if(ret > 0)
      {
        int client_sock_fd = 0;
        if(FD_ISSET(server_sock_fd, &server_fd_set))
        {
          struct sockaddr_in client_address;
          socklen_t address_len;
          int client_sock_fd = accept(server_sock_fd,
                                      (struct sockaddr *)&client_address,
                                      &address_len);
          if(client_sock_fd <= 0)
          {
            continue;
          }
        }

        if(client_sock_fd != 0 && FD_ISSET(client_sock_fd, &server_fd_set))
        {
          GoalType goal;
          bzero(&goal, sizeof(GoalType));

          size_t byte_num = recv(client_sock_fd, (unsigned char*)&goal,
                                 sizeof(GoalType), 0);
          if(byte_num > 0)
          {
            if(byte_num == sizeof(GoalType))
            {
              NS_DataType::PoseStamped pub_goal;
              pub_goal.pose.position.x = goal.x;
              pub_goal.pose.position.y = goal.y;
              pub_goal.pose.orientation = NS_Transform::createQuaternionMsgFromYaw(goal.theta);
              goal_pub->publish(pub_goal);
            }
          }
          else if(byte_num < 0)
          {
            close(client_sock_fd);
            client_sock_fd = 0;
          }
          else
          {
            FD_CLR(client_sock_fd, &server_fd_set);
            client_sock_fd = 0;
          }
        }
      }
    }
  }

  void TrunkApplication::run()
  {
    console.message ("Trunk application is running!");

    loadParameters();

    running = true;

    map_generate_thread = boost::thread (boost::bind (&TrunkApplication::mapGenerateLoop, this));
    get_goal_thread = boost::thread (boost::bind (&TrunkApplication::getGoalLoop, this));
  }

  void TrunkApplication::quit()
  {
    console.message ("Trunk application is quitting!");

    running = false;

    map_generate_thread.join ();
    get_goal_thread.join();
  }

} /* namespace NS_Trunk */
