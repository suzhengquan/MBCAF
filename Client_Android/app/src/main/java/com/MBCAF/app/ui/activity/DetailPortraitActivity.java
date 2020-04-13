package com.MBCAF.app.ui.activity;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;

import com.MBCAF.app.PreDefine;
import com.MBCAF.R;
import com.MBCAF.common.IMUIHelper;
import com.MBCAF.common.Logger;
import com.MBCAF.app.ui.widget.ZoomableImageView;

public class DetailPortraitActivity extends Activity  {

	private Logger logger = Logger.getLogger(DetailPortraitActivity.class);
    public static String imageUri = "";
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.tt_activity_detail_portrait);
		
		Intent intent = getIntent();
		if (intent == null) {
			logger.e("detailPortrait#displayimage#null intent");
			return;
		}

		String resUri = intent.getStringExtra(PreDefine.KEY_AVATAR_URL);
        imageUri = resUri;
		logger.d("detailPortrait#displayimage#resUri:%s", resUri);

		boolean isContactAvatar = intent.getBooleanExtra(PreDefine.KEY_IS_IMAGE_CONTACT_AVATAR, false);
		logger.d("displayimage#isContactAvatar:%s", isContactAvatar);

		final ZoomableImageView portraitView = (ZoomableImageView) findViewById(R.id.detail_portrait);


		if (portraitView == null) {
			logger.e("detailPortrait#displayimage#portraitView is null");
			return;
		}

		logger.d("detailPortrait#displayimage#going to load the detail portrait");


		if (isContactAvatar) {
			IMUIHelper.setEntityImageViewAvatarNoDefaultPortrait(portraitView, resUri, PreDefine.ST_Single, 0);
		} else {
			IMUIHelper.displayImageNoOptions(portraitView, resUri, -1, 0);
		}

        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                portraitView.setFinishActivity(new finishActivity() {
                    @Override
                    public void finish() {
                        if(DetailPortraitActivity.this!=null)
                        {
                            DetailPortraitActivity.this.finish();
                            overridePendingTransition(
                                    R.anim.tt_stay, R.anim.tt_image_exit);
                        }
                    }
                });
            }
        },500);

	}
	@Override
	protected void onDestroy() {
		super.onDestroy();
	}

    public interface finishActivity{
        public void finish();
    }

}
