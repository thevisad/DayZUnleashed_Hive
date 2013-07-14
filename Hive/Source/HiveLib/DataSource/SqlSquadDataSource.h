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
#include "SquadDataSource.h"
#include "Database/SqlStatement.h"

namespace Poco { namespace Util { class AbstractConfiguration; }; };
class SqlSquadDataSource : public SqlDataSource, public SquadDataSource
{
public:

	SqlSquadDataSource(Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf);
	~SqlSquadDataSource() {}

	void populateSquads( int serverId, ServerSquadsQueue& queue ) override;
	void populatePlayerSquads( int serverId, ServerSquadsQueue& queue ) override;
	bool deleteSquad( int serverId, Int64 squadIdent, bool byUID ) override;
	bool deletePlayerSquad( int serverId, Int64 playerSquadIdent, bool byUID ) override;
	bool createSquad( int serverId, const string& squadName ) override;
	bool createPlayerSquad( int squadId, int characterId ) override;
private:
	string _SquadTableName;

	SqlStatementID _stmtCreateSquad;
	SqlStatementID _stmtCreatePlayerSquad;
	SqlStatementID _stmtDeletePlayerSquadByUID;
	SqlStatementID _stmtDeletePlayerSquadByID;
	SqlStatementID _stmtUpdateSquadByID;
	SqlStatementID _stmtDeleteSquadByUID;
	SqlStatementID _stmtDeleteSquadByID;
};