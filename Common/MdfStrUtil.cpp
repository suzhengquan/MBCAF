#include "MdfStrUtil.h"
#include <sstream>
#include <strstream>

using namespace std;

namespace Mdf
{
	//-----------------------------------------------------------------------
	void StrUtil::split(const String & str, StringList & out, const String & delim, bool trimEmpty)
	{
		size_t pos, last_pos = 0, len;
		while (true)
		{
			pos = str.find(delim, last_pos);
			if (pos == String::npos)
			{
				pos = str.size();
			}

			len = pos - last_pos;
			if (!trimEmpty || len != 0)
			{
				out.push_back(str.substr(last_pos, len));
			}

			if (pos == str.size())
			{
				break;
			}
			else
			{
				last_pos = pos + delim.size();
			}
		}
	}
	//-----------------------------------------------------------------------
	void StrUtil::compact(const std::vector<String> & tokens, String & out)
	{
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (tokens[i].empty())
			{
				continue;
			}
			out.append(tokens[i]);
		}
	}
	//-----------------------------------------------------------------------
	String StrUtil::itostr(Mui32 user_id)
	{
		StringStream ss;
		ss << user_id;
		return ss.str();
	}
	//-----------------------------------------------------------------------
	Mui32 StrUtil::strtoi(const String & value)
	{
		return (Mui32)_ttoi(value.c_str());
	}
	//-----------------------------------------------------------------------
	void StrUtil::idtourl(Mui32 id, char * out)
	{
		static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
		Mui32 value = id * 2 + 56;
		char * ptr = out + sizeof(out) - 1;
		*ptr = '\0';

		do
		{
			*--ptr = digits[value % 36];
			value /= 36;
		} while (ptr > out && value);

		*--ptr = '1';
	}
	//-----------------------------------------------------------------------
	Mui32 StrUtil::urltoid(const String & url)
	{
		Mui32 url_len = url.length();
		char c;
		Mui32 number = 0;
		for (Mui32 i = 1; i < url_len; i++)
		{
			c = url[i];

			if (c >= '0' && c <= '9')
			{
				c -= '0';
			}
			else if (c >= 'a' && c <= 'z')
			{
				c -= 'a' - 10;
			}
			else if (c >= 'A' && c <= 'Z')
			{
				c -= 'A' - 10;
			}
			else
			{
				continue;
			}

			number = number * 36 + c;
		}

		return (number - 56) >> 1;
	}
	//-----------------------------------------------------------------------
	void StrUtil::replace(String & str, String & in, char mark, Mui32 & pos)
	{
		String::size_type cpos = str.find(mark, pos);
		if (cpos == String::npos)
		{
			return;
		}

		String prime_new_value = _T("'") + in + _T("'");
		str.replace(cpos, 1, prime_new_value);

		pos = cpos + prime_new_value.size();
	}
	//-----------------------------------------------------------------------
	void StrUtil::replace(String & str, Mui32 in, char mark, Mui32 & pos)
	{
		StringStream ss;
		ss << in;

		String str_value = ss.str();
		String::size_type cpos = str.find(mark, pos);
		if (cpos == String::npos)
		{
			return;
		}

		str.replace(cpos, 1, str_value);
		pos = cpos + str_value.size();
	}
	//-----------------------------------------------------------------------
	unsigned char toHex(unsigned char x)
	{
		return  x > 9 ? x + 55 : x + 48;
	}
	//-----------------------------------------------------------------------
	unsigned char fromHex(unsigned char x)
	{
		unsigned char y;
		if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
		else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
		else if (x >= '0' && x <= '9') y = x - '0';
		else assert(0);
		return y;
	}
	//-----------------------------------------------------------------------
	String StrUtil::UrlEncode(const String & sIn)
	{

		String strTemp = _T("");
		size_t length = sIn.length();
		for (size_t i = 0; i < length; i++)
		{
			if (isalnum((unsigned char)sIn[i]) ||
				(sIn[i] == '-') ||
				(sIn[i] == '_') ||
				(sIn[i] == '.') ||
				(sIn[i] == '~'))
			{
				strTemp += sIn[i];
			}
			else if (sIn[i] == ' ')
			{
				strTemp += _T("+");
			}
			else
			{
				strTemp += '%';
				strTemp += toHex((unsigned char)sIn[i] >> 4);
				strTemp += toHex((unsigned char)sIn[i] % 16);
			}
		}
		return strTemp;
	}
	//-----------------------------------------------------------------------
	String StrUtil::UrlDecode(const String & sIn)
	{
		String strTemp = _T("");
		size_t length = sIn.length();
		for (size_t i = 0; i < length; i++)
		{
			if (sIn[i] == '+')
			{
				strTemp += ' ';
			}
			else if (sIn[i] == '%')
			{
				assert(i + 2 < length);
				unsigned char high = fromHex((unsigned char)sIn[++i]);
				unsigned char low = fromHex((unsigned char)sIn[++i]);
				strTemp += high * 16 + low;
			}
			else
			{
				strTemp += sIn[i];
			}
		}
		return strTemp;
	}
	//-----------------------------------------------------------------------
	const std::string StrUtil::ws2s(const std::wstring& src)
	{
		std::locale sys_locale("");

		const wchar_t* data_from = src.c_str();
		const wchar_t* data_from_end = src.c_str() + src.size();
		const wchar_t* data_from_next = 0;

		int wchar_size = 4;
		char* data_to = new char[(src.size() + 1) * wchar_size];
		char* data_to_end = data_to + (src.size() + 1) * wchar_size;
		char* data_to_next = 0;

		memset(data_to, 0, (src.size() + 1) * wchar_size);

		typedef std::codecvt<wchar_t, char, mbstate_t> convert_facet;
		mbstate_t out_state;
		memset(&out_state, 0, sizeof(mbstate_t));
		auto result = std::use_facet<convert_facet>(sys_locale).out(
			out_state, data_from, data_from_end, data_from_next,
			data_to, data_to_end, data_to_next);
		if (result == convert_facet::ok)
		{
			std::string dst = data_to;
			delete[] data_to;
			return dst;
		}
		else
		{
			printf("convert error!\n");
			delete[] data_to;
			return std::string("");
		}
	}
	//-----------------------------------------------------------------------
	const std::wstring StrUtil::s2ws(const std::string& src)
	{
		std::locale sys_locale("");

		const char* data_from = src.c_str();
		const char* data_from_end = src.c_str() + src.size();
		const char* data_from_next = 0;

		wchar_t* data_to = new wchar_t[src.size() + 1];
		wchar_t* data_to_end = data_to + src.size() + 1;
		wchar_t* data_to_next = 0;

		wmemset(data_to, 0, src.size() + 1);

		typedef std::codecvt<wchar_t, char, mbstate_t> convert_facet;
		mbstate_t in_state;
		memset(&in_state, 0, sizeof(mbstate_t));
		auto result = std::use_facet<convert_facet>(sys_locale).in(
			in_state, data_from, data_from_end, data_from_next,
			data_to, data_to_end, data_to_next);
		if (result == convert_facet::ok)
		{
			std::wstring dst = data_to;
			delete[] data_to;
			return dst;
		}
		else
		{
			printf("convert error!\n");
			delete[] data_to;
			return std::wstring(L"");
		}
	}
	//-----------------------------------------------------------------------
}