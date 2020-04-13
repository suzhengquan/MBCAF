package com.MBCAF.app.ui.fragment;

import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.GridView;
import android.widget.TextView;
import android.widget.Toast;

import com.MBCAF.db.entity.PeerEntity;
import com.MBCAF.app.PreDefine;
import com.MBCAF.db.entity.GroupEntity;
import com.MBCAF.db.entity.UserEntity;
import com.MBCAF.db.sp.ConfigurationSp;
import com.MBCAF.R;
import com.MBCAF.app.ui.adapter.GroupManagerAdapter;
import com.MBCAF.app.ui.helper.CheckboxConfigHelper;
import com.MBCAF.app.event.GroupEvent;
import com.MBCAF.app.network.IMService;
import com.MBCAF.app.ui.base.TTBaseFragment;
import com.MBCAF.app.network.IMServiceConnector;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.listener.PauseOnScrollListener;

import java.util.ArrayList;
import java.util.List;

import de.greenrobot.event.EventBus;

public class GroupManagerFragment extends TTBaseFragment{
    private View curView = null;
    /**adapter配置*/
    private GridView gridView;
    private GroupManagerAdapter adapter;


    /**详情的配置  勿扰以及指定聊天*/
    CheckboxConfigHelper checkBoxConfiger = new CheckboxConfigHelper();
    CheckBox noDisturbCheckbox;
    CheckBox topSessionCheckBox;

    /**需要的状态参数*/
    private IMService imService;
    private String curSessionKey;
    private PeerEntity peerEntity;


    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        imServiceConnector.connect(getActivity());
        EventBus.getDefault().register(this);
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,Bundle savedInstanceState) {
        if (null != curView) {
            ((ViewGroup) curView.getParent()).removeView(curView);
            return curView;
        }
        curView = inflater.inflate(R.layout.tt_fragment_group_manage, topContentView);
        noDisturbCheckbox = (CheckBox) curView.findViewById(R.id.NotificationNoDisturbCheckbox);
        topSessionCheckBox = (CheckBox) curView.findViewById(R.id.NotificationTopMessageCheckbox);
        initRes();
        return curView;
    }

    private void initRes() {
        // 设置标题栏
        setTopLeftButton(R.drawable.tt_top_back);
        setTopLeftText(getActivity().getString(R.string.t058));
        topLeftContainerLayout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                getActivity().finish();
            }
        });
    }


    @Override
    public void onDestroyView() {
        super.onDestroyView();
    }

    /**
     * Called when the fragment is no longer in use.  This is called
     * after {@link #onStop()} and before {@link #onDetach()}.
     */
    @Override
    public void onDestroy() {
        super.onDestroy();
        EventBus.getDefault().unregister(this);
        imServiceConnector.disconnect(getActivity());
    }

    @Override
    protected void initHandler() {
    }


    private IMServiceConnector imServiceConnector = new IMServiceConnector(){
        @Override
        public void onServiceDisconnected() {
        }

        @Override
        public void onIMServiceConnected() {
            logger.d("groupmgr#onIMServiceConnected");
            imService = imServiceConnector.getIMService();
            if(imService == null){
              Toast.makeText(GroupManagerFragment.this.getActivity(),
                        getResources().getString(R.string.t038), Toast.LENGTH_SHORT).show();
               return;
            }
            checkBoxConfiger.init(imService.getConfigSp());
            initView();
            initAdapter();
        }
    };


    private void initView() {
        setTopTitle(getString(R.string.t060));
        if (null == imService || null == curView ) {
            logger.e("groupmgr#init failed,cause by imService or curView is null");
            return;
        }

        curSessionKey =  getActivity().getIntent().getStringExtra(PreDefine.KEY_SESSION_KEY);
        if (TextUtils.isEmpty(curSessionKey)) {
            logger.e("groupmgr#getSessionInfoFromIntent failed");
            return;
        }
        peerEntity = imService.getSessionManager().findPeerEntity(curSessionKey);
        if(peerEntity == null){
            logger.e("groupmgr#findPeerEntity failed,sessionKey:%s",curSessionKey);
            return;
        }
        switch (peerEntity.getType()){
            case PreDefine.ST_Group:{
                GroupEntity groupEntity = (GroupEntity) peerEntity;
                // 群组名称的展示
                TextView groupNameView = (TextView) curView.findViewById(R.id.group_manager_title);
                groupNameView.setText(groupEntity.getMainName());
            }break;

            case PreDefine.ST_Single:{
                // 个人不显示群聊名称
                View groupNameContainerView = curView.findViewById(R.id.group_manager_name);
                groupNameContainerView.setVisibility(View.GONE);
            }break;
        }
        // 初始化配置checkBox
        initCheckbox();
    }

    private void initAdapter(){
        logger.d("groupmgr#initAdapter");

        gridView = (GridView) curView.findViewById(R.id.group_manager_grid);
        gridView.setSelector(new ColorDrawable(Color.TRANSPARENT));// 去掉点击时的黄色背影
        gridView.setOnScrollListener(new PauseOnScrollListener(ImageLoader.getInstance(), true, true));

        adapter = new GroupManagerAdapter(getActivity(),imService,peerEntity);
        gridView.setAdapter(adapter);
    }

    /**事件驱动通知*/
    public void onEventMainThread(GroupEvent event){
        switch (event.getEvent()){

            case CHANGE_GROUP_MEMBER_FAIL:
            case CHANGE_GROUP_MEMBER_TIMEOUT:{
                Toast.makeText(getActivity(), getString(R.string.t123), Toast.LENGTH_SHORT).show();
                return;
            }
            case CHANGE_GROUP_MEMBER_SUCCESS:{
                onMemberChangeSuccess(event);
            }break;
        }
    }

    private void onMemberChangeSuccess(GroupEvent event){
        int groupId = event.getGroupEntity().getPeerId();
        if(groupId != peerEntity.getPeerId()){
            return;
        }
        List<Integer> changeList = event.getChangeList();
        if(changeList == null || changeList.size()<=0){
            return;
        }
        int changeType = event.getChangeType();

        switch (changeType){
            case PreDefine.GMT_Add:
                ArrayList<UserEntity> newList = new ArrayList<>();
                for(Integer userId:changeList){
                    UserEntity userEntity =  imService.getContactManager().findContact(userId);
                    if(userEntity!=null) {
                        newList.add(userEntity);
                    }
                }
                adapter.add(newList);
                break;
            case PreDefine.GMT_delete:
                for(Integer userId:changeList){
                    adapter.removeById(userId);
                }
                break;
        }
    }

	private void initCheckbox() {
        checkBoxConfiger.initCheckBox(noDisturbCheckbox, curSessionKey, ConfigurationSp.CfgDimension.NOTIFICATION);
        checkBoxConfiger.initTopCheckBox(topSessionCheckBox,curSessionKey);
    }
}
