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

#ifndef CATALOG_BUILDER
#define CATALOG_BUILDER


#include <QThread>
#include <boost/shared_ptr.hpp>
#include "catalog_types.h"
#include "plugin_handler.h"


using namespace boost;


class CatalogBuilder : public QThread
{
	Q_OBJECT

public:
	CatalogBuilder(PluginHandler* plugs);
	CatalogBuilder(PluginHandler* plugs, shared_ptr<Catalog> catalog);
	static Catalog* createCatalog();

	shared_ptr<Catalog> getCatalog()
	{
		return catalog;
	}
	void run();

signals:
	void catalogFinished();
	void catalogIncrement(float);

private:
	void buildCatalog();
	void indexDirectory(const QString& dir, const QStringList& filters, bool fdirs, bool fbin, int depth);

	PluginHandler* plugins;
	shared_ptr<Catalog> currentCatalog;
	shared_ptr<Catalog> catalog;
	QHash<QString, bool> indexed;
};


#endif
