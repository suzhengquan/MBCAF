/*
Copyright (c) "2018-2019", Shenzhen Mindeng Technology Co., Ltd(www.niiengine.com),
		Mindeng Base Communication Application Framework
All rights reserved.
	Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
	Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.
	Redistributions in binary form must reproduce the above copyright notice, this list
of conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.
	Neither the name of the "ORGANIZATION" nor the names of its contributors may be used
to endorse or promote products derived from this software without specific prior written
permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
package com.MBCAF.app;

public interface PreDefine
{

    public final int SEX_MAILE = 1;
    public final int SEX_FEMALE = 2;

    public final int  MT_Text = 0x01;
    public final int  MT_Audio = 0x02;
    public final int  MT_GroupText = 0x11;
    public final int  MT_GroupAudio = 0x12;

    public final int  View_Text_Type = 1;
    public final int  View_Image_Type = 2;
    public final int  View_Audio_Type = 3;
    public final int  View_Mix_Type = 4;
    public final int  View_AniImage_Type = 5;

    public final String DISPLAY_FOR_IMAGE = "[图片]";
    public final String DISPLAY_FOR_MIX = "[图文消息]";
    public final String DISPLAY_FOR_AUDIO = "[语音]";
    public final String DISPLAY_FOR_ERROR = "[未知消息]";

    public final int ST_Single = 1;
    public final int ST_Group = 2;
    public final int ST_Error= 3;

    public final int  User_State_Temp = 1;
    public final int  User_State_Normal = 2;
    public final int  User_State_Leave = 3;

    public final int  GROUP_TYPE_NORMAL = 1;
    public final int  GROUP_TYPE_TEMP = 2;

    public final int  Group_State_Normal = 0;
    public final int  Group_State_Shield = 1;

    /**group change Type*/
    public final int  GMT_Add= 0;
    public final int  GMT_delete =1;

    /**depart status Type*/
    public final int  DST_Create= 0;
    public final int  DST_Delete =1;

    public static final int HANDLER_RECORD_FINISHED = 0x01; // 录音结束
    public static final int HANDLER_STOP_PLAY = 0x02;// Speex 通知主界面停止播放
    public static final int RECEIVE_MAX_VOLUME = 0x03;
    public static final int RECORD_AUDIO_TOO_LONG = 0x04;
    public static final int MSG_RECEIVED_MESSAGE = 0x05;

    public static final String KEY_AVATAR_URL = "key_avatar_url";
    public static final String KEY_IS_IMAGE_CONTACT_AVATAR = "is_image_contact_avatar";
    public static final String KEY_LOGIN_NOT_AUTO = "login_not_auto";
    public static final String KEY_LOCATE_DEPARTMENT = "key_locate_department";
    public static final String KEY_SESSION_KEY = "chat_session_key";
    public static final String KEY_PEERID = "key_peerid";

    public static final String PREVIEW_TEXT_CONTENT = "content";

    public static final String EXTRA_IMAGE_LIST = "imagelist";
    public static final String EXTRA_ALBUM_NAME = "name";
    public static final String EXTRA_ADAPTER_NAME = "adapter";
    public static final String EXTRA_CHAT_USER_ID = "chat_user_id";

    public static final String USER_DETAIL_PARAM = "FROM_PAGE";
    public static final String WEBVIEW_URL = "WEBVIEW_URL";

    public static final String CUR_MESSAGE = "CUR_MESSAGE";

    public static final int HANDLER_CHANGE_CONTACT_TAB = 0x10;

    /**基础消息状态，表示网络层收发成功*/
    public final int   MSG_SENDING = 1;
    public final int    MSG_FAILURE = 2;
    public final int    MSG_SUCCESS = 3;

    /**图片消息状态，表示下载到本地、上传到服务器的状态*/
    public final int  IMAGE_UNLOAD=1;
    public final int  IMAGE_LOADING=2;
    public final int IMAGE_LOADED_SUCCESS =3;
    public final int IMAGE_LOADED_FAILURE =4;

    public final int   AUDIO_UNREAD =1;
    public final int   AUDIO_READED = 2;

    public  final String Image_Magic_Begin = "~!@#$%^&*()_+:";
    public  final String Image_Magic_End   = ":+_)(*&^%$#@!~";

    public static final String AVATAR_APPEND_32 ="_32x32.jpg";
    public static final String AVATAR_APPEND_100 ="_100x100.jpg";
    public static final String AVATAR_APPEND_120 ="_100x100.jpg";//头像120*120的pic 没有 所以统一100
    public static final String AVATAR_APPEND_200="_200x200.jpg";

    public static final int PROTOCOL_HEADER_LENGTH = 16;// 默认消息头的长度
    public static final int PROTOCOL_VERSION = 6;
    public static final int PROTOCOL_FLAG = 0;
    public static final char PROTOCOL_ERROR = '0';
    public static final char PROTOCOL_RESERVED = '0';

    // 读取磁盘上文件， 分支判断其类型
    public static final int FILE_SAVE_TYPE_IMAGE = 0X00013;
    public static final int FILE_SAVE_TYPE_AUDIO = 0X00014;

    public static final float MAX_SOUND_RECORD_TIME = 60.0f;// 单位秒
    public static final int MAX_SELECT_IMAGE_COUNT = 6;

    public static final int pageSize = 21;
    public static final int yayaPageSize = 8;

    // 好像设定了，但是好像没有用
    public static final int ALBUM_PREVIEW_BACK = 3;
    // resultCode 返回值
    public static final int ALBUM_BACK_DATA = 5;
    public static final int CAMERA_WITH_DATA = 3023;

    public static final String SETTING_GLOBAL = "Global";
    public static final String UPLOAD_IMAGE_INTENT_PARAMS = "com.MBCAF.upload.image.intent";

    public static final int SERVICE_EVENTBUS_PRIORITY = 10;
    public static final int MESSAGE_EVENTBUS_PRIORITY = 100;

    public static final int MSG_CNT_PER_PAGE = 20;

    public final static String AVATAR_URL_PREFIX = "";

    public final static String ACCESS_MSG_ADDRESS = "http://";
}