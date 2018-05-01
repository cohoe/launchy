Name:           launchy
Version:        2.6
Release:        05%{?dist}
Summary:        Custom spin of the Open Source Keystroke Launcher

Group:          Applications/File
License:        GPL+
URL:            http://www.launchy.net
Source0:        https://github.com/cohoe/launchy/archive/2.6.tar.gz


BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  gcc-c++
BuildRequires:  qt-devel boost-devel
BuildRequires:  desktop-file-utils

%description
Launchy is a free cross-platform utility designed to help you forget about your
start menu, the icons on your desktop, and even your file manager.
Launchy indexes the programs in your start menu and can launch your documents,
project files, folders, and bookmarks with just a few keystrokes!
This release has several bug fixes.


%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}
Requires:       pkgconfig

%description    devel
The %{name}-devel package contains libraries and header files
for developing applications that use %{name}.


%prep
%setup -q
# convert DOS to UNIX
%{__sed} -i 's/\r//' LICENSE.txt readmes/readme.txt


%build
%{_libdir}/qt4/bin/qmake "CONFIG+=debug" Launchy.pro
make %{?_smp_mflags}


%install
%{__rm} -rf %{buildroot}
# prefix is hardcoded in the makefile 
install -Dpm 0755 debug/%{name} $RPM_BUILD_ROOT%{_bindir}/%{name}
mkdir -p $RPM_BUILD_ROOT%{_libdir}/%{name}/plugins
install -Dpm 0755 debug/plugins/*so $RPM_BUILD_ROOT%{_libdir}/%{name}/plugins/
install -Dpm 0755 release/plugins/*so $RPM_BUILD_ROOT%{_libdir}/%{name}/plugins/
mkdir -p $RPM_BUILD_ROOT%{_libdir}/%{name}/plugins/icons/
install -Dpm 0644 plugins/*/*png $RPM_BUILD_ROOT%{_libdir}/%{name}/plugins/icons/
mkdir -p $RPM_BUILD_ROOT%{_datadir}/pixmaps/
install -Dpm 0644 misc/Launchy_Icon/launchy_icon.png $RPM_BUILD_ROOT%{_datadir}/pixmaps/
mkdir -p $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/
install -dpm 0755 skins $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/
install -dpm 0755 skins/Black_Glass $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Black_Glass
install -dpm 0755 skins/Spotlight_Wide $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Spotlight_Wide
install -dpm 0755 skins/Simple $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Simple
install -dpm 0755 skins/Note $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Note
install -dpm 0755 skins/Default $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Default
install -dpm 0755 skins/Mercury_Wide $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Mercury_Wide
install -dpm 0755 skins/Black_Glass_Wide $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Black_Glass_Wide
install -dpm 0755 skins/Mercury $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Mercury
install -dpm 0755 skins/QuickSilver2 $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/QuickSilver2
install -Dpm 0644 skins/Black_Glass/* $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Black_Glass
install -Dpm 0644 skins/Spotlight_Wide/* $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Spotlight_Wide
install -Dpm 0644 skins/Simple/* $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Simple
install -Dpm 0644 skins/Note/* $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Note
install -Dpm 0644 skins/Default/* $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Default
install -Dpm 0644 skins/Mercury_Wide/* $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Mercury_Wide
install -Dpm 0644 skins/Black_Glass_Wide/* $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Black_Glass_Wide
install -Dpm 0644 skins/Mercury/* $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/Mercury
install -Dpm 0644 skins/QuickSilver2/* $RPM_BUILD_ROOT%{_datadir}/%{name}/skins/QuickSilver2
install -dm 0755 $RPM_BUILD_ROOT%{_includedir}/%{name}
install -Dpm 0644 "Plugin API"/*.h $RPM_BUILD_ROOT%{_includedir}/%{name}
desktop-file-install    \
        --dir $RPM_BUILD_ROOT%{_sysconfdir}/xdg/autostart       \
        --add-only-show-in=GNOME                                \
        linux/%{name}.desktop
desktop-file-install   \
        --dir $RPM_BUILD_ROOT%{_datadir}/applications   \
        --remove-category Application \
        linux/%{name}.desktop

# autostart is disabled by default
echo "X-GNOME-Autostart-enabled=false" >> \
    $RPM_BUILD_ROOT%{_sysconfdir}/xdg/autostart/%{name}.desktop


%postun
touch --no-create %{_datadir}/icons/hicolor
if [ -x %{_bindir}/gtk-update-icon-cache ] ; then
  %{_bindir}/gtk-update-icon-cache --quiet %{_datadir}/icons/hicolor || :
fi


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc LICENSE.txt readmes/readme.txt
%{_bindir}/%{name}
%{_libdir}/%{name}/
%{_datadir}/%{name}/
%{_datadir}/pixmaps/launchy_icon.png
%{_datadir}/applications/%{name}.desktop
%config(noreplace) %{_sysconfdir}/xdg/autostart/%{name}.desktop


%files devel
%defattr(-,root,root,-)
%{_includedir}/%{name}/*.h


%changelog
* Tue May 01 2018 Grant Cohoe <grant@grantcohoe.com> - 2.6-05
- Rebuild for FC28

* Sat Mar 03 2018 Grant Cohoe <grant@grantcohoe.com> - 2.6-04
- Rebuild for FC27

* Tue Aug 08 2017 Grant Cohoe <grant@grantcohoe.com> - 2.6-03
- Rebuild for FC26

* Thu May 11 2017 Grant Cohoe <grant@grantcohoe.com> - 2.6-02
- Bugfixes and RPM changelog.

* Thu Feb 04 2016 Fedora Release Engineering <releng@fedoraproject.org> - 2.5-19
- Rebuilt for https://fedoraproject.org/wiki/Fedora_24_Mass_Rebuild

* Fri Jan 15 2016 Jonathan Wakely <jwakely@redhat.com> - 2.5-18
- Rebuilt for Boost 1.60

* Thu Aug 27 2015 Jonathan Wakely <jwakely@redhat.com> - 2.5-17
- Rebuilt for Boost 1.59

* Wed Jul 29 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.5-16
- Rebuilt for https://fedoraproject.org/wiki/Changes/F23Boost159

* Wed Jul 22 2015 David Tardon <dtardon@redhat.com> - 2.5-15
- rebuild for Boost 1.58

* Wed Jun 17 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.5-14
- Rebuilt for https://fedoraproject.org/wiki/Fedora_23_Mass_Rebuild

* Sat May 02 2015 Kalev Lember <kalevlember@gmail.com> - 2.5-13
- Rebuilt for GCC 5 C++11 ABI change

* Tue Jan 27 2015 Petr Machata <pmachata@redhat.com> - 2.5-12
- Rebuild for boost 1.57.0

* Sun Aug 17 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.5-11
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Sat Jun 07 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.5-10
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Fri May 23 2014 Petr Machata <pmachata@redhat.com> - 2.5-9
- Rebuild for boost 1.55.0

* Sat Aug 03 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.5-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_20_Mass_Rebuild

* Tue Jul 30 2013 Petr Machata <pmachata@redhat.com> - 2.5-7
- Rebuild for boost 1.54.0

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.5-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Thu Jul 19 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.5-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Fri Jan 13 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 2.5-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Mon Sep 06 2010 Lukas Zapletal <lzap+rpm@redhat.com> - 2.5-3
- review by msuchy

* Mon Sep 06 2010 Lukas Zapletal <lzap+rpm@redhat.com> - 2.5-2
- correcting XDG icon path

* Mon Sep 06 2010 Lukas Zapletal <lzap+rpm@redhat.com> - 2.5-1
- initial package
