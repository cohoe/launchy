/*
Launchy: Application Launcher
Copyright (C) 2007-2010  Josh Karlin, Simon Capewell

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

#ifdef Q_WS_MAC
#include <QMacStyle>
#endif
#include "icon_delegate.h"
#include "main.h"
#include "globals.h"
#include "options.h"
#include "plugin_interface.h"
#include "FileSearch.h"


#ifdef Q_WS_WIN
void SetForegroundWindowEx(HWND hWnd)
{
        // Attach foreground window thread to our thread
	const DWORD foreGroundID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
	const DWORD currentID = GetCurrentThreadId();

	AttachThreadInput(foreGroundID, currentID, TRUE);
	// Do our stuff here 
	HWND lastActivePopupWnd = GetLastActivePopup(hWnd);
	SetForegroundWindow(lastActivePopupWnd);

	// Detach the attached thread
	AttachThreadInput(foreGroundID, currentID, FALSE);
}
#endif


LaunchyWidget::LaunchyWidget(CommandFlags command) :
#ifdef Q_WS_WIN
	QWidget(NULL, Qt::FramelessWindowHint | Qt::Tool),
#endif
#ifdef Q_WS_X11
        QWidget(NULL, Qt::FramelessWindowHint | Qt::Tool),
#endif
#ifdef Q_WS_MAC
        QWidget(NULL, Qt::FramelessWindowHint),
#endif
     
	frameGraphic(NULL),
	trayIcon(NULL),
	alternatives(NULL),
	updateTimer(NULL),
	dropTimer(NULL),
	condensedTempIcon(NULL)
{
	setObjectName("launchy");
	setWindowTitle(tr("Launchy"));
#ifdef Q_WS_WIN
	setWindowIcon(QIcon(":/resources/launchy128.png"));
#endif
#ifdef Q_WS_MAC
        setWindowIcon(QIcon("../Resources/launchy_icon_mac.icns"));
        //setAttribute(Qt::WA_MacAlwaysShowToolWindow);
#endif

	setAttribute(Qt::WA_AlwaysShowToolTips);
	setAttribute(Qt::WA_InputMethodEnabled);
	if (platform->supportsAlphaBorder())
	{
		setAttribute(Qt::WA_TranslucentBackground);
	}
	setFocusPolicy(Qt::ClickFocus);

	createActions();

	gMainWidget = this;
	menuOpen = false;
	optionsOpen = false;
	dragging = false;
	gSearchText = "";

	alwaysShowLaunchy = false;

	connect(&iconExtractor, SIGNAL(iconExtracted(int, QIcon)), this, SLOT(iconExtracted(int, QIcon)));

	fader = new Fader(this);
	connect(fader, SIGNAL(fadeLevel(double)), this, SLOT(setFadeLevel(double)));

	optionsButton = new QPushButton(this);
	optionsButton->setObjectName("opsButton");
	optionsButton->setToolTip(tr("Launchy Options"));
	optionsButton->setGeometry(QRect());
	connect(optionsButton, SIGNAL(clicked()), this, SLOT(showOptionsDialog()));

	closeButton = new QPushButton(this);
	closeButton->setObjectName("closeButton");
	closeButton->setToolTip(tr("Close Launchy"));
	closeButton->setGeometry(QRect());
	connect(closeButton, SIGNAL(clicked()), qApp, SLOT(quit()));

	output = new QLabel(this);
	output->setObjectName("output");
	output->setAlignment(Qt::AlignHCenter);

	input = new CharLineEdit(this);
#ifdef Q_WS_MAC
        QMacStyle::setFocusRectPolicy(input, QMacStyle::FocusDisabled);
#endif
	input->setObjectName("input");
	connect(input, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(inputKeyPressEvent(QKeyEvent*)));
	connect(input, SIGNAL(focusIn(QFocusEvent*)), this, SLOT(focusInEvent(QFocusEvent*)));
	connect(input, SIGNAL(focusOut(QFocusEvent*)), this, SLOT(focusOutEvent(QFocusEvent*)));
	connect(input, SIGNAL(inputMethod(QInputMethodEvent*)), this, SLOT(inputMethodEvent(QInputMethodEvent*)));

	outputIcon = new QLabel(this);
	outputIcon->setObjectName("outputIcon");

	workingAnimation = new AnimationLabel(this);
	workingAnimation->setObjectName("workingAnimation");
	workingAnimation->setGeometry(QRect());

	dirs = platform->getDirectories();

	// Load settings
	if (QFile::exists(dirs["portConfig"][0]))
		gSettings = new QSettings(dirs["portConfig"][0], QSettings::IniFormat, this);
	else
		gSettings = new QSettings(dirs["config"][0], QSettings::IniFormat, this);

	// If this is the first time running or a new version, call updateVersion
	if (gSettings->value("version", 0).toInt() != LAUNCHY_VERSION)
	{
		updateVersion(gSettings->value("version", 0).toInt());
		command |= ShowLaunchy;
	}

	alternatives = new CharListWidget(this);
	alternatives->setObjectName("alternatives");
	alternatives->setWindowFlags(Qt::Window | Qt::Tool | Qt::FramelessWindowHint);
	alternatives->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	alternatives->setTextElideMode(Qt::ElideLeft);
	alternatives->setUniformItemSizes(true);
	listDelegate = new IconDelegate(this);
	defaultListDelegate = alternatives->itemDelegate();
	setSuggestionListMode(gSettings->value("GenOps/condensedView", 0).toInt());
	altScroll = alternatives->verticalScrollBar();
	altScroll->setObjectName("altScroll");
	connect(alternatives, SIGNAL(currentRowChanged(int)), this, SLOT(alternativesRowChanged(int)));
	connect(alternatives, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(alternativesKeyPressEvent(QKeyEvent*)));
	connect(alternatives, SIGNAL(focusOut(QFocusEvent*)), this, SLOT(focusOutEvent(QFocusEvent*)));

	alternativesPath = new QLabel(alternatives);
	alternativesPath->setObjectName("alternativesPath");
	alternativesPath->hide();
	listDelegate->setAlternativesPathWidget(alternativesPath);

	// Load the plugins
	plugins.loadPlugins();

	// Set the general options
	if (setAlwaysShow(gSettings->value("GenOps/alwaysshow", false).toBool()))
		command |= ShowLaunchy;
	setAlwaysTop(gSettings->value("GenOps/alwaystop", false).toBool());
	setPortable(gSettings->value("GenOps/isportable", false).toBool());

	// Check for udpates?
	if (gSettings->value("GenOps/updatecheck", true).toBool())
	{
		checkForUpdate();
	}

	// Set the hotkey
	QKeySequence hotkey = getHotkey();
	if (!setHotkey(hotkey))
	{
		QMessageBox::warning(this, tr("Launchy"), tr("The hotkey %1 is already in use, please select another.").arg(hotkey.toString()));
		command = ShowLaunchy | ShowOptions;
	}

        sc_options = new QShortcut(QKeySequence(tr("Ctrl+,")), this);
        sc_options->setContext( Qt::ApplicationShortcut );
        connect( sc_options, SIGNAL( activated() ), this, SLOT( showOptionsDialog() ) );
        //showOptionsDialog

	// Set the timers
	updateTimer = new QTimer(this);
	dropTimer = new QTimer(this);
	dropTimer->setSingleShot(true);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateTimeout()));
	connect(dropTimer, SIGNAL(timeout()), this, SLOT(dropTimeout()));
	int time = gSettings->value("GenOps/updatetimer", 10).toInt();
	if (time > 0)
		updateTimer->start(time * 60000);

	// Load the catalog
	catalog.reset(CatalogBuilder::createCatalog());
	catalog->load(dirs["db"][0]);

	// Load the history
	history.load(dirs["history"][0]);

	// Load the skin
	applySkin(gSettings->value("GenOps/skin", "Default").toString());

	// Move to saved position
	loadPosition(gSettings->value("Display/pos", QPoint(0,0)).toPoint());
	loadOptions();

	executeStartupCommand(command);
}


LaunchyWidget::~LaunchyWidget()
{
	delete updateTimer;
	delete dropTimer;
	delete alternatives;
}


void LaunchyWidget::executeStartupCommand(int command)
{
	if (command & ResetPosition)
	{
		QRect r = geometry();
		int primary = qApp->desktop()->primaryScreen();
		QRect scr = qApp->desktop()->availableGeometry(primary);

		QPoint pt(scr.width()/2 - r.width()/2, scr.height()/2 - r.height()/2);
		move(pt);
	}

	if (command & ResetSkin)
	{
		setOpaqueness(100);
		showTrayIcon();
		applySkin("Default");
	}

	if (command & ShowLaunchy)
		showLaunchy();

	if (command & ShowOptions)
		showOptionsDialog();

	if (command & Rescan)
		buildCatalog();

	if (command & Exit)
		close();
}


void LaunchyWidget::paintEvent(QPaintEvent* event)
{
	// Do the default draw first to render any background specified in the stylesheet
	QStyleOption styleOption;
	styleOption.init(this);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
        style()->drawPrimitive(QStyle::PE_Widget, &styleOption, &painter, this);

	// Now draw the standard frame.png graphic if there is one
	if (frameGraphic)
	{
		painter.drawPixmap(0,0, *frameGraphic);
	}

	QWidget::paintEvent(event);
}


void LaunchyWidget::setSuggestionListMode(int mode)
{
	if (mode)
	{
		// The condensed mode needs an icon placeholder or it repositions text when the icon becomes available
		if (!condensedTempIcon)
		{
			QPixmap pixmap(16, 16);
			pixmap.fill(Qt::transparent);
			condensedTempIcon = new QIcon(pixmap);
		}
		alternatives->setItemDelegate(defaultListDelegate);
	}
	else
	{
		delete condensedTempIcon;
		condensedTempIcon = NULL;
		alternatives->setItemDelegate(listDelegate);
	}
}


bool LaunchyWidget::setHotkey(QKeySequence hotkey)
{
	return platform->setHotkey(hotkey, this, SLOT(onHotkey()));
}


void LaunchyWidget::showTrayIcon()
{
	if (!QSystemTrayIcon::isSystemTrayAvailable())
		return;

	if (gSettings->value("GenOps/showtrayicon", true).toBool())
	{
		if (!trayIcon)
			trayIcon = new QSystemTrayIcon(this);
		QKeySequence hotkey = platform->getHotkey();
		trayIcon->setToolTip(tr("Launchy (press %1 to activate)").arg(hotkey.toString()));
		trayIcon->setIcon(QIcon(":/resources/launchy16.png"));
		trayIcon->show();
		connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
			this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

		QMenu* trayMenu = new QMenu(this);
		trayMenu->addAction(actShow);
		trayMenu->addAction(actRebuild);
		trayMenu->addAction(actOptions);
		trayMenu->addSeparator();
		trayMenu->addAction(actExit);

		trayIcon->setContextMenu(trayMenu);
	}
	else if (trayIcon)
	{
		delete trayIcon;
		trayIcon = NULL;
	}
}


void LaunchyWidget::showAlternatives(bool show)
{
	dropTimer->stop();

	// If main launchy window is not visible, do nothing
	if (!isVisible())
		return;

	if (show)
	{
		if (searchResults.size() < 1)
			return;

		int mode = gSettings->value("GenOps/condensedView", 0).toInt();
		int i = 0;
		for (; i < searchResults.size(); ++i)
		{
			QString fullPath = QDir::toNativeSeparators(searchResults[i].fullPath);
			
			QListWidgetItem* item;
			if (i < alternatives->count())
			{
				item = alternatives->item(i);
			}
			else
			{
				item = new QListWidgetItem(fullPath, alternatives);
			}
			if (item->data(mode == 1 ? ROLE_SHORT : ROLE_FULL) != fullPath)
			{
				// condensedTempIcon is a blank icon or null
				item->setData(ROLE_ICON, *condensedTempIcon);
			}
			item->setData(mode == 1 ? ROLE_FULL : ROLE_SHORT, searchResults[i].shortName);
			item->setData(mode == 1 ? ROLE_SHORT : ROLE_FULL, fullPath);
			if (i >= alternatives->count())
				alternatives->addItem(item);
		}
		while (alternatives->count() > i)
		{
			delete alternatives->takeItem(i);
		}
		alternatives->setCurrentRow(0);

                iconExtractor.processIcons(searchResults);

		int numViewable = gSettings->value("GenOps/numviewable", "4").toInt();
		int min = alternatives->count() < numViewable ? alternatives->count() : numViewable;

		// The stylesheet doesn't load immediately, so we cache the rect here
		if (alternativesRect.isNull())
			alternativesRect = alternatives->geometry();
		QRect rect = alternativesRect;
		rect.setHeight(min * alternatives->sizeHintForRow(0));
		rect.translate(pos());

		// Is there room for the dropdown box?
		if (rect.y() + rect.height() > qApp->desktop()->height())
		{
			// Only move it if there's more space above
			// In both cases, ensure it doesn't spill off the screen
			if (pos().y() + input->pos().y() > qApp->desktop()->height() / 2)
			{
				rect.moveTop(pos().y() + input->pos().y() - rect.height());
				if (rect.top() < 0)
					rect.setTop(0);
			}
			else
			{
				rect.setBottom(qApp->desktop()->height());
			}
		}

		alternatives->setGeometry(rect);
		alternatives->show();
		alternatives->setFocus();
		qApp->syncX();
	}
	else
	{
		// clear the selection before hiding to prevent flicker
		alternatives->setCurrentRow(-1);
		alternatives->repaint();
		alternatives->hide();
		iconExtractor.stop();
	}
}


void LaunchyWidget::launchItem(CatItem& item)
{
	if (item.id == HASH_LAUNCHY || item.id == HASH_LAUNCHYFILE)
	{
		QString args = "";
		if (inputData.count() > 1)
			for(int i = 1; i < inputData.count(); ++i)
				args += inputData[i].getText() + " ";

/* UPDATE
#ifdef Q_WS_X11
		if (!platform->Execute(item.fullPath, args))
			runProgram(item.fullPath, args);
#else
*/
		runProgram(item.fullPath, args);
//#endif
	}
	else
	{
		int ops = plugins.execute(&inputData, &item);
		if (ops > 1)
		{
			switch (ops)
			{
			case MSG_CONTROL_EXIT:
				close();
				break;
			case MSG_CONTROL_OPTIONS:
				showOptionsDialog();
				break;
			case MSG_CONTROL_REBUILD:
				buildCatalog();
				break;
			default:
				break;
			}
		}
	}
	catalog->incrementUsage(item);
	history.addItem(inputData);
}


void LaunchyWidget::focusInEvent(QFocusEvent* event)
{
	if (event->gotFocus() && fader->isFading())
		fader->fadeIn(false);

	QWidget::focusInEvent(event);
}


void LaunchyWidget::focusOutEvent(QFocusEvent* event)
{
	if (event->reason() == Qt::ActiveWindowFocusReason)
	{
		if (gSettings->value("GenOps/hideiflostfocus", false).toBool() &&
			!isActiveWindow() && !alternatives->isActiveWindow() && !optionsOpen)
		{
			hideLaunchy();
		}
	}
}


void LaunchyWidget::alternativesRowChanged(int index)
{
	// Check that index is a valid history item index
	// If the current entry is a history item or there is no text entered
	if (index >= 0 && index < searchResults.count())
	{
		CatItem item = searchResults[index];
		if ((inputData.count() > 0 && inputData.first().hasLabel(LABEL_HISTORY)) || input->text().length() == 0)
		{
			// Used a void* to hold an int.. ick!
			// BUT! Doing so avoids breaking existing catalogs
			long long hi = reinterpret_cast<long long>(item.data);
			int historyIndex = static_cast<int>(hi);

			//int historyIndex = (int)item.data;
			if (item.id == HASH_HISTORY && historyIndex < searchResults.count())
			{
				inputData = history.getItem(historyIndex);
				gSearchText = inputData.last().getText();
				input->selectAll();
				input->insert(inputData.toString());
				input->selectAll();
				output->setText(Catalog::decorateText(item.shortName, gSearchText, true));
				iconExtractor.processIcon(item);
			}
		}
		else if (inputData.count() > 0 && 
			(inputData.last().hasLabel(LABEL_AUTOSUGGEST) || inputData.last().hasText() == 0))
		{
			gSearchText = item.shortName;

			inputData.last().setText(item.shortName);
			inputData.last().setLabel(LABEL_AUTOSUGGEST);

			QString root = inputData.toString(true);
			input->selectAll();
			input->insert(root + gSearchText);
			input->setSelection(root.length(), gSearchText.length());

			output->setText(Catalog::decorateText(gSearchText, gSearchText, true));
			iconExtractor.processIcon(item);
		}
	}
}


void LaunchyWidget::inputKeyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Tab)
	{
		keyPressEvent(event);
	}
	else
	{
		event->ignore();
	}

	if (input->text().length() == 0)
		showAlternatives(false);
}


void LaunchyWidget::alternativesKeyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape)
	{
		showAlternatives(false);
		event->ignore();
                this->input->setFocus();
	}
	else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Tab)
	{
		if (searchResults.count() > 0)
		{
			int row = alternatives->currentRow();
			if (row > -1)
			{
				QString location = "History/" + input->text();
				QStringList hist;
				hist << searchResults[row].lowName << searchResults[row].fullPath;
				gSettings->setValue(location, hist);

				if (row > 0)
					searchResults.move(row, 0);

				if (event->key() == Qt::Key_Tab)
				{
					doTab();
					processKey();
				}
				else
				{
					// Load up the inputData properly before running the command
					/* commented out until I find a fix for it breaking the history selection
					inputData.last().setTopResult(searchResults[0]);
					doTab();
					inputData.parse(input->text());
					inputData.erase(inputData.end() - 1);*/

					updateOutputWidgets();
					keyPressEvent(event);
				}
			}
		}
	}
	else if (event->key() == Qt::Key_Delete && (event->modifiers() & Qt::ShiftModifier) != 0)
	{
		// Delete selected history entry
		int row = alternatives->currentRow();
		if (row > -1)
		{
			if (searchResults[row].id != HASH_HISTORY)
			{
				catalog->clearUsage(searchResults[row]);
			}
			else
			{
				history.removeAt(row);
			}
			processKey();
		}
	}
	else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right ||
			 event->text().length() > 0)
	{
		// Send text entry to the input control
		activateWindow();
		input->setFocus();
		event->ignore();
		input->keyPressEvent(event);
		keyPressEvent(event);
	}
        alternatives->setFocus();
}


void LaunchyWidget::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_F5 && event->modifiers() == 0)
	{
		buildCatalog();
	}
	else if (event->key() == Qt::Key_F5 && event->modifiers() == Qt::ShiftModifier)
	{
		setSkin(gSettings->value("GenOps/skin", "Default").toString());
	}

	else if (event->key() == Qt::Key_Escape)
	{
		if (alternatives->isVisible())
			showAlternatives(false);
		else
			hideLaunchy();
	}

	else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
	{
		doEnter();
	}

	else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_PageDown || 
			 event->key() == Qt::Key_Up || event->key() == Qt::Key_PageUp)
	{
		if (alternatives->isVisible())
		{
			if (!alternatives->isActiveWindow())
			{
				// Don't refactor the activateWindow outside the if, it won't work properly any other way!
				if (alternatives->currentRow() < 0 && alternatives->count() > 0)
				{
					alternatives->activateWindow();
					alternatives->setCurrentRow(0);
				}
				else
				{
					alternatives->activateWindow();
					qApp->sendEvent(alternatives, event);
				}
			}
		}
		else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_PageDown)
		{
			// do a search and show the results, selecting the first one
			searchOnInput();
			if (searchResults.count() > 0)
			{
				showAlternatives();
				alternatives->activateWindow();
				if (alternatives->count() > 0)
					alternatives->setCurrentRow(0);
			}
		}
	}

	else if ((event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backspace) && event->modifiers() == Qt::ShiftModifier)
	{
		doBackTab();
		processKey();
	}
	else if (event->key() == Qt::Key_Tab)
	{
		doTab();
		processKey();
	}
	/*
	else if (event->key() == Qt::Key_Slash || event->key() == Qt::Key_Backslash)
	{
		if (inputData.count() > 0 && inputData.last().hasLabel(LABEL_FILE))
			doTab();
		processKey();
	}
	*/
	else if (event->text().length() > 0)
	{
		// process any other key with character output
		event->ignore();
		processKey();
	}
}


// remove input text back to the previous input section
void LaunchyWidget::doBackTab()
{
	QString text = input->text();
	int index = text.lastIndexOf(input->separatorText());
	if (index >= 0)
	{
		text.truncate(index);
		input->selectAll();
		input->insert(text);
	}
	else
	{
		input->clear();
	}
}


void LaunchyWidget::doTab()
{
	if (inputData.count() > 0 && searchResults.count() > 0)
	{
		// If it's an incomplete file or directory, complete it
		QFileInfo info(searchResults[0].fullPath);

		if ((inputData.last().hasLabel(LABEL_FILE) || info.isDir())
			)//	&& input->text().compare(QDir::toNativeSeparators(searchResults[0].fullPath), Qt::CaseInsensitive) != 0)
		{
			
			{ // If multiple paths exist, select the longest intersection (like the bash shell)
				QStringList paths;
				int minLength = -1;
				foreach(const CatItem& item, searchResults) {
					if (item.id == HASH_LAUNCHYFILE) {
						QString p = item.fullPath;
						paths += p;
						if (minLength == -1 || p.length() < minLength)
							minLength = p.length();
						qDebug() << p;
					}
				}
				qDebug() << "";

				if (paths.size() > 1) {
					// Find the longest prefix common to all of the paths
					QChar curChar;
					QString longestPrefix = "";
					for(int i = 0; i < minLength; i++) {
						curChar = paths[0][i];
						bool stop = false;
						foreach(QString path, paths) {
#ifdef Q_OS_WIN
							if (path[i].toLower() != curChar.toLower()) {
#else
							if (path[i] != curChar) {
#endif
								stop = true;
								break;
							}
						}
						if (stop) break;
						longestPrefix += curChar;
					}

					input->selectAll();
					input->insert(inputData.toString(true) + longestPrefix);
					return;
				}
			}


			QString path;
			if (info.isSymLink())
				path = info.symLinkTarget();
			else
				path = searchResults[0].fullPath;

			if (info.isDir() && !path.endsWith(QDir::separator()))
				path += QDir::separator();

			input->selectAll();
			input->insert(inputData.toString(true) + QDir::toNativeSeparators(path));
		}
		else
		{
			inputData.last().setText(searchResults[0].shortName);
			input->selectAll();
			input->insert(inputData.toString() + input->separatorText());

			QRect rect = input->rect();
			repaint(rect);
		}
	}
}


void LaunchyWidget::doEnter()
{
	showAlternatives(false);

	if ((inputData.count() > 0 && searchResults.count() > 0) || inputData.count() > 1)
	{
		CatItem& item = inputData[0].getTopResult();
		launchItem(item);
	}
	hideLaunchy();
}


void LaunchyWidget::inputMethodEvent(QInputMethodEvent* event)
{
        event = event; // Warning removal
	processKey();
}


void LaunchyWidget::processKey()
{
	inputData.parse(input->text());
	searchOnInput();
	updateOutputWidgets();
}


void LaunchyWidget::searchOnInput()
{
	if (catalog == NULL)
		return;

	QString searchText = inputData.count() > 0 ? inputData.last().getText() : "";
	gSearchText = searchText;
	searchResults.clear();

	// Add history items on their own and don't sort them so they remain in most recently used order
	if ((inputData.count() > 0 && inputData.first().hasLabel(LABEL_HISTORY)) || input->text().length() == 0)
	{
		history.search(searchText, searchResults);
	}
	else
	{
		if (inputData.count() == 1)
			catalog->searchCatalogs(gSearchText, searchResults);

		if (searchResults.count() != 0)	
			inputData.last().setTopResult(searchResults[0]);

		plugins.getLabels(&inputData);
		plugins.getResults(&inputData, &searchResults);
		qSort(searchResults.begin(), searchResults.end(), CatLessNoPtr);

		// Is it a file?
		if (searchText.contains(QDir::separator()) || searchText.startsWith("~") ||
			(searchText.size() == 2 && searchText[0].isLetter() && searchText[1] == ':'))
			FileSearch::search(searchText, searchResults, inputData);

		catalog->promoteRecentlyUsedItems(gSearchText, searchResults);
	}
}


// If there are current results, update the output text and icon
void LaunchyWidget::updateOutputWidgets()
{
	if (searchResults.count() > 0 && (inputData.count() > 1 || gSearchText.length() > 0))
	{
		output->setText(Catalog::decorateText(searchResults[0].shortName, gSearchText, true));
                iconExtractor.processIcon(searchResults[0]);

//outputIcon->setPixmap(platform->icon(searchResults[0].fullPath).pixmap((outputIcon->size())));
		if (searchResults[0].id != HASH_HISTORY)
		{
			// Did the plugin take control of the input?
			if (inputData.last().getID() != 0)
				searchResults[0].id = inputData.last().getID();
			inputData.last().setTopResult(searchResults[0]);
		}

		// If the alternatives are already showing, update them
		if (alternatives->isVisible()) 
			showAlternatives();
		// Don't schedule a drop down if there's no input text
		else if (input->text().length() > 0)
			startDropTimer();
		if (searchResults.count() == 0)
			showAlternatives(false);
	}
	else
	{
		output->clear();
		outputIcon->clear();
	}
}


void LaunchyWidget::startDropTimer()
{
	int delay = gSettings->value("GenOps/autoSuggestDelay", 1000).toInt();
	if (delay > 0)
		dropTimer->start(delay);
	else
		showAlternatives(false);
}


void LaunchyWidget::iconExtracted(int itemIndex, QIcon icon)
{
	if (itemIndex == -1)
	{
		// An index of -1 means update the output icon
		outputIcon->setPixmap(icon.pixmap(outputIcon->size()));
	}
	else if (itemIndex < alternatives->count())
	{
		// >=0 is an item in the list
		QListWidgetItem* listItem = alternatives->item(itemIndex);
		listItem->setIcon(icon);
		listItem->setData(ROLE_ICON, icon);

		QRect rect = alternatives->visualItemRect(listItem);
		repaint(rect);
	}
}


void LaunchyWidget::catalogProgressUpdated(float /*val*/)
{
}


void LaunchyWidget::catalogBuilt()
{
	catalog = gBuilder->getCatalog();

	gBuilder->wait();
	gBuilder.reset();

	workingAnimation->Stop();
	// Do a search here of the current input text
	searchOnInput();
	updateOutputWidgets();

	catalog->save(dirs["db"][0]);
}


void LaunchyWidget::checkForUpdate()
{
	http = new QHttp(this);
	verBuffer = new QBuffer(this);
	counterBuffer = new QBuffer(this);
	verBuffer->open(QIODevice::ReadWrite);
	counterBuffer->open(QIODevice::ReadWrite);

	connect(http, SIGNAL(done( bool)), this, SLOT(httpGetFinished(bool)));
	http->setHost("www.launchy.net");
	http->get("http://www.launchy.net/version2.html", verBuffer);
}


void LaunchyWidget::httpGetFinished(bool error)
{
	if (!error)
	{
		QString str(verBuffer->data());
		int ver = str.toInt();
		if (ver > LAUNCHY_VERSION)
		{
			QMessageBox box;
			box.setIcon(QMessageBox::Information);
			box.setTextFormat(Qt::RichText);
			box.setWindowTitle(tr("A new version of Launchy is available"));
			box.setText(tr("A new version of Launchy is available.\n\nYou can download it at \
						   <qt><a href=\"http://www.launchy.net/\">http://www.launchy.net</a></qt>"));
			box.exec();
		}
		if (http != NULL)
			delete http;
		http = NULL;
	}
	verBuffer->close();
	counterBuffer->close();
	delete verBuffer;
	delete counterBuffer;
}


void LaunchyWidget::setSkin(const QString& name)
{
	hideLaunchy(true);
	applySkin(name);
	showLaunchy(false);
}


void LaunchyWidget::updateVersion(int oldVersion)
{
	if (oldVersion < 199)
	{
		// We've completely changed the database and ini between 1.25 and 2.0
		// Erase all of the old information
		QString origFile = gSettings->fileName();
		delete gSettings;

		QFile oldIniPerm(dirs["config"][0]);
		oldIniPerm.remove();
		oldIniPerm.close();

		QFile oldDbPerm(dirs["db"][0]);
		oldDbPerm.remove();
		oldDbPerm.close();

		QFile oldDB(dirs["portDB"][0]);
		oldDB.remove();
		oldDB.close();

		QFile oldIni(dirs["portConfig"][0]);
		oldIni.remove();
		oldIni.close();

		gSettings = new QSettings(origFile, QSettings::IniFormat, this);
	}

	if (oldVersion < 249)
	{
		gSettings->setValue("GenOps/skin", "Default");
	}

	if (oldVersion < LAUNCHY_VERSION)
	{
		gSettings->setValue("donateTime", QDateTime::currentDateTime().addDays(21));
		gSettings->setValue("version", LAUNCHY_VERSION);
	}
}


void LaunchyWidget::loadPosition(QPoint pt)
{
	// Get the dimensions of the screen containing the new center point
	QRect rect = geometry();
	QPoint newCenter = pt + QPoint(rect.width()/2, rect.height()/2);
	QRect screen = qApp->desktop()->availableGeometry(newCenter);

	// See if the new position is within the screen dimensions, if not pull it inside
	if (newCenter.x() < screen.left())
		pt.setX(screen.left());
	else if (newCenter.x() > screen.right())
		pt.setX(screen.right()-rect.width());
	if (newCenter.y() < screen.top())
		pt.setY(screen.top());
	else if (newCenter.y() > screen.bottom())
		pt.setY(screen.bottom()-rect.height());

	int centerOption = gSettings->value("GenOps/alwayscenter", 3).toInt();
	if (centerOption & 1)
		pt.setX(screen.center().x() - rect.width()/2);
	if (centerOption & 2)
		pt.setY(screen.center().y() - rect.height()/2);

	move(pt);
}


void LaunchyWidget::updateTimeout()
{
	// Save the settings periodically
	savePosition();
	gSettings->sync();

	buildCatalog();
	
	int time = gSettings->value("GenOps/updatetimer", 10).toInt();
	updateTimer->stop();
	if (time != 0)
		updateTimer->start(time * 60000);
}


void LaunchyWidget::dropTimeout()
{
	if (searchResults.count() > 0)
		showAlternatives();
}


void LaunchyWidget::onHotkey()
{
	if (menuOpen || optionsOpen)
	{
		showLaunchy(true);
		return;
	}
	if (!alwaysShowLaunchy && isVisible() && !fader->isFading())
	{
		hideLaunchy();
	}
	else
	{
		showLaunchy();
	}
}


void LaunchyWidget::closeEvent(QCloseEvent* event)
{
	fader->stop();
	savePosition();
	gSettings->sync();

	catalog->save(dirs["db"][0]);
	history.save(dirs["history"][0]);

	event->accept();
	qApp->quit();
}


bool LaunchyWidget::setAlwaysShow(bool alwaysShow)
{
	alwaysShowLaunchy = alwaysShow;
	return !isVisible() && alwaysShow;
}


bool LaunchyWidget::setAlwaysTop(bool alwaysTop)
{
	if (alwaysTop && (windowFlags() & Qt::WindowStaysOnTopHint) == 0)
	{
		setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
		alternatives->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
		return true;
	}
	else if (!alwaysTop && (windowFlags() & Qt::WindowStaysOnTopHint) != 0)
	{
		setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
		alternatives->setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
		return true;
	}

	return false;
}


void LaunchyWidget::setPortable(bool portable)
{
	if (portable && gSettings->fileName().compare(dirs["portConfig"][0], Qt::CaseInsensitive) != 0)
	{
		delete gSettings;

		// Copy the old settings
		QFile oldSet(dirs["config"][0]);
		oldSet.copy(dirs["portConfig"][0]);
		oldSet.close();

		QFile oldDB(dirs["db"][0]);
		oldDB.copy(dirs["portDB"][0]);
		oldDB.close();

		gSettings = new QSettings(dirs["portConfig"][0], QSettings::IniFormat, this);
	}
	else if (!portable && gSettings->fileName().compare(dirs["portConfig"][0], Qt::CaseInsensitive) == 0)
	{
		delete gSettings;

		// Remove the ini file we're going to copy to so that copy can work
		QFile newF(dirs["config"][0]);
		newF.remove();
		newF.close();

		// Copy the local ini + db files to the users section
		QFile oldSet(dirs["portConfig"][0]);
		oldSet.copy(dirs["config"][0]);
		oldSet.remove();
		oldSet.close();

		QFile oldDB(dirs["portDB"][0]);
		oldDB.copy(dirs["db"][0]);
		oldDB.remove();
		oldDB.close();

		// Load up the user section ini file
		gSettings = new QSettings(dirs["config"][0], QSettings::IniFormat, this);
	}
}


void LaunchyWidget::setOpaqueness(int level)
{
	double value = level / 100.0;
	setWindowOpacity(value);
	alternatives->setWindowOpacity(value);
}


QString LaunchyWidget::getSkinDir(const QString& name) {
    // Find the skin with this name
    QString directory = dirs["skins"][0] + QString("/") + name + "/";
    if (!QFileInfo(directory).isDir()) {
        foreach(QString dir, dirs["skins"]) {
            if (QFileInfo(dir + QString("/") + name).isDir()) {
                directory = dir + QString("/") + name + "/";
                break;
            }
        }
    }
    return directory;
}

void LaunchyWidget::reloadSkin()
{
    applySkin(currentSkin);
}

void LaunchyWidget::applySkin(const QString& name)
{
        currentSkin = name;

	if (listDelegate == NULL)
		return;

        QString directory = getSkinDir(name);

        QString stylesheetPath = directory + "style.qss";

	// Use default skin if this one doesn't exist
	if (!QFile::exists(stylesheetPath)) 
	{
		directory = dirs["skins"][0] + QString("/") + name + "/";
		gSettings->setValue("GenOps/skin", dirs["defSkin"][0]);
	}

	// If still no good then fail with an ugly default
	if (!QFile::exists(stylesheetPath))
	{
		return;
	}

	// Set a few defaults
	delete frameGraphic;
	frameGraphic = NULL;
	closeButton->setGeometry(QRect());
	optionsButton->setGeometry(QRect());
	input->setAlignment(Qt::AlignLeft);
	output->setAlignment(Qt::AlignCenter);
	alternativesRect = QRect();

	if (!QFile::exists(directory + "misc.txt"))
	{
		// Loading use file:/// syntax allows relative paths in the stylesheet to be rooted
		// in the same directory as the stylesheet
		qApp->setStyleSheet("file:///" + stylesheetPath);
	}
	else
	{
		// Set positions, this will signify an older launchy skin
		// Read the style sheet
		QFile file(directory + "style.qss");
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return;
		QString styleSheet = QLatin1String(file.readAll());
		file.close();

		// This is causing the ::destroyed connect errors
		qApp->setStyleSheet(styleSheet);

		file.setFileName(directory + "misc.txt");
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream in(&file);
			while (!in.atEnd())
			{
				QString line = in.readLine();
				if (line.startsWith(";")) continue;
				QStringList spl = line.split("=");
				if (spl.size() == 2)
				{
					QStringList sizes = spl.at(1).trimmed().split(",");
					QRect rect;
					if (sizes.size() == 4)
					{
						rect.setRect(sizes[0].toInt(), sizes[1].toInt(), sizes[2].toInt(), sizes[3].toInt());
					}

					if (spl.at(0).trimmed().compare("input", Qt::CaseInsensitive) == 0)
						input->setGeometry(rect);
					else if (spl.at(0).trimmed().compare("inputAlignment", Qt::CaseInsensitive) == 0)
						input->setAlignment(
						sizes[0].trimmed().compare("left", Qt::CaseInsensitive) == 0 ? Qt::AlignLeft : 
						sizes[0].trimmed().compare("right", Qt::CaseInsensitive) == 0 ? Qt::AlignRight : Qt::AlignHCenter );
					else if (spl.at(0).trimmed().compare("output", Qt::CaseInsensitive) == 0) 
						output->setGeometry(rect);
					else if (spl.at(0).trimmed().compare("outputAlignment", Qt::CaseInsensitive) == 0)
						output->setAlignment(
						sizes[0].trimmed().compare("left", Qt::CaseInsensitive) == 0 ? Qt::AlignLeft : 
						sizes[0].trimmed().compare("right", Qt::CaseInsensitive) == 0 ? Qt::AlignRight : Qt::AlignHCenter );
					else if (spl.at(0).trimmed().compare("alternatives", Qt::CaseInsensitive) == 0)
						alternativesRect = rect;
					else if (spl.at(0).trimmed().compare("boundary", Qt::CaseInsensitive) == 0)
						resize(rect.size());
					else if (spl.at(0).trimmed().compare("icon", Qt::CaseInsensitive) == 0)
						outputIcon->setGeometry(rect);
					else if (spl.at(0).trimmed().compare("optionsbutton", Qt::CaseInsensitive) == 0)
					{
						optionsButton->setGeometry(rect);
						optionsButton->show();
					}
					else if (spl.at(0).trimmed().compare("closebutton", Qt::CaseInsensitive) == 0)
					{
						closeButton->setGeometry(rect);
						closeButton->show();
					}
					else if (spl.at(0).trimmed().compare("dropPathColor", Qt::CaseInsensitive) == 0)
					{
						listDelegate->setColor(spl.at(1));
					}
					else if (spl.at(0).trimmed().compare("dropPathSelColor", Qt::CaseInsensitive) == 0)
						listDelegate->setColor(spl.at(1),true);
					else if (spl.at(0).trimmed().compare("dropPathFamily", Qt::CaseInsensitive) == 0)
						listDelegate->setFamily(spl.at(1));
					else if (spl.at(0).trimmed().compare("dropPathSize", Qt::CaseInsensitive) == 0)
						listDelegate->setSize(spl.at(1).toInt());
					else if (spl.at(0).trimmed().compare("dropPathWeight", Qt::CaseInsensitive) == 0)
						listDelegate->setWeight(spl.at(1).toInt());
					else if (spl.at(0).trimmed().compare("dropPathItalics", Qt::CaseInsensitive) == 0)
						listDelegate->setItalics(spl.at(1).toInt());
				}
			}
			file.close();
		}
	}

	bool validFrame = false;
	QPixmap frame;

	if (platform->supportsAlphaBorder())
	{
		if (frame.load(directory + "frame.png"))
		{
			validFrame = true;
		}
		else if (frame.load(directory + "background.png"))
		{
			QPixmap border;
			if (border.load(directory + "mask.png"))
			{
				frame.setMask(border);
			}
			if (border.load(directory + "alpha.png"))
			{
				QPainter surface(&frame);
				surface.drawPixmap(0,0, border);
			}
			validFrame = true;
		}
	}
	
	if (!validFrame)
	{
		// Set the background image
		if (frame.load(directory + "background_nc.png"))
		{
			validFrame = true;

			// Set the background mask
			QPixmap mask;
			if (mask.load(directory + "mask_nc.png"))
			{
				// For some reason, w/ compiz setmask won't work
				// for rectangular areas.  This is due to compiz and
				// XShapeCombineMask
				setMask(mask);
			}
		}
	}

	if (QFile::exists(directory + "spinner.mng"))
	{
		workingAnimation->LoadAnimation(directory + "spinner.mng");
	}

	if (validFrame)
	{
		frameGraphic = new QPixmap(frame);
		if (frameGraphic)
		{
			resize(frameGraphic->size());
                }
	}

	int maxIconSize = outputIcon->width();
	maxIconSize = qMax(maxIconSize, outputIcon->height());
	platform->setPreferredIconSize(maxIconSize);
}


void LaunchyWidget::mousePressEvent(QMouseEvent *e)
{
	if (e->buttons() == Qt::LeftButton)
	{
		if (gSettings->value("GenOps/dragmode", 0).toInt() == 0 || (e->modifiers() & Qt::ShiftModifier))
		{
			dragging = true;
			dragStartPoint = e->pos();
		}
	}
	showAlternatives(false);
	activateWindow();
	input->setFocus();
}


void LaunchyWidget::mouseMoveEvent(QMouseEvent *e)
{
	if (e->buttons() == Qt::LeftButton && dragging)
	{
		QPoint p = e->globalPos() - dragStartPoint;
		move(p);
		showAlternatives(false);
		input->setFocus();
	}
}


void LaunchyWidget::mouseReleaseEvent(QMouseEvent* event)
{
        event = event; // Warning removal
	dragging = false;
	showAlternatives(false);
	input->setFocus();
}


void LaunchyWidget::contextMenuEvent(QContextMenuEvent* event)
{
	QMenu menu(this);
	menu.addAction(actRebuild);
        menu.addAction(actReloadSkin);
	menu.addAction(actOptions);        
	menu.addSeparator();
	menu.addAction(actExit);
	menuOpen = true;
	menu.exec(event->globalPos());
	menuOpen = false;
}


void LaunchyWidget::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
        case QSystemTrayIcon::Trigger:
		showLaunchy();
		break;
        case QSystemTrayIcon::Unknown:
        case QSystemTrayIcon::Context:
        case QSystemTrayIcon::DoubleClick:
        case QSystemTrayIcon::MiddleClick:
                break;
	}
}


void LaunchyWidget::buildCatalog()
{
	// Perform the database update
	if (gBuilder == NULL)
	{
		gBuilder.reset(new CatalogBuilder(&plugins, catalog));
		connect(gBuilder.get(), SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));
		connect(gBuilder.get(), SIGNAL(catalogIncrement(float)), this, SLOT(catalogProgressUpdated(float)));
		workingAnimation->Start();
		gBuilder->start(QThread::IdlePriority);
		//gBuilder->run();
	}
}


void LaunchyWidget::showOptionsDialog()
{
	if (!optionsOpen)
	{
		dropTimer->stop();
		showAlternatives(false);
		optionsOpen = true;
		OptionsDialog options(this);
		options.setObjectName("options");
#ifdef Q_WS_WIN
		// need to use this method in Windows to ensure that keyboard focus is set when 
		// being activated via a message from another instance of Launchy
		SetForegroundWindowEx(options.winId());
#endif
		options.exec();

		input->activateWindow();
		input->setFocus();
		optionsOpen = false;
	}
}


void LaunchyWidget::shouldDonate()
{
	QDateTime time = QDateTime::currentDateTime();
	QDateTime donateTime = gSettings->value("donateTime",time.addDays(21)).toDateTime();
	if (donateTime.isNull()) return;
	gSettings->setValue("donateTime", donateTime);

	if (donateTime <= time)
	{
#ifdef Q_WS_WIN
		runProgram("http://www.launchy.net/donate.html", "");
#endif
		QDateTime def;
		gSettings->setValue("donateTime", def);
	}
}


void LaunchyWidget::setFadeLevel(double level)
{
	level = qMin(level, 1.0);
	level = qMax(level, 0.0);
	setWindowOpacity(level);
	alternatives->setWindowOpacity(level);
	if (level <= 0.001)
	{
		hide();
	}
	else
	{
		if (!isVisible())
			show();
	}

        // Make sure we grab focus once we've faded in
        if (level >= 1.0) {
            activateWindow();
            raise();
        }
}


void LaunchyWidget::showLaunchy(bool noFade)
{
	shouldDonate();
	showAlternatives(false);

#ifdef Q_WS_WIN
	// There's a problem with alpha layered windows under Vista after resuming
	// from sleep. The alpha image may need to be reapplied.
#endif
	loadPosition(pos());

	fader->fadeIn(noFade || alwaysShowLaunchy);


        //qApp->syncX();
#ifdef Q_WS_WIN
	// need to use this method in Windows to ensure that keyboard focus is set when 
	// being activated via a hook or message from another instance of Launchy
	SetForegroundWindowEx(winId());
#endif
	input->raise();
	input->activateWindow();
	input->selectAll();
	input->setFocus();
	qApp->syncX();
	// Let the plugins know
	plugins.showLaunchy();
}


void LaunchyWidget::showLaunchy()
{
	showLaunchy(false);
}


void LaunchyWidget::hideLaunchy(bool noFade)
{
	if (!isVisible())
		return;

	savePosition();
	showAlternatives(false);
	if (alwaysShowLaunchy)
		return;


	if (isVisible())
	{
		fader->fadeOut(noFade);
	}

	// let the plugins know
	plugins.hideLaunchy();
}


void LaunchyWidget::loadOptions()
{
	// If a network proxy server is specified, apply an application wide NetworkProxy setting
	QString proxyHost = gSettings->value("WebProxy/hostAddress", "").toString();
	if (proxyHost.length() > 0)
	{
		QNetworkProxy proxy;
		proxy.setType((QNetworkProxy::ProxyType)gSettings->value("WebProxy/type", 0).toInt());
		proxy.setHostName(gSettings->value("WebProxy/hostAddress", "").toString());
		proxy.setPort((quint16)gSettings->value("WebProxy/port", "").toInt());
		QNetworkProxy::setApplicationProxy(proxy);
	}

	showTrayIcon();
}


int LaunchyWidget::getHotkey() const
{
	int hotkey = gSettings->value("GenOps/hotkey", -1).toInt();
	if (hotkey == -1)
	{
#ifdef Q_WS_WIN
		int meta = Qt::AltModifier;
#endif
#ifdef  Q_WS_X11
		int meta = Qt::ControlModifier;
#endif
#ifdef Q_WS_MAC
                int meta = Qt::MetaModifier;
#endif
		hotkey = gSettings->value("GenOps/hotkeyModifier", meta).toInt() |
				 gSettings->value("GenOps/hotkeyAction", Qt::Key_Space).toInt();
	}
	return hotkey;
}


void LaunchyWidget::createActions()
{
	actShow = new QAction(tr("Show Launchy"), this);
	connect(actShow, SIGNAL(triggered()), this, SLOT(showLaunchy()));

	actRebuild = new QAction(tr("Rebuild catalog"), this);
	connect(actRebuild, SIGNAL(triggered()), this, SLOT(buildCatalog()));

        actReloadSkin = new QAction(tr("Reload skin"), this);
        connect(actReloadSkin, SIGNAL(triggered()), this, SLOT(reloadSkin()));

	actOptions = new QAction(tr("Options"), this);
	connect(actOptions, SIGNAL(triggered()), this, SLOT(showOptionsDialog()));

	actExit = new QAction(tr("Exit"), this);
	connect(actExit, SIGNAL(triggered()), this, SLOT(close()));
}


int main(int argc, char *argv[])
{
	createApplication(argc, argv);

	qApp->setQuitOnLastWindowClosed(false);

	QStringList args = qApp->arguments();
	CommandFlags command = None;
	bool allowMultipleInstances = false;

	for (int i = 0; i < args.size(); ++i)
	{
		QString arg = args[i];
		if (arg.startsWith("-") || arg.startsWith("/"))
		{
			arg = arg.mid(1);
			if (arg.compare("rescue", Qt::CaseInsensitive) == 0)
			{
				command = ResetSkin | ResetPosition | ShowLaunchy;
			}
			else if (arg.compare("show", Qt::CaseInsensitive) == 0)
			{
				command |= ShowLaunchy;
			}
			else if (arg.compare("options", Qt::CaseInsensitive) == 0)
			{
				command |= ShowOptions;
			}
			else if (arg.compare("multiple", Qt::CaseInsensitive) == 0)
			{
				allowMultipleInstances = true;
			}
			else if (arg.compare("rescan", Qt::CaseInsensitive) == 0)
			{
				command |= Rescan;
			}
			else if (arg.compare("exit", Qt::CaseInsensitive) == 0)
			{
				command |= Exit;
			}
		}
	}

	if (!allowMultipleInstances && platform->isAlreadyRunning())
	{
		platform->sendInstanceCommand(command);
		exit(1);
	}

	QCoreApplication::setApplicationName("Launchy");
	QCoreApplication::setOrganizationDomain("Launchy");

	QString locale = QLocale::system().name();
	QTranslator translator;
	translator.load(QString("tr/launchy_" + locale));
	qApp->installTranslator(&translator);

	qApp->setStyleSheet("file:///:/resources/basicskin.qss");

#ifdef Q_WS_WIN
        LaunchyWidget* widget = createLaunchyWidget(command);
#else
        LaunchyWidget* widget = new LaunchyWidget(command);
#endif

	qApp->exec();

	if (gBuilder != NULL)
	{
		gBuilder->wait();
	}

	delete widget;
	widget = NULL;

	delete platform;
	platform = NULL;
}
