# Created by: Thomas Dreibholz <dreibh@iem.uni-due.de>
# $FreeBSD$

PORTNAME=	socketapi
PORTVERSION=	2.2.16~rc0
CATEGORIES=	net
MASTER_SITES=	https://www.uni-due.de/~be0001/sctplib/download/

MAINTAINER=	dreibh@iem.uni-due.de
COMMENT=	Socket API library for the SCTPLIB user-space SCTP implementation

LICENSE=	GPLv3+
LICENSE_FILE=   ${WRKSRC}/COPYING

LIB_DEPENDS=    libglib-2.0.so:devel/glib20

GNU_CONFIGURE=  yes
CONFIGURE_ARGS= --enable-sctp-over-udp

.include <bsd.port.mk>
