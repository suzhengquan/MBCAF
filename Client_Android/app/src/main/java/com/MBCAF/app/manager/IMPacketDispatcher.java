package com.MBCAF.app.manager;

import com.google.protobuf.CodedInputStream;
import com.MBCAF.pb.Proto;
import com.MBCAF.pb.MsgServer;
import com.MBCAF.pb.ServerBase;
import com.MBCAF.common.Logger;
import java.io.IOException;

public class IMPacketDispatcher
{
	private static Logger logger = Logger.getLogger(IMPacketDispatcher.class);

    /**
     * @param commandId
     * @param buffer
     *
     * 有没有更加优雅的方式
     */
    public static void loginPacketDispatcher(int commandId,CodedInputStream buffer)
    {
        try
        {
            switch (commandId)
            {
//            case Proto.ServerBaseMsgID.SBID_ObjectLoginA_VALUE :
//                ServerBase.LoginA  LoginA = ServerBase.LoginA.parseFrom(buffer);
//                IMLoginManager.instance().onRepMsgServerLogin(LoginA);
//                return;
            case Proto.ServerBaseMsgID.SBID_ObjectLogoutA_VALUE:
                ServerBase.LogoutA imLogoutRsp = ServerBase.LogoutA.parseFrom(buffer);
                IMLoginManager.instance().onRepLoginOut(imLogoutRsp);
                return;
            case Proto.ServerBaseMsgID.SBID_KickUser_VALUE:
                ServerBase.KickConnect imKickUser = ServerBase.KickConnect.parseFrom(buffer);
                IMLoginManager.instance().onKickout(imKickUser);
            }
        }
        catch (IOException e)
        {
            logger.e("loginPacketDispatcher# error,cid:%d",commandId);
        }
    }

    public static void msgPacketDispatcher(int commandId,CodedInputStream buffer)
    {
        try
        {
            switch (commandId)
            {
            case  Proto.MsgServerMsgID.MSID_DataACK_VALUE:
                // have some problem  todo
                return;
            case Proto.MsgServerMsgID.MSID_MsgListA_VALUE:
                MsgServer.MessageInfoListA rsp = MsgServer.MessageInfoListA.parseFrom(buffer);
                IMMessageManager.instance().onReqHistoryMsg(rsp);
                return;
            case Proto.MsgServerMsgID.MSID_Data_VALUE:
                MsgServer.MessageData imMsgData = MsgServer.MessageData.parseFrom(buffer);
                IMMessageManager.instance().onRecvMessage(imMsgData);
                return;
            case Proto.MsgServerMsgID.MSID_ReadNotify_VALUE:
                MsgServer.MessageRead readNotify = MsgServer.MessageRead.parseFrom(buffer);
                IMUnreadMsgManager.instance().onNotifyRead(readNotify);
                return;
            case Proto.MsgServerMsgID.MSID_UnReadCountA_VALUE:
                MsgServer.MessageUnreadCntA unreadMsgCntRsp = MsgServer.MessageUnreadCntA.parseFrom(buffer);
                IMUnreadMsgManager.instance().onRepUnreadMsgContactList(unreadMsgCntRsp);
                return;
            case Proto.MsgServerMsgID.MSID_MsgA_VALUE:
                MsgServer.MessageInfoByIdA getMsgByIdRsp = MsgServer.MessageInfoByIdA.parseFrom(buffer);
                IMMessageManager.instance().onReqMsgById(getMsgByIdRsp);
                break;
            case Proto.MsgServerMsgID.MSID_BuddyObjectListA_VALUE:
                MsgServer.VaryUserInfoListA imAllUserRsp = MsgServer.VaryUserInfoListA.parseFrom(buffer);
                IMContactManager.instance().onRepAllUsers(imAllUserRsp);
                return;
            case Proto.MsgServerMsgID.MSID_BubbyObjectInfoA_VALUE:
                MsgServer.UserInfoListA imUsersInfoRsp = MsgServer.UserInfoListA.parseFrom(buffer);
                IMContactManager.instance().onRepDetailUsers(imUsersInfoRsp);
                return;
            case Proto.MsgServerMsgID.MSID_BubbyRecentSessionA_VALUE:
                MsgServer.RecentSessionA recentContactSessionRsp = MsgServer.RecentSessionA.parseFrom(buffer);
                IMSessionManager.instance().onRepRecentContacts(recentContactSessionRsp);
                return;
            case Proto.MsgServerMsgID.MSID_BubbyRemoveSessionA_VALUE:
                MsgServer.RemoveSessionA removeSessionRsp = MsgServer.RemoveSessionA.parseFrom(buffer);
                IMSessionManager.instance().onRepRemoveSession(removeSessionRsp);
                return;
            case Proto.MsgServerMsgID.MSID_BuddyPCLoginStateNotify_VALUE:
                MsgServer.PCLoginStateNotify statusNotify = MsgServer.PCLoginStateNotify.parseFrom(buffer);
                IMLoginManager.instance().onLoginStatusNotify(statusNotify);
                return;

            case Proto.MsgServerMsgID.MSID_BuddyOrganizationA_VALUE:
                MsgServer.VaryDepartListA departmentRsp = MsgServer.VaryDepartListA.parseFrom(buffer);
                IMContactManager.instance().onRepDepartment(departmentRsp);
                return;
//          case Proto.MsgServerMsgID.MSID_CreateGroupA_VALUE:
//              MsgServer.GroupCreateA groupCreateRsp = MsgServer.GroupCreateA.parseFrom(buffer);
//              IMGroupManager.instance().onReqCreateTempGroup(groupCreateRsp);
//              return;
            case Proto.MsgServerMsgID.MSID_GroupListA_VALUE:
                MsgServer.GroupListA normalGroupListRsp = MsgServer.GroupListA.parseFrom(buffer);
                IMGroupManager.instance().onRepNormalGroupList(normalGroupListRsp);
                return;
            case Proto.MsgServerMsgID.MSID_GroupInfoA_VALUE:
                MsgServer.GroupInfoListA groupInfoListRsp = MsgServer.GroupInfoListA.parseFrom(buffer);
                IMGroupManager.instance().onRepGroupDetailInfo(groupInfoListRsp);
                return;
//          case Proto.MsgServerMsgID.MSID_GroupMemberA_VALUE:
//              MsgServer.GroupMemberSA groupChangeMemberRsp = MsgServer.GroupMemberSA.parseFrom(buffer);
//              IMGroupManager.instance().onReqChangeGroupMember(groupChangeMemberRsp);
//              return;
            case Proto.MsgServerMsgID.MSID_GroupMemberNotify_VALUE:
                MsgServer.GroupMemberNotify notify = MsgServer.GroupMemberNotify.parseFrom(buffer);
                IMGroupManager.instance().receiveGroupChangeMemberNotify(notify);
            case Proto.MsgServerMsgID.MSID_GroupShieldSA_VALUE:
                //todo
                return;
            }
        }
        catch (IOException e)
        {
            logger.e("msgPacketDispatcher# error,cid:%d",commandId);
        }
    }
}
