package com.MBCAF.app.ui.fragment;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;

import com.MBCAF.db.sp.ConfigurationSp;
import com.MBCAF.R;
import com.MBCAF.app.PreDefine;
import com.MBCAF.app.network.IMService;
import com.MBCAF.app.ui.helper.CheckboxConfigHelper;
import com.MBCAF.app.ui.base.TTBaseFragment;
import com.MBCAF.app.network.IMServiceConnector;

public class SettingFragment extends TTBaseFragment{
	private View curView = null;
	private CheckBox notificationNoDisturbCheckBox;
	private CheckBox notificationGotSoundCheckBox;
	private CheckBox notificationGotVibrationCheckBox;
	CheckboxConfigHelper checkBoxConfiger = new CheckboxConfigHelper();


    private IMServiceConnector imServiceConnector = new IMServiceConnector(){
        @Override
        public void onIMServiceConnected() {
            logger.d("config#onIMServiceConnected");
            IMService imService = imServiceConnector.getIMService();
            if (imService != null) {
                checkBoxConfiger.init(imService.getConfigSp());
                initOptions();
            }
        }

        @Override
        public void onServiceDisconnected() {
        }
    };

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		imServiceConnector.connect(this.getActivity());
		if (null != curView) {
			((ViewGroup) curView.getParent()).removeView(curView);
			return curView;
		}
		curView = inflater.inflate(R.layout.tt_fragment_setting, topContentView);
		initRes();
		return curView;
	}

    /**
     * Called when the fragment is no longer in use.  This is called
     * after {@link #onStop()} and before {@link #onDetach()}.
     */
    @Override
    public void onDestroy() {
        super.onDestroy();
        imServiceConnector.disconnect(getActivity());
    }

    private void initOptions() {
		notificationNoDisturbCheckBox = (CheckBox) curView.findViewById(R.id.NotificationNoDisturbCheckbox);
		notificationGotSoundCheckBox = (CheckBox) curView.findViewById(R.id.notifyGotSoundCheckBox);
		notificationGotVibrationCheckBox = (CheckBox) curView.findViewById(R.id.notifyGotVibrationCheckBox);
//		saveTrafficModeCheckBox = (CheckBox) curView.findViewById(R.id.saveTrafficCheckBox);
		
		checkBoxConfiger.initCheckBox(notificationNoDisturbCheckBox, PreDefine.SETTING_GLOBAL, ConfigurationSp.CfgDimension.NOTIFICATION );
		checkBoxConfiger.initCheckBox(notificationGotSoundCheckBox, PreDefine.SETTING_GLOBAL , ConfigurationSp.CfgDimension.SOUND);
		checkBoxConfiger.initCheckBox(notificationGotVibrationCheckBox, PreDefine.SETTING_GLOBAL,ConfigurationSp.CfgDimension.VIBRATION );
//		checkBoxConfiger.initCheckBox(saveTrafficModeCheckBox, ConfigDefs.SETTING_GLOBAL, ConfigDefs.KEY_SAVE_TRAFFIC_MODE, ConfigDefs.DEF_VALUE_SAVE_TRAFFIC_MODE);
	}

	@Override
	public void onResume() {

		super.onResume();
	}

	/**
	 * @Description 初始化资源
	 */
	private void initRes() {
		// 设置标题栏
		setTopTitle(getActivity().getString(R.string.t105));
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

}
