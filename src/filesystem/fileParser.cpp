#include "filesystem/fileParser.hpp"
#include "stringUtil.hpp"
#include "log.hpp"

#define VALUE_DELIMINATORS "()=,"
#define STRING_DELIMINATORS ",;\n"

#define VALUE_TRUE "true"
#define VALUE_FALSE "false"

fs::fileParser::fileParser(const std::string &filePath)
{
    m_File = std::fopen(filePath.c_str(), "r");
}

fs::fileParser::~fileParser()
{
    std::fclose(m_File);
}

bool fs::fileParser::isOpen(void) const
{
    return m_File != NULL;
}

bool fs::fileParser::readLine(void)
{
    bool lineRead = false;
    char lineBuffer[0x1000];
    while(std::fgets(lineBuffer, 0x1000, m_File))
    {
        char lineStart = lineBuffer[0];
        if(lineStart == '#' || lineStart == '\n' || lineStart == '\r')
        {
            continue;
        }
        else
        {
            m_CurrentLine.assign(lineBuffer);
            lineRead = true;
            break;
        }
    }

    stringUtil::eraseCharacterFromString('\n', m_CurrentLine);
    stringUtil::eraseCharacterFromString('\r', m_CurrentLine);

    m_CurrentLinePosition = m_CurrentLine.find_first_of(VALUE_DELIMINATORS);
    m_ItemName.assign(m_CurrentLine.begin(), m_CurrentLine.begin() + m_CurrentLinePosition);
    stringUtil::eraseCharacterFromString(' ', m_ItemName);

    ++m_CurrentLinePosition;

    return lineRead;
}

std::string fs::fileParser::getCurrentItemName(void) const
{
    return m_ItemName;
}

bool fs::fileParser::getNextValueAsBool(void)
{
    std::string value = getNextValueAsString();
    stringUtil::eraseCharacterFromString(' ', value);
    return value == VALUE_TRUE ? true : false;
}

int fs::fileParser::getNextValueAsInt(void)
{
    std::string value = getNextValueAsString();
    stringUtil::eraseCharacterFromString(' ', value);
    return std::stoi(value);
}

float fs::fileParser::getNextValueAsFloat(void)
{
    std::string value = getNextValueAsString();
    stringUtil::eraseCharacterFromString(' ', value);
    return std::stof(value);
}

uint64_t fs::fileParser::getNextValueAsUint64(void)
{
    std::string value = getNextValueAsString();
    stringUtil::eraseCharacterFromString(' ', value);
    return std::stoul(value, 0, 16);
}

std::string fs::fileParser::getNextValueAsString(void)
{
    m_CurrentLinePosition  = m_CurrentLine.find_first_not_of(" ,", m_CurrentLinePosition);
    size_t startingPosition = m_CurrentLinePosition;

    char firstChar = m_CurrentLine.at(m_CurrentLinePosition);
    if(firstChar == '"')
    {
        m_CurrentLinePosition = m_CurrentLine.find_first_of('"', ++startingPosition);
    }
    else
    {
        m_CurrentLinePosition = m_CurrentLine.find_first_of(STRING_DELIMINATORS);
    }
    return m_CurrentLine.substr(startingPosition, m_CurrentLinePosition++ - startingPosition);
}