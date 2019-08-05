Name: socketapi
Version: 2.2.14~rc1.3
Release: 1
Summary: Userland implementation of the SCTP protocol RFC 4960
License: LGPL-3
Group: Applications/Internet
URL: https://www.uni-due.de/~be0001/sctplib/
Source: https://www.uni-due.de/~be0001/sctplib/download/%{name}-%{version}.tar.gz

AutoReqProv: on
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: libtool
BuildRequires: sctplib-libsctplib-devel
BuildRoot: %{_tmppath}/%{name}-%{version}-build

# TEST ONLY:
# %define _unpackaged_files_terminate_build 0

%description
The socketapi library is a fairly complete prototype implementation of the
Stream Control Transmission Protocol (SCTP), a message-oriented reliable
transport protocol that supports multi-homing, and multiple message streams
multiplexed within an SCTP connection (also named association). SCTP is
described in RFC 4960. This implementation is the product of a cooperation
between Siemens AG (ICN), Munich, Germany and the Computer Networking
Technology Group at the IEM of the University of Essen, Germany.
The API of the library was modeled after Section 10 of RFC 4960, and most
parameters and functions should be self-explanatory to the user familiar with
this document. In addition to these interface functions between an Upper
Layer Protocol (ULP) and an SCTP instance, the library also provides a number
of helper functions that can be used to manage callback function routines to
make them execute at a certain point of time (i.e. timer-based) or open and
bind  UDP sockets on a configurable port, which may then be used for an
asynchronous interprocess communication.

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

%description libsctpsocket
Socket-API provides a socket API for the SCTP userland implementation sctplib,
conforming to draft-ietf-tsvwg-sctpsocket.
This implementation is the product of a cooperation between Siemens AG (ICN),
Munich, Germany and the Computer Networking Technology Group at the IEM of
the University of Essen, Germany.

%files libsctpsocket
%{_libdir}/libsctpsocket.so.*


%package libsctpsocket-devel
Summary: Development package for Socket-API
Group: Development/Libraries
Requires: %{name}-libsctpsocket = %{version}-%{release}

%description libsctpsocket-devel
This package contains development files for  Socket-API.
Socket-API provides a socket API for the SCTP userland implementation sctplib,
conforming to draft-ietf-tsvwg-sctpsocket.
This implementation is the product of a cooperation between Siemens AG (ICN),
Munich, Germany and the Computer Networking Technology Group at the IEM of
the University of Essen, Germany.

%files libsctpsocket-devel
%{_includedir}/ext_socket.h
%{_libdir}/libsctpsocket*.a
%{_libdir}/libsctpsocket*.so


%changelog
* Fri Jun 14 2019 Thomas Dreibholz <dreibh@iem.uni-due.de> - 2.2.14
- New upstream release.
