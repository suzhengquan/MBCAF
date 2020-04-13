package com.MBCAF.app.entity;

import android.text.TextUtils;

import com.MBCAF.db.entity.PeerEntity;
import com.MBCAF.db.entity.UserEntity;
import com.MBCAF.app.PreDefine;
import com.MBCAF.db.entity.MessageEntity;
import com.MBCAF.common.CommonUtil;
import com.MBCAF.common.FileUtil;
import com.MBCAF.app.manager.IMSeqNumManager;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.Serializable;

public class AudioMessage extends MessageEntity implements Serializable{

    private String audioPath = "";
    private int audiolength =0 ;
    private int readStatus = PreDefine.AUDIO_UNREAD;

    public AudioMessage(){
        msgId = IMSeqNumManager.getInstance().makelocalUniqueMsgId();
    }

    private AudioMessage(MessageEntity entity){
        id =  entity.getId();
        msgId  = entity.getMsgId();
        fromId = entity.getFromId();
        toId   = entity.getToId();
        content=entity.getContent();
        msgType=entity.getMsgType();
        sessionKey = entity.getSessionKey();
        displayType=entity.getDisplayType();
        status = entity.getStatus();
        created = entity.getCreated();
        updated = entity.getUpdated();
    }

    public static AudioMessage parseFromDB(MessageEntity entity)  {
        if(entity.getDisplayType() != PreDefine.View_Audio_Type){
           throw new RuntimeException("#AudioMessage# parseFromDB,not View_Audio_Type");
        }
        AudioMessage audioMessage = new AudioMessage(entity);

        String originContent = entity.getContent();

        JSONObject extraContent = null;
        try {
            extraContent = new JSONObject(originContent);
            audioMessage.setAudioPath(extraContent.getString("audioPath"));
            audioMessage.setAudioLength(extraContent.getInt("audiolength"));
            audioMessage.setReadStatus(extraContent.getInt("readStatus"));
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return audioMessage;
    }

    public static AudioMessage buildForSend(float audioLen,String audioSavePath,UserEntity fromUser,PeerEntity peerEntity){
        int tLen = (int) (audioLen + 0.5);
        tLen = tLen < 1 ? 1 : tLen;
        if (tLen < audioLen) {
            ++tLen;
        }

        int nowTime = (int) (System.currentTimeMillis() / 1000);
        AudioMessage audioMessage = new AudioMessage();
        audioMessage.setFromId(fromUser.getPeerId());
        audioMessage.setToId(peerEntity.getPeerId());
        audioMessage.setCreated(nowTime);
        audioMessage.setUpdated(nowTime);
        int peerType = peerEntity.getType();
        int msgType = peerType == PreDefine.ST_Group ? PreDefine.MT_GroupAudio : PreDefine.MT_Audio;
        audioMessage.setMsgType(msgType);

        audioMessage.setAudioPath(audioSavePath);
        audioMessage.setAudioLength(tLen);
        audioMessage.setReadStatus(PreDefine.AUDIO_READED);
        audioMessage.setDisplayType(PreDefine.View_Audio_Type);
        audioMessage.setStatus(PreDefine.MSG_SENDING);
        audioMessage.buildSessionKey(true);
        return audioMessage;
    }

    @Override
    public String getContent() {
        JSONObject extraContent = new JSONObject();
        try {
            extraContent.put("audioPath",audioPath);
            extraContent.put("audiolength",audiolength);
            extraContent.put("readStatus",readStatus);
            String audioContent = extraContent.toString();
            return audioContent;
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return null;
    }

    @Override
    public byte[] getSendContent() {
        byte[] result = new byte[4];
        result = CommonUtil.intToBytes(audiolength);
        if (TextUtils.isEmpty(audioPath)) {
            return result;
        }

        byte[] bytes = FileUtil.getFileContent(audioPath);
        if (bytes == null) {
            return bytes;
        }
        int contentLength = bytes.length;
        byte[] byteAduioContent = new byte[4 + contentLength];
        System.arraycopy(result, 0, byteAduioContent, 0, 4);
        System.arraycopy(bytes, 0, byteAduioContent, 4, contentLength);
        return byteAduioContent;
    }

    public String getAudioPath() {
        return audioPath;
    }

    public void setAudioPath(String path) {
        this.audioPath = path;
    }

    public int getAudioLength() {
        return audiolength;
    }

    public void setAudioLength(int length) { this.audiolength = length; }

    public int getReadStatus() {
        return readStatus;
    }

    public void setReadStatus(int status) {
        this.readStatus = status;
    }
}
