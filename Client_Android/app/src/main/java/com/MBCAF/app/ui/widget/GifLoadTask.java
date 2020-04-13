package com.MBCAF.app.ui.widget;

import android.os.AsyncTask;

import com.MBCAF.common.CommonUtil;
import okhttp3.Cache;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.internal.cache.DiskLruCache;
import okhttp3.internal.io.FileSystem;

import org.apache.commons.io.IOUtil;

import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLDecoder;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class GifLoadTask extends AsyncTask<String, Void, byte[]> {

    static DiskLruCache mCache = DiskLruCache.create(FileSystem.SYSTEM, CommonUtil.getImageSavePath(), 1, 2, 2*1024*1024);

    public GifLoadTask() {
    }

    @Override
    protected void onPreExecute() {
        super.onPreExecute();
    }

    @Override
    protected byte[] doInBackground(final String... params) {
        final String gifUrl = params[0];
        if (gifUrl == null)
            return null;
        byte[] gif = null;
        try {
            gif = byteArrayHttpClient(gifUrl);
            String key = hashKeyForDisk(gifUrl);
// 如果没有找到对应的缓存，则准备从网络上请求数据，并写入缓存
            DiskLruCache.Editor editor = mCache.edit(key);
            if (editor != null) {
                try {
                    okio.Sink sink = editor.newSink(0);
                    okio.Buffer buffer = new okio.Buffer();
                    buffer.write(gif);
                    sink.write(buffer, buffer.size());

                    if (buffer != null) {
                        buffer.clear();
                        buffer.close();
                    }

                    if (sink != null) {
                        sink.flush();
                        sink.close();
                    }
                    editor.commit();
                } catch (IOException e) {
                    editor.abort();
                }
            }
        } catch (OutOfMemoryError e) {
        } catch (IOException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return gif;
    }

    private String hashKeyForDisk(String key) {
        String cacheKey;
        try {
            final MessageDigest mDigest = MessageDigest.getInstance("MD5");
            mDigest.update(key.getBytes());
            cacheKey = bytesToHexString(mDigest.digest());
        } catch (NoSuchAlgorithmException e) {
            cacheKey = String.valueOf(key.hashCode());
        }
        return cacheKey;
    }
    private String bytesToHexString(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < bytes.length; i++) {
            String hex = Integer.toHexString(0xFF & bytes[i]);
            if (hex.length() == 1) {
                sb.append('0');
            }
            sb.append(hex);
        }
        return sb.toString();
    }
    private FilterInputStream getFromCache(String url) throws Exception {
        mCache.flush();
        String key = hashKeyForDisk(url);
        final DiskLruCache.Snapshot snapshot;
        try {
            snapshot = mCache.get(key);
            if (snapshot == null) {
                return null;
            }
        } catch (IOException e) {
            return null;
        }

        final okio.Source source = snapshot.getSource(0);

        final okio.Buffer buffer = new okio.Buffer();
        //读取4*1024数据放入buffer中并返回读取到的数据的字节长度
        long ret = source.read(buffer,4*1024);
        //判断文件是否读完
        while (ret != -1){
            ret = source.read(buffer,4*1024);
        }
        FilterInputStream bodyIn = new FilterInputStream(buffer.inputStream()) {
            @Override
            public void close() throws IOException {
                if (buffer != null) {
                    buffer.clear();
                    buffer.close();
                }
                if(source != null) {
                    source.close();
                }
                if(snapshot != null) {
                    snapshot.close();
                }
                super.close();
            }
        };
        return bodyIn;
    }

    public byte[] byteArrayHttpClient(final String urlString) throws Exception {
        OkHttpClient client = null;
        if (client == null) {
            OkHttpClient.Builder builder = new OkHttpClient.Builder();
            Cache responseCache = new Cache(CommonUtil.getImageSavePath(), 2*1024*1024);
            builder.connectTimeout(30, java.util.concurrent.TimeUnit.SECONDS);
            builder.readTimeout(30, java.util.concurrent.TimeUnit.SECONDS);
            builder.cache(responseCache);
            client = builder.build();
        }
        FilterInputStream inputStream = getFromCache(urlString);
        if (inputStream != null) {
            return IOUtil.toByteArray(inputStream);
        }
        InputStream in = null;
        try {
            final String decodedUrl = URLDecoder.decode(urlString, "UTF-8");
            final URL url = new URL(decodedUrl);
            final Request request = new Request.Builder().url(url).build();
            final Response response = client.newCall(request).execute();
            in = response.body().byteStream();
            return IOUtil.toByteArray(in);
        } catch (final MalformedURLException e) {
        } catch (final OutOfMemoryError e) {
        } catch (final UnsupportedEncodingException e) {
        } catch (final IOException e) {
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (final IOException ignored) {
                }
            }
        }
        return null;
    }
}

