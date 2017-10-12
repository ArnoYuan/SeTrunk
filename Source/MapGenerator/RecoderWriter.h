
#ifndef __RECODER_WRITER_H__
#define __RECODER_WRITER_H__

#include "CdxWriter.h"
#define  MAXROWS  720
#define MAXCOLS 1280
typedef enum CDX_FILE_MODE
{
    FD_FILE_MODE,
    FP_FILE_MODE
}CDX_FILE_MODE;

typedef struct RecoderWriter RecoderWriterT;
struct RecoderWriter
{
    CdxWriterT cdx_writer;
    int        file_mode;
    int        fd_file;
    FILE       *fp_file;
    char       *file_path;
};

int RWOpen(CdxWriterT *writer);
int RWClose(CdxWriterT *writer);

#endif
