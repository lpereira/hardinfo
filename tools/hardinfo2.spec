Name:           hardinfo2
Version:        2.0.12
Release:        %autorelease
Summary:        System Information and Benchmark for Linux Systems

# most of the source code is GPL-2.0-or-later license, except:
# includes/blowfish.h - LGPL-2.1-or-later
# includes/loadgraph.h - LGPL-2.0-only
# deps/uber-graph/g-ring.c: LGPL-2.1-or-later
# deps/uber-graph/g-ring.h: LGPL-2.1-or-later
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
# modules/benchmark/blowfish.c - LGPL-2.1-or-later
License:        GPL-2.0-or-later AND LGPL-2.1-or-later AND LGPL-2.0-only AND GPL-3.0-or-later
URL:            https://github.com/hardinfo2/hardinfo2
Source0:        %{url}/archive/release-%{version}/hardinfo2-release-%{version}.tar.gz

Patch0:         https://github.com/hardinfo2/hardinfo2/pull/14.patch

BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  ninja-build

BuildRequires:  pkgconfig(gtk+-3.0)
BuildRequires:  pkgconfig(cairo)
BuildRequires:  pkgconfig(cairo-png)
BuildRequires:  pkgconfig(gthread-2.0)
BuildRequires:  pkgconfig(gmodule-export-2.0)
# BuildRequires:  pkgconfig(libsoup-3.0)
BuildRequires:  pkgconfig(libsoup-2.4)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(json-glib-1.0)
BuildRequires:  pkgconfig(x11)

BuildRequires:  desktop-file-utils

Obsoletes:      hardinfo

Recommends:     lm_sensors
Recommends:     sysbench
Recommends:     lsscsi
Recommends:     glx-utils
Recommends:     dmidecode
Recommends:     udisks2
Recommends:     xdg-utils
Recommends:     iperf3

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
%cmake \
    -GNinja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \

%cmake_build

%install
%cmake_install

%find_lang %{name}

%check
desktop-file-validate %{buildroot}/%{_datadir}/applications/*.desktop

%files -f %{name}.lang
%license
%doc README.md
%{_bindir}/hardinfo2
%dir %{_libdir}/hardinfo2
%dir %{_libdir}/hardinfo2/modules
%{_libdir}/hardinfo2/modules/benchmark.so
%{_libdir}/hardinfo2/modules/computer.so
%{_libdir}/hardinfo2/modules/devices.so
%{_libdir}/hardinfo2/modules/network.so
%{_datadir}/applications/hardinfo2.desktop
%{_datadir}/hardinfo2/*.ids
%{_datadir}/hardinfo2/benchmark.data
%{_datadir}/hardinfo2/*.json
%{_datadir}/hardinfo2/pixmaps/
%{_datadir}/icons/hicolor/256x256/apps/hardinfo2.png
%{_mandir}/man1/hardinfo2.1*

%changelog
%autochangelog
