<h1 align="center">
 Socket-API<br />
 <span style="font-size: 75%;">Socket API for the SCTPLIB User-Space SCTP Implementation</span><br />
 <a href="https://www.nntb.no/~dreibh/sctplib/">
  <img alt="SCTP Project Logo" src="https://www.nntb.no/~dreibh/sctp/images/SCTPProject.svg" width="25%" /><br />
  <span style="font-size: 75%;">https://www.nntb.no/~dreibh/sctplib</span>
 </a>
</h1>


# 💡 What is Socket-API?

Socket-API is an implementation of the SCTP Socket API for the SCTPLIB user-space SCTP implementation. See
[RFC&nbsp;6458](https://www.rfc-editor.org/rfc/rfc6458.html) for a description of this API.


# 📚 Details

It contains a number of files and directories that are shortly described hereafter:

* [`AUTHORS`](AUTHORS): People who have produced this code.
* [`COPYING`](COPYING): The license to be applied for using/compiling/distributing this code.
* [`INSTALL`](INSTALL): Basic installation notes.
* [`README.md`](README.md): This file.
* [`socketapi`](socketapi): This directory contains the socketapi sources.
* [`socket_programs`](socket_programs): Example programs written in C.
* [`cppsocketapi`](cppsocketapi): Some C++ wrapper classes for easier socket access.
* [`cppsocket_programs`](cppsocket_programs): Example programs written in C++ using the C++ classes.


# 💾 Build from Sources

Socket-API is released under the [GNU General Public Licence&nbsp;(GPL)](https://www.gnu.org/licenses/gpl-3.0.en.html#license-text).

Please use the issue tracker at [https://github.com/dreibh/socketapi/issues](https://github.com/dreibh/socketapi/issues) to report bugs and issues!

## Development Version

The Git repository of the Socket-API sources can be found at [https://github.com/dreibh/socketapi](https://github.com/dreibh/socketapi):

```bash
git clone https://github.com/dreibh/socketapi
cd socketapi
sudo ci/get-dependencies --install
./configure
make
```

Note: The script [`ci/get-dependencies`](https://github.com/dreibh/socketapi/blob/master/ci/get-dependencies) automatically  installs the build dependencies under Debian/Ubuntu Linux, Fedora Linux, and FreeBSD. For manual handling of the build dependencies, see the packaging configuration in [`debian/control`](https://github.com/dreibh/socketapi/blob/master/debian/control) (Debian/Ubuntu Linux), [`socketapi.spec`](https://github.com/dreibh/socketapi/blob/master/rpm/socketapi.spec) (Fedora Linux), and [`Makefile`](https://github.com/dreibh/socketapi/blob/master/freebsd/socketapi/Makefile) FreeBSD.

Contributions:

* Issue tracker: [https://github.com/dreibh/socketapi/issues](https://github.com/dreibh/socketapi/issues).
  Please submit bug reports, issues, questions, etc. in the issue tracker!

* Pull Requests for Socket-API: [https://github.com/dreibh/socketapi/pulls](https://github.com/dreibh/socketapi/pulls).
  Your contributions to Socket-API are always welcome!

* CI build tests of Socket-API: [https://github.com/dreibh/socketapi/actions](https://github.com/dreibh/socketapi/actions).

## Release Versions

See [https://www.nntb.no/~dreibh/socketapi/#current-stable-release](https://www.nntb.no/~dreibh/sctplib/#current-stable-release) for the release packages!


# 🔗 Useful Links

* [HiPerConTracer – High-Performance Connectivity Tracer](https://www.nntb.no/~dreibh/hipercontracer/)
* [NetPerfMeter – A TCP/MPTCP/UDP/SCTP/DCCP Network Performance Meter Tool](https://www.nntb.no/~dreibh/netperfmeter/)
* [SubNetCalc – An IPv4/IPv6 Subnet Calculator](https://www.nntb.no/~dreibh/subnetcalc/)
* [System-Tools – Tools for Basic System Management](https://www.nntb.no/~dreibh/system-tools/)
* [Virtual Machine Image Builder and System Installation Scripts](https://www.nntb.no/~dreibh/vmimage-builder-scripts/)
* [Wireshark](https://www.wireshark.org/)
