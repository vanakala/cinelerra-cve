# This is not relocatable
%define prefix /usr

Summary: Cinelerra CVS
Name: cinelerra
Version: 1.2.0
Release: 5
License: GPL
Group: X11
URL: http://cvs.cinelerra.org
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
Cinelerra unofficial cvs package

%prep
%setup -q

%build
./configure --prefix=%prefix
#./configure
make

%install
rm -rf $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT%{prefix} install
# rename the mpeg3 utils so they can be installed alongside SuSE native versions
( cd $RPM_BUILD_ROOT%{prefix}/bin
  mv mpeg3toc mpeg3toc.hv
  mv mpeg3cat mpeg3cat.hv
  mv mpeg3dump mpeg3dump.hv
)

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc
#%{prefix}/bin/*
%{prefix}/bin/cinelerra
%{prefix}/bin/mplexhi
%{prefix}/bin/mplexlo
#%exclude %{prefix}/bin/mpeg3dump
#%exclude %{prefix}/bin/mpeg3toc
#%exclude %{prefix}/bin/mpeg3cat
%{prefix}/bin/mpeg3dump.hv
%{prefix}/bin/mpeg3toc.hv
%{prefix}/bin/mpeg3cat.hv
#%{prefix}/lib/*
%{prefix}/share/locale/*
%exclude %{prefix}/include/*

%{prefix}/lib/cinelerra/*.so
# remove below for no static libs
# %exclude %{prefix}/lib/cinelerra/*.a
%exclude %{prefix}/lib/cinelerra/*.la

%{prefix}/lib/cinelerra/fonts/*

%{prefix}/lib/libguicast*
%{prefix}/lib/libmpeg3hv*
%{prefix}/lib/libquicktimehv*
%{prefix}/lib/libsndfilehv*
%{prefix}/lib/pkgconfig*


%changelog
* Wed Jun 30 2004 Kevin Brosius <cobra@sea.kevb.net> - 1.2.0-5
- dv crash fix related to number of samples
- allow libdv 0.99 build, dv asset info changes
- fix avi file reading (reverted to 1.1.9)

* Thu Jun 24 2004 Kevin Brosius <cobra@sea.kevb.net> - 1.2.0-4
- raw DV/quicktime read/write support, DV audio read/write support

* Sun Jun 20 2004 Kevin Brosius <cobra@sea.kevb.net> - 1.2.0-3
- package mpeg3 utils as mpeg3toc.hv, mpeg3dump.hv, mpeg3cat.hv

* Sat Jun 19 2004 Kevin Brosius <cobra@sea.kevb.net> - 1.2.0-2
- move feather.h & codegen script, slovenian translation update
- tab shortcut to arm track under cursor

* Fri Jun 11 2004 Kevin Brosius <cobra@sea.kevb.net> - 1.2.0-1
- Merge of 1.2.0 and current cvs
- Feather speedups, tumbler/spinner limit fixes
- spanish translation, buz driver fix
- framecache race fix, trackcanvas fix, gang faders changes

* Fri Apr 30 2004 Kevin Brosius <cobra@compuserve.com> - 1.1.9-11
- change bootstrap technique for PNG files
- gang fading in trackcanvas, snap to value with shift
- plugin drag from window fix

* Wed Apr 28 2004 Kevin Brosius <cobra@compuserve.com> - 1.1.9-10
- Link problem with libXv, new rpm

* Tue Apr 27 2004 Kevin Brosius <cobra@compuserve.com> - 1.1.9-9
- RPM build updates, off by one Xv/zoom bug
- button bug for z change with no x/y
- mp3/ogg vorbis speedups for sample rate conversion
- plugin translation fixes, bug 35-automation with dnd in same track
- input variables crash checks, 
- remove static libs

* Wed Apr 21 2004 Kevin <cobra@compuserve.com> - 1.1.9-8
- Getting ready for cvs commit, no fadeout bug fix

* Sat Apr 17 2004 Kevin <cobra@compuserve.com> - 1.1.9-7
- hscroll plugins fix, fadeengine calculation improvement

* Sun Apr 11 2004 Kevin <cobra@compuserve.com> - 1.1.9-6
- cvs debug support, vscroll plugins fix
- dv1394_init usage crash fix, fix pnp playback issue
- lower bit domain speedup, non-smp overlay plugin fix
- track optimization fix

* Sun Apr  4 2004 Kevin <cobra@compuserve.com> - 1.1.9-5
- hscroll crash fix, cursor change fixes, dnd fixes

* Sat Apr  3 2004 Kevin <cobra@compuserve.com> - 1.1.9-4
- diffkey plugin, cache leak fix, latest cvs

* Sat Mar 27 2004 Kevin <cobra@compuserve.com> - 1.1.9-3
- Remove .la & .a files from plugins in package.  Update from cvs.

* Sun Mar 21 2004 Kevin <cobra@compuserve.com> - 1.1.9-1
- Initial build.


