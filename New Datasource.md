To add a new datasource

1. Copy blank datasource files and replace Blank with the name of the datasource in all files.
	a. SqlBlankDataSource.cpp -> Sql?????DataSource.cpp
	b. SqlBlankDataSource.h -> Sql????DataSource.h
	c. BlankDataSource.h -> ????DataSource.h
	d. edit each of the new files and replace the word Loot with the Name of the datasource you are creating (Blank -> Loot)	
2. Add datasource files to the HiveLib\Datasource folder inside of VS 2010
3. Edit DirectHiveApp.cpp and add in your new SQL datasource such as #include "HiveLib/DataSource/SqlMessagingDataSource.h"
4. Scroll down on the same file and add in a new DB source such as Poco::AutoPtr<Poco::Util::AbstractConfiguration> messagingDBConf(config().createView("MessagingDB"));
5. Again scroll and add in a definition for the db such as 
			_lootDb = _charDb;
			if (lootDBConf->getBool("Use",false))
			{
				Poco::Logger& lootDBLogger = Poco::Logger::get("lootDB");
				_lootDb = DatabaseLoader::Create(lootDBConf);
				if (!_lootDb->initialise(lootDBLogger,DatabaseLoader::MakeConnParams(lootDBConf)))
					return false;
			}
6. Add in your logging definer
	//loot Datasource
	{
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> lootConf(config().createView("Loot"));
		_lootData.reset(new SqllootDataSource(logger(),_lootDb,lootConf.get()));
	}
7. Edit HiveExtApp.h and add in your new data definer such as unique_ptr<MessagingDataSource> _msgData;
8. Edit DirectHiveApp.h and add in your new pointer to the end of the shared_ptr string
9. Edit HiveExtApp.h and add in your new include such as #include "DataSource/MessagingDataSource.h"
10. Edit HiveExtApp.cpp and add in your new handler such as 
11. Add in your SQF code and the rest is history