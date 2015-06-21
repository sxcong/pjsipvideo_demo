这个demo主要演示用pjsip做一个sip client做视频对讲。并且视频源采用基于RTSP的IP摄像机，
(不象一般的SIP client直接采集USB摄像头再编码，pjsip2已经支持此功能)。

IPCAM可以使用海康摄像机做为测试，但视频源一定要RTSP，海康的SDK无法使用。
SIP SERVER是开源的resiprocate，编译出来可直接使用。
DEMO程序是vc2008写的，包括SIP的登录，发送请求，发送和接收视频并解码播放。可在同一台机器上运行两个实例测试。
​不过毕竟是DEMO，只是演示怎么使用，细节还有很多问题。以后会逐步修改再提交。
