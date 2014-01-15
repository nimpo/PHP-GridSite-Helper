Name:		PHP-GridSite-Helper
Version:	0.2
Release:	1
Summary:	gridsite.inc a PHP include script to provide GridSite look and feel to PHP html

Group:		System Environment/Daemons
License:	BSD
URL:		http://www.rcs.manchester.ac.uk/research/NGS
Source0:	PHP-GridSite-Helper-0.2.tgz
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
BuildArch:	noarch

#BuildRequires:	
Requires:	php

%description
The PHP-GridSite-Helper is a php script (gridsite.inc) designed for 
integration with Andrew McNab's GridSite.
 1 It incorporates a GridSite header and footer (usually called 
   gridsitehead.txt and gridsitefoot.txt) if available when the output of the
   PHP is html or xhtml.
 2 It performs a GACL access control check (online dn-lists may also be used).
   Checks are performed at the point when the gridsite.inc file is included.
 3 It can present a rendering of the GACL and which rules have been triggered
   providing access to that particular script. This is achieved by adding the 
   query ?cmd=list_acl to the resource URI, this option will take precedence
   over the underlying PHP file.

This script provides NO ADDED SECURITY.  Its function is an attempt to present
a coherent GridSite experience for content rendered through the php interpreter.

%prep
%setup -q

%build

%install
install -m 0755 -d $RPM_BUILD_ROOT/var/www/etc
install -m 0644 gridsite.inc $RPM_BUILD_ROOT/var/www/etc/gridsite.inc
install -m 0755 -d $RPM_BUILD_ROOT/usr/share/php-gridsite
install -m 0644 README $RPM_BUILD_ROOT/usr/share/php-gridsite/README
install -m 0644 LICENCE $RPM_BUILD_ROOT/usr/share/php-gridsite/LICENCE
install -m 0644 INSTALL $RPM_BUILD_ROOT/usr/share/php-gridsite/INSTALL
install -m 0644 t.php $RPM_BUILD_ROOT/usr/share/php-gridsite/t.php
install -m 0644 PHP-GridSite-Helper.spec $RPM_BUILD_ROOT/usr/share/php-gridsite/PHP-GridSite-Helper.spec
install -m 0644 .gacl $RPM_BUILD_ROOT/usr/share/php-gridsite/.gacl

%clean
rm -rf $RPM_BUILD_ROOT

%files
%dir /var/www/etc
/var/www/etc/gridsite.inc
%dir /usr/share/php-gridsite
/usr/share/php-gridsite/README
/usr/share/php-gridsite/LICENCE
/usr/share/php-gridsite/INSTALL
/usr/share/php-gridsite/t.php
/usr/share/php-gridsite/PHP-GridSite-Helper.spec
/usr/share/php-gridsite/.gacl

%changelog
* Wed Jan 15 2014 Mike AS Jones <dr.mike.jones@gmail.com> 0.1
- Added gridsite.inc location example to INSTALL file
- Added cmd=_debug method
- Added a Back option from cmd=list_acl
* Fri Apr 27 2012 Mike AS Jones <mike.jones@manchester.ac.uk> 0.1
- RPM Packaged
