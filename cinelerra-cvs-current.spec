# This is not relocatable
%define prefix /usr

Summary: Cinelerra CVS
Name: cinelerra
Version: 1.1.9
Release: 8
License: GPL
Group: X11
URL: http://cvs.cinelerra.org
Packager: Kevin Brosius <cobra@compuserve.com>
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

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc
#%{prefix}/bin/*
%{prefix}/bin/cinelerra
%{prefix}/bin/mplexhi
%{prefix}/bin/mplexlo
%exclude %{prefix}/bin/mpeg3dump
%exclude %{prefix}/bin/mpeg3toc
%exclude %{prefix}/bin/mpeg3cat
#%{prefix}/lib/*
%{prefix}/share/locale/*
%exclude %{prefix}/include/*

%{prefix}/lib/cinelerra/*.so
%exclude %{prefix}/lib/cinelerra/*.a
%exclude %{prefix}/lib/cinelerra/*.la

%{prefix}/lib/cinelerra/fonts/*

%{prefix}/lib/libguicast*
%{prefix}/lib/libmpeg3hv*
%{prefix}/lib/libquicktimehv*
%{prefix}/lib/libsndfilehv*
%{prefix}/lib/pkgconfig*


%changelog
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


