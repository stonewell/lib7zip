
# lib7zip

![](https://img.shields.io/badge/maintained%3F-no!-red.svg)

This is a fork of <https://github.com/stonewell/lib7zip>.

The original description follows:

  * A library using 7z.dll/7z.so (from 7-Zip) to handle different archive types.
  * lib7zip is based on 7zip/p7zip source code, but NOT including any source code from 7zip/p7zip.

This fork includes the following changes:

  * Port from autotools to CMake
  * Add automatic downloading of 7-zip sources with cmake
  * Work around "redefining GUIDs", see [this discussion][mingw-guid] for an actual explanation
  * Only look for `7z.dll` or `7z.so` in the executable's directory, as opposed to:
    * a bunch of paths on linux/mac (in /usr, /usr/local, and ".")
    * all entries of `%PATH%` on windows (which includes ".")
  * Add a new API, `ExtractSeveral`
    * Pass a subclass of `C7ZipExtractCallback`
    * This allows extracting formats like .7z faster, otherwise it keeps re-extracting the same blocks

This fork was made for internal purposes, to expose the 7-zip API to
<https://github.com/itchio/butler>.

As a result, I probably won't be accepting issues/PRs on this repo. Cheers!

[mingw-guid]: https://sourceforge.net/p/mingw-w64/mailman/message/35821021/

### License

This lib7zip fork is distributed under the MPL 2.0 license, as the original. See the
`COPYING` file.

### Rest of README

Visit the [original project page](https://github.com/stonewell/lib7zip) for thanks,
the original changelog, and so on.
