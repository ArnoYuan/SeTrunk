#include <CdxQueue.h>
#include <AwPool.h>
#include <CdxBinary.h>
#include <CdxMuxer.h>
#include "memoryAdapter.h"
#include "awencoder.h"
#include <vector>
typedef struct DemoRecoderContext
{
    AwEncoder*       mAwEncoder;
    VideoEncodeConfig videoConfig;

    CdxMuxerT* pMuxer;
    CdxMuxerTypeT muxType;
    char pUrl[1024];
    CdxWriterT* pStream;
   
    unsigned char* extractDataBuff;
    unsigned int extractDataLength;

    pthread_t muxerThreadId ;
    pthread_t videoDataThreadId ;
    int exitFlag ;
    char* pAddrPhyY;
    char* pAddrPhyC;
    int   bUsed;
    FILE* fpSaveVideoFrame;
    CdxMuxerPacketT *packet;
    int hasPacket;

    std::vector< int > yuvData;

}__attribute__((packed))DemoRecoderContext;

