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
using boost::lexical_cast;
using boost::bad_lexical_cast;
#include <Poco/Util/AbstractConfiguration.h>
#include <ctime>
#include <sstream>
#include <string> 

namespace
{
	typedef boost::optional<Sqf::Value> BuildingInfo;



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
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz";

int stringLength = sizeof(alphanum) - 1;

char genRandom()
{

    return alphanum[rand() % stringLength];
}


void SqlLootDataSource::fetchLootPiles( string buildingName, string buildingName1, string buildingName2, string buildingName3, int limitAmount, ServerLootsQueue& queue )
{

	clock_t begin = clock();
	int count = 0;
	int count2 = 0;
	srand(time(0));
	std::string tablename1;
	std::string tablename2;
    for(unsigned int i = 0; i < 20; ++i)
    {
		tablename1 += genRandom();
		tablename2 += genRandom();
    }

	Sqf::Parameters variables;

	{
		auto createLootBoxStmt = getDB()->makeStatement( _stmtCreateLootBoxControlTable,"CREATE TABLE IF NOT EXISTS " + tablename1 + " (loot_name VARCHAR(50) NOT NULL);");
		bool exRes = createLootBoxStmt->execute();
		poco_assert(exRes == true);
	}
	{
		auto bldTempTable  = getDB()->makeStatement( _stmtCreateTempLoot,"CREATE TABLE IF NOT EXISTS " + tablename2 + " (loot_name VARCHAR(50) NOT NULL,loot_type VARCHAR(50) NOT NULL,loot_max INT(11) NOT NULL,game_max INT(11) NOT NULL);");
		bool exRes = bldTempTable->execute();
		poco_assert(exRes == true);
	}


	auto possibleLoot = getDB()->queryParams("SELECT building_loot.loot_name, building_loot.loot_type, building_loot.loot_max, building_loot.game_max FROM building_data "
		"INNER JOIN building_type ON building_data.building_type = building_type.building_type INNER JOIN building_loot ON building_type.loot_level = building_loot.loot_level "
		"WHERE building_data.building_name = '%s' or building_data.building_name = '%s' or building_data.building_name = '%s' or building_data.building_name = '%s' order by rand()"
		, getDB()->escape(buildingName).c_str(), getDB()->escape(buildingName1).c_str(), getDB()->escape(buildingName2).c_str(), getDB()->escape(buildingName3).c_str());

		
	{

		if (!possibleLoot || !possibleLoot->fetchRow())
		{
			_logger.error("Error fetching loot for building: " + buildingName);
			return;
		}
		while (possibleLoot->fetchRow())
		{
			string loot_name;
			try {
				auto possibleLootRow = possibleLoot->fields();
				loot_name = possibleLootRow[0].getString();
				string loot_type = possibleLootRow[1].getString();
				int loot_amount = possibleLootRow[2].getInt16();
				int game_max = possibleLootRow[3].getInt16();
				for (int i = 1; i <= loot_amount; i++)
				{
					auto stmt = getDB()->makeStatement( _stmtInsertTempLoot,"INSERT " + tablename2 + "  (loot_name,loot_type, loot_max, game_max ) VALUES (?,?,?,?);");
					stmt->addString(loot_name);
					stmt->addString(loot_type);
					stmt->addInt16(loot_amount);
					stmt->addInt16(game_max);
					bool exRes = stmt->execute();
					poco_assert(exRes == true);
					count++;
				}
				count2++;
			}
			
			catch (const bad_lexical_cast&)
			{
				_logger.error("Skipping building loot " + lexical_cast<string>(loot_name) + " load because of invalid data in db");
				continue;
			}
		}
		{
			auto buildingLoot = getDB()->queryParams("SELECT loot_name, loot_type,loot_max, game_max FROM `%s` order by rand() limit %d", getDB()->escape(tablename2).c_str(), limitAmount);
			if (!buildingLoot || !buildingLoot->fetchRow())
			{
				_logger.error("Error fetching loot for building: " + buildingName);
				return;
			}
			int game_max = 0;
			int lootMax_amount = 0;
			int boxmax_amount = 0;
			int boxcount = 0;
			while (buildingLoot->fetchRow())
			{
				string tempLoot_name;
				try
				{
					game_max=0;
					lootMax_amount=0;
					boxmax_amount=0;
					boxcount=0;
				
					auto buildingLootRow = buildingLoot->fields();
					tempLoot_name = buildingLootRow[0].getString();
					string tempLoot_type = buildingLootRow[1].getString();
					boxmax_amount = buildingLootRow[2].getInt16();
					game_max = buildingLootRow[3].getInt16();
					{
						auto lootMax = getDB()->queryParams("SELECT count(loot_name) FROM ServerControlTable where loot_name = '%s'", getDB()->escape(tempLoot_name).c_str());
						auto lootMaxRow = lootMax->fields();
						lootMax_amount = lootMaxRow[0].getInt16();
					}
					if (lootMax_amount >= game_max) {
						_logger.information("Too many items in game for (" + tempLoot_name + ")" );
					}
					else 
					{
						{
							auto controlstmt = getDB()->makeStatement( _stmtInsertControlLoot,"INSERT `ServerControlTable` (loot_name) VALUES (?);");
							controlstmt->addString(tempLoot_name);
							bool exRes = controlstmt->execute();
							poco_assert(exRes == true);
						}
						{
							auto lootBoxMax = getDB()->queryParams("SELECT count(loot_name) FROM `%s`  where loot_name = '%s'", getDB()->escape(tablename1).c_str(), getDB()->escape(tempLoot_name).c_str());
							auto lootBoxMaxRow = lootBoxMax->fields();
							boxcount = lootBoxMaxRow[0].getInt16();
						}
						if (boxcount >= boxmax_amount) {
							_logger.information("Too many items in box for (" + tempLoot_name + ")" );
						} else {
							{
								auto lootboxcontrolStmt = getDB()->makeStatement( _stmtInsertLootBoxControlLoot,"INSERT " + tablename1 + "  (loot_name) VALUES (?);");
								lootboxcontrolStmt->addString(tempLoot_name);
								bool exRes = lootboxcontrolStmt->execute();
								poco_assert(exRes == true);
							}
							variables.push_back(tempLoot_name);
							variables.push_back(tempLoot_type);
							_logger.information("Pushed Loot (" + lexical_cast<string>(tempLoot_name)+ ")");
							
						}
					}
				}
			
				catch (const bad_lexical_cast&)
				{
					_logger.error("Skipping building loot " + lexical_cast<string>(tempLoot_name) + " load because of invalid data in db");
					continue;
				}
			}
			queue.push(variables);
		}



		/*{
				auto dropStmt = getDB()->makeStatement( _stmtDeleteTempLoot,"DROP TABLE tempLootTable;");
				bool exRes = dropStmt->execute();
				poco_assert(exRes == true);
		}*/

	}
	
	{
		auto tempLootBoxDropStmt = getDB()->makeStatement(_stmtDropLootBoxControlTable ,"DROP TABLE " + tablename1);
		bool exRes = tempLootBoxDropStmt->execute();
		poco_assert(exRes == true);
	}

	{
		auto dropLootTablesStmt = getDB()->makeStatement( _stmtDeleteTempLoot,"DROP TABLE " + tablename2);
		//auto dropLootTablesStmt = getDB()->makeStatement( _stmtDeleteTempLoot,"TRUNCATE tempLootTable;");
		bool exRes = dropLootTablesStmt->execute();
		poco_assert(exRes == true);
	}

	_logger.information("Pulled building loot for building (" + buildingName + ")" );
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	std::string str = boost::lexical_cast<std::string>(elapsed_secs);
	std::string str2 = boost::lexical_cast<std::string>(count);
	std::string str3 = boost::lexical_cast<std::string>(count2);
	_logger.information("Total time to run building loot query (" + str + ")" );
	_logger.information("Total items pushed to temp loot table (" + str2 + ")" );
	_logger.information("Total items looped through (" + str3 + ")" );

}
