/*
 * MapGenerator.h
 *
 *  Created on: Aug 8, 2017
 *      Author: root
 */

#ifndef _MAPGENERATOR_H_
#define _MAPGENERATOR_H_

#include <vector>
#include <string>
#include "DemoRecoderContext.h"
#include "RecoderWriter.h"

namespace NS_NaviCommon
{

  class MapGenerator
  {
  public:
    static bool saveMapInPGM(std::vector< char > map_data,
                             int height, int width,
                             std::string pgm_file);

    static bool readMapFromPGM(std::string pgm_file,
                               std::vector< char >& map_data,
                               int& height, int& width);

    static bool addRobotPoseInMap(std::vector< char >& map_data,
                                  int map_height, int map_width,
                                  int robot_pose_x, int robot_pose_y);

    static void mapToYuv(std::vector< char >& pgm,
                         std::vector< char >& yuv,
                         int height, int width,
                         int& resize_height, int& resize_width);

    static bool compressYuvToJpeg(std::vector< char >& yuv,
                                  int height, int width,
                                  std::string jpeg_file);

  private:
    static bool lock(int fd);
    static bool unlock(int fd);

    static void* yuvInputThread(void* context);
    static void* muxerThread(void* context);

    static int onVideoDataEnc(void *context, CdxMuxerPacketT *buff);
    static void notifyForAwEncorder(void* context, int msg, void* param);
  };

} /* namespace NS_NaviCommon */

#endif /* MAPGENERATOR_MAPGENERATOR_H_ */
