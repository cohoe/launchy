 TEMPLATE      = lib
 QT			+= network
 CONFIG       += plugin debug_and_release
 VPATH 		  += ../../src/
 INCLUDEPATH += ../../src/
 UI_DIR		   = ../../plugins/gcalc/
 HEADERS       = plugin_interface.h gcalc.h 
 SOURCES       = plugin_interface.cpp gcalc.cpp 
 TARGET		   = gcalc

 win32 {
	CONFIG -= embed_manifest_dll
	LIBS += shell32.lib
%	LIBS += user32.lib
%	LIBS += Gdi32.lib
%	LIBS += comctl32.lib
}
 
 *:debug {
        CONFIG -= release
	DESTDIR = ../../debug/plugins/
 }
 *:release {
        CONFIG -= debug
	DESTDIR = ../../release/plugins/
 }

unix {
}
