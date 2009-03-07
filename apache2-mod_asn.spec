#
# spec file for package apache2-mod_asn
#
# Copyright (c) 2009 SUSE LINUX Products GmbH, Nuernberg, Germany.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

# norootforbuild

Name:           apache2-mod_asn
BuildRequires:  apache2-devel apache2-prefork apache2-worker
%define apxs /usr/sbin/apxs2
%define apache apache2
%define apache_libexecdir %(%{apxs} -q LIBEXECDIR)
%define apache_sysconfdir %(%{apxs} -q SYSCONFDIR)
%define apache_includedir %(%{apxs} -q INCLUDEDIR)
%define apache_serverroot %(%{apxs} -q PREFIX)
%define apache_localstatedir %(%{apxs} -q LOCALSTATEDIR)
%define apache_mmn        %(MMN=$(%{apxs} -q LIBEXECDIR)_MMN; test -x $MMN && $MMN)
Version:        1.0
Release:        0
License:        Apache License, Version 2.0
Group:          Productivity/Networking/Web/Servers
Requires:       apache2 %{apache_mmn} 
Autoreqprov:    on
Summary:        mod_asn looks up the AS and network prefix of IP address
#
Source10:       mod_asn.c
Source11:       mod_asn.conf
Source20:       asn_import.py
Source21:       asn_get_routeviews.py
Source22:       asn.sql
Source30:       README
Source31:       INSTALL
#
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description

mod_asn is an Apache module doing lookups of the autonomous system (AS)[1], and
the network prefix[2] which contains a given (clients) IP address.

It is written with scalability in mind. To do high-speed lookups, it uses the
PostgreSQL ip4r datatype[3] that is indexable with a Patricia Trie[4] algorithm to
store network prefixes. This is the only algorithm that can search through the
~250.000 existing prefixes in a breeze.

It comes with script to create such a database (and keep it up to date) with
snapshots from global routing data - from a router's "view of the
world", so to speak.

Apache-internally, the module sets the looked up data as env table variables,
for perusal by other Apache modules. In addition, it can send it as response
headers to the client.

It is published under the Apache License, Version 2.0.

Links:
[1] http://en.wikipedia.org/wiki/Autonomous_system_(Internet)
[2] http://en.wikipedia.org/wiki/Subnetwork
[3] http://pgfoundry.org/projects/ip4r/
[4] http://en.wikipedia.org/wiki/Radix_tree

Author: Peter Poeml


%prep
cp -p %{S:10} %{S:11} %{S:20} %{S:21} %{S:22} %{S:30} %{S:31} .

%build
%{apxs} -c mod_asn.c



%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{apache_libexecdir}
cp -p .libs/mod_asn.so $RPM_BUILD_ROOT/%{apache_libexecdir}

install -D -m 0755 asn_import.py $RPM_BUILD_ROOT/%{_bindir}/asn_import
install -D -m 0755 asn_get_routeviews.py $RPM_BUILD_ROOT/%{_bindir}/asn_get_routeviews

%files
%defattr(-,root,root)
%doc README
%doc INSTALL
%doc asn.sql
%doc mod_asn.conf
%{apache_libexecdir}/*.so
%{_bindir}/*

%changelog -n apache2-asn
