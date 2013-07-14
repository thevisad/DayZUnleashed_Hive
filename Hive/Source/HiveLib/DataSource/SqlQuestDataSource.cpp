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

#include "SqlQuestDataSource.h"
#include "Database/Database.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
using boost::lexical_cast;
using boost::bad_lexical_cast;

namespace
{
	typedef boost::optional<Sqf::Value> PositionInfo;
	class PositionFixerVisitor : public boost::static_visitor<PositionInfo>
	{
	public:
		PositionInfo operator()(Sqf::Parameters& pos) const 
		{ 
			if (pos.size() != 3)
				return PositionInfo();

			try
			{
				double x = Sqf::GetDouble(pos[0]);
				double y = Sqf::GetDouble(pos[1]);
				double z = Sqf::GetDouble(pos[2]);

				if (x < 0 || y > 15360)
				{
					PositionInfo fixed(pos);
					pos.clear();
					return fixed;
				}
			}
			catch(const boost::bad_get&) {}

			return PositionInfo();
		}
		template<typename T> PositionInfo operator()(const T& other) const	{ return PositionInfo(); }
	};

	class WorldspaceFixerVisitor : public boost::static_visitor<PositionInfo>
	{
	public:
		PositionInfo operator()(Sqf::Parameters& ws) const 
		{ 
			if (ws.size() != 2)
				return PositionInfo();

			return boost::apply_visitor(PositionFixerVisitor(),ws[1]);
		}
		template<typename T> PositionInfo operator()(const T& other) const	{ return PositionInfo(); }
	};

	PositionInfo FixOOBWorldspace(Sqf::Value& v) { return boost::apply_visitor(WorldspaceFixerVisitor(),v); }
};

#include <Poco/Util/AbstractConfiguration.h>

SqlQuestDataSource::SqlQuestDataSource( Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf ) : SqlDataSource(logger,db)
{
	static const string defaultTable = "instance_Quests"; 
	/*
	if (conf != NULL)
	{
		_objTableName = getDB()->escape(conf->getString("Table",defaultTable));
		_cleanupPlacedDays = conf->getInt("CleanupPlacedAfterDays",6);
		_vehicleOOBReset = conf->getBool("ResetOOBVehicles",false);
	}
	else
	{
		_objTableName = defaultTable;
		_cleanupPlacedDays = -1;
		_vehicleOOBReset = false;
	}*/
}


void SqlQuestDataSource::populateQuests( int serverId, ServerQuestsQueue& queue )
{

	auto worldBuildRes = getDB()->queryParams("SELECT instance_Quest.objectUID, Quest.class_name, instance_Quest.characterId, instance_Quest.worldspace, instance_Quest.inventory, instance_Quest.hitpoints, instance_Quest.QuestId, instance_Quest.combination FROM Quest INNER JOIN instance_Quest ON instance_Quest.QuestId = Quest.id WHERE instance_Quest.instanceId = '%d'", serverId);

	if (!worldBuildRes)
	{
		_logger.error("Failed to fetch objects from database");
		return;
	}
	while (worldBuildRes->fetchRow())
	{
		auto row = worldBuildRes->fields();

		Sqf::Parameters qstParams;
		//qstParams.push_back(string("OBJ"));

		int objectId = row[0].getInt32();
		qstParams.push_back(lexical_cast<string>(objectId));
			try
			{
				qstParams.push_back(row[1].getString()); //objectId should be stringified 
				qstParams.push_back(lexical_cast<string>(row[2].getInt32())); //ownerId should be stringified
				Sqf::Value worldSpace = lexical_cast<Sqf::Value>(row[3].getString());

				_logger.information("Pushed QuestID (" + lexical_cast<string>(objectId) + ") class name (" + row[1].getString() + ") tp position  (" + row[2].getString() + ")");
				qstParams.push_back(worldSpace);
				//Inventory can be NULL
				{
					string invStr = "[]";
					if (!row[4].isNull())
						invStr = row[4].getString();

					qstParams.push_back(lexical_cast<Sqf::Value>(invStr));
				}	
				qstParams.push_back(lexical_cast<Sqf::Value>(row[5].getCStr()));
				qstParams.push_back(row[6].getInt32());
				qstParams.push_back(row[7].getInt32());
			}
			
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping QuestID " + lexical_cast<string>(objectId) + " load because of invalid data in db");
			continue;
		}

		queue.push(qstParams);
	}
}

void SqlQuestDataSource::populatePlayerQuests( int serverId, ServerQuestsQueue& queue )
{

	auto worldBuildRes = getDB()->queryParams("SELECT instance_Quest.objectUID, Quest.class_name, instance_Quest.characterId, instance_Quest.worldspace, instance_Quest.inventory, instance_Quest.hitpoints, instance_Quest.QuestId, instance_Quest.combination FROM Quest INNER JOIN instance_Quest ON instance_Quest.QuestId = Quest.id WHERE instance_Quest.instanceId = '%d'", serverId);

	if (!worldBuildRes)
	{
		_logger.error("Failed to fetch objects from database");
		return;
	}
	while (worldBuildRes->fetchRow())
	{
		auto row = worldBuildRes->fields();

		Sqf::Parameters qstParams;
		//qstParams.push_back(string("OBJ"));

		int objectId = row[0].getInt32();
		qstParams.push_back(lexical_cast<string>(objectId));
			try
			{
				qstParams.push_back(row[1].getString()); //objectId should be stringified 
				qstParams.push_back(lexical_cast<string>(row[2].getInt32())); //ownerId should be stringified
				Sqf::Value worldSpace = lexical_cast<Sqf::Value>(row[3].getString());

				_logger.information("Pushed QuestID (" + lexical_cast<string>(objectId) + ") class name (" + row[1].getString() + ") tp position  (" + row[2].getString() + ")");
				qstParams.push_back(worldSpace);
				//Inventory can be NULL
				{
					string invStr = "[]";
					if (!row[4].isNull())
						invStr = row[4].getString();

					qstParams.push_back(lexical_cast<Sqf::Value>(invStr));
				}	
				qstParams.push_back(lexical_cast<Sqf::Value>(row[5].getCStr()));
				qstParams.push_back(row[6].getInt32());
				qstParams.push_back(row[7].getInt32());
			}
			
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping QuestID " + lexical_cast<string>(objectId) + " load because of invalid data in db");
			continue;
		}

		queue.push(qstParams);
	}
}

bool SqlQuestDataSource::deletePlayerQuest( int serverId, Int64 playerQuestIdent, bool byUID )
{
	unique_ptr<SqlStatement> stmt;
	if (byUID)
		stmt = getDB()->makeStatement(_stmtDeletePlayerQuestByUID, "DELETE FROM `instance_Quest` WHERE `ObjectUID` = ? AND `Instance` = ?");
	else
		stmt = getDB()->makeStatement(_stmtDeletePlayerQuestByID, "DELETE FROM `instance_Quest` WHERE `ObjectID` = ? AND `Instance` = ?");

	stmt->addInt64(playerQuestIdent);
	stmt->addInt32(serverId);

	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}



bool SqlQuestDataSource::createQuest( int serverId, const string& QuestName )
{
	auto stmt = getDB()->makeStatement(_stmtCreateQuest, 
		"INSERT INTO `Quest` (`instance_id`, `Quest_name`) "
		"VALUES ('?' '?',CURRENT_TIMESTAMP )");

	stmt->addInt32(serverId);
	stmt->addString(QuestName);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlQuestDataSource::createPlayerQuest( int QuestId, int characterId )
{
	auto stmt = getDB()->makeStatement(_stmtCreatePlayerQuest, 
		"INSERT INTO `instance_Quest` (`QuestId`, `CharacterID`) "
		"VALUES (?, ?, CURRENT_TIMESTAMP)");

	stmt->addInt32(QuestId);
	stmt->addInt32(characterId);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}





