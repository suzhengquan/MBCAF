package com.MBCAF.common;

import android.text.Html;
import android.text.Spanned;
import android.util.Log;
import java.util.List;

public class StringUtil{
	
	public static boolean isEmpty(String str){
		if(str != null && str.length() > 0){
			return false;
		}
		return true;
	}

	public static String makeHtmlStr(String text, String color){
		return "<font color=\"" + color + "\">" + text + "</font>";
	}
	
	public static String makeHtmlStr(String text, int color){
		String strColor = String.valueOf(color);
		return makeHtmlStr(text, strColor);
	}
	
    public static String getUnNullString(String content) {
        if (null == content) {
            return "";
        }
        return content;
    }
    
    public static Spanned getColorfulString(String color, String content, String rightContent,
        String leftContent) {
      return Html.fromHtml(rightContent + "<font color=\"" + color + "\">" + content + "</font>"
          + leftContent);
    }

	public static void dumpStringList(Logger logger, String desc,
									  List<String> memberList) {
		String log = String.format("%s, members:", desc);
		for (String memberId : memberList) {
			log += memberId + ",";
		}

		logger.d("%s", log);
	}

	public static void dumpIntegerList(Logger logger, String desc,
									   List<Integer> memberList) {
		String log = String.format("%s, members:", desc);
		for (int memberId : memberList) {
			log += memberId + ",";
		}

		logger.d("%s", log);
	}

	//oneLine for purpose of "tail -f", so you can track them at one line
	public static void dumpStacktrace(Logger logger, String desc,
									  boolean oneLine) {
		String stackTraceString = Log.getStackTraceString(new Throwable());

		if (oneLine) {
			stackTraceString = stackTraceString.replace("\n", "####");
		}

		logger.d("%s:%s", desc, stackTraceString);
	}
}