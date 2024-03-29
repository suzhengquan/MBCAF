
package com.MBCAF.app.ui.adapter;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView.LayoutParams;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;

import com.MBCAF.common.Logger;

public class YayaEmoGridViewAdapter extends BaseAdapter {
    private Context context = null;
    private int[] emoResIds = null;
    private static Logger logger = Logger.getLogger(YayaEmoGridViewAdapter.class);

    public YayaEmoGridViewAdapter(Context cxt, int[] ids) {
        this.context = cxt;
        this.emoResIds = ids;
    }

    @Override
    public int getCount() {
        return emoResIds.length;
    }

    @Override
    public Object getItem(int position) {
        return emoResIds[position];
    }

    @Override
    public long getItemId(int position) {
        return emoResIds[position];
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        try {
            GridViewHolder gridViewHolder = null;
            if (null == convertView && null != context) {
                gridViewHolder = new GridViewHolder();
                convertView = gridViewHolder.layoutView;
                if (convertView != null) {
                    convertView.setTag(gridViewHolder);
                }
            } else {
                gridViewHolder = (GridViewHolder) convertView.getTag();
            }
            if (null == gridViewHolder || null == convertView) {
                return null;
            }
            gridViewHolder.faceIv.setImageBitmap(getBitmap(position));
            return convertView;
        } catch (Exception e) {
            logger.e(e.getMessage());
            return null;
        }
    }

    private Bitmap getBitmap(int position) {
        Bitmap bitmap = null;
        try {
            bitmap = BitmapFactory.decodeResource(context.getResources(),
                    emoResIds[position]);
        } catch (Exception e) {
            logger.e(e.getMessage());
        }
        return bitmap;
    }

    public class GridViewHolder {
        public LinearLayout layoutView;
        public ImageView faceIv;

        public GridViewHolder() {
            try {
                LayoutParams layoutParams = new LayoutParams(
                        LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
                layoutView = new LinearLayout(context);
                faceIv = new ImageView(context);
                layoutView.setLayoutParams(layoutParams);
                layoutView.setOrientation(LinearLayout.VERTICAL);
                layoutView.setGravity(Gravity.CENTER);
                LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                        105,
                        115);
                params.gravity = Gravity.CENTER;
                layoutView.addView(faceIv, params);
            } catch (Exception e) {
                logger.e(e.getMessage());
            }
        }
    }
}
