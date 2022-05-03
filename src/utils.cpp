#include "common.h"
#include "utils.h"
#include <cwctype>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdarg.h>

//--

namespace prv
{
    ///---

    template< typename T >
    inline static bool IsFloatNum(uint32_t index, T ch)
    {
        if (ch == '.') return true;
        if (ch >= '0' && ch <= '9') return true;
        if (ch == 'f' && (index > 0)) return true;
        if ((ch == '+' || ch == '-') && (index == 0)) return true;
        return false;
    }

    template< typename T >
    inline static bool IsAlphaNum(T ch)
    {
        if (ch >= '0' && ch <= '9') return true;
        if (ch >= 'A' && ch <= 'Z') return true;
        if (ch >= 'a' && ch <= 'z') return true;
        if (ch == '_') return true;
        return false;
    }

    template< typename T >
    inline static bool IsStringChar(T ch, const T* additionalDelims)
    {
        if (ch <= ' ') return false;
        if (ch == '\"' || ch == '\'') return false;

        if (strchr(additionalDelims, ch))
            return false;

        return true;
    }

    template< typename T >
    inline static bool IsIntNum(uint32_t index, T ch)
    {
        if (ch >= '0' && ch <= '9') return true;
        if ((ch == '+' || ch == '-') && (index == 0)) return true;
        return false;
    }

    template< typename T >
    inline static bool IsHex(T ch)
    {
        if (ch >= '0' && ch <= '9') return true;
        if (ch >= 'A' && ch <= 'F') return true;
        if (ch >= 'a' && ch <= 'f') return true;
        return false;
    }

    template< typename T >
    inline uint64_t GetHexValue(T ch)
    {
        switch (ch)
        {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a': return 10;
        case 'A': return 10;
        case 'b': return 11;
        case 'B': return 11;
        case 'c': return 12;
        case 'C': return 12;
        case 'd': return 13;
        case 'D': return 13;
        case 'e': return 14;
        case 'E': return 14;
        case 'f': return 15;
        case 'F': return 15;
        }

        return 0;
    }

    static const uint32_t OffsetsFromUTF8[6] =
    {
        0x00000000UL,
        0x00003080UL,
        0x000E2080UL,
        0x03C82080UL,
        0xFA082080UL,
        0x82082080UL
    };

    static const uint8_t TrailingBytesForUTF8[256] =
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
        3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
    };

    inline static bool IsUTF8(char c)
    {
        return (c & 0xC0) != 0x80;
    }

    uint32_t NextChar(const char*& ptr, const char* end)
    {
        if (ptr >= end)
            return 0;

        uint32_t ch = 0;
        size_t sz = 0;
        do
        {
            ch <<= 6;
            ch += (uint8_t)*ptr++;
            sz++;
        }
        while (ptr < end && !IsUTF8(*ptr));
        if (sz > 1)
            ch -= OffsetsFromUTF8[sz - 1];
        return ch;
    }

    //--

    static const char Digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E','F' };

    static inline bool GetNumberValueForDigit(char ch, uint32_t base, uint8_t& outDigit)
    {
        for (uint8_t i = 0; i < base; ++i)
        {
            if (Digits[i] == ch)
            {
                outDigit = i;
                return true;
            }
        }

        return false;
    }

    template<typename T>
    static inline bool CheckNumericalOverflow(T val, T valueToAdd)
    {
        if (valueToAdd > 0)
        {
            auto left = std::numeric_limits<T>::max() - val;
            return valueToAdd > left;
        }
        else if (valueToAdd < 0)
        {
            auto left = std::numeric_limits<T>::lowest() - val;
            return valueToAdd < left;
        }

        return false;
    }

    template<typename Ch, typename T>
    inline static bool MatchInteger(const Ch* str, T& outValue, size_t strLength, uint32_t base)
    {
        static_assert(std::is_signed<T>::value || std::is_unsigned<T>::value, "Only integer types are allowed here");

        // empty strings are not valid input to this function
        if (!str || !*str)
            return false;

        // determine start and end of parsing range as well as the sign
        auto negative = (*str == '-');
        auto strStart = (*str == '+' || *str == '-') ? str + 1 : str;
        auto strEnd = str + strLength;

        // unsigned values cannot be negative :)
        if (std::is_unsigned<T>::value && negative)
            return false;

        T minValue = std::numeric_limits<T>::min();
        T maxValue = std::numeric_limits<T>::max();

        T value = 0;
        T mult = negative ? -1 : 1;

        // assemble number
        auto pos = strEnd;
        bool overflowed = false;
        while (pos > strStart)
        {
            auto ch = *(--pos);

            // if a non-zero digit is encountered we must make sure that he mult is not overflowed already
            uint8_t digitValue;
            if (!GetNumberValueForDigit((char)ch, base, digitValue))
                return false;

            // apply
            if (digitValue != 0 && overflowed)
                return false;

            // validate that we will not overflow the type
            auto valueToAdd = (T)(digitValue * mult);
            if ((valueToAdd / mult) != digitValue)
                return false;
            if (prv::CheckNumericalOverflow<T>(value, valueToAdd))
                return false;

            // accumulate
            value += valueToAdd;

            // advance to next multiplier
            T newMult = mult * 10;
            if (newMult / 10 != mult)
                overflowed = true;
            mult = newMult;
        }

        outValue = value;
        return true;
    }

    template<typename Ch>
    inline bool MatchFloat(const Ch* str, double& outValue, size_t strLength)
    {
        // empty strings are not valid input to this function
        if (!str || !*str)
            return false;

        // determine start and end of parsing range as well as the sign
        auto negative = (*str == '-');
        auto strEnd = str + strLength;
        auto strStart = (*str == '+' || *str == '-') ? str + 1 : str;

        // validate that we have a proper characters, discover the decimal point position
        auto strDecimal = strEnd; // if decimal point was not found assume it's at the end
        {
            auto pos = strStart;
            while (pos < strEnd)
            {
                auto ch = *pos++;

                if (pos == strEnd && ch == 'f')
                    break;

                if (ch == '.')
                {
                    strDecimal = pos - 1;
                }
                else
                {
                    uint8_t value = 0;
                    if (!prv::GetNumberValueForDigit((char)ch, 10, value))
                        return false;
                }
            }
        }

        // accumulate values
        double value = 0.0f;

        // TODO: this is tragic where it comes to the precision loss....
        // TODO: overflow/underflow
        {
            double mult = 1.0f;

            auto pos = strDecimal;
            while (pos > strStart)
            {
                auto ch = *(--pos);

                uint8_t digitValue = 0;
                if (!prv::GetNumberValueForDigit((char)ch, 10, digitValue))
                    return false;

                // accumulate
                value += (double)digitValue * mult;
                mult *= 10.0;
            }
        }

        // Fractional part
        if (strDecimal < strEnd)
        {
            double mult = 0.1f;

            auto pos = strDecimal + 1;
            while (pos < strEnd)
            {
                auto ch = *(pos++);

                if (pos == strEnd && ch == 'f')
                    break;

                uint8_t digitValue = 0;
                if (!prv::GetNumberValueForDigit((char)ch, 10, digitValue))
                    return false;

                // accumulate
                value += (double)digitValue * mult;
                mult /= 10.0;
            }
        }

        outValue = negative ? -value : value;
        return true;
    }

} // prv

//--



//--

Parser::Parser(std::string_view txt)
{
    m_start = txt.data();
    m_end = m_start + txt.length();
    m_cur = m_start;
}

bool Parser::testKeyword(std::string_view keyword) const
{
    auto cur = m_cur;
    while (cur < m_end && *cur <= ' ')
        ++cur;

    auto keyLength = keyword.length();
    for (uint32_t i = 0; i < keyLength; ++i)
    {
        if (cur >= m_end || *cur != keyword.data()[i])
            return false;

        cur += 1;
    }

    return true;
}

void Parser::push()
{

}

void Parser::pop()
{

}

bool Parser::parseWhitespaces()
{
    while (m_cur < m_end && *m_cur <= ' ')
    {
        if (*m_cur == '\n')
            m_line += 1;
        m_cur++;
    }

    return m_cur < m_end;
}

bool Parser::parseTillTheEndOfTheLine(std::string_view* outIdent /*= nullptr*/)
{
    const char* firstNonEmptyChar = nullptr;
    const char* lastNonEmptyChar = nullptr;

    while (m_cur < m_end)
    {
        if (*m_cur > ' ')
        {
            if (!firstNonEmptyChar)
                firstNonEmptyChar = m_cur;

            lastNonEmptyChar = m_cur;
        }

        if (*m_cur++ == '\n')
            break;
    }

    if (outIdent)
    {
        if (lastNonEmptyChar != nullptr && m_cur < m_end)
            *outIdent = std::string_view(firstNonEmptyChar, (lastNonEmptyChar + 1) - firstNonEmptyChar);
        else
            *outIdent = std::string_view();
    }

    return m_cur < m_end;
}

bool Parser::parseIdentifier(std::string_view& outIdent)
{
    if (!parseWhitespaces())
        return false;

    if (!(*m_cur == '_' || *m_cur == ':' || std::iswalpha(*m_cur)))
        return false;

    auto identStart = m_cur;
    while (m_cur < m_end && (*m_cur == '_' || *m_cur == ':' || std::iswalnum(*m_cur)))
        m_cur += 1;

    assert(m_cur > identStart);
    outIdent = std::string_view(identStart, m_cur - identStart);
    return true;
}

bool Parser::parseString(std::string_view& outValue, const char* additionalDelims/* = ""*/)
{
    if (!parseWhitespaces())
        return false;

    auto startPos = m_cur;
    auto startLine = m_line;

    if (*m_cur == '\"' || *m_cur == '\'')
    {
        auto quote = *m_cur++;
        auto stringStart = m_cur;
        while (m_cur < m_end && *m_cur != quote)
            if (*m_cur++ == '\n')
                m_line += 1;

        if (m_cur >= m_end)
        {
            m_cur = startPos;
            m_line = startLine;
            return false;
        }

        outValue = std::string_view(stringStart, m_cur - stringStart);
        m_cur += 1;

        return true;
    }
    else
    {
        while (m_cur < m_end && prv::IsStringChar(*m_cur, additionalDelims))
            m_cur += 1;

        outValue = std::string_view(startPos, m_cur - startPos);
        return true;
    }
}

bool Parser::parseStringWithScapement(std::string& outValue, const char* additionalDelims/* = ""*/)
{
	if (!parseWhitespaces())
		return false;

	auto startPos = m_cur;
	auto startLine = m_line;

	if (*m_cur == '\"' || *m_cur == '\'')
	{
		auto quote = *m_cur++;
		auto stringStart = m_cur;

        std::stringstream txt;
        while (m_cur < m_end && *m_cur != quote)
        {
            auto ch = *m_cur++;

            if (ch == '\n')
                m_line += 1;

            if (ch == '\\')
            {
                const auto next = (m_cur < m_end) ? *m_cur++ : 0;
                if (next == 'n')
                    txt << "\n";
                else if (next == 'r')
                    txt << "\r";
                else if (next == 'b')
                    txt << "\b";
				else if (next == 't')
					txt << "\t";
				else if (next == '\"')
					txt << "\"";
				else if (next == '\'')
					txt << "\'";
                else
                    txt << next;
            }
            else
            {
                txt << ch;
            }
        }

		if (m_cur >= m_end)
		{
			m_cur = startPos;
			m_line = startLine;
			return false;
		}

        outValue = txt.str();
		m_cur += 1;

		return true;
	}
	else
	{
		while (m_cur < m_end && prv::IsStringChar(*m_cur, additionalDelims))
			m_cur += 1;

		outValue = std::string_view(startPos, m_cur - startPos);
		return true;
	}
}

bool Parser::parseLine(std::string_view& outValue, const char* additionalDelims/* = ""*/, bool eatLeadingWhitespaces/*= true*/)
{
    while (m_cur < m_end)
    {
        if (*m_cur == '\n')
        {
            m_line += 1;
            m_cur++;
            return false;
        }

        if (*m_cur > ' ' || !eatLeadingWhitespaces)
            break;

        ++m_cur;
    }

    auto startPos = m_cur;
    auto startLine = m_line;

    while (m_cur < m_end)
    {
        if (*m_cur == '\n')
        {
            outValue = std::string_view(startPos, m_cur - startPos);
            m_line += 1;
            m_cur++;
            return true;
        }

        if (strchr(additionalDelims, *m_cur))
        {
            outValue = std::string_view(startPos, m_cur - startPos);
            m_cur++;
            return true;
        }

        ++m_cur;
    }

    if (startPos == m_cur)
        return false;

    outValue = std::string_view(startPos, m_cur - startPos);
    return true;
}

bool Parser::parseKeyword(std::string_view keyword)
{
    if (!parseWhitespaces())
        return false;

    auto keyStart = m_cur;
    auto keyLength = keyword.length();
    for (uint32_t i = 0; i < keyLength; ++i)
    {
        if (m_cur >= m_end || *m_cur != keyword.data()[i])
        {
            m_cur = keyStart;
            return false;
        }

        m_cur += 1;
    }

    return true;
}

bool Parser::parseChar(uint32_t& outChar)
{
    auto* cur = m_cur;
    const auto ch = prv::NextChar(cur, m_end);
    if (ch != 0)
    {
        outChar = ch;
        m_cur = cur;
        return true;
    }

    return false;
}

bool Parser::parseFloat(float& outValue)
{
    double doubleValue;
    if (parseDouble(doubleValue))
    {
        outValue = (float)doubleValue;
        return true;
    }

    return false;
}

bool Parser::parseDouble(double& outValue)
{
    if (!parseWhitespaces())
        return false;

    auto originalPos = m_cur;
    if (*m_cur == '-' || *m_cur == '+')
    {
        m_cur += 1;
    }

    uint32_t numChars = 0;
    while (m_cur < m_end && prv::IsFloatNum(numChars, *m_cur))
    {
        ++numChars;
        m_cur += 1;
    }

    if (numChars && prv::MatchFloat(originalPos, outValue, numChars))
        return true;

    m_cur = originalPos;
    return false;
}

bool Parser::parseBoolean(bool& outValue)
{
    if (parseKeyword("true"))
    {
        outValue = true;
        return true;
    }
    else if (parseKeyword("false"))
    {
        outValue = false;
        return true;
    }

    int64_t numericValue = 0;
    if (parseInt64(numericValue))
    {
        outValue = (numericValue != 0);
        return true;
    }

    return false;
}

bool Parser::parseHex(uint64_t& outValue, uint32_t maxLength /*= 0*/, uint32_t* outValueLength /*= nullptr*/)
{
    if (!parseWhitespaces())
        return false;

    const char* original = m_cur;
    const char* maxEnd = maxLength ? (m_cur + maxLength) : m_end;
    uint64_t ret = 0;
    while (m_cur < m_end && prv::IsHex(*m_cur) && (m_cur < maxEnd))
    {
        ret = (ret << 4) | prv::GetHexValue(*m_cur);
        m_cur += 1;
    }

    if (original == m_cur)
        return false;

    outValue = ret;
    if (outValueLength)
        *outValueLength = (uint32_t)(m_cur - original);
    return true;
}

bool Parser::parseInt8(char& outValue)
{
    auto start = m_cur;

    int64_t bigVal = 0;
    if (!parseInt64(bigVal))
        return false;

    if (bigVal < std::numeric_limits<char>::min() || bigVal > std::numeric_limits<char>::max())
    {
        m_cur = start;
        return false;
    }

    outValue = (char)bigVal;
    return true;
}

bool Parser::parseInt16(short& outValue)
{
    auto start = m_cur;

    int64_t bigVal = 0;
    if (!parseInt64(bigVal))
        return false;

    if (bigVal < std::numeric_limits<short>::min() || bigVal > std::numeric_limits<short>::max())
    {
        m_cur = start;
        return false;
    }

    outValue = (short)bigVal;
    return true;
}

bool Parser::parseInt32(int& outValue)
{
    auto start = m_cur;

    int64_t bigVal = 0;
    if (!parseInt64(bigVal))
        return false;

    if (bigVal < std::numeric_limits<int>::min() || bigVal > std::numeric_limits<int>::max())
    {
        m_cur = start;
        return false;
    }

    outValue = (int)bigVal;
    return true;
}

bool Parser::parseInt64(int64_t& outValue)
{
    if (!parseWhitespaces())
        return false;

    auto originalPos = m_cur;
    if (*m_cur == '-' || *m_cur == '+')
        m_cur += 1;

    uint32_t numChars = 0;
    while (m_cur < m_end && prv::IsIntNum(numChars, *m_cur))
    {
        ++numChars;
        ++m_cur;
    }

    if (numChars && prv::MatchInteger(originalPos, outValue, numChars, 10))
        return true;

    m_cur = originalPos;
    return false;
}

bool Parser::parseUint8(uint8_t& outValue)
{
    auto start = m_cur;

    uint64_t bigVal = 0;
    if (!parseUint64(bigVal))
        return false;

    if (bigVal > std::numeric_limits<uint8_t>::max())
    {
        m_cur = start;
        return false;
    }

    outValue = (uint8_t)bigVal;
    return true;
}

bool Parser::parseUint16(uint16_t& outValue)
{
    auto start = m_cur;

    uint64_t bigVal = 0;
    if (!parseUint64(bigVal))
        return false;

    if (bigVal > std::numeric_limits<uint16_t>::max())
    {
        m_cur = start;
        return false;
    }

    outValue = (uint16_t)bigVal;
    return true;
}

bool Parser::parseUint32(uint32_t& outValue)
{
    auto start = m_cur;

    uint64_t bigVal = 0;
    if (!parseUint64(bigVal))
        return false;

    if (bigVal > std::numeric_limits<uint32_t>::max())
    {
        m_cur = start;
        return false;
    }

    outValue = (uint32_t)bigVal;
    return true;
}

bool Parser::parseUint64(uint64_t& outValue)
{
    if (!parseWhitespaces())
        return false;

    uint32_t numChars = 0;
    auto originalPos = m_cur;
    while (m_cur < m_end && prv::IsIntNum(numChars, *m_cur))
    {
        ++numChars;
        ++m_cur;
    }

    if (numChars && prv::MatchInteger(originalPos, outValue, numChars, 10))
        return true;

    m_cur = originalPos;
    return false;
}

//--

RequestArgs::RequestArgs()
{}

RequestArgs::~RequestArgs()
{}

RequestArgs& RequestArgs::clear()
{
    m_arguments.clear();
    return *this;
}

RequestArgs& RequestArgs::setText(std::string_view name, std::string_view value)
{
    if (value.empty())
        return remove(name);

    if (!name.empty())
        m_arguments[std::string(name)] = value;    
    return *this;
}

RequestArgs& RequestArgs::remove(std::string_view name)
{
    auto it = m_arguments.find(std::string(name));
    if (it != m_arguments.end())
        m_arguments.erase(it);
    return *this;
}

RequestArgs& RequestArgs::setNumber(std::string_view name, int value)
{
    char txt[64];
    sprintf_s(txt, sizeof(txt), "%d", value);
    return setText(name, txt);
}

RequestArgs& RequestArgs::setBool(std::string_view name, bool value)
{
	return setText(name, value ? "true" : "false");
}

static bool IsSafeURLChar(char ch)
{
    if (ch >= 'a' && ch <= 'z') return true;
    if (ch >= 'A' && ch <= 'Z') return true;
    if (ch >= '0' && ch <= '9') return true;
    if (ch == '_' || ch == '!' || ch == '*' || ch == '(' || ch == ')' || ch == '~' || ch == '.' || ch == '\'') return true;
    return false;
}

static void PrintURL(std::stringstream& f, std::string_view txt)
{
    for (auto ch : txt)
    {
        if (ch == ' ')
            f << "+";
        else if (IsSafeURLChar(ch))
            f << ch;
        else
        {
            char txt[16];
            sprintf_s(txt, sizeof(txt), "%%%02X", (unsigned char)ch);
            f << txt;
        }
    }
}

void RequestArgs::print(std::stringstream& f) const
{
    bool separator = false;

    for (const auto& it : m_arguments)
    {
        f << (separator ? "&" : "?");
        f << it.first;
        f << "=";
        PrintURL(f, it.second);
        separator = true;
    }
}

//--

std::string_view Commandline::get(std::string_view name, std::string_view defaultValue) const
{
	for (const auto& entry : args)
		if (entry.key == name)
			return entry.value;

	return defaultValue;
}

const std::string& Commandline::get(std::string_view name) const
{
    for (const auto& entry : args)
        if (entry.key == name)
            return entry.value;

    static std::string theEmptyString;
    return theEmptyString;
}

const std::vector<std::string>& Commandline::getAll(std::string_view name) const
{
    for (const auto& entry : args)
        if (entry.key == name)
            return entry.values;

    static std::vector<std::string> theEmptyStringArray;
    return theEmptyStringArray;
}

bool Commandline::has(std::string_view name) const
{
    for (const auto& entry : args)
        if (entry.key == name)
            return true;

    return false;
}

bool Commandline::parse(std::string_view text)
{
    Parser parser(text);

    bool parseInitialChar = true;
    while (parser.parseWhitespaces())
    {
        if (parser.parseKeyword("-"))
        {
            parseInitialChar = false;
            break;
        }

        // get the command
        std::string_view commandName;
        if (!parser.parseIdentifier(commandName))
        {
            std::cout << "Commandline parsing error: expecting command name. Application may not work as expected.\n";
            return false;
        }

        commands.push_back(std::string(commandName));
    }

    while (parser.parseWhitespaces())
    {
        if (!parser.parseKeyword("-") && parseInitialChar)
            break;
        parseInitialChar = true;

        std::string_view paramName;
        if (!parser.parseIdentifier(paramName))
        {
            std::cout << "Commandline parsing error: expecting param name after '-'. Application may not work as expected.\n";
            return false;
        }

        std::string_view paramValue;
        if (parser.parseKeyword("="))
        {
            // Read value
            if (!parser.parseString(paramValue))
            {
                std::cout << "Commandline parsing error: expecting param value after '=' for param '" << paramName << "'. Application may not work as expected.\n";
                return false;
            }
        }

        bool exists = false;
        for (auto& param : this->args)
        {
            if (param.key == paramName)
            {
                if (!paramValue.empty())
                {
                    param.values.push_back(std::string(paramValue));
                    param.value = paramValue;
                }

                exists = true;
                break;
            }
        }

        if (!exists)
        {
            Arg arg;
            arg.key = paramName;

            if (!paramValue.empty())
            {
                arg.values.push_back(std::string(paramValue));
                arg.value = paramValue;
            }

            this->args.push_back(arg);
        }
    }

    return true;
}

//--

bool LoadFileToString(const fs::path& path, std::string& outText)
{
    try
    {
        std::ifstream f(path);
        if (!f.is_open())
            return false;

        std::stringstream buffer;
        buffer << f.rdbuf();
        outText = buffer.str();
        return true;
    }
    catch (std::exception& e)
    {
        std::cout << "Error reading file " << path << ": " << e.what() << "\n";
        return false;
    }
}

bool LoadFileToBuffer(const fs::path& path, std::vector<uint8_t>& outBuffer)
{
	try
	{
		std::ifstream file(path, std::ios::binary);
		file.unsetf(std::ios::skipws);

		file.seekg(0, std::ios::end);
		auto fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

        outBuffer.resize(fileSize);
        file.read((char*)outBuffer.data(), fileSize);

		return true;
	}
	catch (std::exception& e)
	{
		std::cout << "Error reading file " << path << ": " << e.what() << "\n";
		return false;
	}
}

bool SaveFileFromString(const fs::path& path, std::string_view txt, bool print /*=true*/, bool force /*= false*/, uint32_t* outCounter, fs::file_time_type customTime /*= fs::file_time_type()*/)
{
    std::string newContent(txt);

    if (!force)
    {
        std::string currentContent;
        if (LoadFileToString(path, currentContent))
        {
            if (currentContent == txt)
            {
                if (customTime != fs::file_time_type())
                {
                    if (print)
                    {
                        const auto currentTimeStamp = fs::last_write_time(path);
                        std::cout << "File " << path << " is the same, updating timestamp only to " << customTime.time_since_epoch().count() << ", current: " << currentTimeStamp.time_since_epoch().count() << "\n";
                    }

                    fs::last_write_time(path, customTime);
                }

                return true;
            }
        }

        if (print)
            std::cout << "File " << path << " has changed and has to be saved\n";
    }

    {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
    }

    try
    {
        std::ofstream file(path);
        file << txt;
    }
    catch (std::exception& e)
    {
        std::cout << "Error writing file " << path << ": " << e.what() << "\n";
        return false;
    }

    if (outCounter)
        (*outCounter) += 1;

    return true;
}

//--

void SplitString(std::string_view txt, std::string_view delim, std::vector<std::string_view>& outParts)
{
    size_t prev = 0, pos = 0;
    do
    {
        pos = txt.find(delim, prev);
        if (pos == std::string::npos) 
            pos = txt.length();

        std::string_view token = txt.substr(prev, pos - prev);
        if (!token.empty()) 
            outParts.push_back(token);

        prev = pos + delim.length();
    }
    while (pos < txt.length() && prev < txt.length());
}

bool BeginsWith(std::string_view txt, std::string_view end)
{
    if (txt.length() >= end.length())
        return (0 == txt.compare(0, end.length(), end));
    return false;
}

bool EndsWith(std::string_view txt, std::string_view end)
{
    if (txt.length() >= end.length())
        return (0 == txt.compare(txt.length() - end.length(), end.length(), end));
    return false;
}

std::string_view PartBefore(std::string_view txt, std::string_view end)
{
    auto pos = txt.find(end);
    if (pos != -1)
        return txt.substr(0, pos);
    return "";
}

std::string_view PartBeforeLast(std::string_view txt, std::string_view end)
{
	auto pos = txt.rfind(end);
	if (pos != -1)
		return txt.substr(0, pos);
	return "";
}

static bool NeedsEscapeArgument(std::string_view txt)
{
    for (const auto ch : txt)
    {
        if (ch == ' ') return true;
        if (ch == '\"') return true;
    }

    return false;
}

std::string EscapeArgument(std::string_view txt)
{
    if (!NeedsEscapeArgument(txt))
        return std::string(txt);

    std::stringstream str;
    str << "\"";

    for (const auto ch : txt)
    {
        if (ch == '\"')
            str << "\\\"";
        else
            str << ch;
    }

    str << "\"";

    return str.str();
}

std::string_view Trim(std::string_view txt)
{
    const auto* start = txt.data();
    const auto* end = txt.data() + txt.length() - 1;

    while (start < end)
    {
        if (*start > 32) break;
        ++start;
    }

    while (start < end)
    {
        if (*end > 32) break;
        --end;
    }

    return std::string_view(start, (end - start) + 1);
}

std::pair<std::string_view, std::string_view> SplitIntoKeyValue(std::string_view txt)
{
	auto pos = txt.find("=");
    if (pos == -1)
        return std::make_pair(txt, "");

    const auto key = txt.substr(0, pos);
    const auto value = txt.substr(pos + 1);

    return std::make_pair(key, value);
}

void TokenizeIntoParts(std::string_view txt, std::vector<std::string_view>& outOptions)
{
    outOptions.erase(outOptions.begin(), outOptions.end());

    const auto* ch = txt.data();
    const auto* chEnd = ch + txt.length();
    
    while (ch < chEnd)
    {
        if (*ch <= ' ')
        {
            ++ch;
            continue;
        }

		if (*ch == '#')
			break; // comment

        const auto* start = ch;
        
        char quoteChar = 0;
        if (*start == '\"' || *start == '\'')
            quoteChar = *start++;

        while (ch < chEnd)
        {
            if (quoteChar)
            {
                if (*ch == quoteChar)
                    break;
            }
            else
            {
                if (*ch <= ' ')
                    break;
                if (*ch == '#')
                    break; // comment
            }

            ++ch;
        }

        auto option = std::string_view(start, ch - start);
        outOptions.push_back(option);

        if (quoteChar)
            ch += 1;
    }
}

std::string_view PartAfter(std::string_view txt, std::string_view end)
{
    auto pos = txt.find(end);
    if (pos != -1)
        return txt.substr(pos + end.length());
    return "";
}

std::string_view PartAfterLast(std::string_view txt, std::string_view end, bool fullOnEmpty)
{
    auto pos = txt.rfind(end);
    if (pos != -1)
        return txt.substr(pos + end.length());
    return fullOnEmpty ? txt : "";
}

std::string MakeGenericPath(std::string_view txt)
{
    auto ret = std::string(txt);
    std::replace(ret.begin(), ret.end(), '\\', '/');
    return ret;
}

std::string MakeGenericPathEx(const fs::path& path)
{
    return MakeGenericPath(path.u8string());
}

static bool IsValidSymbolChar(char ch)
{
    if (ch >= 'a' && ch <= 'z') return true;
    if (ch >= 'A' && ch <= 'Z') return true;
    if (ch >= '0' && ch <= '9') return true;
    return false;
}

std::string MakeSymbolName(std::string_view txt)
{
    std::stringstream str;

    bool lastValid = false;
    for (uint8_t ch : txt)
    {
        if (IsValidSymbolChar(ch))
        {
            str << ch;
            lastValid = true;
        }
        else
        {
            if (lastValid)
                str << "_";
            lastValid = false;
        }
    }

    return str.str();
}

std::string ToUpper(std::string_view txt)
{
    std::string ret(txt);
    transform(ret.begin(), ret.end(), ret.begin(), ::toupper);    
    return ret;
}

std::string ToLower(std::string_view txt)
{
	std::string ret(txt);
	transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
	return ret;
}

std::string ReplaceAll(std::string_view txt, std::string_view from, std::string_view replacement)
{
	if (txt.empty())
		return "";

    std::string str(txt);
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
		str.replace(start_pos, from.length(), replacement);
		start_pos += replacement.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}

    return str;
}

void writeln(std::stringstream& s, std::string_view txt)
{
    s << txt;
    s << "\n";
}

void writelnf(std::stringstream& s, const char* txt, ...)
{
    char buffer[8192];
    va_list args;
    va_start(args, txt);
    vsprintf_s(buffer, sizeof(buffer), txt, args);
    va_end(args);

    s << buffer;
    s << "\n";
}

std::string GuidFromText(std::string_view txt)
{
    union {        
        struct {
            uint32_t Data1;
            uint16_t Data2;
            uint16_t Data3;
            uint8_t Data4[8];
        } g;

        uint32_t data[4];
    } guid;

    guid.data[0] = (uint32_t)std::hash<std::string_view>()(txt);
    guid.data[1] = (uint32_t)std::hash<std::string>()("part1_" + std::string(txt));
    guid.data[2] = (uint32_t)std::hash<std::string>()("part2_" + std::string(txt));
    guid.data[3] = (uint32_t)std::hash<std::string>()("part3_" + std::string(txt));

    // 2150E333-8FDC-42A3-9474-1A3956D46DE8

    char str[128];
    sprintf_s(str, sizeof(str), "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        guid.g.Data1, guid.g.Data2, guid.g.Data3,
        guid.g.Data4[0], guid.g.Data4[1], guid.g.Data4[2], guid.g.Data4[3],
        guid.g.Data4[4], guid.g.Data4[5], guid.g.Data4[6], guid.g.Data4[7]);

    return str;
}

//--

#define MATCH(_txt, _val) if (txt == _txt) { outType = _val; return true; }

template< typename T >
bool ParseEnumValue(std::string_view txt, T& outType)
{
    for (int i = 0; i < (int)(T::MAX); ++i)
    {
        const auto valueName = NameEnumOption((T)i);
        if (!valueName.empty() && valueName == txt)
        {
            outType = (T)i;
            return true;
        }
    }

    return false;
}

std::string_view DefaultPlatformStr()
{
#ifdef _WIN32
	return "windows";
#else
    return "linux";
#endif
}

PlatformType DefaultPlatform()
{
#ifdef _WIN32
	return PlatformType::Windows;
#else
    return PlatformType::Linux;
#endif
}

bool ParseConfigurationType(std::string_view txt, ConfigurationType& outType)
{
    return ParseEnumValue(txt, outType);
}

bool ParseBuildType(std::string_view txt, BuildType& outType)
{
    return ParseEnumValue(txt, outType);
}

bool ParseLibraryType(std::string_view txt, LibraryType& outType)
{
    return ParseEnumValue(txt, outType);
}

bool ParsePlatformType(std::string_view txt, PlatformType& outType)
{
    return ParseEnumValue(txt, outType);
}

bool ParseGeneratorType(std::string_view txt, GeneratorType& outType)
{
    return ParseEnumValue(txt, outType);
}


//--

std::string_view NameEnumOption(ConfigurationType type)
{
    switch (type)
    {
    case ConfigurationType::Checked: return "checked";
    case ConfigurationType::Debug: return "debug";
    case ConfigurationType::Release: return "release";
    case ConfigurationType::Final: return "final";
    }
    return "";
}

std::string_view NameEnumOption(BuildType type)
{
    switch (type)
    {
    case BuildType::Development: return "dev";
    //case BuildType::Standalone: return "standalone";
    case BuildType::Shipment: return "ship";
    }
    return "";
}

std::string_view NameEnumOption(LibraryType type)
{
    switch (type)
    {
    case LibraryType::Shared: return "shared";
    case LibraryType::Static: return "static";
    }
    return "";
}

std::string_view NameEnumOption(PlatformType type)
{
    switch (type)
    {
    case PlatformType::Linux: return "linux";
    case PlatformType::Windows: return "windows";
    case PlatformType::UWP: return "uwp";
    case PlatformType::Scarlett: return "scarlett";
    case PlatformType::Prospero: return "prospero";
    case PlatformType::iOS: return "ios";
    case PlatformType::Android: return "android";
    }
    return "";
}

std::string_view NameEnumOption(GeneratorType type)
{
    switch (type)
    {
    case GeneratorType::VisualStudio19: return "vs2019"; 
    case GeneratorType::VisualStudio22: return "vs2022";
    case GeneratorType::CMake: return "cmake";
    }
    return "";
}

//--

bool IsFileSourceNewer(const fs::path& source, const fs::path& target)
{
    try
    {
        if (!fs::is_regular_file(source))
            return false;

        if (!fs::is_regular_file(target))
            return true;

        auto sourceTimestamp = fs::last_write_time(source);
        auto targetTimestamp = fs::last_write_time(target);
        return sourceTimestamp > targetTimestamp;
    }
    catch (std::exception & e)
    {
        std::cout << "Failed to check file write time: " << e.what() << "\n";
        return false;
    }    
}

bool CreateDirectories(const fs::path& path)
{
    if (!fs::is_directory(path))
    {
		try
		{
            fs::create_directories(path);
		}
		catch (std::exception& e)
		{
			std::cout << "Failed to create directories: " << e.what() << "\n";
			return false;
		}
    }

    return true;
}

bool CopyNewerFile(const fs::path& source, const fs::path& target, bool* outActuallyCopied/*= nullptr*/)
{
    try
    {
        if (!fs::is_regular_file(source))
            return false;

        if (fs::is_regular_file(target))
        {
            auto sourceTimestamp = fs::last_write_time(source);
            auto targetTimestamp = fs::last_write_time(target);
            if (targetTimestamp >= sourceTimestamp)
            {
                if (outActuallyCopied)
                    *outActuallyCopied = false;
                return true;
            }
        }

        std::cout << "Copying " << target << "\n";
        fs::remove(target);
        fs::create_directories(target.parent_path());
        fs::copy(source, target);

        if (outActuallyCopied)
            *outActuallyCopied = true;

        return true;
    }
    catch (std::exception & e)
    {
        std::cerr << KRED << "[BREAKING] Failed to copy file: " << e.what() << "\n" << RST;
        return false;
    }
}

bool CopyNewerFilesRecursive(const fs::path& sourceDir, const fs::path& targetDir, uint32_t* outActuallyCopied)
{
	try
	{
		if (!fs::is_directory(sourceDir))
			return false;

        if (!fs::is_directory(targetDir))
        {
            std::error_code ec;
            if (!fs::create_directories(targetDir, ec))
            {
                std::cerr << KRED << "[BREAKING] Failed to create directory: " << targetDir << ": " << ec << "\n" << RST;
                return false;
            }
        }

        for (const auto& entry : fs::directory_iterator(sourceDir))
        {
            const auto name = entry.path().filename().u8string();
            const auto targetChildName = targetDir / name;

            if (entry.is_directory())
            {                
                if (!CopyNewerFilesRecursive(entry.path(), targetChildName, outActuallyCopied))
                    return false;
            }
            else if (entry.is_regular_file())
            {
                bool copied = false;
                if (!CopyNewerFile(entry.path(), targetChildName, &copied))
                    return false;

                if (copied && outActuallyCopied)
                    *outActuallyCopied += 1;
            }
        }

		return true;
	}
	catch (std::exception& e)
	{
		std::cerr << KRED << "[BREAKING] Failed to copy directories: " << e.what() << "\n" << RST;
		return false;
	}
}


//--

int GetWeek(struct tm* date)
{
	if (NULL == date)
		return 0; // or -1 or throw exception

	if (::mktime(date) < 0) // Make sure _USE_32BIT_TIME_T is NOT defined.
		return 0; // or -1 or throw exception

	// The basic calculation:
	// {Day of Year (1 to 366) + 10 - Day of Week (Mon = 1 to Sun = 7)} / 7
	int monToSun = (date->tm_wday == 0) ? 7 : date->tm_wday; // Adjust zero indexed week day
	int week = ((date->tm_yday + 11 - monToSun) / 7); // Add 11 because yday is 0 to 365.

	// Now deal with special cases:
	// A) If calculated week is zero, then it is part of the last week of the previous year.
	if (week == 0)
	{
		// We need to find out if there are 53 weeks in previous year.
		// Unfortunately to do so we have to call mktime again to get the information we require.
		// Here we can use a slight cheat - reuse this function!
		// (This won't end up in a loop, because there's no way week will be zero again with these values).
		tm lastDay = { 0 };
		lastDay.tm_mday = 31;
		lastDay.tm_mon = 11;
		lastDay.tm_year = date->tm_year - 1;
		// We set time to sometime during the day (midday seems to make sense)
		// so that we don't get problems with daylight saving time.
		lastDay.tm_hour = 12;
		week = GetWeek(&lastDay);
	}

	// B) If calculated week is 53, then we need to determine if there really are 53 weeks in current year
	//    or if this is actually week one of the next year.
	else if (week == 53)
	{
		// We need to find out if there really are 53 weeks in this year,
		// There must be 53 weeks in the year if:
		// a) it ends on Thurs (year also starts on Thurs, or Wed on leap year).
		// b) it ends on Friday and starts on Thurs (a leap year).
		// In order not to call mktime again, we can work this out from what we already know!
		int lastDay = date->tm_wday + 31 - date->tm_mday;
		if (lastDay == 5) // Last day of the year is Friday
		{
			// How many days in the year?
			int daysInYear = date->tm_yday + 32 - date->tm_mday; // add 32 because yday is 0 to 365
			if (daysInYear < 366)
			{
				// If 365 days in year, then the year started on Friday
				// so there are only 52 weeks, and this is week one of next year.
				week = 1;
			}
		}
		else if (lastDay != 4) // Last day is NOT Thursday
		{
			// This must be the first week of next year
			week = 1;
		}
		// Otherwise we really have 53 weeks!
	}

	return week;
}

int GetWeek
(          // Valid values:
	int day,   // 1 to 31
	int month, // 1 to 12.  1 = Jan.
	int year   // 1970 to 3000
)
{
	tm date = { 0 };
	date.tm_mday = day;
	date.tm_mon = month - 1;
	date.tm_year = year - 1900;
	// We set time to sometime during the day (midday seems to make sense)
	// so that we don't get problems with daylight saving time.
	date.tm_hour = 12;
	return GetWeek(&date);
}

int GetCurrentWeek()
{
	tm today;
	time_t now;
	time(&now);
	errno_t error = ::localtime_s(&today, &now);
	return GetWeek(&today);
}

std::string GetCurrentWeeklyTimestamp()
{
	time_t now;
	time(&now);

	tm today;
	::localtime_s(&today, &now);

    int week = GetWeek(&today);
    int year = today.tm_year % 100;

    char txt[64];
    sprintf_s(txt, sizeof(txt), "%02d%02d", year, week);
    return txt;    
}