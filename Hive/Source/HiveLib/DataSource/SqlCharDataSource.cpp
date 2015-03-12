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

#include "SqlCharDataSource.h"
#include "Database/Database.h"

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;
using boost::bad_lexical_cast;

SqlCharDataSource::SqlCharDataSource( Poco::Logger& logger, shared_ptr<Database> db, const string& idFieldName, const string& wsFieldName ) : SqlDataSource(logger,db)
{
	_idFieldName = getDB()->escape(idFieldName);
	_wsFieldName = getDB()->escape(wsFieldName);
}

SqlCharDataSource::~SqlCharDataSource() {}

Sqf::Value SqlCharDataSource::fetchCharacterInitial( string playerId, int serverId, const string& playerName )
{
	bool newPlayer = false;

	//make sure player exists in db
	{
		auto playerRes(getDB()->queryParams(("SELECT `PlayerName`, `PlayerSex` FROM `Player_DATA` WHERE `"+_idFieldName+"`='%s'").c_str(), getDB()->escape(playerId).c_str()));
		if (playerRes && playerRes->fetchRow())
		{
			newPlayer = false;
			//update player name if not current 
			if (playerRes->at(0).getString() != playerName)
			{
				auto stmt = getDB()->makeStatement(_stmtChangePlayerName, "UPDATE `Player_DATA` SET `PlayerName`=? WHERE `"+_idFieldName+"`=?");
				stmt->addString(playerName);
				stmt->addString(playerId);
				bool exRes = stmt->execute();
				poco_assert(exRes == true);
				_logger.information("Changed name of player " + playerId + " from '" + playerRes->at(0).getString() + "' to '" + playerName + "'");
			}
		}
		else
		{
			newPlayer = true;
			//insert new player into db
			auto stmt = getDB()->makeStatement(_stmtInsertPlayer, "INSERT INTO `Player_DATA` (`"+_idFieldName+"`, `PlayerName`) VALUES (?, ?)");
			stmt->addString(playerId);
			stmt->addString(playerName);
			bool exRes = stmt->execute();
			poco_assert(exRes == true);
			_logger.information("Created a new player " + playerId + " named '" + playerName + "'");
		}
	}

	//get characters from db
	auto charsRes = getDB()->queryParams(
		("SELECT `CharacterID`, `"+_wsFieldName+"`, `Inventory`, `Backpack`, "
		"TIMESTAMPDIFF(MINUTE,`Datestamp`,`LastLogin`) as `SurvivalTime`, "
		"TIMESTAMPDIFF(MINUTE,`LastAte`,NOW()) as `MinsLastAte`, "
		"TIMESTAMPDIFF(MINUTE,`LastDrank`,NOW()) as `MinsLastDrank`, "
		"`Model` FROM `Character_DATA` WHERE `"+_idFieldName+"` = '%s' AND `Alive` = 1 ORDER BY `CharacterID` DESC LIMIT 1").c_str(), getDB()->escape(playerId).c_str());

	bool newChar = false; //not a new char
	int characterId = -1; //invalid charid
	Sqf::Value worldSpace = Sqf::Parameters(); //empty worldspace
	Sqf::Value inventory = lexical_cast<Sqf::Value>("[]"); //empty inventory
	Sqf::Value backpack = lexical_cast<Sqf::Value>("[]"); //empty backpack
	Sqf::Value survival = lexical_cast<Sqf::Value>("[0,0,0]"); //0 mins alive, 0 mins since last ate, 0 mins since last drank
	string model = ""; //empty models will be defaulted by scripts
	if (charsRes && charsRes->fetchRow())
	{
		newChar = false;
		characterId = charsRes->at(0).getInt32();
		try
		{
			worldSpace = lexical_cast<Sqf::Value>(charsRes->at(1).getString());
		}
		catch(bad_lexical_cast)
		{
			_logger.warning("Invalid Worldspace for CharacterID("+lexical_cast<string>(characterId)+"): "+charsRes->at(1).getString());
		}
		if (!charsRes->at(2).isNull()) //inventory can be null
		{
			try
			{
				inventory = lexical_cast<Sqf::Value>(charsRes->at(2).getString());
				try { SanitiseInv(boost::get<Sqf::Parameters>(inventory)); } catch (const boost::bad_get&) {}
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid Inventory for CharacterID("+lexical_cast<string>(characterId)+"): "+charsRes->at(2).getString());
			}
		}		
		if (!charsRes->at(3).isNull()) //backpack can be null
		{
			try
			{
				backpack = lexical_cast<Sqf::Value>(charsRes->at(3).getString());
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid Backpack for CharacterID("+lexical_cast<string>(characterId)+"): "+charsRes->at(3).getString());
			}
		}
		//set survival info
		{
			Sqf::Parameters& survivalArr = boost::get<Sqf::Parameters>(survival);
			survivalArr[0] = charsRes->at(4).getInt32();
			survivalArr[1] = charsRes->at(5).getInt32();
			survivalArr[2] = charsRes->at(6).getInt32();
		}
		try
		{
			model = boost::get<string>(lexical_cast<Sqf::Value>(charsRes->at(7).getString()));
		}
		catch(...)
		{
			model = charsRes->at(7).getString();
		}

		//update last login
		{
			//update last character login
			auto stmt = getDB()->makeStatement(_stmtUpdateCharacterLastLogin, "UPDATE `Character_DATA` SET `LastLogin` = CURRENT_TIMESTAMP WHERE `CharacterID` = ?");
			stmt->addInt32(characterId);
			bool exRes = stmt->execute();
			poco_assert(exRes == true);
		}
	}
	else //inserting new character
	{
		newChar = true;

		int generation = 1;
		int humanity = 2500;
		//try getting previous character info
		{
			auto prevCharRes = getDB()->queryParams(
				("SELECT `Generation`, `Humanity`, `Model` FROM `Character_DATA` WHERE `" + _idFieldName + "` = '%s' AND `Alive` = 0 ORDER BY `CharacterID` DESC LIMIT 1").c_str(), getDB()->escape(playerId).c_str());
			if (prevCharRes && prevCharRes->fetchRow())
			{
				generation = prevCharRes->at(0).getInt32();
				generation++; //apparently this was the correct behaviour all along

				humanity = prevCharRes->at(1).getInt32();
				try
				{
					model = boost::get<string>(lexical_cast<Sqf::Value>(prevCharRes->at(2).getString()));
				}
				catch (...)
				{
					model = prevCharRes->at(2).getString();
				}
			}
		}

		{
			auto playerData(getDB()->queryParams(
				"SELECT cust_loadout.inventory,cust_loadout.backpack,cust_loadout.model FROM cust_loadout_profile INNER JOIN cust_loadout ON cust_loadout_profile.cust_loadout_id = cust_loadout.id where `unique_id` = '%s'", getDB()->escape(playerId).c_str()));
			if (playerData && playerData->fetchRow())
			{
				if (!playerData->at(0).isNull()) //inventory can be null
				{
					try
					{
						//inventory = playerData->at(0).getString();
						inventory = lexical_cast<Sqf::Value>(playerData->at(0).getString());
						try { SanitiseInv(boost::get<Sqf::Parameters>(inventory)); }
						catch (const boost::bad_get&) {}
					}
					catch (bad_lexical_cast)
					{
						_logger.warning("Invalid Custom Inventory for playerId(" + lexical_cast<string>(playerId)+"): " + lexical_cast<string>(inventory));
					}
				}

				if (!playerData->at(1).isNull()) //inventory can be null
				{
					try
					{
						backpack = lexical_cast<Sqf::Value>(playerData->at(1).getString());
					}
					catch (bad_lexical_cast)
					{
						_logger.warning("Invalid Custom Backpack for playerId(" + lexical_cast<string>(playerId)+"): " + lexical_cast<string>(backpack));
					}
				}
				
				if (!playerData->at(2).isNull()) //inventory can be null
				{
					model = playerData->at(2).getString();
					//model = boost::get<string>(lexical_cast<Sqf::Value>(playerData->at(2).getString()));
				}
				
				
			}

		}


		Sqf::Value medical = Sqf::Parameters(); //script will fill this in if empty
		//insert new char into db
		{
			auto stmt = getDB()->makeStatement(_stmtInsertNewCharacter, 
				"INSERT INTO `Character_DATA` (`"+_idFieldName+"`, `InstanceID`, `"+_wsFieldName+"`, `Inventory`, `Backpack`, `Medical`, `Generation`, `Datestamp`, `LastLogin`, `LastAte`, `LastDrank`, `Humanity`) "
				"VALUES (?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, ?)");
			stmt->addString(playerId);
			stmt->addInt32(serverId);
			stmt->addString(lexical_cast<string>(worldSpace));
			stmt->addString(lexical_cast<string>(inventory));
			stmt->addString(lexical_cast<string>(backpack));
			stmt->addString(lexical_cast<string>(medical));
			stmt->addInt32(generation);
			stmt->addInt32(humanity);
			bool exRes = stmt->directExecute(); //need sync as we will be getting the CharacterID right after this
			if (exRes == false)
			{
				_logger.error("Error creating character for playerId " + playerId);
				Sqf::Parameters retVal;
				retVal.push_back(string("ERROR"));
				return retVal;
			}
		}
		//get the new character's id
		{
			auto newCharRes = getDB()->queryParams(
				("SELECT `CharacterID` FROM `Character_DATA` WHERE `"+_idFieldName+"` = '%s' AND `Alive` = 1 ORDER BY `CharacterID` DESC LIMIT 1").c_str(), getDB()->escape(playerId).c_str());
			if (!newCharRes || !newCharRes->fetchRow())
			{
				_logger.error("Error fetching created character for playerId " + playerId);
				Sqf::Parameters retVal;
				retVal.push_back(string("ERROR"));
				return retVal;
			}
			characterId = newCharRes->at(0).getInt32();
		}
		_logger.information("Created a new character " + lexical_cast<string>(characterId) + " for player '" + playerName + "' (" + playerId + ")" );
	}

	Sqf::Parameters retVal; //current/original/new/
	retVal.push_back(string("PASS")); //0/0/0/
	retVal.push_back(newChar); //1/1/1/
	retVal.push_back(lexical_cast<string>(characterId));//2/2/2/
	if (!newChar)
	{
		retVal.push_back(worldSpace);//3/3/
		//retVal.push_back(inventory);//0/4/
		//retVal.push_back(backpack);//0/5/
		retVal.push_back(survival);//4/6/
	}
	retVal.push_back(inventory);//5/0/3/
	retVal.push_back(backpack);//6/0/4/
	retVal.push_back(model);//7/7/5/
	//hive interface version
	retVal.push_back(0.96f);//8/8/6/

	return retVal;
}

Sqf::Value SqlCharDataSource::fetchCharacterMedical( string playerId, int serverId )
{

	Sqf::Value medical = lexical_cast<Sqf::Value>("[]"); //empty inventory
	
	//get the characters medical
	{
		auto newCharRes = getDB()->queryParams(
			"SELECT character_data.Medical FROM character_data WHERE character_data.PlayerUID = '%s' AND character_data.Alive = 0 ORDER BY character_data.CharacterID DESC LIMIT 1", getDB()->escape(playerId).c_str());
		if (!newCharRes || !newCharRes->fetchRow())
		{
			_logger.error("Error fetching created character for playerId " + playerId);
			Sqf::Parameters retVal;
			retVal.push_back(string("ERROR"));
			return retVal;
		}
		medical = lexical_cast<Sqf::Value>(newCharRes->at(0).getString());
	}
	_logger.information("Pulled Medical information '" + lexical_cast<string>(medical) + "' for player (" + playerId + ")" );
	

	Sqf::Parameters retVal;
	retVal.push_back(string("PASS"));
	retVal.push_back(medical);

	return retVal;
}


Sqf::Value SqlCharDataSource::fetchCustomInventory( string playerId )
{

	Sqf::Value inventory = lexical_cast<Sqf::Value>("[]"); //empty inventory
	Sqf::Value backpack = lexical_cast<Sqf::Value>("[]"); //empty inventory
	string model = ""; //empty inventory
	
	//get the characters medical
	{
		auto CharRes = getDB()->queryParams(
			"select replace(cl.`inventory`, '""', '""""') inventory, replace(cl.`backpack`, '""', '""""') backpack, replace(coalesce(cl.`model`, 'Survivor2_DZ'), '""', '""""') model from `cust_loadout` cl join `cust_loadout_profile` clp on clp.`cust_loadout_id` = cl.`id` where clp.`unique_id` = '%s'", getDB()->escape(playerId).c_str());
		if (!CharRes || !CharRes->fetchRow())
		{
			_logger.error("Error fetching created character for playerId " + playerId);
			Sqf::Parameters retVal;
			retVal.push_back(string("ERROR"));
			return retVal;
		}

		inventory = lexical_cast<Sqf::Value>(CharRes->at(0).getString());
		backpack = lexical_cast<Sqf::Value>(CharRes->at(1).getString());
		model = CharRes->at(2).getString();
	}
	_logger.debug("Pulled custom inventory for player (" + playerId + ")");
	

	Sqf::Parameters retVal;
	retVal.push_back(string("PASS"));
	retVal.push_back(inventory);
	retVal.push_back(backpack);
	retVal.push_back(model);

	return retVal;
}

Sqf::Value SqlCharDataSource::fetchCharacterVariable( int characterID, string variableName )
{
	string testvariables;
	int _characterVariable = -1;

	//get the characters medical
	{
		string test = "SELECT character_variables.variable_value FROM character_variables WHERE character_variables.characterID = " + lexical_cast<string>(characterID) + " AND character_variables.variable_name = '" + variableName + "'";
		auto newCharRes(getDB()->queryParams(
			("SELECT character_variables.variable_value FROM character_variables WHERE character_variables.characterID = " + lexical_cast<string>(characterID) +" AND character_variables.variable_name = '%s'").c_str(), getDB()->escape(variableName).c_str()));

		if (newCharRes && newCharRes->fetchRow())
		{

				_characterVariable = newCharRes->at(0).getInt32();
				testvariables = lexical_cast<string>(newCharRes->at(0).getString());
		}
		else
		{
			_logger.error("Error fetching variable " + variableName + "  for characterID " + lexical_cast<string>(characterID));
			Sqf::Parameters retVal;
			retVal.push_back(string("ERROR"));
			return retVal;
		}
	}
	_logger.debug("Pulled variable information '" + lexical_cast<string>(variableName)+"' for player (" + lexical_cast<string>(characterID)+")");
	
	Sqf::Parameters retVal;
	retVal.push_back(string("PASS"));
	if (_characterVariable >= 0 )
	{
		retVal.push_back(_characterVariable);
	}
	else
	{
		retVal.push_back(testvariables);
	}
	

	return retVal;
}

Sqf::Value SqlCharDataSource::fetchCharacterVariableArray( int characterID )
{

	Sqf::Parameters variables;
	{
		auto newCharRes(getDB()->queryParams(
			"SELECT character_variables.variable_name FROM character_variables WHERE character_variables.characterID = %d", characterID));
		if (!newCharRes)
		{
			_logger.error("Error fetching variable data for characterID " + characterID);
			Sqf::Parameters retVal;
			retVal.push_back(string("ERROR"));
			return retVal;
		}
		while (newCharRes->fetchRow())
		{
			auto row = newCharRes->fields();
			
			string variable_name = row[0].getString();
			variables.push_back(lexical_cast<string>(variable_name));
		}
	}

	_logger.debug("Pulled variable array '" + lexical_cast<string>(variables)+"' for player (" + lexical_cast<string>(characterID)+")");
	

	Sqf::Parameters retVal;
	retVal.push_back(string("PASS"));
	retVal.push_back(variables);

	return retVal;
}

Sqf::Value SqlCharDataSource::fetchCharacterDetails( int characterId )
{
	Sqf::Parameters retVal;
	//get details from db
	auto charDetRes = getDB()->queryParams(
		"SELECT `%s`, `Medical`, `Generation`, `KillsZ`, `HeadshotsZ`, `KillsH`, `KillsB`, `CurrentState`, `Humanity` "
		"FROM `Character_DATA` WHERE `CharacterID`=%d", _wsFieldName.c_str(), characterId);

	if (charDetRes && charDetRes->fetchRow())
	{
		Sqf::Value worldSpace = Sqf::Parameters(); //empty worldspace
		Sqf::Value medical = Sqf::Parameters(); //script will fill this in if empty
		int generation = 1;
		Sqf::Value stats = lexical_cast<Sqf::Value>("[0,0,0,0]"); //killsZ, headZ, killsH, killsB
		Sqf::Value currentState = Sqf::Parameters(); //empty state (aiming, etc)
		int humanity = 2500;
		//get stuff from row
		{
			try
			{
				worldSpace = lexical_cast<Sqf::Value>(charDetRes->at(0).getString());
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid Worldspace (detail load) for CharacterID("+lexical_cast<string>(characterId)+"): "+charDetRes->at(0).getString());
			}
			try
			{
				medical = lexical_cast<Sqf::Value>(charDetRes->at(1).getString());
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid Medical (detail load) for CharacterID("+lexical_cast<string>(characterId)+"): "+charDetRes->at(1).getString());
			}
			generation = charDetRes->at(2).getInt32();
			//set stats
			{
				Sqf::Parameters& statsArr = boost::get<Sqf::Parameters>(stats);
				statsArr[0] = charDetRes->at(3).getInt32();
				statsArr[1] = charDetRes->at(4).getInt32();
				statsArr[2] = charDetRes->at(5).getInt32();
				statsArr[3] = charDetRes->at(6).getInt32();
			}
			try
			{
				currentState = lexical_cast<Sqf::Value>(charDetRes->at(7).getString());
			}
			catch(bad_lexical_cast)
			{
				_logger.warning("Invalid CurrentState (detail load) for CharacterID("+lexical_cast<string>(characterId)+"): "+charDetRes->at(7).getString());
			}
			humanity = charDetRes->at(8).getInt32();
		}

		retVal.push_back(string("PASS"));
		retVal.push_back(medical);
		retVal.push_back(stats);
		retVal.push_back(currentState);
		retVal.push_back(worldSpace);
		retVal.push_back(humanity);
	}
	else
	{
		retVal.push_back(string("ERROR"));
	}

	return retVal;
}

bool SqlCharDataSource::updateCharacter( int characterId, const FieldsType& fields )
{
	map<string,string> sqlFields;

	for (auto it=fields.begin();it!=fields.end();++it)
	{
		const string& name = it->first;
		const Sqf::Value& val = it->second;

		//arrays
		if (name == "Worldspace" || name == "Inventory" || name == "Backpack" || name == "Medical" || name == "CurrentState")
			sqlFields[name] = "'"+getDB()->escape(lexical_cast<string>(val))+"'";
		//booleans
		else if (name == "JustAte" || name == "JustDrank")
		{
			if (boost::get<bool>(val))
			{
				string newName = "LastAte";
				if (name == "JustDrank")
					newName = "LastDrank";

				sqlFields[newName] = "CURRENT_TIMESTAMP";
			}
		}
		//addition integeroids
		else if (name == "KillsZ" || name == "HeadshotsZ" || name == "DistanceFoot" || name == "Duration" ||
			name == "KillsH" || name == "KillsB" || name == "Humanity" )
		{
			int integeroid = static_cast<int>(Sqf::GetDouble(val));
			char intSign = '+';
			if (integeroid < 0)
			{
				intSign = '-';
				integeroid = abs(integeroid);
			}

			if (integeroid > 0) 
				sqlFields[name] = "(`"+name+"` "+intSign+" "+lexical_cast<string>(integeroid)+")";
		}
		//strings
		else if (name == "Model")
			sqlFields[name] = "'"+getDB()->escape(boost::get<string>(val))+"'";

	}

	if (sqlFields.size() > 0)
	{
		string query = "UPDATE `Character_DATA` SET ";
		for (auto it=sqlFields.begin();it!=sqlFields.end();)
		{
			string fieldName = it->first;
			if (fieldName == "Worldspace")
				fieldName = _wsFieldName;

			query += "`" + fieldName + "` = " + it->second;
			++it;
			if (it != sqlFields.end())
				query += " , ";
		}
		query += " WHERE `CharacterID` = " + lexical_cast<string>(characterId);
		bool exRes = getDB()->execute(query.c_str());
		poco_assert(exRes == true);

		return exRes;
	}

	return true;
}

/*
bool SqlCharDataSource::updateVariables( int characterId, string variableName, string variableValue )
{

	Sqf::Parameters retVal;
	int survivalArr;
	int testing;
	//get details from db
	{

		auto characterVariables(getDB()->queryParams("SELECT count(*) as count FROM character_variables WHERE character_variables.characterID = %d AND character_variables.variable_name = '%s'",characterId,getDB()->escape(variableName).c_str()));
		if (!characterVariables)
		{
				survivalArr = 0;
		}
		else do
		{
			try {
				auto row = characterVariables->fields();
				survivalArr = 1;
				survivalArr = characterVariables->at(0).getInt32();
			}
			catch (const bad_lexical_cast&)
			{
				_logger.error("Skipping variables for character " + lexical_cast<string>(characterId) + " invalid data in db");
			}
		} while(characterVariables && characterVariables->fetchRow()) ;

		bool exRes;
		if (survivalArr >= 1)
		{
			//update character_variables set variable_value = ? where variable_name = ? and character_variables.characterID = ?
			auto stmt = getDB()->makeStatement(_stmtUpdateCharacterVariables,"update character_variables set variable_value = ? where variable_name = ? and character_variables.characterID = ?");
			stmt->addString(lexical_cast<string>(variableValue));
			stmt->addString(lexical_cast<string>(variableName));
			stmt->addInt32(characterId);
			exRes = stmt->execute();
			poco_assert(exRes == true);
			_logger.information(" Updated variables for character " + characterId);
			return exRes;
		}
		else if (survivalArr == 0) 
		{
			//insert into character_variables set characterID = ?, variable_name = ?, variable_value = ?
			auto stmt = getDB()->makeStatement(_stmtInsertCharacterVariables,"insert into character_variables set characterID = ?, variable_name = ?, variable_value = ?");
			stmt->addInt32(characterId);
			stmt->addString(lexical_cast<string>(variableName));
			stmt->addString(lexical_cast<string>(variableValue));
			exRes = stmt->execute();
			poco_assert(exRes == true);
			_logger.information(" Inserted variables for character " + characterId);
			return exRes;
		}
	}
}*/


bool SqlCharDataSource::updateVariables( int characterId, string variableName, string variableValue )
{

	Sqf::Parameters retVal;
	bool exRes;
	//get details from db
	{

		auto characterVariables(getDB()->queryParams("SELECT character_variables.variable_value FROM character_variables WHERE character_variables.characterID = %d AND character_variables.variable_name = '%s'",characterId,getDB()->escape(variableName).c_str()));
		try {
			if (characterVariables && characterVariables->fetchRow())
			{
				//update character_variables set variable_value = ? where variable_name = ? and character_variables.characterID = ?
				auto stmt = getDB()->makeStatement(_stmtUpdateCharacterVariables,"update character_variables set variable_value = ? where variable_name = ? and character_variables.characterID = ?");
				stmt->addString(lexical_cast<string>(variableValue));
				stmt->addString(lexical_cast<string>(variableName));
				stmt->addInt32(characterId);
				exRes = stmt->execute();
				poco_assert(exRes == true);
				_logger.information(" Updated variables for character " + characterId);
				return exRes;
			}
			else
			{
				//insert into character_variables set characterID = ?, variable_name = ?, variable_value = ?
				auto stmt = getDB()->makeStatement(_stmtInsertCharacterVariables,"insert into character_variables set characterID = ?, variable_name = ?, variable_value = ?");
				stmt->addInt32(characterId);
				stmt->addString(lexical_cast<string>(variableName));
				stmt->addString(lexical_cast<string>(variableValue));
				exRes = stmt->execute();
				poco_assert(exRes == true);
				_logger.information(" Inserted variables for character " + characterId);
				return exRes;
			};
		}
		catch (const bad_lexical_cast&)
		{
			_logger.error("Skipping variables for character " + lexical_cast<string>(characterId) + " invalid data in db");
		}

	}
}


bool SqlCharDataSource::initCharacter( int characterId, const Sqf::Value& inventory, const Sqf::Value& backpack )
{
	auto stmt = getDB()->makeStatement(_stmtInitCharacter, "UPDATE `Character_DATA` SET `Inventory` = ? , `Backpack` = ? WHERE `CharacterID` = ?");
	stmt->addString(lexical_cast<string>(inventory));
	stmt->addString(lexical_cast<string>(backpack));
	stmt->addInt32(characterId);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlCharDataSource::killCharacter( int characterId, int duration )
{
	auto stmt = getDB()->makeStatement(_stmtKillCharacter, 
		"UPDATE `Character_DATA` SET `Alive` = 0, `LastLogin` = DATE_SUB(CURRENT_TIMESTAMP, INTERVAL ? MINUTE) WHERE `CharacterID` = ? AND `Alive` = 1");
	stmt->addInt32(duration);
	stmt->addInt32(characterId);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlCharDataSource::recordLogin( string playerId, int characterId, int action )
{	
	auto stmt = getDB()->makeStatement(_stmtRecordLogin, 
		"INSERT INTO `Player_LOGIN` (`"+_idFieldName+"`, `CharacterID`, `Datestamp`, `Action`) VALUES (?, ?, CURRENT_TIMESTAMP, ?)");
	stmt->addString(playerId);
	stmt->addInt32(characterId);
	stmt->addInt32(action);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

bool SqlCharDataSource::recordActivity(int serverID, string playerId, string action, string maplocation)
{
	auto stmt = getDB()->makeStatement(_stmtRecordLogin,
		"INSERT INTO `playeractivity` ( `serverid`, `playerid`, `datastamp`, `action`, `maplocation`) VALUES (?, ?, CURRENT_TIMESTAMP, ?, ?)");
	stmt->addInt32(serverID);
	stmt->addString(playerId);
	stmt->addString(action);
	stmt->addString(maplocation);
	bool exRes = stmt->execute();
	poco_assert(exRes == true);

	return exRes;
}

