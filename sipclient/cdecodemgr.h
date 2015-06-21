#ifndef CDECODEMGR_H
#define CDECODEMGR_H

/*
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif
*/




//#include "MyAPI.h"
//#include "const.h"
#include "stdint.h"

// NALU
typedef struct _MP4ENC_NaluUnit
{
    int type;
    int size;
    unsigned char *data;
}MP4ENC_NaluUnit;

typedef struct _MP4ENC_Metadata
{
    // video, must be h264 type
    unsigned int	nSpsLen;
    unsigned char	Sps[1024];
    unsigned int	nPpsLen;
    unsigned char	Pps[1024];

} MP4ENC_Metadata,*LPMP4ENC_Metadata;

class CDecodeCB
{
public:
    virtual void videoCB(int width, int height, uint8_t* buff, int len)=0;
    virtual void rtspDataCB(uint8_t* buff, int len)=0;
};

class CFfmpegDecode
{
public:
    CFfmpegDecode();
    ~CFfmpegDecode();
    int initFFMPEG();
    int openDecoder(int width, int height, CDecodeCB* pCB);
    int closeDecoder();
    int decode_rtsp_frame(uint8_t* input,int nLen,bool bWaitIFrame /*= false*/);

    CDecodeCB* m_pCB;

    static int PraseMetadata(const unsigned char* pData,int size,MP4ENC_Metadata &metadata);
    static int ReadOneNaluFromBuf(const unsigned char *buffer,unsigned int nBufferSize,unsigned int offSet,MP4ENC_NaluUnit &nalu);
private:
    bool m_bInit;
    AVCodec *decode_codec;
    AVCodecContext *decode_c;
    AVFrame *decode_picture;
    struct SwsContext *img_convert_ctx;

    int m_nWidth;
    int m_nHeight;
};

#endif // CDECODEMGR_H
