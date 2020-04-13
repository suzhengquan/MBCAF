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
package com.MBCAF.db.entity;

// THIS CODE IS GENERATED BY greenDAO, EDIT ONLY INSIDE THE "KEEP"-SECTIONS

// KEEP INCLUDES - put your custom includes here

import com.MBCAF.app.PreDefine;
import com.MBCAF.pb.base.EntityChangeEngine;
/**
 * 这个类不同与其他自动生成代码
 * 需要依赖conten与display 依赖不同的状态
 * */
// KEEP INCLUDES END
/**
 * Entity mapped to table Message.
 */
public class MessageEntity implements java.io.Serializable {

    protected Long id;
    protected int msgId;
    protected int fromId;
    protected int toId;
    /** Not-null value. */
    protected String sessionKey;
    /** Not-null value. */
    protected String content;
    protected int msgType;
    protected int displayType;
    protected int status;
    protected int created;
    protected int updated;

    // KEEP FIELDS - put your custom fields here

    protected boolean isGIfEmo;
    // KEEP FIELDS END

    public MessageEntity() {
    }

    public MessageEntity(Long id) {
        this.id = id;
    }

    public MessageEntity(Long id, int msgId, int fromId, int toId, String sessionKey, String content, int msgType, int displayType, int status, int created, int updated) {
        this.id = id;
        this.msgId = msgId;
        this.fromId = fromId;
        this.toId = toId;
        this.sessionKey = sessionKey;
        this.content = content;
        this.msgType = msgType;
        this.displayType = displayType;
        this.status = status;
        this.created = created;
        this.updated = updated;
    }

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public int getMsgId() {
        return msgId;
    }

    public void setMsgId(int msgId) {
        this.msgId = msgId;
    }

    public int getFromId() {
        return fromId;
    }

    public void setFromId(int fromId) {
        this.fromId = fromId;
    }

    public int getToId() {
        return toId;
    }

    public void setToId(int toId) {
        this.toId = toId;
    }

    /** Not-null value. */
    public String getSessionKey() {
        return sessionKey;
    }

    /** Not-null value; ensure this value is available before it is saved to the database. */
    public void setSessionKey(String sessionKey) {
        this.sessionKey = sessionKey;
    }

    /** Not-null value. */
    public String getContent() {
        return content;
    }

    /** Not-null value; ensure this value is available before it is saved to the database. */
    public void setContent(String content) {
        this.content = content;
    }

    public int getMsgType() {
        return msgType;
    }

    public void setMsgType(int msgType) {
        this.msgType = msgType;
    }

    public int getDisplayType() {
        return displayType;
    }

    public void setDisplayType(int displayType) {
        this.displayType = displayType;
    }

    public int getStatus() {
        return status;
    }

    public void setStatus(int status) {
        this.status = status;
    }

    public int getCreated() {
        return created;
    }

    public void setCreated(int created) {
        this.created = created;
    }

    public int getUpdated() {
        return updated;
    }

    public void setUpdated(int updated) {
        this.updated = updated;
    }

    // KEEP METHODS - put your custom methods here
    /**
     * -----根据自身状态判断的---------
     */
    public int getSessionType() {
        switch (msgType) {
            case  PreDefine.MT_Text:
            case  PreDefine.MT_Audio:
                return PreDefine.ST_Single;
            case PreDefine.MT_GroupText:
            case PreDefine.MT_GroupAudio:
                return PreDefine.ST_Group;
            default:
                //todo 有问题
                return PreDefine.ST_Single;
        }
    }


    public String getMessageDisplay() {
        switch (displayType){
            case PreDefine.View_Audio_Type:
                return PreDefine.DISPLAY_FOR_AUDIO;
            case PreDefine.View_Text_Type:
                return content;
            case PreDefine.View_Image_Type:
                return PreDefine.DISPLAY_FOR_IMAGE;
            case PreDefine.View_Mix_Type:
                return PreDefine.DISPLAY_FOR_MIX;
            default:
                return PreDefine.DISPLAY_FOR_ERROR;
        }
    }

    @Override
    public String toString() {
        return "MessageEntity{" +
                "id=" + id +
                ", msgId=" + msgId +
                ", fromId=" + fromId +
                ", toId=" + toId +
                ", content='" + content + '\'' +
                ", msgType=" + msgType +
                ", displayType=" + displayType +
                ", status=" + status +
                ", created=" + created +
                ", updated=" + updated +
                '}';
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof MessageEntity)) return false;

        MessageEntity that = (MessageEntity) o;

        if (created != that.created) return false;
        if (displayType != that.displayType) return false;
        if (fromId != that.fromId) return false;
        if (msgId != that.msgId) return false;
        if (msgType != that.msgType) return false;
        if (status != that.status) return false;
        if (toId != that.toId) return false;
        if (updated != that.updated) return false;
        if (!content.equals(that.content)) return false;
        if (!id.equals(that.id)) return false;
        if (!sessionKey.equals(that.sessionKey)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        int result = id.hashCode();
        result = 31 * result + msgId;
        result = 31 * result + fromId;
        result = 31 * result + toId;
        result = 31 * result + sessionKey.hashCode();
        result = 31 * result + content.hashCode();
        result = 31 * result + msgType;
        result = 31 * result + displayType;
        result = 31 * result + status;
        result = 31 * result + created;
        result = 31 * result + updated;
        return result;
    }


    /**
     * 获取会话的sessionId
     * @param isSend
     * @return
     */
    public int getPeerId(boolean isSend){
        if(isSend){
            /**自己发出去的*/
            return toId;
        }else{
            /**接受到的*/
            switch (getSessionType()){
                case PreDefine.ST_Single:
                    return fromId;
                case PreDefine.ST_Group:
                    return toId;
                default:
                    return toId;
            }
        }
    }

    public byte[] getSendContent(){
        return null;
    }

    public boolean isGIfEmo() {
        return isGIfEmo;
    }

    public void setGIfEmo(boolean isGIfEmo) {
        this.isGIfEmo = isGIfEmo;
    }

    public boolean isSend(int loginId){
        boolean isSend = (loginId==fromId)?true:false;
        return isSend;
    }

    public String buildSessionKey(boolean isSend){
        int sessionType = getSessionType();
        int peerId = getPeerId(isSend);
        sessionKey = EntityChangeEngine.getSessionKey(peerId,sessionType);
        return sessionKey;
    }
    // KEEP METHODS END

}
