package com.MBCAF.app.ui.widget.message;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.ImageView;

import com.MBCAF.db.entity.MessageEntity;
import com.MBCAF.db.entity.UserEntity;
import com.MBCAF.R;
import com.MBCAF.app.entity.TextMessage;
import com.MBCAF.app.ui.helper.Emoparser;
import com.MBCAF.app.ui.widget.GifView;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

/**
 * 虽然gif与text在服务端是同一种消息类型，但是在客户端应该区分开来
 */
public class EmojiRenderView extends  BaseMsgRenderView {
    private GifView messageContent;

    public static EmojiRenderView inflater(Context context,ViewGroup viewGroup,boolean isMine){
        int resource = isMine?R.layout.tt_mine_emoji_message_item :R.layout.tt_other_emoji_message_item;
        EmojiRenderView gifRenderView = (EmojiRenderView) LayoutInflater.from(context).inflate(resource, viewGroup, false);
        gifRenderView.setMine(isMine);
        gifRenderView.setParentView(viewGroup);
        return gifRenderView;
    }

    public EmojiRenderView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    protected void onFinishInflate() {
        super.onFinishInflate();
        messageContent = (GifView) findViewById(R.id.message_content);
    }

    /**
     * 控件赋值
     * @param messageEntity
     * @param userEntity
     */
    @Override
    public void render(MessageEntity messageEntity, UserEntity userEntity,Context context) {
        super.render(messageEntity, userEntity,context);
        TextMessage textMessage = (TextMessage) messageEntity;
        String content = textMessage.getContent();

        InputStream is = getResources().openRawResource(Emoparser.getInstance(getContext()).getResIdByCharSequence(content));
        int lenght = 0;
        try {
            lenght = is.available();
            byte[]  buffer = ByteBuffer.allocate(lenght).array();
            is.read(buffer);
            messageContent.setBytes(buffer);
            messageContent.startAnimation();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void msgFailure(MessageEntity messageEntity) {
        super.msgFailure(messageEntity);
    }


    /**----------------set/get---------------------------------*/
    public ImageView getMessageContent() {
        return messageContent;
    }

    public void setMessageContent(GifView messageContent) {
        this.messageContent = messageContent;
    }

    public boolean isMine() {
        return isMine;
    }

    public void setMine(boolean isMine) {
        this.isMine = isMine;
    }

    public ViewGroup getParentView() {
        return parentView;
    }

    public void setParentView(ViewGroup parentView) {
        this.parentView = parentView;
    }

    private byte[] Bitmap2Bytes(Bitmap bm){
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        bm.compress(Bitmap.CompressFormat.PNG, 100, baos);
        return baos.toByteArray();
    }
}

