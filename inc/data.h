#ifndef DATA_H
#define DATA_H

#include <switch.h>

#include <vector>
#include <string>

#include "gfx.h"

namespace data
{
	extern bool sysSave, forceMountable;

	//Loads user + title info
	void loadDataInfo();
	void exit();

	//Class to help not load the same icons over and over
	class icn
	{
		public:
			void load(const uint64_t& _id, const uint8_t *jpegData, const size_t& jpegSize);
			void setIcon(const uint64_t& _id, const gfx::tex& _set);
			void draw(unsigned x, unsigned y);
			void drawHalf(unsigned x, unsigned y);
			uint64_t getTitleID();

			void deleteData();

		private:
			uint64_t titleID;
			gfx::tex iconTex;
	};

	//Class to store title info
	class titledata
	{
		public:
			//Attempts to read title's info. Returns false if failed
			bool init(const FsSaveDataInfo& inf);

			//Attempts to mount data with uID + id. Returns false if fails. For filtering.
			bool isMountable(const u128& uID);

			//Returns title + title without forbidden chars
			std::string getTitle();
			std::string getTitleSafe();

			//Returns ID
			uint64_t getID();

			//Game icon
			icn icon;

			FsSaveDataType getType();

		private:
			FsSaveDataType type;
			std::string title, titleSafe;
			uint64_t id;
			u128 uID;
	};

	//Class to store user info + titles
	class user
	{
		public:
			//Attempts to read user data using _id
			bool init(const u128& _id);

			//Allows user to init without reading data. For fun.
			bool initNoChk(const u128& _id);

			//Returns user ID
			u128 getUID();

			//Returns username
			std::string getUsername();
			std::string getUsernameSafe();

			//Vector for storing save data info for user
			std::vector<titledata> titles;

			gfx::tex icn;

		private:
			u128 userID;
			std::string username, userSafe;
	};

	//User vector
	extern std::vector<user> users;

	//Stores current data we're using so I don't have to type so much.
	extern titledata curData;
	extern user curUser;
}

#endif // DATA_H
