// MyAPI.cpp: implementation of the CMyAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MyAPI.h"
/*
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
*/
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMyAPI::CMyAPI()
{

}

CMyAPI::~CMyAPI()
{

}

int CMyAPI::BufQueueInput(StreamBufQueue_t *pBufQue,unsigned char* buf,UINT len)
{
	int ret = 0;
	if(pBufQue == NULL)
		return 0;
	
	EnterCriticalSection(&pBufQue->mutex);

	//printf("1\n");
	
	//printf("Buffer len is %d\n", StreamBufLen);

#if 1 //队列满不写？,会造成帧不连续
	if( (pBufQue->tail+1)%MAX_NUM_BUFFER == pBufQue->head )
	//queue full ,nothing to do;服务端一段时间没有读出数据，tail=head,清空队列，重新开始，
	//继续,故无需考虑队列满的问题。
	{
		ret = 0;
		//printf("full head = %d ,tail = %d\n",pBufQue->head,pBufQue->tail);
	}
	else
#endif
	{	
		
		memcpy(pBufQue->stream_buf[pBufQue->tail].p,(char *) buf,len);
		pBufQue->stream_buf[pBufQue->tail].len = len;
		
		pBufQue->stream_buf[pBufQue->tail].cur = pBufQue->stream_buf[pBufQue->tail].p;
		
		pBufQue->tail++;
		if (pBufQue->tail >= MAX_NUM_BUFFER)
			pBufQue->tail = 0;//0 ,????第一个空出 by luoqt
		
		
		ret = len; 
	}
	
	LeaveCriticalSection(&pBufQue->mutex);

	//printf("2\n");
	//printf("get one frame!\n");
	
	return ret;
	
}


int CMyAPI::BufQueueOutput(StreamBufQueue_t *pBufQue,unsigned char* buf,UINT len)
{
	
	int ret = 0;
	
	EnterCriticalSection(&pBufQue->mutex);
	
	if( (pBufQue->tail != pBufQue->head) &&
		(pBufQue->stream_buf[pBufQue->head].len != 0)
		)
	{
		ret = min(len,pBufQue->stream_buf[pBufQue->head].len);
		memcpy(buf, pBufQue->stream_buf[pBufQue->head].p, ret);

		pBufQue->head++;
		if(pBufQue->head >= MAX_NUM_BUFFER)
			pBufQue->head = 0;

		// OutputDebugString("输出数据！");
	}
	
	LeaveCriticalSection(&pBufQue->mutex);
	
	return ret;
	

}
int CMyAPI::BufQueueInit(StreamBufQueue_t *pBufQue,int streamType)//VIDEO_STREAM_BUFQUE)
{
	int i;

	memset(pBufQue,0,sizeof(StreamBufQueue_t));
	
	pBufQue->head = 0;
	if(streamType == VIDEO_STREAM_BUFQUE)//视频
		pBufQue->tail = 0;//???
	else
		pBufQue->tail = 0;
	
	pBufQue->type = streamType;	
	
	
	InitializeCriticalSection(&pBufQue->mutex);

	if(streamType == VIDEO_STREAM_BUFQUE)//视频
	{
		for (i = 0; i < MAX_NUM_BUFFER; i++) 
		{
			pBufQue->stream_buf[i].p = new char[MAX_LEN_BUFFER];			
			pBufQue->stream_buf[i].len = 0;//默认0
		}
	}	
	else//音频
	{
		for (i = 0; i < MAX_NUM_BUFFER; i++) 
		{
			pBufQue->stream_buf[i].p = new char[MAX_LEN_BUFFER_AUDIO];	//音频缓存减少，节省mem		
			pBufQue->stream_buf[i].len = 0;//默认0
		}
	}
	

	return 0;
}


int CMyAPI::BufQueueFree(StreamBufQueue_t *pBufQue)
{
	int i;
	if(!pBufQue)
		return -1;
	
	DeleteCriticalSection(&pBufQue->mutex);
	
	for (i = 0; i < MAX_NUM_BUFFER; i++)
	{
		if(pBufQue->stream_buf[i].p)
		{
			delete[] pBufQue->stream_buf[i].p;
			pBufQue->stream_buf[i].p=NULL;
		}		
	}

	return 0;

}

int CMyAPI::GetBufQueueSize(StreamBufQueue_t *pBufQue)
{

	int ret = 0;
	if(!pBufQue)
		return 0;	
		
	EnterCriticalSection(&pBufQue->mutex);

    ret = pBufQue->tail-pBufQue->head;
	if(ret<0)
		ret += MAX_NUM_BUFFER;

	LeaveCriticalSection(&pBufQue->mutex);
	
	return ret;

}


int CMyAPI::SetBufQueueSize(StreamBufQueue_t *pBufQue,UINT uSize)//设置当前buffer的大小，tail-head=？？？
{

	if(!pBufQue)
		return 0;	
	
	EnterCriticalSection(&pBufQue->mutex);
	
    pBufQue->tail = (pBufQue->head + uSize)%MAX_NUM_BUFFER;	
	
	LeaveCriticalSection(&pBufQue->mutex);
	
	return 0;
	
}

void CMyAPI::GetExeDir(char* pstr)
{
	char dir[255];
	char *p = NULL;

	memset( dir,0,sizeof(dir) );
	::GetModuleFileName( NULL,dir,sizeof(dir) ); //获取当前可执行文件的路径
	p = strrchr(dir,'\\');
	if( p == NULL )
		return;
	*p = '\0'; //截断

	strcpy(pstr,dir);
	
}


int CMyAPI::GetStreamFormat(StreamBufQueue_t *pBufQue,STREAMFORMAT *pStreamFormat)
{
	//获取流格式

	if(!pBufQue)
		return 0;	
	
	EnterCriticalSection(&pBufQue->mutex);
	
    pStreamFormat->type = pBufQue->type;
	pStreamFormat->videoWidth = pBufQue->videoWidth;
	pStreamFormat->videoHeight = pBufQue->videoHeight;
	pStreamFormat->videoFPS = pBufQue->videoFPS;
	
	LeaveCriticalSection(&pBufQue->mutex);
	
	return 0;


}


 int CMyAPI::ChangeFileName(CString & strOldName)
 {
	 /*
	 strOldName:  设备名_开始时间.avi------->strNewName:  设备名_开始时间_结束时间.avi
	 */
	 //在旧文件名后加上 "_结束时间.avi"

	 CTime time;
	 CString strNewName,strTime;
	 int len;
	 len = strOldName.GetLength();

	 if(len<4)//至少有.avi
		 return 0;
	 


	 
	 time = CTime::GetCurrentTime();
	 strTime.Format("%d%02d%02d%02d%02d%02d",
		 time.GetYear(),
		 time.GetMonth(),
		 time.GetDay(),
		 time.GetHour(),
		 time.GetMinute(),
		 time.GetSecond()
		 );
	 strNewName.Format("%s_%s.avi",
		 strOldName.Left(len-4),//去掉原来的.avi
		 strTime
		 );
	 
	 
	 if(!::MoveFile((LPSTR)(LPCTSTR)strOldName,(LPSTR)(LPCTSTR)strNewName))
	 {
		 printf("\n%d\n",::GetLastError());
		 return 0;
	 }
	 
	 return 1;

 }

