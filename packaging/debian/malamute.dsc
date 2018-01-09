Format: 1.0
Source: malamute
Binary: libmlm1, libmlm-dev, malamute-dbg, malamute
Architecture: any
Version: 1.1~gitb95d8e4-1
Maintainer: Michal Hrusecky <MichalHrusecky@eaton.com>
Uploaders: Michal Hrusecky <MichalHrusecky@eaton.com>
Homepage: https://github.com/Malamute/malamute-core
Standards-Version: 3.9.6
Build-Depends: debhelper (>= 9),
 autoconf,
 automake,
 libtool-bin,
 dh-autoreconf,
 libpgm-dev,
 libzmq3-dev,
 libczmq-dev,
 pkg-config,
 systemd,
 dh-systemd
Package-List:
 libmlm1 deb libs optional arch=any
 malamute deb net optional arch=any
 libmlm-dev deb libdevel optional arch=any
 malamute-dbg deb debug optional arch=any
DEBTRANSFORM-TAR: malamute-1.1~gitb95d8e4.tar.gz
