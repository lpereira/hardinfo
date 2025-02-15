%if 0%{?rhel} <= 8
%undefine __cmake_in_source_build
%undefine __cmake3_in_source_build
%endif

Name:           hardinfo2
Version:        2.2.7
Release:        %autorelease
Summary:        System Information and Benchmark for Linux Systems

# most of the source code is GPL-2.0-or-later license, except:

# hardinfo2/gg_key_file_parse_string_as_value.c: LGPL-2.1-or-later
# includes/blowfish.h: LGPL-2.1-or-later
# deps/uber-graph/g-ring.c: LGPL-2.1-or-later
# deps/uber-graph/g-ring.h: LGPL-2.1-or-later
# modules/benchmark/blowfish.c: LGPL-2.1-or-later

# hardinfo2/gg_strescape.c: LGPL-2.0-or-later
# hardinfo2/util.c: GPL-2.0-or-later AND LGPL-2.0-or-later
# deps/uber-graph/uber-frame-source.c: LGPL-2.0-or-later
# deps/uber-graph/uber-frame-source.h: LGPL-2.0-or-later
# deps/uber-graph/uber-timeout-interval.c: LGPL-2.0-or-later
# deps/uber-graph/uber-timeout-interval.h: LGPL-2.0-or-later

# deps/uber-graph/uber-graph.c: GPL-3.0-or-later
# deps/uber-graph/uber-graph.h: GPL-3.0-or-later
# deps/uber-graph/uber-heat-map.c: GPL-3.0-or-later
# deps/uber-graph/uber-heat-map.h: GPL-3.0-or-later
# deps/uber-graph/uber-label.c: GPL-3.0-or-later
# deps/uber-graph/uber-label.h: GPL-3.0-or-later
# deps/uber-graph/uber-line-graph.c: GPL-3.0-or-later
# deps/uber-graph/uber-line-graph.h: GPL-3.0-or-later
# deps/uber-graph/uber-range.c: GPL-3.0-or-later
# deps/uber-graph/uber-range.h: GPL-3.0-or-later
# deps/uber-graph/uber-scale.c: GPL-3.0-or-later
# deps/uber-graph/uber-scale.h: GPL-3.0-or-later
# deps/uber-graph/uber-scatter.c: GPL-3.0-or-later
# deps/uber-graph/uber-scatter.h: GPL-3.0-or-later
# deps/uber-graph/uber-window.c: GPL-3.0-or-later
# deps/uber-graph/uber-window.h: GPL-3.0-or-later
# deps/uber-graph/uber.h: GPL-3.0-or-later

# includes/loadgraph.h: LGPL-2.1-only
# shell/loadgraph.c: LGPL-2.1-only

License:        GPL-2.0-or-later AND LGPL-2.1-or-later AND LGPL-2.0-or-later AND GPL-3.0-or-later AND LGPL-2.1-only
URL:            https://github.com/hardinfo2/hardinfo2
Source0:        %{url}/archive/release-%{version}/hardinfo2-release-%{version}.tar.gz

BuildRequires:  gcc-c++
%if 0%{?rhel} < 8
BuildRequires:  cmake3
%else
BuildRequires:  cmake
%endif

BuildRequires:  cmake(Qt5Core)
BuildRequires:  cmake(Qt5Gui)
BuildRequires:  cmake(Qt5Widgets)
BuildRequires:  cmake(Qt5OpenGL)

BuildRequires:  pkgconfig(gtk+-3.0)
BuildRequires:  pkgconfig(cairo)
BuildRequires:  pkgconfig(cairo-png)
BuildRequires:  pkgconfig(gthread-2.0)
BuildRequires:  pkgconfig(gmodule-export-2.0)
%if 0%{?fedora} || 0%{?rhel} >= 10
BuildRequires:  pkgconfig(libsoup-3.0)
%else
BuildRequires:  pkgconfig(libsoup-2.4)
%endif
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(json-glib-1.0)
BuildRequires:  pkgconfig(x11)
BuildRequires:  zlib-devel

BuildRequires:  systemd-rpm-macros
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib

%if 0%{?rhel} >= 8 || 0%{?fedora}
Recommends:     lm_sensors
Recommends:     sysbench
Recommends:     lsscsi
Recommends:     glx-utils
Recommends:     dmidecode
Recommends:     udisks2
Recommends:     xdg-utils
Recommends:     iperf3
%endif

%description
Hardinfo2 is based on hardinfo, which have not been released >10 years.
Hardinfo2 is the reboot that was needed.

Hardinfo2 offers System Information and Benchmark for Linux Systems. It is able
to obtain information from both hardware and basic software. It can benchmark
your system and compare to other machines online.

Features include:
- Report generation (in either HTML or plain text)
- Online Benchmarking - compare your machine against other machines

%prep
%autosetup -p1 -n hardinfo2-release-%{version}

%build
%if 0%{?rhel} < 8
%cmake3 -DCMAKE_BUILD_TYPE=Release
%cmake3_build
%else
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build
%endif

%install
%if 0%{?rhel} < 8
%cmake3_install
%else
%cmake_install
%endif

%find_lang %{name}

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/*.desktop
appstream-util validate-relax --nonet %{buildroot}%{_metainfodir}/*.metainfo.xml

%files -f %{name}.lang
%license LICENSE
%doc README.md
%{_bindir}/hardinfo2
%{_bindir}/hwinfo2_fetch_sysdata
%{_unitdir}/hardinfo2.service
%dir %{_libdir}/hardinfo2
%dir %{_libdir}/hardinfo2/modules
%{_libdir}/hardinfo2/modules/benchmark.so
%{_libdir}/hardinfo2/modules/computer.so
%{_libdir}/hardinfo2/modules/devices.so
%{_libdir}/hardinfo2/modules/network.so
%{_libdir}/hardinfo2/modules/qgears2
%{_metainfodir}/org.hardinfo2.hardinfo2.metainfo.xml
%{_datadir}/applications/hardinfo2.desktop
%dir %{_datadir}/hardinfo2
%{_datadir}/hardinfo2/*.ids
%{_datadir}/hardinfo2/benchmark.data
%{_datadir}/hardinfo2/*.json
%{_datadir}/hardinfo2/pixmaps/
%{_datadir}/icons/hicolor/scalable/apps/hardinfo2.svg
%{_mandir}/man1/hardinfo2.1*

%changelog
%if %{defined autochangelog}
%autochangelog
%else
* Mon May 01 2023 RH Container Bot <rhcontainerbot@fedoraproject.org>
- Placeholder changelog for envs that are not autochangelog-ready
%endif

