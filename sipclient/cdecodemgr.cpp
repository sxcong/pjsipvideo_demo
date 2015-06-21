
#include "stdafx.h"
#include "pthread.h"
#include "cdecodemgr.h"
#include "src/pjsua_app.h"

static int sws_flags = SWS_BICUBIC;

static int lockmgr(void **mtx, enum AVLockOp op)
{
    pthread_mutex_t**m = (pthread_mutex_t**)mtx;
    int ret = 0;
    switch(op)
     {
        case AV_LOCK_CREATE:
            *m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
            ret = pthread_mutex_init(*m, NULL);
            break;
        case AV_LOCK_OBTAIN:
            ret = pthread_mutex_lock(*m);
            break;
        case AV_LOCK_RELEASE:
            ret = pthread_mutex_unlock(*m);
            break;
        case AV_LOCK_DESTROY:
            ret = pthread_mutex_destroy(*m);
            free(*m);
            break;
     }
     return ret;
}

CFfmpegDecode::CFfmpegDecode()
{
    m_bInit = false;
    img_convert_ctx = NULL;
}

CFfmpegDecode::~CFfmpegDecode()
{
    closeDecoder();
    av_lockmgr_register(NULL);
}

int CFfmpegDecode::initFFMPEG()
{
    //m_state = RC_STATE_INIT;
    avcodec_register_all();
    av_register_all();
    //avformat_network_init();

    if (av_lockmgr_register(lockmgr))
    {
       // m_state = RC_STATE_INIT_ERROR;
       // return -1;
    }
    return 0;
}
int CFfmpegDecode::openDecoder(int width, int height,CDecodeCB* pCB)
{
	//pjcall_thread_register();

    m_nWidth = width;
    m_nHeight = height;
    m_pCB = pCB;
    if (m_bInit)
        return -1;
    decode_codec = avcodec_find_decoder(CODEC_ID_H264);
    if (!decode_codec)
    {
        fprintf(stderr, "codec not found\n");
        return -2;
    }

    decode_c= avcodec_alloc_context3(decode_codec);
    decode_c->codec_id= CODEC_ID_H264;
    decode_c->codec_type = AVMEDIA_TYPE_VIDEO;
    decode_c->pix_fmt = PIX_FMT_YUV420P;

    decode_picture= av_frame_alloc();

    if (avcodec_open2(decode_c, decode_codec, NULL) < 0)
    {
     //  fprintf(stderr, "could not open codec\n");
       return -3;
    }
    m_bInit = true;
    return 0;
}

int CFfmpegDecode::closeDecoder()
{
    if(decode_c)
    {
        avcodec_close(decode_c);
        av_free(decode_c);
    }
    if(decode_picture)
        av_free(decode_picture);

    m_bInit = false;
    return 0;
}

int CFfmpegDecode::decode_rtsp_frame(uint8_t* input,int nLen,bool bWaitIFrame /*= false*/)
{
	//pjcall_thread_register();
    if(!m_bInit)
        return -1;

    if(input == NULL || nLen <= 0)
        return -2;


    try{
        int got_picture;
        int size = nLen;


        AVPacket avpkt;
        av_init_packet(&avpkt);
        avpkt.size = size;
        avpkt.data = input;

        //while (avpkt.size > 0)
        {

            int len = avcodec_decode_video2(decode_c, decode_picture, &got_picture, &avpkt);

            if(len <= -1)
            {
                return -3;
            }

            if (got_picture)
            {
                int w = decode_c->width;
                int h = decode_c->height;
                int numBytes=avpicture_get_size(PIX_FMT_BGR24, w,h);
                uint8_t * buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

                AVFrame *pFrameRGB = av_frame_alloc();
                avpicture_fill((AVPicture *)pFrameRGB, buffer,PIX_FMT_BGR24,  w, h);//PIX_FMT_RGB24

                img_convert_ctx = sws_getCachedContext(img_convert_ctx,
                                            w, h, (PixelFormat)(decode_picture->format), w, h,PIX_FMT_BGR24, sws_flags, NULL, NULL, NULL);
                if (img_convert_ctx == NULL)
                {
                    fprintf(stderr, "Cannot initialize the conversion context\n");
                    //exit(1);
                    return -4;
                }
                sws_scale(img_convert_ctx, decode_picture->data, decode_picture->linesize,
                    0, h, pFrameRGB->data, pFrameRGB->linesize);

                if (m_pCB)
                {
                    m_pCB->videoCB(w, h, pFrameRGB->data[0], numBytes*sizeof(uint8_t));
                }

                av_free(buffer);
                av_free(pFrameRGB);
                return 0;

                if (avpkt.data)
                {
                    avpkt.size -= len;
                    avpkt.data += len;
                }
            }
            else
            {
                return -5;
            }
            //return 0;
        }

        //return 0;


    }
    catch(...)
    {
    }
    return -6;
}
int CFfmpegDecode::PraseMetadata(const unsigned char* pData, int size,MP4ENC_Metadata &metadata)
{
    if(pData == NULL || size<4)
    {
        return -1;
    }
    MP4ENC_NaluUnit nalu;
    int pos = 0;
    bool bRet1 = false,bRet2 = false;
    while (int len = ReadOneNaluFromBuf(pData,size,pos,nalu))
    {
        if(nalu.type == 0x07)
        {
            memcpy(metadata.Sps,nalu.data,nalu.size);
            metadata.nSpsLen = nalu.size;
            bRet1 = true;

            //h264_decode_sps(pData, size, m_nWidth, m_nHeight);
        }
        else if((nalu.type == 0x08))
        {
            memcpy(metadata.Pps,nalu.data,nalu.size);
            metadata.nPpsLen = nalu.size;
            bRet2 = true;
        }
        pos += len;
    }
    if (!bRet1)
        return -2;
    if (!bRet2)
        return -3;


    return 0;
}
int CFfmpegDecode::ReadOneNaluFromBuf(const unsigned char *buffer,unsigned int nBufferSize,unsigned int offSet,MP4ENC_NaluUnit &nalu)
{
    int i = offSet;
    while(i<nBufferSize)
    {
        if(buffer[i++] == 0x00 &&
            buffer[i++] == 0x00 &&
            buffer[i++] == 0x00 &&
            buffer[i++] == 0x01
            )
        {
            int pos = i;
            while (pos<nBufferSize)
            {
                if(buffer[pos++] == 0x00 &&
                    buffer[pos++] == 0x00 &&
                    buffer[pos++] == 0x00 &&
                    buffer[pos++] == 0x01
                    )
                {
                    break;
                }
            }
            if(pos == nBufferSize)
            {
                nalu.size = pos-i;
            }
            else
            {
                nalu.size = (pos-4)-i;
            }

            nalu.type = buffer[i]&0x1f;
            nalu.data =(unsigned char*)&buffer[i];
            return (nalu.size+i-offSet);
        }
    }
    return 0;
}
