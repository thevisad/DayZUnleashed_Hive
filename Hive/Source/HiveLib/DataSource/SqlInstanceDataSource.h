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
#include "InstanceDataSource.h"
#include "Database/SqlStatement.h"

namespace Poco { namespace Util { class AbstractConfiguration; }; };
class SqlInstanceDataSource : public SqlDataSource, public InstanceDataSource
{
public:

	SqlInstanceDataSource(Poco::Logger& logger, shared_ptr<Database> db, const Poco::Util::AbstractConfiguration* conf);
	~SqlInstanceDataSource() {}

	void populateInstances( int serverId, ServerInstancesQueue& queue ) override;
	void populatePlayerInstances( int serverId, ServerInstancesQueue& queue ) override;
	bool deleteInstance( int serverId, Int64 InstanceIdent, bool byUID ) override;
	bool deletePlayerInstance( int serverId, Int64 playerInstanceIdent, bool byUID ) override;
	bool createInstance( int serverId, const string& InstanceName ) override;
	bool createPlayerInstance( int InstanceId, int characterId ) override;
private:
	string _InstanceTableName;

	SqlStatementID _stmtCreateInstance;
	SqlStatementID _stmtCreatePlayerInstance;
	SqlStatementID _stmtDeletePlayerInstanceByUID;
	SqlStatementID _stmtDeletePlayerInstanceByID;
	SqlStatementID _stmtUpdateInstanceByID;
	SqlStatementID _stmtDeleteInstanceByUID;
	SqlStatementID _stmtDeleteInstanceByID;
};