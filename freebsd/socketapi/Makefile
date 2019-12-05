# Created by: Thomas Dreibholz <dreibh@iem.uni-due.de>
# $FreeBSD: head/net/socketapi/Makefile 509202 2019-08-18 11:31:10Z pi $

PORTNAME=	socketapi
PORTVERSION=	2.2.18~rc2
CATEGORIES=	net
MASTER_SITES=	https://www.uni-due.de/~be0001/sctplib/download/

MAINTAINER=	dreibh@iem.uni-due.de
COMMENT=	Socket API library for the SCTPLIB user-space SCTP implementation

LICENSE=	GPLv3+
LICENSE_FILE=	${WRKSRC}/COPYING

LIB_DEPENDS=	libsctplib.so:net/sctplib

USES=		gnome libtool pkgconfig
USE_LDCONFIG=	yes
USE_GNOME=	glib20

GNU_CONFIGURE=	yes
CONFIGURE_ARGS+=	--enable-sctp-over-udp
LDFLAGS+=	-L${LOCALBASE}/lib -lsctplib
INSTALL_TARGET=	install-strip

.include <bsd.port.mk>
