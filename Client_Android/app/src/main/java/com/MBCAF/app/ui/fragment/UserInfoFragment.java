package com.MBCAF.app.ui.fragment;

import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.MBCAF.db.entity.DepartmentEntity;
import com.MBCAF.db.entity.UserEntity;
import com.MBCAF.R;
import com.MBCAF.app.PreDefine;
import com.MBCAF.common.IMUIHelper;
import com.MBCAF.app.event.CommonEvent;
import com.MBCAF.app.manager.IMLoginManager;
import com.MBCAF.app.network.IMService;
import com.MBCAF.app.ui.activity.DetailPortraitActivity;
import com.MBCAF.app.network.IMServiceConnector;
import com.MBCAF.app.ui.widget.IMBaseImageView;

import java.util.ArrayList;

public class UserInfoFragment extends MainFragment {

	private View curView = null;
    private IMService imService;
    private UserEntity currentUser;
    private int currentUserId;
    private IMServiceConnector imServiceConnector = new IMServiceConnector(){
        @Override
        public void onIMServiceConnected() {
            logger.d("detail#onIMServiceConnected");

            imService = imServiceConnector.getIMService();
            if (imService == null) {
                logger.e("detail#imService is null");
                return;
            }

            currentUserId = getActivity().getIntent().getIntExtra(PreDefine.KEY_PEERID,0);
            if(currentUserId == 0){
                logger.e("detail#intent params error!!");
                return;
            }
            currentUser = imService.getContactManager().findContact(currentUserId);
            if(currentUser != null) {
                initBaseProfile();
                initDetailProfile();
            }
            ArrayList<Integer> userIds = new ArrayList<>(1);
            //just single type
            userIds.add(currentUserId);
            imService.getContactManager().reqGetDetaillUsers(userIds);
        }
        @Override
        public void onServiceDisconnected() {}
    };

	@Override
	public void onDestroyView() {
		super.onDestroyView();
		imServiceConnector.disconnect(getActivity());
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {

		imServiceConnector.connect(getActivity());
		if (null != curView) {
			((ViewGroup) curView.getParent()).removeView(curView);
			return curView;
		}
		curView = inflater.inflate(R.layout.tt_fragment_user_detail, topContentView);
		super.init(curView);
		showProgressBar();
		initRes();
		return curView;
	}

	@Override
	public void onResume() {
		Intent intent = getActivity().getIntent();
		if (null != intent) {
			String fromPage = intent.getStringExtra(PreDefine.USER_DETAIL_PARAM);
			setTopLeftText(fromPage);
		}
		super.onResume();
	}

	/**
	 * @Description 初始化资源
	 */
	private void initRes() {
		// 设置标题栏
		setTopTitle(getActivity().getString(R.string.t007));
		setTopLeftButton(R.drawable.tt_top_back);
		topLeftContainerLayout.setOnClickListener(new View.OnClickListener() {

			@Override
			public void onClick(View arg0) {
				getActivity().finish();
			}
		});
		setTopLeftText(getResources().getString(R.string.t058));
	}

	@Override
	protected void initHandler() {
	}

    public void onEventMainThread(CommonEvent event){
        switch (event){
            case CE_User_InfoUpdate:
                UserEntity entity  = imService.getContactManager().findContact(currentUserId);
                if(entity !=null && currentUser.equals(entity)){
                    initBaseProfile();
                    initDetailProfile();
                }
                break;
        }
    }


	private void initBaseProfile() {
		logger.d("detail#initBaseProfile");
        IMBaseImageView portraitImageView = (IMBaseImageView) curView.findViewById(R.id.user_portrait);

		setTextViewContent(R.id.nickName, currentUser.getMainName());
		setTextViewContent(R.id.userName, currentUser.getRealName());
        //头像设置
        portraitImageView.setDefaultImageRes(R.drawable.tt_default_user_portrait_corner);
        portraitImageView.setCorner(8);
        portraitImageView.setImageResource(R.drawable.tt_default_user_portrait_corner);
        portraitImageView.setImageUrl(currentUser.getAvatar());

		portraitImageView.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				Intent intent = new Intent(getActivity(), DetailPortraitActivity.class);
				intent.putExtra(PreDefine.KEY_AVATAR_URL, currentUser.getAvatar());
				intent.putExtra(PreDefine.KEY_IS_IMAGE_CONTACT_AVATAR, true);
				
				startActivity(intent);
			}
		});

		// 设置界面信息
		Button chatBtn = (Button) curView.findViewById(R.id.chat_btn);
		if (currentUserId == imService.getLoginManager().getLoginId()) {
			chatBtn.setVisibility(View.GONE);
		}else{
            chatBtn.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View arg0) {
                    IMUIHelper.openChatActivity(getActivity(),currentUser.getSessionKey());
                    getActivity().finish();
                }
            });

        }
	}

	private void initDetailProfile() {
		logger.d("detail#initDetailProfile");
		hideProgressBar();
        DepartmentEntity deptEntity = imService.getContactManager().findDepartment(currentUser.getDepartmentId());
		setTextViewContent(R.id.department,deptEntity.getDepartName());
		setTextViewContent(R.id.telno, currentUser.getPhone());
		setTextViewContent(R.id.email, currentUser.getEmail());

		View phoneView = curView.findViewById(R.id.phoneArea);
        View emailView = curView.findViewById(R.id.emailArea);
		IMUIHelper.setViewTouchHightlighted(phoneView);
        IMUIHelper.setViewTouchHightlighted(emailView);

        emailView.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View view) {
                if (currentUserId == IMLoginManager.instance().getLoginId())
                    return;
                IMUIHelper.showCustomDialog(getActivity(),View.GONE,String.format(getString(R.string.t104),currentUser.getEmail()),new IMUIHelper.dialogCallback() {
                    @Override
                    public void callback() {
                        Intent data=new Intent(Intent.ACTION_SENDTO);
                        data.setData(Uri.parse("mailto:" + currentUser.getEmail()));
                        data.putExtra(Intent.EXTRA_SUBJECT, "");
                        data.putExtra(Intent.EXTRA_TEXT, "");
                        startActivity(data);
                    }
                });
            }
        });

		phoneView.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (currentUserId == IMLoginManager.instance().getLoginId())
					return;
                IMUIHelper.showCustomDialog(getActivity(),View.GONE,String.format(getString(R.string.t103),currentUser.getPhone()),new IMUIHelper.dialogCallback() {
                    @Override
                    public void callback() {
                        new Handler().postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                IMUIHelper.callPhone(getActivity(), currentUser.getPhone());
                            }
                        },0);
                    }
                });
			}
		});

		setSex(currentUser.getGender());
	}

	private void setTextViewContent(int id, String content) {
		TextView textView = (TextView) curView.findViewById(id);
		if (textView == null) {
			return;
		}

		textView.setText(content);
	}

	private void setSex(int sex) {
		if (curView == null) {
			return;
		}

		TextView sexTextView = (TextView) curView.findViewById(R.id.sex);
		if (sexTextView == null) {
			return;
		}

		int textColor = Color.rgb(255, 138, 168); //xiaoxian
		String text = getString(R.string.t117);

		if (sex == PreDefine.SEX_MAILE) {
			textColor = Color.rgb(144, 203, 1);
			text = getString(R.string.t118);
		}

		sexTextView.setVisibility(View.VISIBLE);
		sexTextView.setText(text);
		sexTextView.setTextColor(textColor);
	}

}
