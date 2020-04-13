package com.MBCAF.app.manager;

import android.text.TextUtils;

import com.google.protobuf.CodedInputStream;
import com.MBCAF.db.DBInterface;
import com.MBCAF.db.entity.UserEntity;
import com.MBCAF.db.sp.LoginSp;
import com.MBCAF.app.event.LoginEvent;
import com.MBCAF.pb.base.ProtoBuf2JavaBean;
import com.MBCAF.pb.Proto;
import com.MBCAF.pb.MsgServer;
import com.MBCAF.pb.ServerBase;
import com.MBCAF.common.Logger;

import java.io.IOException;

import de.greenrobot.event.EventBus;

/**
 * 很多情况下都是一种权衡
 * 登陆控制
 * @yingmu
 */
public class IMLoginManager extends IMManager 
{
    private Logger logger = Logger.getLogger(IMLoginManager.class);

    /**单例模式*/
    private static IMLoginManager inst = new IMLoginManager();
    public static IMLoginManager instance() 
    {
        return inst;
    }
    public IMLoginManager() 
    {
        logger.d("login#creating IMLoginManager");
    }
    IMSocketManager imSocketManager = IMSocketManager.instance();

    /**登陆参数 以便重试*/
    private String loginUserName;
    private String loginPwd;
    private int loginId;
    private UserEntity loginInfo;


    /**loginManger 自身的状态 todo 状态太多就采用enum的方式*/
    private boolean  identityChanged = false;
    private boolean isKickout = false;
    private boolean isPcOnline = false;
    //以前是否登陆过，用户重新登陆的判断
    private boolean everLogined = false;
    //本地包含登陆信息了[可以理解为支持离线登陆了]
    private boolean isLocalLogin = false;

    private LoginEvent loginStatus= LoginEvent.NONE;

    /**-------------------------------功能方法--------------------------------------*/

    @Override
    public void doOnStart() {
    }

    @Override
    public void reset() {
        loginUserName = null;
        loginPwd = null;
        loginId = -1;
        loginInfo = null;
        identityChanged = false;
        isKickout=false;
        isPcOnline = false;
        everLogined = false;
        loginStatus= LoginEvent.NONE;
        isLocalLogin = false;
    }

    /**
     * 实现自身的事件驱动
     * @param event
     */
    public void triggerEvent(LoginEvent event) 
    {
        loginStatus = event;
        EventBus.getDefault().postSticky(event);
    }

    /**
     * if not login, do nothing
     send logOuting message, so reconnect won't react abnormally
     but when reconnect start to work again?use isEverLogined
     close the socket
     send logOuteOk message
     mainactivity jumps to login page
     *
     */
    public void logOut() 
    {
        logger.d("login#logOut");
        logger.d("login#stop reconnecting");
        //		everlogined is enough to stop reconnecting
        everLogined =  false;
        isLocalLogin = false;
        reqLoginOut();
    }

    /**
     * 退出登陆
     */
    private void reqLoginOut()
    {
        ServerBase.LogoutQ imLogoutReq = ServerBase.LogoutQ.newBuilder()
                .build();
        int sid = Proto.MsgTypeID.MTID_ServerBase_VALUE;
        int cid = Proto.ServerBaseMsgID.SBID_ObjectLogoutQ_VALUE;
        try {
            imSocketManager.sendRequest(imLogoutReq, sid, cid);
        }catch (Exception e){
            logger.e("#reqLoginOut#sendRequest error,cause by"+e.toString());
        }finally {
            LoginSp.instance().setLoginInfo(loginUserName,null,loginId);
            logger.d("login#send logout finish message");
            triggerEvent(LoginEvent.LOGIN_OUT);
        }
    }

    /**
     * 现在这种模式 req与rsp之间没有必然的耦合关系。是不是太松散了
     * @param imLogoutRsp
     */
    public void onRepLoginOut(ServerBase.LogoutA imLogoutRsp)
    {
        int code = imLogoutRsp.getResultCode();
        logger.d("login#send logout finish message");
    }

    /**
     * 重新请求登陆 IMReconnectManager
     * 1. 检测当前的状态
     * 2. 请求msg server的地址
     * 3. 建立链接
     * 4. 验证登陆信息
     * @return
     */
    public void relogin() 
    {
        if(!TextUtils.isEmpty(loginUserName) && !TextUtils.isEmpty(loginPwd))
        {
            logger.d("reconnect#login#relogin");
            imSocketManager.reqMsgServerAddrs();
        }else{
            logger.d("reconnect#login#userName or loginPwd is null!!");
            everLogined = false;
            triggerEvent(LoginEvent.LOGIN_AUTH_FAILED);
        }
    }

    // 自动登陆流程
    public void login(LoginSp.SpLoginIdentity identity)
    {
        if(identity == null)
        {
            triggerEvent(LoginEvent.LOGIN_AUTH_FAILED);
            return;
        }
        loginUserName = identity.getLoginName();
        loginPwd = identity.getPwd();
        identityChanged = false;

        int mLoginId = identity.getLoginId();
        // 初始化数据库
        DBInterface.instance().initDbHelp(ctx,mLoginId);
        UserEntity loginEntity = DBInterface.instance().getByLoginId(mLoginId);
        do
        {
            if(loginEntity == null)
            {
                break;
            }
            loginInfo = loginEntity;
            loginId = loginEntity.getPeerId();
            // 这两个状态不要忘记掉
            isLocalLogin = true;
            everLogined = true;
            triggerEvent(LoginEvent.LOCAL_LOGIN_SUCCESS);
        }while(false);
        // 开始请求网络
        imSocketManager.reqMsgServerAddrs();
    }


    public void login(String userName, String password) 
    {
        logger.i("login#login -> userName:%s", userName);

        //test 使用
        LoginSp.SpLoginIdentity identity = LoginSp.instance().getLoginIdentity();
        if(identity !=null && !TextUtils.isEmpty(identity.getPwd())) 
        {
            if (identity.getPwd().equals(password) && identity.getLoginName().equals(userName)) 
            {
                login(identity);
                return;
            }
        }
        //test end
        loginUserName = userName;
        loginPwd = password;
        identityChanged = true;
        imSocketManager.reqMsgServerAddrs();
    }

    /**
     * 链接成功之后
     * */
    public void reqLoginMsgServer() 
    {
        logger.i("login#reqLoginMsgServer");
        triggerEvent(LoginEvent.LOGINING);
        /** 加密 */
        String desPwd = new String(com.MBCAF.app.network.Security.getInstance().EncryptPass(loginPwd));

        ServerBase.LoginQ imLoginReq = ServerBase.LoginQ.newBuilder()
                    .setUserName(loginUserName)
                    .setPassword(desPwd)
                    .setOnlineStatus(Proto.ObjectStateType.OST_Online)
                    .setClientType(Proto.ClientType.CT_Android)
                    .setClientVersion("1.0.0").build();

       int sid = Proto.MsgTypeID.MTID_ServerBase_VALUE;
       int cid = Proto.ServerBaseMsgID.SBID_ObjectLoginQ_VALUE;
       imSocketManager.sendRequest(imLoginReq,sid,cid,new IMPacketManager.PacketListener()
       {
           @Override
           public void onSuccess(Object response) 
           {
               try {
                   ServerBase.LoginA  LoginA = ServerBase.LoginA.parseFrom((CodedInputStream)response);
                   onRepMsgServerLogin(LoginA);
               } catch (IOException e) {
                   triggerEvent(LoginEvent.LOGIN_INNER_FAILED);
                   logger.e("login failed,cause by %s",e.getCause());
               }
           }

           @Override
           public void onFaild() 
           {
               triggerEvent(LoginEvent.LOGIN_INNER_FAILED);
           }

           @Override
           public void onTimeout() 
           {
               triggerEvent(LoginEvent.LOGIN_INNER_FAILED);
           }
       });
    }

    /**
     * 验证登陆信息结果
     * @param loginRes
     */
    public void onRepMsgServerLogin(ServerBase.LoginA loginRes) 
    {
        logger.i("login#onRepMsgServerLogin");

        if (loginRes == null) 
        {
            logger.e("login#decode LoginResponse failed");
            triggerEvent(LoginEvent.LOGIN_AUTH_FAILED);
            return;
        }

        Proto.ResultType  code = loginRes.getResultCode();
        switch (code){
            case RT_OK:
            {
                Proto.ObjectStateType userStatType = loginRes.getOnlineStatus();
                Proto.UserInfo userInfo =  loginRes.getUserInfo();
                loginId = userInfo.getUserId();
                loginInfo = ProtoBuf2JavaBean.getUserEntity(userInfo);
                onLoginOk();
            }break;

            case RT_NoValidateFail:
            {
                logger.e("login#login msg server failed, result:%s", code);
                triggerEvent(LoginEvent.LOGIN_AUTH_FAILED);
            }break;

            default:
            {
                logger.e("login#login msg server inner failed, result:%s", code);
                triggerEvent(LoginEvent.LOGIN_INNER_FAILED);
            }break;
        }
    }

    public void onLoginOk() 
    {
        logger.i("login#onLoginOk");
        everLogined = true;
        isKickout = false;

        // 判断登陆的类型
        if(isLocalLogin)
        {
            triggerEvent(LoginEvent.LOCAL_LOGIN_MSG_SERVICE);
        }
        else
        {
            isLocalLogin = true;
            triggerEvent(LoginEvent.LOGIN_OK);
        }

        // 发送token
//        reqDeviceToken();
        if (identityChanged) 
        {
            LoginSp.instance().setLoginInfo(loginUserName,loginPwd,loginId);
            identityChanged = false;
        }
    }



    private void reqDeviceToken()
    {
//        String token = PushManager.getInstance().getToken();
//        ServerBase.TrayMsgQ req = ServerBase.TrayMsgQ.newBuilder()
//                .setUserId(loginId)
//                .setClientType(Proto.ClientType.CT_Android)
//                .setDeviceToken(token)
//                .build();
//        int sid = Proto.MsgTypeID.MTID_ServerBase_VALUE;
//        int cid = Proto.ServerBaseMsgID.SBID_REQ_DEVICETOKEN_VALUE;
//
//        imSocketManager.sendRequest(req,sid,cid,new PacketListener() {
//            @Override
//            public void onSuccess(Object response) {
//                //?? nothing to do
////                try {
////                    ServerBase.TrayMsgA rsp = ServerBase.TrayMsgA.parseFrom((CodedInputStream) response);
////                    int userId = rsp.getUserId();
////                } catch (IOException e) {
////                    e.printStackTrace();
////                }
//            }
//
//            @Override
//            public void onFaild() {}
//
//            @Override
//            public void onTimeout() {}
//        });
    }


    public void onKickout(ServerBase.KickConnect imKickUser)
    {
        logger.i("login#onKickout");
        int kickUserId = imKickUser.getUserId();
        Proto.KickReasonType reason = imKickUser.getKickReason();
        isKickout=true;
        imSocketManager.onMsgServerDisconn();
    }


    // 收到PC端登陆的通知，另外登陆成功之后，如果PC端在线，也会立马收到该通知
    public void onLoginStatusNotify(MsgServer.PCLoginStateNotify statusNotify){
        int userId = statusNotify.getUserId();
        // todo 由于交互不太友好 暂时先去掉
        if(true || userId !=loginId){
            logger.i("login#onLoginStatusNotify userId ≠ loginId");
            return;
        }

        if(isKickout){
            logger.i("login#already isKickout");
            return;
        }

        switch (statusNotify.getLoginStat()){
            case OST_Online:{
                isPcOnline = true;
                EventBus.getDefault().postSticky(LoginEvent.PC_ONLINE);
            }break;

            case OST_Offline:{
                isPcOnline = false;
                EventBus.getDefault().postSticky(LoginEvent.PC_OFFLINE);
            }break;
        }
    }

    // 踢出PC端登陆
    public void reqKickPCClient()
	{
        ServerBase.KickPCConnectQ req = ServerBase.KickPCConnectQ.newBuilder()
                .setUserId(loginId)
                .build();
        int sid = Proto.MsgTypeID.MTID_ServerBase_VALUE;
        int cid = Proto.ServerBaseMsgID.SBID_KickPCClientQ_VALUE;
        imSocketManager.sendRequest(req,sid,cid,new IMPacketManager.PacketListener()
		{
            @Override
            public void onSuccess(Object response) 
			{
                triggerEvent(LoginEvent.KICK_PC_SUCCESS);
            }
            @Override
            public void onFaild() 
			{
                triggerEvent(LoginEvent.KICK_PC_FAILED);
            }
            @Override
            public void onTimeout() 
			{
                triggerEvent(LoginEvent.KICK_PC_FAILED);
            }
        });
    }

    /**------------------状态的 set  get------------------------------*/
    public int getLoginId() {
        return loginId;
    }

    public void setLoginId(int loginId) {
        logger.d("login#setLoginId -> loginId:%d", loginId);
        this.loginId = loginId;

    }

    public boolean isEverLogined() {
        return everLogined;
    }

    public void setEverLogined(boolean everLogined) {
        this.everLogined = everLogined;
    }

    public UserEntity getLoginInfo() {
        return loginInfo;
    }

    public void setLoginInfo(UserEntity loginInfo) {
        this.loginInfo = loginInfo;
    }

    public LoginEvent getLoginStatus() {
        return loginStatus;
    }

    public boolean isKickout() {
        return isKickout;
    }

    public void setKickout(boolean isKickout) {
        this.isKickout = isKickout;
    }

    public boolean isPcOnline() {
        return isPcOnline;
    }

    public void setPcOnline(boolean isPcOnline) {
        this.isPcOnline = isPcOnline;
    }
}
