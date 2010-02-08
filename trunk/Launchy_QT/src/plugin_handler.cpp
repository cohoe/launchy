/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "precompiled.h"
#include "plugin_handler.h"
#include "main.h"


PluginHandler::PluginHandler()
{
}


PluginHandler::~PluginHandler()
{
}


void PluginHandler::showLaunchy()
{
	foreach(PluginInfo info, plugins)
	{
		if (info.loaded)
			info.obj->msg(MSG_LAUNCHY_SHOW);
	}
}


void PluginHandler::hideLaunchy()
{
	foreach(PluginInfo info, plugins)
	{
		if (info.loaded)
			info.obj->msg(MSG_LAUNCHY_HIDE);
	}
}


void PluginHandler::getLabels(QList<InputData>* inputData) 
{
	if (inputData->count() > 0)
	{
		foreach(PluginInfo info, plugins)
		{
			if (info.loaded)
				info.obj->msg(MSG_GET_LABELS, (void*) inputData);
		}
	}
}


void PluginHandler::getResults(QList<InputData>* inputData, QList<CatItem>* results)
{
	if (inputData->count() > 0)
	{
		foreach(PluginInfo info, plugins)
		{
			if (info.loaded)
				info.obj->msg(MSG_GET_RESULTS, (void*) inputData, (void*) results);
		}
	}
}


void PluginHandler::getCatalogs(QList<CatItem>* items)
{
	foreach(PluginInfo info, plugins)
	{
		if (info.loaded)
			info.obj->msg(MSG_GET_CATALOG, (void*) items);
	}
}


int PluginHandler::execute(QList<InputData>* inputData, CatItem* result)
{
	if (!plugins.contains(result->id) || !plugins[result->id].loaded) 
		return 0;
	return plugins[result->id].obj->msg(MSG_LAUNCH_ITEM, (void*) inputData, (void*) result);
}


QWidget* PluginHandler::doDialog(QWidget* parent, uint id)
{
	if (!plugins.contains(id) || !plugins[id].loaded)
		return NULL;
	QWidget* newBox = NULL;
	plugins[id].obj->msg(MSG_DO_DIALOG, (void*) parent, (void*) &newBox);
	return newBox;
}


void PluginHandler::endDialog(uint id, bool accept)
{
	if (!plugins.contains(id) || !plugins[id].loaded)
		return;
	plugins[id].obj->msg(MSG_END_DIALOG, (void*) accept);
}


void PluginHandler::loadPlugins()
{
	// Get the list of loadable plugins
	QHash<uint,bool> loadable;

	int size = gSettings->beginReadArray("plugins");
	for(int i = 0; i < size; ++i)
	{
		gSettings->setArrayIndex(i);
		uint id = gSettings->value("id").toUInt();
		bool toLoad = gSettings->value("load").toBool();
		loadable[id] = toLoad;
	}
	gSettings->endArray();

	foreach(QString szDir, gMainWidget->dirs["plugins"])
	{
		// Load up the plugins in the plugins/ directory
		QDir pluginsDir(szDir);

		foreach (QString fileName, pluginsDir.entryList(QDir::Files))
		{
			if (!QLibrary::isLibrary(fileName)) continue;
			QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
			QObject *plugin = loader.instance();
			if (!plugin)
				continue;
			
			PluginInterface *plug = qobject_cast<PluginInterface *>(plugin);
			if (!plug)
				continue;
				
			plug->settings = &gSettings;
			PluginInfo info;
			uint id;
			bool handled = plug->msg(MSG_GET_ID, (void*) &id) != 0;
			info.id = id;
			QString name;
			plug->msg(MSG_GET_NAME, (void*) &name);
			info.name = name;
			info.obj = plug;
			info.path = pluginsDir.absoluteFilePath(fileName);

			if (handled && (!loadable.contains(id) || loadable[id]))
			{
				info.loaded = true;
				plug->msg(MSG_INIT);
				plug->msg(MSG_PATH, &szDir);

				// Load any of the plugin's plugins of its own
				QList<PluginInfo> additionalPlugins;
				plug->msg(MSG_LOAD_PLUGINS, &additionalPlugins);

				foreach(PluginInfo pluginInfo, additionalPlugins)
				{
					const bool isValidPlugin = 
						pluginInfo.obj && 
						!pluginInfo.name.isNull() &&
						pluginInfo.id > 0;
					if (!isValidPlugin)
					{
						continue;
					}

					const bool isPluginLoadable = 
						!loadable.contains(pluginInfo.id) || loadable[pluginInfo.id];

					if (isPluginLoadable)
					{
						pluginInfo.obj->msg(MSG_INIT);
						pluginInfo.loaded = true;
					}
					else
					{
						pluginInfo.obj->msg(MSG_UNLOAD_PLUGIN, (void*) pluginInfo.id);
						pluginInfo.loaded = false;
					}
					plugins[pluginInfo.id] = pluginInfo;
				}


			}
			else
			{
				info.loaded = false;
				loader.unload();
			}
			plugins[id] = info;
		}
	}
}
