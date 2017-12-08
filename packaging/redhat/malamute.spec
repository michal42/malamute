#
# spec file for package
#
# Copyright (c) 2014 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

%global soname libmlm1

Name:           malamute
Version:        1.1~gitb95d8e4
Release:        1
License:        MPL-2.0
Summary:        All the enterprise messaging patterns in one box
Url:            https://github.com/zeromq/malamute
Group:          Development/Libraries/C and C++
#Source0:        https://github.com/zeromq/malamute/archive/v1.0.tar.gz
Source0:        %{name}-%{version}.tar.gz
Group:          System/Libraries
BuildRequires:  libsodium-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  zeromq-devel >= 4.2
BuildRequires:  czmq-devel >= 3.0
BuildRequires:  pkg-config
BuildRequires:  libtool
BuildRequires:  autoconf
BuildRequires:  automake
# documentation
BuildRequires:  asciidoc
BuildRequires:  xmlto
BuildRequires:  systemd-devel
%if %{defined opensuse_version}
BuildRequires:  systemd-rpm-macros
Requires(pre):  systemd-rpm-macros
Requires(preun):systemd-rpm-macros
Requires(post): systemd-rpm-macros
Requires(postun):systemd-rpm-macros
%endif
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
Malamute is a messaging broker and a protocol for enterprise messaging.
Malamute helps to connect the pieces in a distributed software system. Before
we explain what Malamute does, and why, let's discuss the different kinds of
data we most often send around a distributed system.

 * Live Streamed Data
 * On-Demand Streamed Data
 * Publish-and-Subscribe (Pub-Sub)
 * Direct Messaging
 * Service Requests

%package -n %{soname}
Summary:    Shared library of %{name}
Group:      Development/Languages/C and C++

%description -n %{soname}
malamute zeromq message broker.
This package contains shared library of %{name}.

%post -n libmlm1 -p /sbin/ldconfig
%postun -n libmlm1 -p /sbin/ldconfig

%files -n libmlm1
%defattr(-,root,root)
%doc COPYING
%{_libdir}/libmlm.so.*

%package devel
Summary:        zeromq message broker
Group:          Development/Languages/C and C++
Requires:       %{soname} = %{version}
Requires:       zeromq-devel
Requires:       czmq-devel

%description devel
malamute zeromq message broker.
This package contains development files.

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libmlm.so
%{_libdir}/pkgconfig/libmlm.pc

%prep
%setup -q

%build
autoreconf -fiv
# MVY: there's a bug in v1.0 build system, so mlm_proto_t is marked as draft API, but
#      the MLM_BUILD_DRAFT_API is not defined with --enable-drafts
#      one need to padd --enable-drafts and define the macro at the SAME time

CFLAGS="%{optflags} -DMLM_BUILD_DRAFT_API"

%configure \
    --disable-static \
    --enable-drafts \
    --with-systemd-units
make %{?_smp_mflags} V=1

%install
%make_install

rm -f %{buildroot}/%{_bindir}/*.gsl
rm -f %{buildroot}/%{_libdir}/*.la
rm -f %{buildroot}/%{_libdir}/*.a

%check
echo "Skipped for now"
#make check

%if %{defined opensuse_version}
%pre
%service_add_pre malamute

%post
%service_add_post malamute

%preun
%service_del_preun malamute

%postun
%service_del_postun malamute

%else
#%post -n %{soname} -p /sbin/ldconfig

#%postun -n %{soname} -p /sbin/ldconfig
%endif

%files
%defattr(-,root,root)
%doc LICENSE
%dir %{_sysconfdir}/malamute
%config(noreplace) %{_sysconfdir}/malamute/malamute.cfg
%{_bindir}/malamute
%{_bindir}/mlm_perftest
%{_bindir}/mlm_tutorial
%{_bindir}/mshell
%{_unitdir}/malamute*.service

%files -n %{soname}
%defattr(-,root,root)
%{_libdir}/libmlm.so.*

%files devel
%defattr(-,root,root)
%doc AMQP.md AUTHORS CONTRIBUTING.md LICENSE README.md MALAMUTE.md STREAM.md
%{_includedir}/*.h
%{_libdir}/libmlm.so
%{_libdir}/pkgconfig/libmlm.pc
%{_mandir}/man1/*
%{_mandir}/man3/*
%dir %{_datadir}/zproject
%{_datadir}/zproject/malamute/

%changelog
