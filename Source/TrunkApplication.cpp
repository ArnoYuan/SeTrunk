/*
 * TrunkApplication.cpp
 *
 *  Created on: Oct 11, 2017
 *      Author: root
 */

#include "TrunkApplication.h"
#include <Parameter/Parameter.h>
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
    app_ip_addr_ = parameter.getParameter("app_ip_addr", "127.0.0.1");
    app_ip_port_ = parameter.getParameter("app_ip_port", 12345);
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
      printf("Create socket failed %d\n", sock_id);
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
      printf("Connect socket failed\n");
      close(sock_id);
      return false;
    }

    if((fp = fopen(map_path.c_str(), "r")) == NULL)
    {
      printf("Open file failed\n");
      close(sock_id);
      return false;
    }

    bzero(buf, sizeof(buf));

    while((read_len = fread(buf, sizeof(char), sizeof(buf), fp)) > 0)
    {
      send_len = send(sock_id, buf, read_len, 0);

      if(send_len < 0)
      {
        printf("Send file failed\n");
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
          std::vector< int > yuv_data;
          NS_NaviCommon::MapGenerator::pgmToYuv(map.map.data, yuv_data, map.map.info.height, map.map.info.width);
          NS_NaviCommon::MapGenerator::compressYuvToJpeg(yuv_data, map.map.info.height, map.map.info.width, map_path_);
          sendMap(map_path_, app_ip_addr_, app_ip_port_);
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
