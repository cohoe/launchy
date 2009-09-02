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

#include "options.h"
#include "main.h"
#include "globals.h"
#include "plugin_handler.h"
#include "FileBrowserDelegate.h"
#include <QSettings>
#include <QDir>
#include <QPixmap>
#include <QBitmap>
#include <QPainter>
#include <QFileDialog>
#include <QTextStream>


QByteArray OptionsDialog::windowGeometry;
int OptionsDialog::currentTab;
int OptionsDialog::currentPlugin;


OptionsDialog::OptionsDialog(QWidget * parent) :
	QDialog(parent),
	directoryItemDelegate(this, FileBrowser::Directory)
{
	setupUi(this);
	curPlugin = -1;
	LaunchyWidget* main = qobject_cast<LaunchyWidget*>(gMainWidget);
	if (main == NULL)
		return;

	restoreGeometry(windowGeometry);
	tabWidget->setCurrentIndex(currentTab);

	// Load General Options
	genAlwaysShow->setChecked(gSettings->value("GenOps/alwaysshow", false).toBool());
	genAlwaysTop->setChecked(gSettings->value("GenOps/alwaystop", false).toBool());
	genPortable->setChecked(gSettings->value("GenOps/isportable", false).toBool());
	genHideFocus->setChecked(gSettings->value("GenOps/hideiflostfocus", false).toBool());
	int center = gSettings->value("GenOps/alwayscenter", 3).toInt();
	genHCenter->setChecked((center & 1) != 0);
	genVCenter->setChecked((center & 2) != 0);
	//    genFastIndex->setChecked(gSettings->value("GenOps/fastindexer",false).toBool());
	genUpdateCheck->setChecked(gSettings->value("GenOps/updatecheck", true).toBool());
	genShowHidden->setChecked(gSettings->value("GenOps/showHiddenFiles", false).toBool());
	genCondensed->setChecked(gSettings->value("GenOps/condensedView",false).toBool());
	genAutoSuggestDelay->setText(gSettings->value("GenOps/autoSuggestDelay","1000").toString());
	genUpMinutes->setText(gSettings->value("GenOps/updatetimer", "10").toString());
	genMaxViewable->setText(gSettings->value("GenOps/numviewable", "4").toString());
	genNumResults->setText(gSettings->value("GenOps/numresults", "10").toString());
	genOpaqueness->setValue(gSettings->value("GenOps/opaqueness", 100).toInt());
	genFadeIn->setValue(gSettings->value("GenOps/fadein", 0).toInt());
	genFadeOut->setValue(gSettings->value("GenOps/fadeout", 20).toInt());
	connect(genOpaqueness, SIGNAL(sliderMoved(int)), main, SLOT(setOpaqueness(int)));
	metaKeys << tr("") << tr("Alt") << tr("Win") << tr("Shift") << tr("Control");
	iMetaKeys << Qt::NoModifier << Qt::AltModifier << Qt::MetaModifier << Qt::ShiftModifier << Qt::ControlModifier;

	actionKeys << tr("Space") << tr("Tab") << tr("Backspace") << tr("Enter") << tr("Esc") << tr("Home") << 
		tr("End") << tr("Pause") << tr("Print") << tr("Up") << tr("Down") << tr("Left") << tr("Right") << tr("F1") <<
		tr("F2") << tr("F3") << tr("F4") << tr("F5") << tr("F6") << tr("F7") << tr("F8") << tr("F9") << tr("F10") <<
		tr("F11") << tr("F12") << tr("F13") << tr("F14") << tr("F15") << tr("Caps Lock");

	for (int i = 'A'; i <= 'Z'; i++) 
		actionKeys << QString(QChar(i));

	iActionKeys << Qt::Key_Space << Qt::Key_Tab << Qt::Key_Backspace << Qt::Key_Enter << Qt::Key_Escape << Qt::Key_Home <<
		Qt::Key_End << Qt::Key_Pause << Qt::Key_Print << Qt::Key_Up << Qt::Key_Down << Qt::Key_Left << Qt::Key_Right << Qt::Key_F1 <<
		Qt::Key_F2 << Qt::Key_F3 << Qt::Key_F4 << Qt::Key_F5 << Qt::Key_F6 << Qt::Key_F7 << Qt::Key_F8 << Qt::Key_F9 << Qt::Key_F10 <<
		Qt::Key_F11 << Qt::Key_F12 << Qt::Key_F13 << Qt::Key_F14 << Qt::Key_F15 << Qt::Key_CapsLock;

	for (int i = 'A'; i <= 'Z'; i++) 
		iActionKeys << i;

	// Find the current hotkey
	QKeySequence keys = gSettings->value("Options/hotkey", QKeySequence(Qt::ControlModifier +  Qt::Key_Space)).value<QKeySequence>();
#ifdef Q_WS_WIN
	int curMeta = gSettings->value("GenOps/hotkeyModifier", Qt::AltModifier).toInt();
#endif
#ifdef Q_WS_X11
	int curMeta = gSettings->value("GenOps/hotkeyModifier", Qt::ControlModifier).toInt();
#endif
	int curAction = gSettings->value("GenOps/hotkeyAction", Qt::Key_Space).toInt();
	for (int i = 0; i < metaKeys.count(); ++i)
	{
		genModifierBox->addItem(metaKeys[i]);
		if (iMetaKeys[i] == curMeta) 
			genModifierBox->setCurrentIndex(i);
	}
	for (int i = 0; i < actionKeys.count(); ++i)
	{
		genKeyBox->addItem(actionKeys[i]);
		if (iActionKeys[i] == curAction) 
			genKeyBox->setCurrentIndex(i);
	}

	// Load up web proxy settings
	genProxyHostname->setText(gSettings->value("WebProxy/hostAddress").toString());
	genProxyPort->setText(gSettings->value("WebProxy/port").toString());

	// Load up the skins list
	QString skinName = gSettings->value("GenOps/skin", main->dirs["defSkin"][0]).toString();

	connect(skinList, SIGNAL(currentTextChanged(const QString)), this, SLOT(skinChanged(const QString)));

	int skinRow = 0;

	QHash<QString, bool> knownSkins;

	foreach(QString szDir, main->dirs["skins"])
	{
		QDir dir(szDir);
		QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

		foreach(QString d, dirs)
		{
			if (knownSkins.contains(d))
				continue;
			knownSkins[d] = true;

			QFile f(dir.absolutePath() + "/" + d + "/misc.txt");
			// Only look for 2.0+ skins
			if (!f.exists()) continue;

			QListWidgetItem * item = new QListWidgetItem(d);
			item->setData(Qt::UserRole, szDir);
			skinList->addItem(item);



			if (skinName.compare(szDir + "/" + d, Qt::CaseInsensitive) == 0)
				skinRow = skinList->count() - 1;
		}
	}

	skinList->setCurrentRow(skinRow);
	connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

	// Load the directories and types
	catDirectories->setItemDelegate(&directoryItemDelegate);

	connect(catDirectories, SIGNAL(currentRowChanged(int)), this, SLOT(dirRowChanged(int)));
	connect(catDirectories, SIGNAL(dragEnter(QDragEnterEvent*)), this, SLOT(catDirDragEnter(QDragEnterEvent*)));
	connect(catDirectories, SIGNAL(drop(QDropEvent*)), this, SLOT(catDirDrop(QDropEvent*)));
	connect(catDirectories, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(catDirItemChanged(QListWidgetItem*)));
	connect(catDirPlus, SIGNAL(clicked(bool)), this, SLOT(catDirPlusClicked(bool)));
	connect(catDirMinus, SIGNAL(clicked(bool)), this, SLOT(catDirMinusClicked(bool)));
	connect(catTypes, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(catTypesItemChanged(QListWidgetItem*)));
	connect(catTypesPlus, SIGNAL(clicked(bool)), this, SLOT(catTypesPlusClicked(bool)));
	connect(catTypesMinus, SIGNAL(clicked(bool)), this, SLOT(catTypesMinusClicked(bool)));
	connect(catCheckDirs, SIGNAL(stateChanged(int)), this, SLOT(catTypesDirChanged(int)));
	connect(catCheckBinaries, SIGNAL(stateChanged(int)), this, SLOT(catTypesExeChanged(int)));
	connect(catDepth, SIGNAL(valueChanged(int)),this, SLOT(catDepthChanged(int)));
	connect(catRescan, SIGNAL(clicked(bool)), this, SLOT(catRescanClicked(bool)));
	int size = gSettings->beginReadArray("directories");
	for (int i = 0; i < size; ++i)
	{
		gSettings->setArrayIndex(i);
		Directory tmp;
		tmp.name = gSettings->value("name").toString();
		tmp.types = gSettings->value("types").toStringList();
		tmp.indexDirs = gSettings->value("indexDirs", false).toBool();
		tmp.indexExe = gSettings->value("indexExes", false).toBool();
		tmp.depth = gSettings->value("depth", 100).toInt();
		memDirs.append(tmp);
	}
	gSettings->endArray();
	if (memDirs.count() == 0)
	{
		memDirs = main->platform->GetInitialDirs();
	}

	for (int i = 0; i < memDirs.count(); ++i)
	{
		catDirectories->addItem(memDirs[i].name);
		QListWidgetItem* it = catDirectories->item(i);
		it->setFlags(it->flags() | Qt::ItemIsEditable);
	}
	if (catDirectories->count() > 0)
		catDirectories->setCurrentRow(0);

	genOpaqueness->setRange(15,100);

	if (main->catalog != NULL)
	{
		QString txt = tr("Index has ");
		txt += QString::number(main->catalog->count());
		txt += tr(" items");
		catSize->setText(txt);
	}
	if (gBuilder != NULL)
	{
		connect(gBuilder.get(), SIGNAL(catalogIncrement(float)), this, SLOT(catProgressUpdated(float)));
		connect(gBuilder.get(), SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));
	}
	// Load up the plugins		
	connect(plugList, SIGNAL(currentRowChanged(int)), this, SLOT(pluginChanged(int)));
	connect(plugList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(pluginItemChanged(QListWidgetItem*)));
	main->plugins.loadPlugins();
	foreach(PluginInfo info, main->plugins.getPlugins())
	{
		plugList->addItem(info.name);
		QListWidgetItem* item = plugList->item(plugList->count()-1);
		item->setData(Qt::UserRole, info.id);
		item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		if (info.loaded)
			item->setCheckState(Qt::Checked);
		else
			item->setCheckState(Qt::Unchecked);
	}
	if (plugList->count() > 0)
	{
		plugList->setCurrentRow(currentPlugin);
	}
	aboutVer->setText(QString(tr("This is Launchy version ")) + QString(LAUNCHY_VERSION_STRING));
	catDirectories->installEventFilter(this);
}


OptionsDialog::~OptionsDialog()
{
	currentTab = tabWidget->currentIndex();
	windowGeometry = saveGeometry();
}


void OptionsDialog::accept()
{
	LaunchyWidget* main = qobject_cast<LaunchyWidget*>(gMainWidget);
	if (main == NULL || gSettings == NULL)
		return;

	// See if the new hotkey works, if not we're not leaving the dialog.
	bool newKey = main->setHotkey(iMetaKeys[genModifierBox->currentIndex()], iActionKeys[genKeyBox->currentIndex()]);

	if (!newKey)
	{
		QMessageBox::warning(this, tr("Launchy"), tr("The hotkey you have chosen is already in use, please select another."));
		return;
	}

	gSettings->setValue("GenOps/hotkeyModifier", iMetaKeys[genModifierBox->currentIndex()]);
	gSettings->setValue("GenOps/hotkeyAction", iActionKeys[genKeyBox->currentIndex()]);

	// Save General Options
	gSettings->setValue("GenOps/alwaysshow", genAlwaysShow->isChecked());
	gSettings->setValue("GenOps/alwaystop", genAlwaysTop->isChecked());
	gSettings->setValue("GenOps/isportable", genPortable->isChecked());
	gSettings->setValue("GenOps/updatecheck", genUpdateCheck->isChecked());
	gSettings->setValue("GenOps/hideiflostfocus", genHideFocus->isChecked());
	gSettings->setValue("GenOps/alwayscenter", (genHCenter->isChecked() ? 1 : 0) | (genVCenter->isChecked() ? 2 : 0));
	gSettings->setValue("GenOps/showHiddenFiles", genShowHidden->isChecked());
	gSettings->setValue("GenOps/condensedView", genCondensed->isChecked());
	gSettings->setValue("GenOps/autoSuggestDelay", genAutoSuggestDelay->text());
	gSettings->setValue("GenOps/updatetimer", genUpMinutes->text());
	gSettings->setValue("GenOps/numviewable", genMaxViewable->text());
	gSettings->setValue("GenOps/numresults", genNumResults->text());
	gSettings->setValue("GenOps/opaqueness", genOpaqueness->value());
	gSettings->setValue("GenOps/fadein", genFadeIn->value());
	gSettings->setValue("GenOps/fadeout", genFadeOut->value());

	gSettings->setValue("WebProxy/hostAddress", genProxyHostname->text());
	gSettings->setValue("WebProxy/port", genProxyPort->text());

	// Apply General Options	
	main->setAlwaysShow(genAlwaysShow->isChecked());
	main->setAlwaysTop(genAlwaysTop->isChecked());
	main->setPortable(genPortable->isChecked());
	main->setCondensed(genCondensed->isChecked());
	main->setOpaqueness(genOpaqueness->value());
	main->loadOptions();

	// Apply Skin Options
	QString prevSkinName = gSettings->value("GenOps/skin", main->dirs["defSkin"][0]).toString();

	if (skinList->currentRow() >= 0 && skinList->currentItem()->text() != prevSkinName)
	{
		QString path = skinList->currentItem()->data(Qt::UserRole).toString();
		gSettings->setValue("GenOps/skin", path + "/" + skinList->currentItem()->text());
		main->setSkin(path, skinList->currentItem()->text());
	}

	// Apply Directory Options
	gSettings->beginWriteArray("directories");
	for (int i = 0; i < memDirs.count(); ++i)
	{
		gSettings->setArrayIndex(i);
		gSettings->setValue("name", memDirs[i].name);
		gSettings->setValue("types", memDirs[i].types);
		gSettings->setValue("indexDirs", memDirs[i].indexDirs);
		gSettings->setValue("indexExes", memDirs[i].indexExe);
		gSettings->setValue("depth", memDirs[i].depth);
	}

	gSettings->endArray();

	// Compare the checks to the plugins
	QHash<uint, PluginInfo> plugins = main->plugins.getPlugins();
	bool changed = false;

	if (changed)
	{
		main->plugins.loadPlugins();
	}

	if (curPlugin >= 0)
	{
		QListWidgetItem* item = plugList->item(curPlugin);
		main->plugins.endDialog(item->data(Qt::UserRole).toUInt(), true);
	}

	gSettings->sync();

	QDialog::accept();
}


void OptionsDialog::tabChanged(int tab)
{
	tab = tab; // Compiler warning
	// Redraw the current skin
	// (necessary because of dialog resizing issues)
	if (tabWidget->currentWidget()->objectName() == "Skins")
	{
		int row = skinList->currentRow();
		skinList->setCurrentRow(-1);
		skinList->setCurrentRow(row);
	}
}


void OptionsDialog::pluginChanged(int row)
{
	LaunchyWidget* main = qobject_cast<LaunchyWidget*>(gMainWidget);
	if (main == NULL)
		return;

	if (plugBox->layout() != NULL)
		for (int i = 1; i < plugBox->layout()->count(); i++) 
			plugBox->layout()->removeItem(plugBox->layout()->itemAt(i));

	// Close any current plugin dialogs
	if (curPlugin >= 0)
	{
		QListWidgetItem* item = plugList->item(curPlugin);
		main->plugins.endDialog(item->data(Qt::UserRole).toUInt(), true);
	}

	// Open the new plugin dialog
	curPlugin = row;
	currentPlugin = row;
	if (row < 0)
		return;	
	QListWidgetItem* item = plugList->item(row);
	QWidget* win = main->plugins.doDialog(plugBox, item->data(Qt::UserRole).toUInt());

	if (win != NULL)
	{
		if (plugBox->layout() != NULL)
			plugBox->layout()->addWidget(win);
		win->show();
	}
}


void OptionsDialog::pluginItemChanged(QListWidgetItem* iz)
{

	LaunchyWidget* main = qobject_cast<LaunchyWidget*>(gMainWidget);
	if (main == NULL)
		return;
	int row = plugList->currentRow();
	if (row == -1)
		return;

	// Close any current plugin dialogs
	if (curPlugin >= 0)
	{
		QListWidgetItem* item = plugList->item(curPlugin);
		main->plugins.endDialog(item->data(Qt::UserRole).toUInt(), true);
	}

	// Write out the new config
	gSettings->beginWriteArray("plugins");
	for (int i = 0; i < plugList->count(); i++)
	{
		QListWidgetItem* item = plugList->item(i);
		gSettings->setArrayIndex(i);
		gSettings->setValue("id", item->data(Qt::UserRole).toUInt());
		if (item->checkState() == Qt::Checked)
		{
			gSettings->setValue("load", true);
		}
		else
		{
			gSettings->setValue("load", false);
		}
	}
	gSettings->endArray();

	// Reload the plugins
	main->plugins.loadPlugins();

	// If enabled, reload the dialog
	if (iz->checkState() == Qt::Checked)
	{
		main->plugins.doDialog(plugBox, iz->data(Qt::UserRole).toUInt());
	}
}


void OptionsDialog::catProgressUpdated(float val)
{
	catProgress->setValue((int) val);
}


void OptionsDialog::catalogBuilt()
{
	LaunchyWidget* main = qobject_cast<LaunchyWidget*>(gMainWidget);
	if (main->catalog != NULL)
		catSize->setText(tr("Index has ") + QString::number(main->catalog->count()) + tr(" items"));
}


void OptionsDialog::catRescanClicked(bool val)
{
	val = val; // Compiler warning
	LaunchyWidget* main = qobject_cast<LaunchyWidget*>(gMainWidget);
	if (main == NULL)
		return;

	// Apply Directory Options
	gSettings->beginWriteArray("directories");
	for (int i = 0; i < memDirs.count(); ++i)
	{
		gSettings->setArrayIndex(i);
		gSettings->setValue("name", memDirs[i].name);
		gSettings->setValue("types", memDirs[i].types);
		gSettings->setValue("indexDirs", memDirs[i].indexDirs);
		gSettings->setValue("indexExes", memDirs[i].indexExe);
		gSettings->setValue("depth", memDirs[i].depth);
	}

	gSettings->endArray();

	if (gBuilder == NULL)
	{
		gBuilder.reset(new CatalogBuilder(false, &main->plugins));
		gBuilder->setPreviousCatalog(main->catalog);
		connect(gBuilder.get(), SIGNAL(catalogFinished()), main, SLOT(catalogBuilt()));
		connect(gBuilder.get(), SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));
		connect(gBuilder.get(), SIGNAL(catalogIncrement(float)), this, SLOT(catProgressUpdated(float)));
		gBuilder->start(QThread::IdlePriority);
	}
}


void OptionsDialog::catTypesDirChanged(int state)
{
	state = state; // Compiler warning
	int row = catDirectories->currentRow();
	if (row == -1)
		return;
	memDirs[row].indexDirs = catCheckDirs->isChecked();
}


void OptionsDialog::catTypesExeChanged(int state)
{
	state = state; // Compiler warning
	int row = catDirectories->currentRow();
	if (row == -1)
		return;
	memDirs[row].indexExe = catCheckBinaries->isChecked();
}


void OptionsDialog::catDirItemChanged(QListWidgetItem* item)
{
	int row = catDirectories->currentRow();
	if (row == -1)
		return;
	if (item != catDirectories->item(row))
		return;	

	memDirs[row].name = item->text();
}


void OptionsDialog::catDirDragEnter(QDragEnterEvent *event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData && mimeData->hasUrls())
		event->acceptProposedAction();
}


void OptionsDialog::catDirDrop(QDropEvent *event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData && mimeData->hasUrls())
	{
		foreach(QUrl url, mimeData->urls())
		{
			QFileInfo info(url.toLocalFile());
			if(info.exists() && info.isDir())
			{
				addDirectory(info.filePath());
			}
		}
	}
}


void OptionsDialog::dirRowChanged(int row)
{
	if (row == -1)
		return;

	catTypes->clear();
	foreach(QString str, memDirs[row].types)
	{
		QListWidgetItem* item = new QListWidgetItem(str, catTypes);
		item->setFlags(item->flags() | Qt::ItemIsEditable);
	}
	catCheckDirs->setChecked(memDirs[row].indexDirs);
	catCheckBinaries->setChecked(memDirs[row].indexExe);
	catDepth->setValue(memDirs[row].depth);
}


void OptionsDialog::catDirMinusClicked(bool c)
{
	c = c; // Compiler warning
	int dirRow = catDirectories->currentRow();

	delete catDirectories->takeItem(dirRow);
	catTypes->clear();

	memDirs.removeAt(dirRow);

	if (dirRow >= catDirectories->count() && catDirectories->count() > 0)
		catDirectories->setCurrentRow(catDirectories->count() - 1);
}


void OptionsDialog::catDirPlusClicked(bool c)
{
	c = c; // Compiler warning
	QString dir = QFileDialog::getExistingDirectory(this, tr("Select a directory"),
		lastDir,
		QFileDialog::ShowDirsOnly );
	if (dir == "")
		return;
	lastDir = dir;

	addDirectory(dir);
}


void OptionsDialog::addDirectory(const QString& directory)
{
	QString nativeDir = QDir::toNativeSeparators(directory);
	Directory dir(nativeDir);
	memDirs.append(dir);

	catTypes->clear();
	QListWidgetItem* item = new QListWidgetItem(nativeDir, catDirectories);
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	catDirectories->setCurrentItem(item);
}


void OptionsDialog::catTypesItemChanged(QListWidgetItem* item)
{
	int row = catDirectories->currentRow();
	if (row == -1)
		return;
	int typesRow = catTypes->currentRow();
	if (typesRow == -1)
		return;

	memDirs[row].types[typesRow] = item->text();
}


void OptionsDialog::catTypesPlusClicked(bool c)
{
	c = c; // Compiler warning
	int row = catDirectories->currentRow();
	if (row == -1)
		return;

	memDirs[row].types << "";
	QListWidgetItem* item = new QListWidgetItem(catTypes);
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	catTypes->setCurrentItem(item);
	catTypes->editItem(item);
}


void OptionsDialog::catTypesMinusClicked(bool c)
{
	c = c; // Compiler warning
	int dirRow = catDirectories->currentRow();
	if (dirRow == -1)
		return;

	int typesRow = catTypes->currentRow();
	if (typesRow == -1)
		return;

	memDirs[dirRow].types.removeAt(typesRow);
	delete catTypes->takeItem(typesRow);

	if (typesRow >= catTypes->count() && catTypes->count() > 0)
		catTypes->setCurrentRow(catTypes->count() - 1);
}


void OptionsDialog::catDepthChanged(int d)
{
	int row = catDirectories->currentRow();
	if (row != -1)
		memDirs[row].depth = d;
}


void OptionsDialog::skinChanged(const QString newSkin)
{
	LaunchyWidget* main = qobject_cast<LaunchyWidget*>(gMainWidget);
	if (main == NULL)
		return;

	if (skinList->currentRow() == -1)
		return;

	QString path = skinList->currentItem()->data(Qt::UserRole).toString();  

	// Load up the author file
	QFile author(path + "/" + newSkin + "/author.txt"); 
	if (!author.open(QIODevice::ReadOnly))
	{
		authorInfo->setText("");
		skinPreview->clear();
		return;
	}

	QTextStream in(&author);

	QString line, total;
	line = in.readLine();
	while (!line.isNull())
	{
		total += line + "\n";
		line = in.readLine();
	}
	authorInfo->setText(total);
	author.close();

	// Set the image preview
	QPixmap pix;
	QString sDir(path + "/" + newSkin + "/");

	if (pix.load(sDir + "background.png"))
	{
		if (!main->platform->SupportsAlphaBorder() && QFile::exists(sDir + "background_nc.png"))
			pix.load(sDir + "background_nc.png");
		if (pix.hasAlpha())
			pix.setMask(pix.mask());
		if (!main->platform->SupportsAlphaBorder() && QFile::exists(sDir + "mask_nc.png"))
			pix.setMask(QPixmap(sDir + "mask_nc.png"));
		else if (QFile::exists(sDir + "mask.png"))
			pix.setMask(QPixmap(sDir + "mask.png"));

		if (main->platform->SupportsAlphaBorder())
		{
			// Compose the alpha image with the background
			QImage sourceImage(pix.toImage());
			QImage destinationImage(sDir + "alpha.png");
			QImage resultImage(destinationImage.size(), QImage::Format_ARGB32_Premultiplied);

			QPainter painter(&resultImage);
			painter.setCompositionMode(QPainter::CompositionMode_Source);
			painter.fillRect(resultImage.rect(), Qt::transparent);
			painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
			painter.drawImage(0, 0, sourceImage);
			painter.drawImage(0, 0, destinationImage);

			painter.end();

			pix = QPixmap::fromImage(resultImage);
		}

		QPixmap scaled;
		scaled = pix.scaled(skinPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		skinPreview->setPixmap(scaled);
	}
	else
	{
		skinPreview->clear();
	}
}


void OptionsDialog::reject()
{
	LaunchyWidget* main = qobject_cast<LaunchyWidget*>(gMainWidget);
	if (main == NULL)
		return;

	if (curPlugin >= 0)
	{
		QListWidgetItem* item = plugList->item(curPlugin);
		main->plugins.endDialog(item->data(Qt::UserRole).toUInt(), false);
	}

	QDialog::reject();
}
