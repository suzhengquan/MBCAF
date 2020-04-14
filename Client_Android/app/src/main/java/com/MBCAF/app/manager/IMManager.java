package com.MBCAF.app.manager;

import android.content.Context;

public abstract class IMManager {
	protected Context ctx;

    public void  setup(Context context){
		if (context == null) {
			throw new RuntimeException("context is null");
		}
		ctx = context;
        onStart();
    }

    public abstract  void onStart();

	public abstract void reset();
}
