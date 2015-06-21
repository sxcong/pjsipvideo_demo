/* $Id: pjsua_app.h 4489 2013-04-23 07:53:25Z riza $ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef __PJSUA_APP_H__
#define __PJSUA_APP_H__

#define PJMEDIA_HAS_VIDEO 1
#define PJMEDIA_VIDEO_DEV_HAS_AVI 1

/**
 * Interface for user application to use pjsua with CLI/menu based UI. 
 */

#include "pjsua_app_common.h"

PJ_BEGIN_DECL

/**
 * This structure contains the configuration of application.
 */
typedef struct pjsua_app_cfg_t
{
    /**
     * The number of runtime arguments passed to the application.
     */
    int       argc;

    /**
     * The array of arguments string passed to the application. 
     */
    char    **argv;

    /** 
     * Tell app that CLI (and pjsua) is (re)started.
     * msg will contain start error message such as ìTelnet to X:Y?
     * ìfailed to start pjsua lib? ìport busy?.
     */
    void (*on_started)(pj_status_t status, const char* title);

    /**
     * Tell app that library request to stopped/restart.
     * GUI app needs to use a timer mechanism to wait before invoking the 
     * cleanup procedure.
     */
    void (*on_stopped)(pj_bool_t restart, int argc, char** argv);

    /**
     * This will enable application to supply customize configuration other than
     * the basic configuration provided by pjsua. 
     */
    void (*on_config_init)(pjsua_app_config *cfg);
} pjsua_app_cfg_t;

/**
 * This will initiate the pjsua and the user interface (CLI/menu UI) based on 
 * the provided configuration.
 */
pj_status_t pjsua_app_init(const pjsua_app_cfg_t *app_cfg);

/**
 * This will run the CLI/menu based UI.
 * wait_telnet_cli is used for CLI based UI. It will tell the library to block
 * or wait until user invoke the "shutdown"/"restart" command. GUI based app
 * should define this param as PJ_FALSE.
 */
pj_status_t pjsua_app_run(pj_bool_t wait_telnet_cli);

/**
 * This will destroy/cleanup the application library.
 */
pj_status_t pjsua_app_destroy();

PJ_END_DECL

typedef enum
{
    PJSUA_STATUS_CALLING = 1,
    PJSUA_STATUS_DISCONNECTED = 2,
    PJSUA_STATUS_CONFIRMED = 3,
    PJSUA_STATUS_EARLY = 4,
    PJSUA_STATUS_INCOMING_CALL = 5,
	PJSUA_STATUS_CONNECTING = 6,
	PJSUA_STATUS_STARTRECORD = 7,
	PJSUA_STATUS_RECVMSG = 8,

};

#include <vector>
using namespace std;

typedef struct
{
    pjsua_call_id CallId;
    CString RemoteUser;
    CString LocalZUser;
}CALL_INFO;

typedef  vector<CALL_INFO> CALLINFOLIST;

typedef void (*status_cb)(int status, pjsua_call_info *data, void* pParam);
typedef void (*register_status_cb)(int accid, bool bRet, void* pParam);
typedef void (*pjsip_log_cb)(int level, const char *data, int len, void* pParam);
typedef void (*pjsip_msg_cb)(int accid,char* remote, char* msg, int len, void* pParam);

typedef void (*pj_data_cb)(const char *data, int len, int ip, int port, void* pParam);

int  sip_register(const char* strUserName, const char* strPw, const char* strServer, int& acc_id);
void sip_upregister(int acc_id);
int  sip_call(int acc_id, const char* strCalledNO, const char* strServer, const char* recfile);
void sip_answer(int callid, const char* recfile);
void sip_hangup(int callid);
void set_callback(status_cb cb, void* param);
void set_register_callback(register_status_cb cb, void* param);
void set_log_callback(pjsip_log_cb cb, void* param);
void set_data_callback(pj_data_cb cb, void* param);
void set_msg_callback(pjsip_msg_cb cb, void* param);

bool send_dtmf(int callid, const char* buff);

int app_init();
int app_destroy();

void set_udp_port(int nPort);

int send_message(int callid, int accid, char* dest, char* server, const char* buff);

pj_status_t pjcall_thread_register();

int enum_codecs();


//int init_video_module();
//int decode(char* buff, int len, char* outbuff, int& outlen);
//void deinit_video_module();

void destroy_media();

void set_local_rtpport(int port);
pj_status_t init_media(int local_port, char* destip, int destport);
int init_local_rtp();
//int init_recv_rtp(char* destip, int destport);
void send_rtp(char* buff, int len);

void get_remote_addr(string& ip, int& port);


//on_pager: Ω” ’MESSAGE

#endif	/* __PJSUA_APP_H__ */
