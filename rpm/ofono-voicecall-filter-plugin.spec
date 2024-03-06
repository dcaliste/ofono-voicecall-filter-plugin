Name: ofono-voicecall-filter-plugin
Version: 0.1
Release: 1
Summary: a voicecall filter plugin for Ofono
License: GPLv2
Source0: %{name}-%{version}.tar.gz
BuildRequires: pkgconfig(ofono)
BuildRequires: cmake

%description
Provides a plugin to filter received voice calls based on
phone numbers.

%prep
%setup -q -n %{name}-%{version}

%build
%cmake
make %{?_smp_mflags}

%install
make DESTDIR=%{buildroot} install

%files
%defattr(-,root,root,-)
%license LICENSE
%{_libdir}/ofono/plugins/*.so
