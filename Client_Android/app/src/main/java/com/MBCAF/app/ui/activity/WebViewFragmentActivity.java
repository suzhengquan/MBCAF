package com.MBCAF.app.ui.activity;

import android.content.Intent;
import android.os.Bundle;

import com.MBCAF.R;
import com.MBCAF.app.PreDefine;
import com.MBCAF.app.ui.base.TTBaseFragmentActivity;
import com.MBCAF.app.ui.fragment.WebviewFragment;

public class WebViewFragmentActivity extends TTBaseFragmentActivity {
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Intent intent=getIntent();
		if (intent.hasExtra(PreDefine.WEBVIEW_URL)) {
			WebviewFragment.setUrl(intent.getStringExtra(PreDefine.WEBVIEW_URL));
		}
		setContentView(R.layout.tt_fragment_activity_webview);
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
	}
}
