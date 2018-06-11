#ifndef DATA_H
#define DATA_H

#include <switch.h>

#include <vector>
#include <string>

namespace data
{
    bool init();
    bool fini();

    void loadDataInfo();

    class titledata
    {
        public:
            bool init(const uint64_t& _id, const u128& _uID);

            const std::string getTitle();
            const std::string getTitleSafe();

            const uint64_t getID();

        private:
            std::string title, titleSafe;
            uint64_t id;
            u128 uID;
    };

    class user
    {
        public:
            bool init(const u128& _id);
            const u128 getUID();
            const std::string getUsername();

            std::vector<titledata> titles;

        private:
            u128 userID;
            std::string username;
    };

    extern std::vector<user> users;
    extern titledata curData;
    extern user curUser;
}

#endif // DATA_H
