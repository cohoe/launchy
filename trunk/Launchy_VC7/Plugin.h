#pragma once

#include <vector>
#include <map>
#include "FileRecord.h"

using namespace std;

typedef uint PluginID;


/*
	Plugins Can:
	. Add items to the main Launchy list which can optionally point to their plugin when executed or selected
	. Register ownership of searches that match regular expressions
	. Store data in the launchy db
	. Store configuration options
*/

/*
	The plugin API: Functions that the plugins can call

//	Launchy_SetTimer // So that plugins can have timed events
	Launchy_GetIcon // Get the icon for a file
	Launchy_GetSpecialPath // Path to special folder
	Launchy_Launch // Use Launchy to launch a file
	Launchy_Search(txt, List) // Tell Launchy to use its standard search algorithm on the list
*/



struct SearchResult {
	TCHAR* DisplayString; // Must be set
	TCHAR* FullPath; // Can be NULL
	TCHAR* Location; // Can be NULL
	HICON DisplayIcon; // Make sure you 0 this on creation!!
};

struct IndexItem {
	char* DisplayString;
	char* FullPath; // Can be NULL
};


typedef vector<SearchResult> SearchResults;
typedef vector<IndexItem> IndexItems;

typedef bool (* PLUGINOWNSSEARCH) (TCHAR*);
typedef IndexItems (* PLUGINGETINDEXITEMS) (void);
typedef SearchResults (* PLUGINUPDATESEARCH) (	int NumStrings, const TCHAR** Strings);
typedef void (* PLUGINDOACTION) (	int NumStrings, const TCHAR** Strings);
typedef void (* PLUGINADDINDEXITEMS) (IndexItems);
typedef  SearchResult* (* PLUGINGETIDENTIFIERS) (int*);
typedef void (* PLUGINFREERESULTS) ( SearchResult*, int);
struct PluginFunctions {
	PLUGINOWNSSEARCH PluginOwnsSearch;
	PLUGINGETINDEXITEMS PluginGetIndexItems;
	PLUGINUPDATESEARCH PluginUpdateSearch;
	PLUGINDOACTION PluginDoAction;
	PLUGINGETIDENTIFIERS PluginGetIdentifiers;
	PLUGINFREERESULTS PluginFreeResults;
};



class Plugin
{

private:
	vector<PluginFunctions> pfuncs;
	vector<HINSTANCE> loadedPlugins;
public:
	Plugin(void);
	~Plugin(void);

	void LoadDlls();
	vector<FileRecordPtr> Plugin::GetIdentifiers();
	/*

	LoadDlls(void);
	UnloadDlls(void);

	// Initialization Functions
	PluginID PluginOwnsSearch(CString txt); // Defaults to 0 (for launchy)
	IndexItems PluginGetIndexItems();
	
	// Runtime Functions
	SearchResults PluginDoSearch(PluginID, CString txt);
	void PluginDoAction(PluginID, uint searchID);

	// Indexing Functions
	IndexItems
	// Shutdown Functions
	*/
};

