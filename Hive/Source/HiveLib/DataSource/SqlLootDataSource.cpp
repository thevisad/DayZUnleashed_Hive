/*
* Copyright (C) 2009-2012 Rajko Stojadinovic <http://github.com/rajkosto/hive>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "SqlLootDataSource.h"
#include "Database/Database.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
using boost::lexical_cast;
using boost::bad_lexical_cast;
#include <Poco/Util/AbstractConfiguration.h>
#include <ctime>
#include <sstream>
#include <iostream>
#include <string> 

namespace
{
	typedef boost::optional<Sqf::Value> BuildingInfo;


	class object{
	public:
		object()
		: tag(boost::uuids::random_generator()())
		, state(0)
		{}
	private:
		boost::uuids::uuid tag;

		int state;
	};

	class GetOptionalBuildings : public boost::static_visitor<BuildingInfo>
	{

	public:
		BuildingInfo operator()(Sqf::Parameters& pos) const 
		{ 
			if (pos.size() != 3)
				return BuildingInfo();

			try
			{
				string x = Sqf::GetStringAny(pos[0]);
				string y = Sqf::GetStringAny(pos[1]);
				string z = Sqf::GetStringAny(pos[2]);

				if (y == "")
				{
					BuildingInfo fixed(pos);
					pos.clear();
					return fixed;
				}
			}
			catch(const boost::bad_get&) {}

			return BuildingInfo();
		}
		template<typename T> BuildingInfo operator()(const T& other) const	{ return BuildingInfo(); }
	};
	BuildingInfo GetBuildings(Sqf::Value& v) { return boost::apply_visitor(GetOptionalBuildings(),v); }


	        
}

SqlLootDataSource::SqlLootDataSource( Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf ) : SqlDataSource(logger,db)
{
	static const string defaultTable = "instance_Instances"; 

}

static const char alphanum[] =
"0123456789"
"abcdefghijklmnopqrstuvwxyz";

int stringLength = sizeof(alphanum) - 1;

char genRandom()
{
    return alphanum[rand() % stringLength];
}

int loot = 0;

string SqlLootDataSource::createHouseTable(string buildingNameID)
{

	try {
		auto possibleLoot = getDB()->queryParams("SELECT building_loot.loot_name, building_loot.loot_type, building_loot.loot_max, building_loot.game_max FROM building_data "
			"INNER JOIN building_type ON building_data.building_type = building_type.building_type INNER JOIN building_loot ON building_type.loot_level = building_loot.loot_level "
			"WHERE building_data.building_name = '%s' or building_data.building_name = '%s' or building_data.building_name = '%s' order by rand()"
			, getDB()->escape(buildingName).c_str(), getDB()->escape(buildingNameAlt1).c_str(), getDB()->escape(buildingNameAlt2).c_str());


		if (!possibleLoot) //if (!possibleLoot || !possibleLoot->fetchRow())
		{
			clock_t end = clock();
			double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
			std::string str = boost::lexical_cast<std::string>(elapsed_secs);
			//std::string str2 = boost::lexical_cast<std::string>(count);
			//std::string str3 = boost::lexical_cast<std::string>(count2);
			_logger.information("Failed Loot: Total time to run building loot query (" + str + ")");
			//_logger.information("Total items pushed to temp loot table (" + str2 + ")");
			//_logger.information("Total items looped through (" + str3 + ")");
			return;
		}
	}

	catch (...) //catch (const bad_lexical_cast&)
	{
		return "";
	}

	boost::uuids::uuid houseuuid = boost::uuids::random_generator()();
	//std::cout << uuid << std::endl;
	std::string houseUUID = boost::lexical_cast<std::string>(houseuuid);

	return houseUUID;
}

string SqlLootDataSource::getUUID()
{
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	std::string stringuuid = boost::lexical_cast<std::string>(uuid);

	return stringuuid;
}



void SqlLootDataSource::fetchLootPiles( string buildingName, string buildingNameID, string buildingNameAlt1, string buildingNameAlt2, int limitAmount, ServerLootsQueue& queue ) {
		clock_t begin = clock();
		int count = 0;
		int count2 = 0;
		srand(time(0));

		
		loot++;
		std::string loop = boost::lexical_cast<std::string>(loot);
		string loot_name;

		string houseUUID = SqlLootDataSource::createHouseTable(buildingNameID);


		auto possibleLoot = getDB()->queryParams("SELECT building_loot.loot_name, building_loot.loot_type, building_loot.loot_max, building_loot.game_max FROM building_data "
			"INNER JOIN building_type ON building_data.building_type = building_type.building_type INNER JOIN building_loot ON building_type.loot_level = building_loot.loot_level "
			"WHERE building_data.building_name = '%s' or building_data.building_name = '%s' or building_data.building_name = '%s' order by rand()"
			, getDB()->escape(buildingName).c_str(), getDB()->escape(buildingNameAlt1).c_str(), getDB()->escape(buildingNameAlt2).c_str());


		if (!possibleLoot) //if (!possibleLoot || !possibleLoot->fetchRow())
		{
			clock_t end = clock();
			double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
			std::string str = boost::lexical_cast<std::string>(elapsed_secs);
			//std::string str2 = boost::lexical_cast<std::string>(count);
			//std::string str3 = boost::lexical_cast<std::string>(count2);
			_logger.information("Failed Loot: Total time to run building loot query (" + str + ")");
			//_logger.information("Total items pushed to temp loot table (" + str2 + ")");
			//_logger.information("Total items looped through (" + str3 + ")");
			return;
		}
		while (possibleLoot->fetchRow())
		{
			
			try {
				auto possibleLootRow = possibleLoot->fields();
				loot_name = possibleLootRow[0].getString();
				string loot_type = possibleLootRow[1].getString();
				int loot_amount = possibleLootRow[2].getInt16();
				int game_max = possibleLootRow[3].getInt16();
				for (int i = 1; i <= loot_amount; i++)
				{
					auto stmt = getDB()->makeStatement(_stmtInsertTempLoot, "INSERT INTO lootboxtemp (loot_guid,loot_name,loot_type, loot_max, game_max ) VALUES (?,?,?,?,?);");
					stmt->addString(loot_guid);
					stmt->addString(loot_name);
					stmt->addString(loot_type);
					stmt->addInt16(loot_amount);
					stmt->addInt16(game_max);
					bool exRes = stmt->directExecute();
					//poco_assert(exRes == true);
					//_logger.error("Inserting into table guid: " + loot_guid + " loop" + loop);
					count++;
				}
				count2++;
			}

			catch (...) //catch (const bad_lexical_cast&)
			{
				_logger.error("Skipping building loot " + lexical_cast<string>(loot_name)+" load because of invalid data in db");
				continue;
			}
		}

		auto buildingLoot = getDB()->queryParams("SELECT loot_name, loot_type,loot_max, game_max FROM `lootboxtemp` where loot_guid = '%s' order by rand() limit %d", getDB()->escape(loot_guid).c_str(), limitAmount);
		if (!buildingLoot || !buildingLoot->fetchRow())
		{

			auto lootboxcontrolStmt = getDB()->makeStatement(_stmtDeleteLootBoxControlTable, "delete from lootboxcontrol where loot_guid = ?;");
			lootboxcontrolStmt->addString(loot_guid);
			bool exRes9 = lootboxcontrolStmt->directExecute();
			//_logger.error("Deleting the guid id: " + loot_guid + " in loop" + loop);
			//poco_assert(exRes9 == true);

			auto loottempboxStmt = getDB()->makeStatement(_stmtDeleteTempLoot, "delete from lootboxtemp where loot_guid = ?;");
			loottempboxStmt->addString(loot_guid);
			bool exRes0 = loottempboxStmt->directExecute();
			//_logger.error("Deleting the guid id: " + loot_guid + " in loop" + loop);
			//poco_assert(exRes0 == true);

			//_logger.error("Error: fetching loot for guid: " + loot_guid + " loop" + loop);
			clock_t end = clock();
			double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
			std::string str = boost::lexical_cast<std::string>(elapsed_secs);
			std::string str2 = boost::lexical_cast<std::string>(count);
			std::string str3 = boost::lexical_cast<std::string>(count2);
			_logger.information("Total time to run building loot query (" + str + ")");
			_logger.information("Total items pushed to temp loot table (" + str2 + ")");
			_logger.information("Total items looped through (" + str3 + ")");
			return;
		}
		int game_max = 0;
		int lootMax_amount = 0;
		int boxmax_amount = 0;
		int boxcount = 0;
		while (buildingLoot->fetchRow())
		{
			string tempLoot_name;
			auto buildingLootRow = buildingLoot->fields();
			
			try
			{
				game_max = 0;
				lootMax_amount = 0;
				boxmax_amount = 0;
				boxcount = 0;

				
				tempLoot_name = buildingLootRow[0].getString();
				string tempLoot_type = buildingLootRow[1].getString();
				boxmax_amount = buildingLootRow[2].getInt16();
				game_max = buildingLootRow[3].getInt16();

				auto lootMax = getDB()->queryParams("SELECT count(loot_name) FROM ServerControlTable where loot_name = '%s'", getDB()->escape(tempLoot_name).c_str());
				auto lootMaxRow = lootMax->fields();
				lootMax_amount = lootMaxRow[0].getInt16();

				if (lootMax_amount >= game_max) {
					//_logger.information("Too many items in game for (" + tempLoot_name + ")");
				}
				else
				{
						auto controlstmt = getDB()->makeStatement(_stmtInsertControlLoot, "INSERT INTO `ServerControlTable` (loot_name) VALUES (?);");
						controlstmt->addString(tempLoot_name);
						bool exRes = controlstmt->directExecute();
						//poco_assert(exRes == true);
						auto lootBoxMax = getDB()->queryParams("SELECT count(loot_name) FROM `lootboxcontrol` where loot_name = '%s' and loot_guid = '%s'", getDB()->escape(tempLoot_name).c_str(), getDB()->escape(loot_guid).c_str());
						auto lootBoxMaxRow = lootBoxMax->fields();
						//_logger.error("Selecting loot for guid id: " + loot_guid + " loop" + loop);
						boxcount = lootBoxMaxRow[0].getInt16();
					if (boxcount >= boxmax_amount) {
						//_logger.information("Too many items in box for (" + tempLoot_name + ")");
					}
					else {

						auto lootboxcontrolStmt = getDB()->makeStatement(_stmtInsertLootBoxControlLoot, "INSERT INTO lootboxcontrol (loot_guid, loot_name) VALUES (?,?);");
						lootboxcontrolStmt->addString(loot_guid);
						lootboxcontrolStmt->addString(tempLoot_name);
						bool exRes = lootboxcontrolStmt->directExecute();
						//_logger.error("Inserting loot for guid id: " + loot_guid + " lootboxcontrol loop" + loop);
						//poco_assert(exRes == true);
						//variables.push_back("LootStart");
						Sqf::Parameters variables;
						variables.push_back(tempLoot_name);
						variables.push_back(tempLoot_type);
						queue.push(variables);
						//_logger.information("Pushed Loot (" + lexical_cast<string>(tempLoot_name)+")");

					}
				}
			}

			catch (const bad_lexical_cast&)
			{
				_logger.error("Skipping building loot " + lexical_cast<string>(tempLoot_name)+" load because of invalid data in db");
				continue;
			}
			
		}
		

		{
			auto lootboxcontrolStmt = getDB()->makeStatement(_stmtDeleteLootBoxControlTable, "delete from lootboxcontrol where loot_guid = ?;");
			lootboxcontrolStmt->addString(loot_guid);
			bool exRes9 = lootboxcontrolStmt->execute();
			_logger.information("Deleting the guid id: " + loot_guid + " in loop" + loop);
			poco_assert(exRes9 == true);

			auto loottempboxStmt = getDB()->makeStatement(_stmtDeleteTempLoot, "delete from lootboxtemp where loot_guid = ?;");
			loottempboxStmt->addString(loot_guid);
			bool exRes0 = loottempboxStmt->execute();
			_logger.information("Deleting the guid id: " + loot_guid + " in loop" + loop);
			poco_assert(exRes0 == true);
		}

		//_logger.information("Pulled building loot for building (" + buildingName + ")");
		clock_t end = clock();
		double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
		std::string str = boost::lexical_cast<std::string>(elapsed_secs);
		//std::string str2 = boost::lexical_cast<std::string>(count);
		//std::string str3 = boost::lexical_cast<std::string>(count2);
		_logger.information("Total time to run building loot query (" + str + ")");
		//_logger.information("Total items pushed to temp loot table (" + str2 + ")");
		//_logger.information("Total items looped through (" + str3 + ")");
}
