package com.MBCAF.pb.base;

import com.google.protobuf.ByteString;
import com.MBCAF.db.entity.DepartmentEntity;
import com.MBCAF.app.PreDefine;
import com.MBCAF.db.entity.GroupEntity;
import com.MBCAF.db.entity.MessageEntity;
import com.MBCAF.db.entity.SessionEntity;
import com.MBCAF.db.entity.UserEntity;
import com.MBCAF.app.entity.AudioMessage;
import com.MBCAF.app.entity.MsgAnalyzeEngine;
import com.MBCAF.app.entity.UnreadEntity;
import com.MBCAF.pb.Proto;
import com.MBCAF.pb.MsgServer;
import com.MBCAF.common.CommonUtil;
import com.MBCAF.common.FileUtil;
import com.MBCAF.common.pinyin.PinYin;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.UnsupportedEncodingException;

public class ProtoBuf2JavaBean {

    public static DepartmentEntity getDepartEntity(Proto.DepartInfo departInfo){
        DepartmentEntity departmentEntity = new DepartmentEntity();

        int timeNow = (int) (System.currentTimeMillis()/1000);

        departmentEntity.setDepartId(departInfo.getDeptId());
        departmentEntity.setDepartName(departInfo.getDeptName());
        departmentEntity.setPriority(departInfo.getPriority());
        departmentEntity.setStatus(getDepartStatus(departInfo.getDeptStatus()));

        departmentEntity.setCreated(timeNow);
        departmentEntity.setUpdated(timeNow);

        // 设定pinyin 相关
        PinYin.getPinYin(departInfo.getDeptName(), departmentEntity.getPinyinElement());

        return departmentEntity;
    }

    public static UserEntity getUserEntity(Proto.UserInfo userInfo){
        UserEntity userEntity = new UserEntity();
        int timeNow = (int) (System.currentTimeMillis()/1000);

        userEntity.setStatus(userInfo.getStatus());
        userEntity.setAvatar(userInfo.getAvatarUrl());
        userEntity.setCreated(timeNow);
        userEntity.setDepartmentId(userInfo.getDepartmentId());
        userEntity.setEmail(userInfo.getEmail());
        userEntity.setGender(userInfo.getUserGender());
        userEntity.setMainName(userInfo.getUserNickName());
        userEntity.setPhone(userInfo.getUserTel());
        userEntity.setPinyinName(userInfo.getUserDomain());
        userEntity.setRealName(userInfo.getUserRealName());
        userEntity.setUpdated(timeNow);
        userEntity.setPeerId(userInfo.getUserId());

        PinYin.getPinYin(userEntity.getMainName(), userEntity.getPinyinElement());
        return userEntity;
    }

    public static SessionEntity getSessionEntity(Proto.ContactSessionInfo sessionInfo){
        SessionEntity sessionEntity = new SessionEntity();

        int msgType = getJavaMsgType(sessionInfo.getLatestMsgType());
        sessionEntity.setLatestMsgType(msgType);
        sessionEntity.setPeerType(getJavaSessionType(sessionInfo.getSessionType()));
        sessionEntity.setPeerId(sessionInfo.getSessionId());
        sessionEntity.buildSessionKey();
        sessionEntity.setTalkId(sessionInfo.getLatestMsgFromUserId());
        sessionEntity.setLatestMsgId(sessionInfo.getLatestMsgId());
        sessionEntity.setCreated(sessionInfo.getUpdatedTime());

        String content  = sessionInfo.getLatestMsgData().toStringUtf8();
        String desMessage = new String(com.MBCAF.app.network.Security.getInstance().DecryptMsg(content));
        // 判断具体的类型是什么
        if(msgType == PreDefine.MT_GroupText ||
                msgType ==PreDefine.MT_Text){
            desMessage =  MsgAnalyzeEngine.analyzeMessageDisplay(desMessage);
        }

        sessionEntity.setLatestMsgData(desMessage);
        sessionEntity.setUpdated(sessionInfo.getUpdatedTime());

        return sessionEntity;
    }


    public static GroupEntity getGroupEntity(Proto.GroupInfo groupInfo){
        GroupEntity groupEntity = new GroupEntity();
        int timeNow = (int) (System.currentTimeMillis()/1000);
        groupEntity.setUpdated(timeNow);
        groupEntity.setCreated(timeNow);
        groupEntity.setMainName(groupInfo.getGroupName());
        groupEntity.setAvatar(groupInfo.getGroupAvatar());
        groupEntity.setCreatorId(groupInfo.getGroupCreatorId());
        groupEntity.setPeerId(groupInfo.getGroupId());
        groupEntity.setGroupType(getJavaGroupType(groupInfo.getGroupType()));
        groupEntity.setStatus(groupInfo.getShieldStatus());
        groupEntity.setUserCnt(groupInfo.getGroupMemberListCount());
        groupEntity.setVersion(groupInfo.getVersion());
        groupEntity.setlistGroupMemberIds(groupInfo.getGroupMemberListList());

        // may be not good place
        PinYin.getPinYin(groupEntity.getMainName(), groupEntity.getPinyinElement());

        return groupEntity;
    }


    /**
     * 创建群时候的转化
     * @param groupCreateRsp
     * @return
     */
    public static GroupEntity getGroupEntity(MsgServer.GroupCreateA groupCreateRsp){
        GroupEntity groupEntity = new GroupEntity();
        int timeNow = (int) (System.currentTimeMillis()/1000);
        groupEntity.setMainName(groupCreateRsp.getGroupName());
        groupEntity.setlistGroupMemberIds(groupCreateRsp.getUserIdListList());
        groupEntity.setCreatorId(groupCreateRsp.getUserId());
        groupEntity.setPeerId(groupCreateRsp.getGroupId());

        groupEntity.setUpdated(timeNow);
        groupEntity.setCreated(timeNow);
        groupEntity.setAvatar("");
        groupEntity.setGroupType(PreDefine.GROUP_TYPE_TEMP);
        groupEntity.setStatus(PreDefine.Group_State_Normal);
        groupEntity.setUserCnt(groupCreateRsp.getUserIdListCount());
        groupEntity.setVersion(1);

        PinYin.getPinYin(groupEntity.getMainName(), groupEntity.getPinyinElement());
        return groupEntity;
    }


    /**
     * 拆分消息在上层做掉 图文混排
     * 在这判断
    */
    public static MessageEntity getMessageEntity(Proto.MsgInfo msgInfo) {
        MessageEntity messageEntity = null;
        Proto.MessageType msgType = msgInfo.getMsgType();
        switch (msgType) {
            case MT_Audio:
            case MT_GroupAudio:
                try {
                    /**语音的解析不能转自 string再返回来*/
                    messageEntity = analyzeAudio(msgInfo);
                } catch (JSONException e) {
                    return null;
                } catch (UnsupportedEncodingException e) {
                    return null;
                }
                break;

            case MT_GroupText:
            case MT_Text:
                messageEntity = analyzeText(msgInfo);
                break;
            default:
                throw new RuntimeException("ProtoBuf2JavaBean#getMessageEntity wrong type!");
        }
        return messageEntity;
    }

    public static MessageEntity analyzeText(Proto.MsgInfo msgInfo){
       return MsgAnalyzeEngine.analyzeMessage(msgInfo);
    }


    public static AudioMessage analyzeAudio(Proto.MsgInfo msgInfo) throws JSONException, UnsupportedEncodingException {
        AudioMessage audioMessage = new AudioMessage();
        audioMessage.setFromId(msgInfo.getFromSessionId());
        audioMessage.setMsgId(msgInfo.getMsgId());
        audioMessage.setMsgType(getJavaMsgType(msgInfo.getMsgType()));
        audioMessage.setStatus(PreDefine.MSG_SUCCESS);
        audioMessage.setReadStatus(PreDefine.AUDIO_UNREAD);
        audioMessage.setDisplayType(PreDefine.View_Audio_Type);
        audioMessage.setCreated(msgInfo.getCreateTime());
        audioMessage.setUpdated(msgInfo.getCreateTime());

        ByteString bytes = msgInfo.getMsgData();

        byte[] audioStream = bytes.toByteArray();
        if(audioStream.length < 4){
            audioMessage.setReadStatus(PreDefine.AUDIO_READED);
            audioMessage.setAudioPath("");
            audioMessage.setAudioLength(0);
        }else {
            int msgLen = audioStream.length;
            byte[] playTimeByte = new byte[4];
            byte[] audioContent = new byte[msgLen - 4];

            System.arraycopy(audioStream, 0, playTimeByte, 0, 4);
            System.arraycopy(audioStream, 4, audioContent, 0, msgLen - 4);
            int playTime = CommonUtil.byteArray2int(playTimeByte);
            String audioSavePath = FileUtil.saveAudioResourceToFile(audioContent, audioMessage.getFromId());
            audioMessage.setAudioLength(playTime);
            audioMessage.setAudioPath(audioSavePath);
        }

        /**抽离出来 或者用gson*/
        JSONObject extraContent = new JSONObject();
        extraContent.put("audioPath",audioMessage.getAudioPath());
        extraContent.put("audiolength",audioMessage.getAudioLength());
        extraContent.put("readStatus",audioMessage.getReadStatus());
        String audioContent = extraContent.toString();
        audioMessage.setContent(audioContent);

        return audioMessage;
    }


    public static MessageEntity getMessageEntity(MsgServer.MessageData msgData){

        MessageEntity messageEntity = null;
        Proto.MessageType msgType = msgData.getMsgType();
        Proto.MsgInfo msgInfo = Proto.MsgInfo.newBuilder()
                .setMsgData(msgData.getMsgData())
                .setMsgId(msgData.getMsgId())
                .setMsgType(msgType)
                .setCreateTime(msgData.getCreateTime())
                .setFromSessionId(msgData.getFromUserId())
                .build();

        switch (msgType) {
            case MT_Audio:
            case MT_GroupAudio:
                try {
                    messageEntity = analyzeAudio(msgInfo);
                } catch (JSONException e) {
                    e.printStackTrace();
                } catch (UnsupportedEncodingException e) {
                    e.printStackTrace();
                }
                break;
            case MT_GroupText:
            case MT_Text:
                messageEntity = analyzeText(msgInfo);
                break;
            default:
                throw new RuntimeException("ProtoBuf2JavaBean#getMessageEntity wrong type!");
        }
        if(messageEntity != null){
            messageEntity.setToId(msgData.getToSessionId());
        }

        /**
         消息的发送状态与 展示类型需要在上层做掉
         messageEntity.setStatus();
         messageEntity.setDisplayType();
         */
        return messageEntity;
    }

    public static UnreadEntity getUnreadEntity(Proto.UnreadInfo pbInfo){
        UnreadEntity unreadEntity = new UnreadEntity();
        unreadEntity.setSessionType(getJavaSessionType(pbInfo.getSessionType()));
        unreadEntity.setLatestMsgData(pbInfo.getLatestMsgData().toString());
        unreadEntity.setPeerId(pbInfo.getSessionId());
        unreadEntity.setLaststMsgId(pbInfo.getLatestMsgId());
        unreadEntity.setUnReadCnt(pbInfo.getUnreadCnt());
        unreadEntity.buildSessionKey();
        return unreadEntity;
    }

    /**----enum 转化接口--*/
    public static int getJavaMsgType(Proto.MessageType msgType){
        switch (msgType){
            case MT_GroupText:
                return PreDefine.MT_GroupText;
            case MT_GroupAudio:
                return PreDefine.MT_GroupAudio;
            case MT_Audio:
                return PreDefine.MT_Audio;
            case MT_Text:
                return PreDefine.MT_Text;
            default:
                throw new IllegalArgumentException("msgType is illegal,cause by #getProtoMsgType#" +msgType);
        }
    }

    public static int getJavaSessionType(Proto.SessionType sessionType){
        switch (sessionType){
            case ST_Single:
                return PreDefine.ST_Single;
            case ST_Group:
                return PreDefine.ST_Group;
            default:
                throw new IllegalArgumentException("sessionType is illegal,cause by #getProtoSessionType#" +sessionType);
        }
    }

    public static int getJavaGroupType(Proto.GroupType groupType){
        switch (groupType){
            case GROUP_TYPE_NORMAL:
                return PreDefine.GROUP_TYPE_NORMAL;
            case GROUP_TYPE_TMP:
                return PreDefine.GROUP_TYPE_TEMP;
            default:
                throw new IllegalArgumentException("sessionType is illegal,cause by #getProtoSessionType#" +groupType);
        }
    }

    public static int getGroupChangeType(Proto.GroupModifyType modifyType){
        switch (modifyType){
            case GMT_Add:
                return PreDefine.GMT_Add;
            case GMT_delete:
                return PreDefine.GMT_delete;
            default:
                throw new IllegalArgumentException("GroupModifyType is illegal,cause by " +modifyType);
        }
    }

    public static int getDepartStatus(Proto.DepartmentStateType statusType){
        switch (statusType){
            case DST_Create:
                return PreDefine.DST_Create;
            case DST_Delete:
                return PreDefine.DST_Delete;
            default:
                throw new IllegalArgumentException("getDepartStatus is illegal,cause by " +statusType);
        }

    }
}
