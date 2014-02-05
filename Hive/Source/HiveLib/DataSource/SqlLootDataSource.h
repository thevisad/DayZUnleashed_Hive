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
#include "LootDataSource.h"
#include "Database/SqlStatement.h"

namespace Poco { namespace Util { class AbstractConfiguration; }; };
class SqlLootDataSource : public SqlDataSource, public LootDataSource
{
public:

	SqlLootDataSource(Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf);
	~SqlLootDataSource() {}

	void fetchLootPiles( string buildingName, string buildingName1, string buildingName2, string buildingName3, int limitAmount, ServerLootsQueue& queue ) override;

private:
	string _LootTableName;

	SqlStatementID _stmtInsertLootBoxControlLoot;
	SqlStatementID _stmtInsertControlLoot;
	SqlStatementID _stmtInsertTempLoot;
	SqlStatementID _stmtDeleteTempLoot;
	SqlStatementID _stmtCreateTempLoot;
	SqlStatementID _stmtCreateTempTable;
	SqlStatementID _stmtDropServerControlTable;
	SqlStatementID _stmtDropLootBoxControlTable;
	SqlStatementID _stmtCreateServerControlTable;
	SqlStatementID _stmtCreateLootBoxControlTable;
};