/*
Launchy: Application Launcher
Copyright (C) 2007  Josh Karlin

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

#include "gui.h"
#include "weby.h"


Gui::Gui(QWidget* parent) 
	: QWidget(parent)
{
	setupUi(this);
	QSettings* settings = *gWebyInstance->settings;
	if (settings == NULL) return;
	booksFirefox->setChecked(settings->value("weby/firefox", true).toBool());
	booksIE->setChecked(settings->value("weby/ie", true).toBool());
	
	// Read in the array of websites from options
	table->setSortingEnabled(false);
	int count = settings->beginReadArray("weby/sites");
	table->setRowCount(count);
	for(int i = 0; i < count; ++i) {
		settings->setArrayIndex(i);
		table->setItem(i, 0, new QTableWidgetItem(settings->value("name").toString()));
		table->setItem(i, 1, new QTableWidgetItem(settings->value("base").toString()));
		table->setItem(i, 2, new QTableWidgetItem(settings->value("query").toString()));
	}
	settings->endArray();
	table->setSortingEnabled(true);

	connect(tableNew, SIGNAL(clicked(bool)), this, SLOT(newRow(void)));
	connect(tableRemove, SIGNAL(clicked(bool)), this, SLOT(remRow(void)));
}

void Gui::writeOptions()
{
	QSettings* settings = *gWebyInstance->settings;
	if (settings == NULL) return;
	settings->setValue("weby/firefox", booksFirefox->isChecked());
	settings->setValue("weby/ie", booksIE->isChecked());

	settings->beginWriteArray("weby/sites");
	for(int i = 0; i < table->rowCount(); ++i) {
		settings->setArrayIndex(i);
		settings->setValue("name", table->item(i, 0)->text());
		settings->setValue("base", table->item(i, 1)->text());
		settings->setValue("query", table->item(i, 2)->text());
	}
	settings->endArray();
}

void Gui::newRow() 
{
	table->insertRow(table->rowCount());
	table->setCurrentCell(table->rowCount()-1, 0);
}

void Gui::remRow()
{
	if (table->currentRow() != -1)
		table->removeRow(table->currentRow());
}