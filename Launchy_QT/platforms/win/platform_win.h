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

#ifndef PLATFORM_WIN
#define PLATFORM_WIN


#define VC_EXTRALEAN
#define WINVER 0x05100
#define _WIN32_WINNT 0x0510	
#define _WIN32_WINDOWS 0x0510 
#define _WIN32_IE 0x0600


#ifndef _UNICODE
#define _UNICODE
#endif

#include <QFileIconProvider>
#include <windows.h>
#include <shlobj.h>
#include <userenv.h>
#include <TCHAR.h>

#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QtDebug>
#include <QMouseEvent>
#include "platform_base.h"
#include "platform_win_util.h"
#include "platform_base_hotkey.h"


class PlatformWin : public QObject, public PlatformBase 
{
	Q_OBJECT
	Q_INTERFACES(PlatformBase)

private:
	shared_ptr<QLaunchyAlphaBorder> alpha;
	HANDLE m1, mg1;
	QString lastImageName;
	LimitSingleInstance* instance;
public:
	PlatformWin();
	~PlatformWin();

	virtual shared_ptr<QApplication> init(int& argc, char** argv);

/*
	void KillLaunchys() {
		
		qint64 mypid = qApp->applicationPid();
		QString toRun = "TASKKILL /F /FI \"PID lt " + QString::number(mypid) + "\" /IM Launchy.exe";
		QProcess proc;
		proc.execute(toRun);	
		
	}
*/

	// Mandatory functions
	bool SetHotkey(const QKeySequence& key, QObject* receiver, const char* slot)
	{
		GlobalShortcutManager::disconnect(oldKey, receiver, slot);
		GlobalShortcutManager::connect(key, receiver, slot);
		oldKey = key;
		return GlobalShortcutManager::isConnected(key);
	}

	QString GetSettingsDirectory() { 
		QString ret;	
		GetShellDir(CSIDL_APPDATA, ret);
		return ret;
	};

	virtual QHash<QString, QList<QString> > GetDirectories();

	QList<Directory> GetInitialDirs() {
		QList<Directory> list;
		Directory tmp;
		QString ret;
		GetShellDir(CSIDL_COMMON_STARTMENU, ret);
		tmp.name = ret;
		tmp.types << "*.lnk";
		list.append(tmp);
		GetShellDir(CSIDL_STARTMENU, ret);		
		tmp.name = ret;
		list.append(tmp);
		tmp.name = "Utilities\\";
		tmp.indexDirs = true;
		list.append(tmp);


		Directory tmp2;
		tmp2.name = "%appdata%\\Microsoft\\Internet Explorer\\Quick Launch";
		tmp2.types << "*.*";
		list.append(tmp2);
		return list;
	}


	//	void Execute(QString path, QString args);
	QString expandEnvironmentVars(QString);
	void AddToNotificationArea() {};
	void RemoveFromNotificationArea() {};

	// Alpha border functions	
	shared_ptr<QWidget> getAlphaWidget() {
		return alpha;
	}
	bool SupportsAlphaBorder() { return true; }
	bool CreateAlphaBorder(QWidget* w, QString ImageName) { 
		if (w == NULL) return false;
		if (ImageName == "")
			ImageName = lastImageName;
		if (!QFile::exists(ImageName)) 
			return false;

		if (alpha != NULL) {
			// Save the position
			//			RECT r;
			//			GetWindowRect(alpha->winId(), &r);
			DestroyAlphaBorder();
			
			alpha.reset(new QLaunchyAlphaBorder(w));
			//			alpha->hide();
			alpha->SetImage(ImageName);
			alpha->setGeometry(w->geometry());
			//QRect rect(r.left, r.top, r.right, r.bottom);
			//alpha->setGeometry(rect);
			//			QPoint pos(r.left, r.top);
			alpha->RepositionWindow(w->pos());
			// 			alpha->show();

		} else {
			alpha.reset(new QLaunchyAlphaBorder(w));
			alpha->SetImage(ImageName);
		}
		lastImageName = ImageName;
		return true;
	}
	void DestroyAlphaBorder() { alpha.reset(); /*delete alpha; alpha = NULL;*/ return;}
	void MoveAlphaBorder(QPoint pos) { if (alpha != NULL) alpha->RepositionWindow(pos); }
	void HideAlphaBorder() { if (alpha != NULL) alpha->hide(); }
	void ShowAlphaBorder()
	{
		if (alpha != NULL) 
			alpha->show(); 
	}
	bool isAlreadyRunning()
	{
		return instance->IsAnotherInstanceRunning();
	}
	void showOtherInstance();
	void SetAlphaOpacity(double trans)
	{
		if (alpha != NULL)
			alpha->SetAlphaOpacity(trans);
	}

};
#endif
