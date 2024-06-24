#include <array>
#include <string>
#include "filesystem/fileParser.hpp"
#include "stringUtil.hpp"
#include "log.hpp"

namespace
{
    // Strings needed for this file
    const std::string VALUE_DELIMINATORS = "()=,";
    const std::string STRING_DELIMINATORS = ",;\n";
    const std::string VALUE_TRUE = "true";
    const std::string VALUE_FALSE = "false";

    // Length limit for line reading
    const int FILE_PARSER_LINE_SIZE_LIMIT = 0x1000;
}

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
    // To return whether a new line was successfully read
    bool lineRead = false;
    // Buffer to read lines into
    std::array<char, FILE_PARSER_LINE_SIZE_LIMIT> lineBuffer;

    // Loop until a line isn't a comment or new line
    while(std::fgets(lineBuffer.data(), FILENAME_MAX, m_File))
    {
        // Get first char in line. If it's a comment or new line, continue loop
        char lineStart = lineBuffer.at(0);
        if(lineStart == '#' || lineStart == '\n' || lineStart == '\r')
        {
            continue;
        }
        else
        {
            // Assign lineBuffer to current line C++ string
            m_CurrentLine.assign(lineBuffer.data());
            // Line read was successful, break the loop
            lineRead = true;
            break;
        }
    }

    // These need to be removed because they can cause issues
    stringUtil::eraseCharacterFromString('\n', m_CurrentLine);
    stringUtil::eraseCharacterFromString('\r', m_CurrentLine);

    // Find first "word" of line. 
    m_CurrentLinePosition = m_CurrentLine.find_first_of(VALUE_DELIMINATORS);
    // Assign it
    m_ItemName.assign(m_CurrentLine.begin(), m_CurrentLine.begin() + m_CurrentLinePosition);
    // Remove spaces
    stringUtil::eraseCharacterFromString(' ', m_ItemName);
    // Increment after value deliminator
    ++m_CurrentLinePosition;

    return lineRead;
}

std::string fs::fileParser::getCurrentItemName(void) const
{
    return m_ItemName;
}

// All of these just us getNextValueAsString and converts on return
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
    // Find the first char that isn't a space or comma
    m_CurrentLinePosition  = m_CurrentLine.find_first_not_of(" ,", m_CurrentLinePosition);
    // Starting position of string we're extracting
    size_t startingPosition = m_CurrentLinePosition;

    // Need to test for quotes
    char firstChar = m_CurrentLine.at(m_CurrentLinePosition);
    // If it's a quote, just keep going until the ending quote.
    if(firstChar == '"')
    {
        m_CurrentLinePosition = m_CurrentLine.find_first_of('"', ++startingPosition);
    }
    else
    {
        m_CurrentLinePosition = m_CurrentLine.find_first_of(STRING_DELIMINATORS);
    }
    // Return the substring 
    return m_CurrentLine.substr(startingPosition, m_CurrentLinePosition++ - startingPosition);
}