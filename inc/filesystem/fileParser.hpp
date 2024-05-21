#pragma once
#include <cstdio>
#include <cstdint>
#include <string>

namespace fs
{
    class fileParser
    {
        public:
            // Just opens m_File with filePath
            fileParser(const std::string &filePath);
            ~fileParser();
            // Returns if file was opened
            bool isOpen(void) const;
            // Returns false if line could not be read from file
            bool readLine(void);
            // Returns the string in the beginning of the line before a ( or = is found
            std::string getCurrentItemName(void) const;
            // Returns values/arguments. All rely on getNextValueAsString
            bool getNextValueAsBool(void);
            int getNextValueAsInt(void);
            float getNextValueAsFloat(void);
            uint64_t getNextValueAsUint64(void);
            std::string getNextValueAsString(void);

        private:
            // Using C files to make getting lines easier
            std::FILE *m_File;
            // Current line
            std::string m_CurrentLine;
            // Name/value at the beginning
            std::string m_ItemName;
            // Current position in line
            size_t m_CurrentLinePosition = 0;
    };
}
