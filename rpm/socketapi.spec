Name: socketapi
Version: 2.2.15
Release: 1
Summary: Socket API library for the SCTPLIB user-space SCTP implementation
License: GPL-3+
Group: Applications/Internet
URL: https://www.uni-due.de/~be0001/sctplib/
Source: https://www.uni-due.de/~be0001/sctplib/download/%{name}-%{version}.tar.gz

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

%configure --prefix=/usr --enable-static --enable-shared --enable-sctp-over-udp
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
%{_libdir}/libsctpsocket*.la
%{_libdir}/libsctpsocket*.so
%ghost %{_libdir}/libcppsocketapi*.a
%ghost %{_libdir}/libcppsocketapi*.la
%ghost %{_libdir}/libcppsocketapi*.so
%ghost %{_includedir}/cppsocketapi/*.h
%ghost %{_includedir}/cppsocketapi/*.icc


%changelog
* Wed Aug 07 2019 Thomas Dreibholz <dreibh@iem.uni-due.de> - 2.2.15
- New upstream release.
* Tue Aug 06 2019 Thomas Dreibholz <dreibh@iem.uni-due.de> - 2.2.14
- New upstream release.
