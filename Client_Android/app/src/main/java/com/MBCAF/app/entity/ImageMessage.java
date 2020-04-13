package com.MBCAF.app.entity;

import com.MBCAF.db.entity.MessageEntity;
import com.MBCAF.db.entity.PeerEntity;
import com.MBCAF.db.entity.UserEntity;
import com.MBCAF.app.PreDefine;
import com.MBCAF.app.manager.IMSeqNumManager;
import com.MBCAF.app.ui.adapter.album.ImageItem;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.Serializable;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

/**
 */
public class ImageMessage extends MessageEntity implements Serializable {

    /**本地保存的path*/
    private String path = "";
    /**图片的网络地址*/
    private String url = "";
    private int loadStatus;

    //存储图片消息
    private static java.util.HashMap<Long,ImageMessage> imageMessageMap = new java.util.HashMap<Long,ImageMessage>();
    private static ArrayList<ImageMessage> imageList=null;
    /**
     * 添加一条图片消息
     * @param msg
     */
    public static synchronized void addToImageMessageList(ImageMessage msg){
        try {
            if(msg!=null && msg.getId()!=null)
            {
                imageMessageMap.put(msg.getId(),msg);
            }
        }catch (Exception e){
        }
    }

    /**
     * 获取图片列表
     * @return
     */
    public static ArrayList<ImageMessage> getImageMessageList(){
        imageList = new ArrayList<>();
        java.util.Iterator it = imageMessageMap.keySet().iterator();
        while (it.hasNext()) {
            imageList.add(imageMessageMap.get(it.next()));
        }
        Collections.sort(imageList, new Comparator<ImageMessage>(){
            public int compare(ImageMessage image1, ImageMessage image2) {
                Integer a =  image1.getUpdated();
                Integer b = image2.getUpdated();
                if(a.equals(b))
                {
                    return image2.getId().compareTo(image1.getId());
                }
                // 升序
                //return a.compareTo(b);
                // 降序
                return b.compareTo(a);
            }
        });
        return imageList;
    }

    /**
     * 清除图片列表
     */
    public static synchronized void clearImageMessageList(){
        imageMessageMap.clear();
        imageMessageMap.clear();
    }



    public ImageMessage(){
        msgId = IMSeqNumManager.getInstance().makelocalUniqueMsgId();
    }

    /**消息拆分的时候需要*/
    private ImageMessage(MessageEntity entity){
        /**父类的id*/
         id =  entity.getId();
         msgId  = entity.getMsgId();
         fromId = entity.getFromId();
         toId   = entity.getToId();
        sessionKey = entity.getSessionKey();
         content=entity.getContent();
         msgType=entity.getMsgType();
         displayType=entity.getDisplayType();
         status = entity.getStatus();
         created = entity.getCreated();
         updated = entity.getUpdated();
    }

    /**接受到网络包，解析成本地的数据*/
    public static ImageMessage parseFromNet(MessageEntity entity) throws JSONException {
        String strContent = entity.getContent();
        // 判断开头与结尾
        if (strContent.startsWith(PreDefine.Image_Magic_Begin)
                && strContent.endsWith(PreDefine.Image_Magic_End)) {
            // image message todo 字符串处理下
            ImageMessage imageMessage = new ImageMessage(entity);
            imageMessage.setDisplayType(PreDefine.View_Image_Type);
            String imageUrl = strContent.substring(PreDefine.Image_Magic_Begin.length());
            imageUrl = imageUrl.substring(0,imageUrl.indexOf(PreDefine.Image_Magic_End));

            /**抽离出来 或者用gson*/
            JSONObject extraContent = new JSONObject();
            extraContent.put("path","");
            extraContent.put("url",imageUrl);
            extraContent.put("loadStatus", PreDefine.IMAGE_UNLOAD);
            String imageContent = extraContent.toString();
            imageMessage.setContent(imageContent);

            imageMessage.setUrl(imageUrl.isEmpty() ? null : imageUrl);
            imageMessage.setContent(strContent);
            imageMessage.setLoadStatus(PreDefine.IMAGE_UNLOAD);
            imageMessage.setStatus(PreDefine.MSG_SUCCESS);
            return imageMessage;
        }else{
            throw new RuntimeException("no image type,cause by [start,end] is wrong!");
        }
    }


    public static ImageMessage parseFromDB(MessageEntity entity)  {
        if(entity.getDisplayType() != PreDefine.View_Image_Type){
            throw new RuntimeException("#ImageMessage# parseFromDB,not View_Image_Type");
        }
        ImageMessage imageMessage = new ImageMessage(entity);
        String originContent = entity.getContent();
        JSONObject extraContent;
        try {
            extraContent = new JSONObject(originContent);
            imageMessage.setPath(extraContent.getString("path"));
            imageMessage.setUrl(extraContent.getString("url"));
            int loadStatus = extraContent.getInt("loadStatus");

            //todo temp solution
            if(loadStatus == PreDefine.IMAGE_LOADING){
                loadStatus = PreDefine.IMAGE_UNLOAD;
            }
            imageMessage.setLoadStatus(loadStatus);
        } catch (JSONException e) {
            e.printStackTrace();
        }

        return imageMessage;
    }

    // 消息页面，发送图片消息
    public static ImageMessage buildForSend(ImageItem item,UserEntity fromUser,PeerEntity peerEntity){
        ImageMessage msg = new ImageMessage();
        if (new File(item.getImagePath()).exists()) {
            msg.setPath(item.getImagePath());
        } else {
            if (new File(item.getThumbnailPath()).exists()) {
                msg.setPath(item.getThumbnailPath());
            } else {
                // 找不到图片路径时使用加载失败的图片展示
                msg.setPath(null);
            }
        }
        // 将图片发送至服务器
        int nowTime = (int) (System.currentTimeMillis() / 1000);

        msg.setFromId(fromUser.getPeerId());
        msg.setToId(peerEntity.getPeerId());
        msg.setCreated(nowTime);
        msg.setUpdated(nowTime);
        msg.setDisplayType(PreDefine.View_Image_Type);
        // content 自动生成的
        int peerType = peerEntity.getType();
        int msgType = peerType == PreDefine.ST_Group ? PreDefine.MT_GroupText :
                PreDefine.MT_Text;
        msg.setMsgType(msgType);

        msg.setStatus(PreDefine.MSG_SENDING);
        msg.setLoadStatus(PreDefine.IMAGE_UNLOAD);
        msg.buildSessionKey(true);
        return msg;
    }

    public static ImageMessage buildForSend(String takePhotoSavePath,UserEntity fromUser,PeerEntity peerEntity){
        ImageMessage imageMessage = new ImageMessage();
        int nowTime = (int) (System.currentTimeMillis() / 1000);
        imageMessage.setFromId(fromUser.getPeerId());
        imageMessage.setToId(peerEntity.getPeerId());
        imageMessage.setUpdated(nowTime);
        imageMessage.setCreated(nowTime);
        imageMessage.setDisplayType(PreDefine.View_Image_Type);
        imageMessage.setPath(takePhotoSavePath);
        int peerType = peerEntity.getType();
        int msgType = peerType == PreDefine.ST_Group ? PreDefine.MT_GroupText
                : PreDefine.MT_Text;
        imageMessage.setMsgType(msgType);

        imageMessage.setStatus(PreDefine.MSG_SENDING);
        imageMessage.setLoadStatus(PreDefine.IMAGE_UNLOAD);
        imageMessage.buildSessionKey(true);
        return imageMessage;
    }

    /**
     * Not-null value.
     */
    @Override
    public String getContent() {
        JSONObject extraContent = new JSONObject();
        try {
            extraContent.put("path",path);
            extraContent.put("url",url);
            extraContent.put("loadStatus",loadStatus);
            String imageContent = extraContent.toString();
            return imageContent;
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return null;
    }

    @Override
    public byte[] getSendContent() {
        // 发送的时候非常关键
        String sendContent = PreDefine.Image_Magic_Begin
                + url + PreDefine.Image_Magic_End;
        /**
         * 加密
         */
       String  encrySendContent =new String(com.MBCAF.app.network.Security.getInstance().EncryptMsg(sendContent));

        try {
            return encrySendContent.getBytes("utf-8");
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        return null;
    }

    /**-----------------------set/get------------------------*/
    public String getPath() {
        return path;
    }

    public void setPath(String path) {
        this.path = path;
    }

    public String getUrl() {
        return url;
    }

    public void setUrl(String url) {
        this.url = url;
    }

    public int getLoadStatus() {
        return loadStatus;
    }

    public void setLoadStatus(int loadStatus) {
        this.loadStatus = loadStatus;
    }
}
