package com.MBCAF.app.ui.activity;

import android.os.Bundle;

import com.MBCAF.R;
import com.MBCAF.app.manager.IMStackManager;
import com.MBCAF.app.ui.base.TTBaseFragmentActivity;

public class SearchActivity extends   TTBaseFragmentActivity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
        IMStackManager.getStackManager().pushActivity(this);
		setContentView(R.layout.tt_fragment_activity_search);
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
        IMStackManager.getStackManager().popActivity(this);
		super.onDestroy();
	}

}
