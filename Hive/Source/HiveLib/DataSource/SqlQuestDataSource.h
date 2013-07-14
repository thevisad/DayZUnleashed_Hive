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

#pragma once

#include "SqlDataSource.h"
#include "QuestDataSource.h"
#include "Database/SqlStatement.h"

namespace Poco { namespace Util { class AbstractConfiguration; }; };
class SqlQuestDataSource : public SqlDataSource, public QuestDataSource
{
public:
	SqlQuestDataSource(Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf);
	~SqlQuestDataSource() {}

	void populateQuests( int serverId, ServerQuestsQueue& queue ) override;
	void populatePlayerQuests( int serverId, ServerQuestsQueue& queue ) override;
	bool deletePlayerQuest( int serverId, Int64 playerQuestIdent, bool byUID ) override;
	bool createQuest( int serverId, const string& QuestName ) override;
	bool createPlayerQuest( int QuestId, int characterId ) override;
private:
	string _QuestTableName;

	SqlStatementID _stmtCreateQuest;
	SqlStatementID _stmtCreatePlayerQuest;
	SqlStatementID _stmtUpdateQuestByID;
	SqlStatementID _stmtDeletePlayerQuestByUID;
	SqlStatementID _stmtDeletePlayerQuestByID;
};