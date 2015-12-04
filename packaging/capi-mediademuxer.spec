Name:       capi-mediademuxer
Summary:    A Media Demuxer library in Tizen Native API
Version:    0.1.1
Release:    1
Group:      Multimedia/API
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(capi-media-tool)
BuildRequires:  pkgconfig(capi-media-codec)
BuildRequires:  pkgconfig(libtbm)
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires:  pkgconfig(gstreamer-video-1.0)
BuildRequires:  pkgconfig(gstreamer-app-1.0)
BuildRequires:  pkgconfig(iniparser)

Requires(post): /sbin/ldconfig
Requires(post): libprivilege-control
Requires(postun): /sbin/ldconfig

%description

%package devel
Summary:    Multimedia Framework Demuxer Library (DEV)
Group:      Multimedia/API
Requires:   %{name} = %{version}-%{release}

%description devel

%prep
%setup -q


%build
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif
%ifarch %{arm}
export CFLAGS="$CFLAGS -DENABLE_FFMPEG_CODEC"
%endif

MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
%cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DFULLVER=%{version} -DMAJORVER=${MAJORVER}


make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_datadir}/license
mkdir -p %{buildroot}/usr/bin
cp test/mediademuxer_test %{buildroot}/usr/bin
cp LICENSE.APLv2 %{buildroot}%{_datadir}/license/%{name}

%make_install

%post
/sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest capi-mediademuxer.manifest
%{_libdir}/libcapi-mediademuxer.so.*
%{_datadir}/license/%{name}
%{_bindir}/*

%files devel
%{_includedir}/media/*.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/libcapi-mediademuxer.so

