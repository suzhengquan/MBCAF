package com.MBCAF.common;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.DisplayMetrics;
import android.view.WindowManager;

import com.MBCAF.R;
import com.MBCAF.app.PreDefine;
import com.MBCAF.app.ui.helper.CircleBitmapDisplayer;
import com.nostra13.universalimageloader.cache.disc.impl.UnlimitedDiscCache;
import com.nostra13.universalimageloader.cache.disc.naming.Md5FileNameGenerator;
import com.nostra13.universalimageloader.cache.memory.impl.UsingFreqLimitedMemoryCache;
import com.nostra13.universalimageloader.core.DisplayImageOptions;
import com.nostra13.universalimageloader.core.ImageLoader;
import com.nostra13.universalimageloader.core.ImageLoaderConfiguration;
import com.nostra13.universalimageloader.core.assist.ImageScaleType;
import com.nostra13.universalimageloader.core.assist.QueueProcessingType;
import com.nostra13.universalimageloader.core.display.RoundedBitmapDisplayer;
import com.nostra13.universalimageloader.utils.StorageUtils;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.util.DisplayMetrics;

import com.MBCAF.app.ui.helper.PhotoHelper;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

public class ImageUtil {
    private static Logger logger = Logger.getLogger(ImageUtil.class);
    private static ImageLoaderConfiguration IMImageLoaderConfig;
    private static ImageLoader IMImageLoadInstance;
    private static Map<Integer,Map<Integer,DisplayImageOptions>> avatarOptionsMaps=new HashMap<Integer,Map<Integer,DisplayImageOptions>>();
    public final static int CIRCLE_CORNER = -10;

    public static Bitmap getBigBitmapForDisplay(String imagePath,
                                                Context context) {
        if (null == imagePath || !new File(imagePath).exists())
            return null;
        try {
            int degeree = PhotoHelper.readPictureDegree(imagePath);
            Bitmap bitmap = BitmapFactory.decodeFile(imagePath);
            if (bitmap == null)
                return null;
            DisplayMetrics dm = new DisplayMetrics();
            ((Activity) context).getWindowManager().getDefaultDisplay().getMetrics(dm);
            float scale = bitmap.getWidth() / (float) dm.widthPixels;
            Bitmap newBitMap = null;
            if (scale > 1) {
                newBitMap = zoomBitmap(bitmap, (int) (bitmap.getWidth() / scale), (int) (bitmap.getHeight() / scale));
                bitmap.recycle();
                Bitmap resultBitmap = PhotoHelper.rotaingImageView(degeree, newBitMap);
                return resultBitmap;
            }
            Bitmap resultBitmap = PhotoHelper.rotaingImageView(degeree, bitmap);
            return resultBitmap;
        } catch (Exception e) {
            logger.e(e.getMessage());
            return null;
        }
    }

    private static Bitmap zoomBitmap(Bitmap bitmap, int width, int height) {
        if (null == bitmap) {
            return null;
        }
        try {
            int w = bitmap.getWidth();
            int h = bitmap.getHeight();
            Matrix matrix = new Matrix();
            float scaleWidth = ((float) width / w);
            float scaleHeight = ((float) height / h);
            matrix.postScale(scaleWidth, scaleHeight);
            Bitmap newbmp = Bitmap.createBitmap(bitmap, 0, 0, w, h, matrix, true);
            return newbmp;
        } catch (Exception e) {
            logger.e(e.getMessage());
            return null;
        }
    }

    public static void initImageLoaderConfig(Context context) {
        try {
            File cacheDir = StorageUtils.getOwnCacheDirectory(context, CommonUtil.getSavePath(PreDefine.FILE_SAVE_TYPE_IMAGE));
            File reserveCacheDir = StorageUtils.getCacheDirectory(context);

            int maxMemory = (int) (Runtime.getRuntime().maxMemory() );
            // 使用最大可用内存值的1/8作为缓存的大小。
            int cacheSize = maxMemory/8;
            DisplayMetrics metrics=new DisplayMetrics();
            WindowManager mWm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
            mWm.getDefaultDisplay().getMetrics(metrics);

            IMImageLoaderConfig = new ImageLoaderConfiguration.Builder(context)
                    .memoryCacheExtraOptions(metrics.widthPixels, metrics.heightPixels)
                    .threadPriority(Thread.NORM_PRIORITY-2)
//                    .denyCacheImageMultipleSizesInMemory()
                    .memoryCache(new UsingFreqLimitedMemoryCache(cacheSize))
                    .diskCacheFileNameGenerator(new Md5FileNameGenerator())
                    .tasksProcessingOrder(QueueProcessingType.LIFO)
                    .diskCacheExtraOptions(metrics.widthPixels, metrics.heightPixels, null)
                    .diskCache(new UnlimitedDiscCache(cacheDir,reserveCacheDir,new Md5FileNameGenerator()))
                    .diskCacheSize(1024 * 1024 * 1024)
                    .diskCacheFileCount(1000)
                    .build();

            IMImageLoadInstance = ImageLoader.getInstance();
            IMImageLoadInstance.init(IMImageLoaderConfig);
        }catch (Exception e){
            logger.e(e.toString());
        }
    }

    public static ImageLoader getImageLoaderInstance() {
        return IMImageLoadInstance;
    }

    public static DisplayImageOptions getAvatarOptions(int corner,int defaultRes){
        try {
            if (defaultRes <= 0) {
                defaultRes = R.drawable.tt_default_user_portrait_corner;
            }
            if (avatarOptionsMaps.containsKey(defaultRes)) {
                Map<Integer, DisplayImageOptions> displayOption = avatarOptionsMaps.get(defaultRes);
                if (displayOption.containsKey(corner)) {
                    return displayOption.get(corner);
                }
            }
            DisplayImageOptions newDisplayOption = null;
            if (corner==CIRCLE_CORNER) {
                newDisplayOption = new DisplayImageOptions.Builder()
                        .showImageOnFail(defaultRes)
                        .showImageForEmptyUri(defaultRes)
                        .cacheInMemory(true)
                        .resetViewBeforeLoading(true)
                        .displayer(new CircleBitmapDisplayer())
                        .build();
            } else {
                if (corner < 0) {
                    corner = 0;
                }
                newDisplayOption = new DisplayImageOptions.Builder()
                        .showImageOnLoading(defaultRes)
                        .showImageForEmptyUri(defaultRes)
                        .showImageOnFail(defaultRes)
                        .cacheInMemory(true)
                        .cacheOnDisk(true)
                        .considerExifParams(true)
                        .imageScaleType(ImageScaleType.EXACTLY)
                        .bitmapConfig(Bitmap.Config.RGB_565)
                        .resetViewBeforeLoading(false)
//                        .displayer(new FadeInBitmapDisplayer(200))
                        .displayer(new RoundedBitmapDisplayer(corner))
                        .build();
            }

            Map<Integer, DisplayImageOptions> cornerDisplayOptMap = new HashMap<Integer, DisplayImageOptions>();
            cornerDisplayOptMap.put(corner, newDisplayOption);
            avatarOptionsMaps.put(defaultRes, cornerDisplayOptMap);
            return newDisplayOption;
        }catch (Exception e){
            logger.e(e.toString());
            return null;
        }
    }

    public static DisplayImageOptions getAvatarOptions2(int corner,int defaultRes){
        try {
            if (defaultRes <= 0) {
                defaultRes = R.drawable.tt_default_user_portrait_corner;
            }
            if (avatarOptionsMaps.containsKey(defaultRes)) {
                Map<Integer, DisplayImageOptions> displayOption = avatarOptionsMaps.get(defaultRes);
                if (displayOption.containsKey(corner)) {
                    return displayOption.get(corner);
                }
            }
            DisplayImageOptions newDisplayOption = null;
            if (corner==CIRCLE_CORNER) {
                newDisplayOption = new DisplayImageOptions.Builder()
                        .showImageOnFail(defaultRes)
                        .cacheInMemory(true)
                        .build();
            } else {
                if (corner < 0) {
                    corner = 0;
                }
                newDisplayOption = new DisplayImageOptions.Builder()
                        .cacheInMemory(true)
                        .cacheOnDisk(true)
                        .build();
            }

            Map<Integer, DisplayImageOptions> cornerDisplayOptMap = new HashMap<Integer, DisplayImageOptions>();
            cornerDisplayOptMap.put(corner, newDisplayOption);
            avatarOptionsMaps.put(defaultRes, cornerDisplayOptMap);
            return newDisplayOption;
        }catch (Exception e){
            logger.e(e.toString());
            return null;
        }
    }

    /**
     * 清除缓存
     */
    public static void clearCache() {
        try {
            if (IMImageLoadInstance != null) {
                IMImageLoadInstance.clearMemoryCache();
                IMImageLoadInstance.clearDiskCache();
            }
            if(null!=avatarOptionsMaps)
            {
                avatarOptionsMaps.clear();
            }
        } catch (Exception e) {
            logger.e(e.toString());
        }
    }
}
