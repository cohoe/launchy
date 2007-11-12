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

#ifndef MAIN_H
#define MAIN_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QString>
#include <QtNetwork/QHttp>
#include <QBuffer>
#include <QKeyEvent>
#include <QScrollBar>
#include "plugin_handler.h"
#include "platform_util.h"
#include "catalog.h"
#include "catalog_builder.h"
#include "icon_delegate.h"

class QLineEditMenu : public QLineEdit
{
	Q_OBJECT
public:
	QLineEditMenu(QWidget* parent = 0) :
	QLineEdit(parent) {}
	void contextMenuEvent(QContextMenuEvent *evt) {
		emit menuEvent(evt);
	}
signals:
	void menuEvent(QContextMenuEvent*);
};

class QCharLineEdit : public QLineEdit
{
	Q_OBJECT
public:
	QCharLineEdit(QWidget* parent = 0) : 
		QLineEdit(parent) 
		{
		}
		void keyPressEvent(QKeyEvent* key) {
			QLineEdit::keyPressEvent(key);
			emit keyPressed(key);
		}
		// This is how you pick up the tab key
		bool focusNextPrevChild(bool next) {
			QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, NULL);
			emit keyPressed(&key);
			return true;
		}
		void focusOutEvent ( QFocusEvent * evt) {
			emit focusOut(evt);
		}
		/*
		void mouseReleaseEvent(QMouseEvent* e)
			{
				if (event->button() & RightButton) {
					QContextMenuEvent
					emit contextMenuEvent
				}
			}
			*/
signals:
	void keyPressed(QKeyEvent*);
	void focusOut(QFocusEvent* evt);
};

class QCharListWidget : public QListWidget
{
	Q_OBJECT
public:
	QCharListWidget(QWidget* parent = 0) : 
		QListWidget(parent) 
		{
			setAttribute(Qt::WA_AlwaysShowToolTips);
			setAlternatingRowColors(true);
		}
		void keyPressEvent(QKeyEvent* key) {
			emit keyPressed(key);
			QListWidget::keyPressEvent(key);
			key->ignore();
		}
		void mouseDoubleClickEvent( QMouseEvent * event  ) {
			QKeyEvent key(QEvent::KeyPress, Qt::Key_Enter, NULL);
			emit keyPressed(&key);
		}
		void focusOutEvent ( QFocusEvent * evt) {
			emit focusOut(evt);
		}

signals:
	void keyPressed(QKeyEvent*);
	void focusOut(QFocusEvent* evt);
};

class Fader : public QThread
{
	Q_OBJECT
private:
	bool keepRunning;
	bool fadeType;
public:
	Fader(QObject* parent = NULL)
		: QThread(parent), keepRunning(true) {}
	~Fader() {}
	void stop() { keepRunning = false; }
	void run();
	void fadeIn();
	void fadeOut();
	void setFadeType(bool type) { fadeType = type; }
signals:
	void fadeLevel(double);
	void finishedFade(double);
};

class MyWidget : public QWidget
{
	Q_OBJECT  // Enable signals and slots
public:



	MyWidget(QWidget *parent = 0);
	~MyWidget();
	Fader* fader;
	QPoint moveStartPoint;
	PlatformImp platform;	
	QLabel* label;
	QLineEditMenu *output;
	QCharLineEdit *input;
	QCharListWidget *alternatives;
	QPushButton *opsButton;
	QPushButton *closeButton;
	QRect altRect;
	QLabel * licon;
	QTimer* updateTimer;
	QTimer* dropTimer;
	QScrollBar* altScroll;
	Catalog* catalog;
	CatBuilder* catBuilder;
	QList<CatItem> searchResults;
	QList<InputData> inputData;
	PluginHandler plugins;
	bool visible;
	bool alwaysShowLaunchy;
	bool menuOpen;
	
	IconDelegate* listDelegate;
	QAbstractItemDelegate * defaultDelegate;

	QIcon getIcon(CatItem & item);
	void mousePressEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void MyWidget::MoveFromAlpha(QPoint pos);
	void applySkin(QString);
	void contextMenuEvent(QContextMenuEvent *event);
	void closeEvent(QCloseEvent *event);
	void showLaunchy(bool now = false);
	void hideLaunchy(bool now = false);
	void updateVersion(int oldVersion);
	void checkForUpdate();
	void shouldDonate();
	void setCondensed(int condensed);
	void setHotkey(int, int);
	void showAlternatives(bool show=true);
	void launchObject(int obj);
	void searchFiles(const QString & input, QList<CatItem>& searchResults);
	void parseInput(QString text);
	void resetLaunchy();
	void applySkin2(QString directory);
	void updateDisplay();
	void searchOnInput();
	void fadeIn();
	void fadeOut();

	void doTab();
	void doEnter();
	void setNumViewable(int val);
	QChar sepChar();
	QString printInput();
	void printGeometry();
private:
    QHttp *http;
    QBuffer *verBuffer;
	QBuffer *counterBuffer;

public slots:
	void menuOptions();
	void onHotKey();
	void updateTimeout();
	void dropTimeout();
	void setAlwaysShow(bool);
	void setAlwaysTop(bool);
	void setPortable(bool);
	void setUpdateCheck(bool);
	void setUpdateTimer(int);
	void setNumResults(int);
	void setSkin(QString);
	void httpGetFinished(bool result);
	void catalogBuilt();
	void keyPressEvent(QKeyEvent*);
	void inputKeyPressEvent(QKeyEvent* key);
	void altKeyPressEvent(QKeyEvent* key);
	void altIndexChanged(int index);
	void focusOutEvent(QFocusEvent* evt);
	void setFadeTimes(int, int);
	void setOpaqueness(int val);
	void setFadeLevel(double);
	void finishedFade(double d);
	void menuEvent(QContextMenuEvent*);
};

#endif