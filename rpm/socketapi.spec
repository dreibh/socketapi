Name: socketapi
Version: 2.2.25
Release: 1
Summary: Socket API library for the SCTPLIB user-space SCTP implementation
License: GPL-3.0-or-later
Group: Applications/Internet
URL: https://www.nntb.no/~dreibh/sctplib/
Source: https://www.nntb.no/~dreibh/sctplib/download/%{name}-%{version}.tar.gz

AutoReqProv: on
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: gcc-c++
BuildRequires: glib2-devel
BuildRequires: libtool
BuildRequires: sctplib-libsctplib-devel
BuildRoot: %{_tmppath}/%{name}-%{version}-build

# TEST ONLY:
# define _unpackaged_files_terminate_build 0

%description
SocketAPI is the socket API library for the SCTPLIB user-space
SCTP implementation.

%prep
%setup -q

%build
autoreconf -i

%configure --prefix=/usr --enable-sctp-over-udp --disable-maintainer-mode
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install


%package libsctpsocket
Summary: Socket API library for sctplib
Group: System Environment/Libraries
Requires: sctplib-libsctplib

%description libsctpsocket
Socket-API provides a socket API for the SCTP user-space implementation SCTPLIB,
conforming to RFC 6458.
This implementation is the product of a cooperation between Siemens AG (ICN),
Munich, Germany and the Computer Networking Technology Group at the IEM of
the University of Essen, Germany.

%files libsctpsocket
%{_libdir}/libsctpsocket.so.*
%ghost %{_libdir}/libcppsocketapi.so.*


%package libsctpsocket-devel
Summary: Development package for Socket-API
Group: Development/Libraries
Requires: %{name}-libsctpsocket = %{version}-%{release}

%description libsctpsocket-devel
This package contains development files for Socket-API.
Socket-API provides a socket API for the SCTP user-space implementation SCTPLIB,
conforming to RFC 6458.
This implementation is the product of a cooperation between Siemens AG (ICN),
Munich, Germany and the Computer Networking Technology Group at the IEM of
the University of Essen, Germany.

%files libsctpsocket-devel
%{_includedir}/ext_socket.h
%{_libdir}/libsctpsocket*.a
%{_libdir}/libsctpsocket*.so
%ghost %{_libdir}/libcppsocketapi*.a
%ghost %{_libdir}/libcppsocketapi*.so
%ghost %{_includedir}/cppsocketapi/*.h
%ghost %{_includedir}/cppsocketapi/*.icc


%changelog
* Wed Jul 09 2025 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.2.25
- New upstream release.
* Wed Dec 06 2023 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.2.24
- New upstream release.
* Sun Jan 22 2023 Thomas Dreibholz <thomas.dreibholz@gmail.com> - 2.2.23
- New upstream release.
* Sun Sep 11 2022 Thomas Dreibholz <dreibh@iem.uni-due.de> - 2.2.22
- New upstream release.
* Thu Feb 17 2022 Thomas Dreibholz <dreibh@iem.uni-due.de> - 2.2.21
- New upstream release.
* Wed Feb 16 2022 Thomas Dreibholz <dreibh@iem.uni-due.de> - 2.2.20
- New upstream release.
* Fri Nov 13 2020 Thomas Dreibholz <dreibh@iem.uni-due.de> - 2.2.19
- New upstream release.
* Fri Feb 07 2020 Thomas Dreibholz <dreibh@iem.uni-due.de> - 2.2.18
- New upstream release.
* Tue Sep 10 2019 Thomas Dreibholz <dreibh@iem.uni-due.de> - 2.2.17
- New upstream release.
* Wed Aug 14 2019 Thomas Dreibholz <dreibh@iem.uni-due.de> - 2.2.16
- New upstream release.
* Wed Aug 07 2019 Thomas Dreibholz <dreibh@iem.uni-due.de> - 2.2.15
- New upstream release.
* Tue Aug 06 2019 Thomas Dreibholz <dreibh@iem.uni-due.de> - 2.2.14
- New upstream release.
