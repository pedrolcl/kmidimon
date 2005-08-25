SUBDIRS = icons

# set the include path for X, qt and KDE
INCLUDES = $(all_includes) $(ALSA_CFLAGS)

# these are the headers for your project
noinst_HEADERS = kmidimon.h kmidimonwidget.h sequencerclient.h configdialog.h connectdlg.h debugdef.h fancylistviewitem.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kmidimon.pot

KDE_ICON = kmidimon

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = kmidimon

# the application source, library search path, and link libraries
kmidimon_SOURCES = main.cpp kmidimon.cpp kmidimonwidget.cpp kmidimonwidgetbase.ui sequencerclient.cpp configdialogbase.ui configdialog.cpp connectdlg.cpp fancylistviewitem.cpp
kmidimon_LDFLAGS = $(KDE_RPATH) $(all_libraries)
kmidimon_LDADD = $(LIB_KDEUI) $(LIB_KIO) $(ALSA_LIBS)

# this is where the desktop file will go 
shelldesktopdir   = $(kde_appsdir)/Utilities
shelldesktop_DATA = kmidimon.desktop

# this is where the shell's XML-GUI resource file goes
shellrcdir   = $(kde_datadir)/kmidimon
shellrc_DATA = kmidimonui.rc
