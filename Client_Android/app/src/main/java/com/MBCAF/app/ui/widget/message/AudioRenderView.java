package com.MBCAF.app.ui.widget.message;

import android.content.Context;
import android.graphics.drawable.AnimationDrawable;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.MBCAF.db.entity.MessageEntity;
import com.MBCAF.db.entity.UserEntity;
import com.MBCAF.R;
import com.MBCAF.app.PreDefine;
import com.MBCAF.app.ui.helper.AudioPlayerHandler;
import com.MBCAF.app.entity.AudioMessage;
import com.MBCAF.common.CommonUtil;
import com.MBCAF.common.SysUtil;

import java.io.File;

public class AudioRenderView extends  BaseMsgRenderView {
    /**可点击的消息体*/
    private View messageLayout;
    /** 播放动画的view*/
    private View audioAnttView;
    private View audioUnreadNotify;
    private TextView audioDuration;

    private BtnImageListener btnImageListener;
    public interface  BtnImageListener{
        public void  onClickUnread();
        public void  onClickReaded();
    }

    public void setBtnImageListener(BtnImageListener btnImageListener){
        this.btnImageListener = btnImageListener;
    }

    public AudioRenderView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public static AudioRenderView inflater(Context ctx,ViewGroup viewGroup,boolean isMine){

        int resoure = isMine?R.layout.tt_mine_audio_message_item:R.layout.tt_other_audio_message_item;
        //tt_other_audio_message_item
        AudioRenderView audioRenderView = (AudioRenderView) LayoutInflater.from(ctx).inflate(resoure,viewGroup,false);
        audioRenderView.setMine(isMine);
        audioRenderView.setParentView(viewGroup);
        return audioRenderView;
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        messageLayout= findViewById(R.id.message_layout);
        audioAnttView = findViewById(R.id.audio_antt_view);
        audioDuration = (TextView) findViewById(R.id.audio_duration);
        audioUnreadNotify = findViewById(R.id.audio_unread_notify);
    }

    @Override
    public void render(final MessageEntity messageEntity,final UserEntity userEntity,final Context ctx) {
        super.render(messageEntity, userEntity,ctx);
        final AudioMessage audioMessage = (AudioMessage)messageEntity;

        final String audioPath = audioMessage.getAudioPath();
        final int audioReadStatus = audioMessage.getReadStatus();

        messageLayout.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                // 上层回调事件
                if (!new File(audioPath).exists()) {
                    Toast.makeText(ctx, ctx.getResources().getString(R.string.t030), Toast.LENGTH_LONG).show();
                    return;
                }
                switch (audioReadStatus){
                    case PreDefine.AUDIO_UNREAD:
                        if(btnImageListener != null){
                            btnImageListener.onClickUnread();
                            audioUnreadNotify.setVisibility(View.GONE);
                        }
                        break;
                    case PreDefine.AUDIO_READED:
                        if(btnImageListener != null){
                            btnImageListener.onClickReaded();
                        }
                        break;
                }


                // 获取播放路径，播放语音
                String currentPlayPath = AudioPlayerHandler.getInstance().getCurrentPlayPath();
                if (currentPlayPath !=null && AudioPlayerHandler.getInstance().isPlaying()) {
                    AudioPlayerHandler.getInstance().stopPlayer();
                    if (currentPlayPath.equals(audioPath)) {
                        // 关闭当前的
                        return;
                    }
                }

                final AnimationDrawable animationDrawable = (AnimationDrawable) audioAnttView.getBackground();
                AudioPlayerHandler.getInstance().setAudioListener(new AudioPlayerHandler.AudioListener() {
                    @Override
                    public void onStop() {
                        if(animationDrawable!=null && animationDrawable.isRunning()){
                            animationDrawable.stop();
                            animationDrawable.selectDrawable(0);
                        }
                    }
                });

                // 延迟播放
                Thread myThread = new Thread() {
                    public void run() {
                        try {
                            Thread.sleep(200);
                            AudioPlayerHandler.getInstance().startPlay(audioPath);
                            animationDrawable.start();
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                };
                myThread.start();
            }
        });

        //针对path 的设定
        if (null != audioPath) {
            int resource = isMine?R.drawable.tt_voice_play_mine:R.drawable.tt_voice_play_other;
            audioAnttView.setBackgroundResource(resource);
            AnimationDrawable animationDrawable = (AnimationDrawable) audioAnttView.getBackground();

            if (null != AudioPlayerHandler.getInstance().getCurrentPlayPath()
                    && AudioPlayerHandler.getInstance().getCurrentPlayPath().equals(audioPath)) {
                animationDrawable.start();
            } else {
                if (animationDrawable.isRunning()) {
                    animationDrawable.stop();
                    animationDrawable.selectDrawable(0);
                }
            }

            switch (audioReadStatus){
                case PreDefine.AUDIO_READED:
                    audioAlreadyRead();
                    break;
                case PreDefine.AUDIO_UNREAD:
                    audioUnread();
                    break;
            }

            int audioLength =  audioMessage.getAudioLength();
            audioDuration.setText(String.valueOf(audioLength) + '"');
            // messageLayout 的长按事件绑定 在上层做掉，有时间了再迁移

            //getAudioBkSize ,在看一下
            int width = CommonUtil.getAudioBkSize(audioLength, ctx);
            if(width< SysUtil.instance(ctx).dip2px(45))
            {
                width = SysUtil.instance(ctx).dip2px(45);
            }
            RelativeLayout.LayoutParams layoutParam = new RelativeLayout.LayoutParams(width, LayoutParams.WRAP_CONTENT);
            messageLayout.setLayoutParams(layoutParam);
            if (isMine) {
                layoutParam.addRule(RelativeLayout.RIGHT_OF, R.id.audio_duration);
                RelativeLayout.LayoutParams param = (android.widget.RelativeLayout.LayoutParams) audioDuration.getLayoutParams();
                param.addRule(RelativeLayout.RIGHT_OF, R.id.message_state_failed);
                param.addRule(RelativeLayout.RIGHT_OF, R.id.progressBar1);
            }
        }
    }

    // 是否开启播放动画
    public void  startAnimation(){
        AnimationDrawable animationDrawable = (AnimationDrawable) audioAnttView.getBackground();
        animationDrawable.start();
    }

    public void stopAnimation(){
        AnimationDrawable animationDrawable = (AnimationDrawable) audioAnttView.getBackground();
        if (animationDrawable.isRunning()) {
                animationDrawable.stop();
                animationDrawable.selectDrawable(0);
        }
    }

    // unread 与 alreadRead的区别是什么
    private void audioUnread(){
        if(isMine){
            audioUnreadNotify.setVisibility(View.GONE);
        }else{
            audioUnreadNotify.setVisibility(View.VISIBLE);
        }
    }

    private void  audioAlreadyRead(){
        audioUnreadNotify.setVisibility(View.GONE);
    }

    public View getMessageLayout() {
        return messageLayout;
    }

    public void setMine(boolean isMine) {
        this.isMine = isMine;
    }

    public void setParentView(ViewGroup parentView) {
        this.parentView = parentView;
    }
}
