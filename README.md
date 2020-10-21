# thameSrc

This repository contains a modified version of John Elliott's thames partial ISIS II emulator. The original source can be found at [thames: An ISIS-II Emulator (v0.1.1) (seasip.info)](http://www.seasip.info/Unix/Thames/thames.html)

The differences, along with John's original help file are in the doc directory in markdown, pdf and html formats.

Note this version now builds using cmake or the provided visual studio solution file rather than using autotools. Additionally I have added my automated build versioning model based on git, see the repository [ogdenpm/versionTools (github.com)](https://github.com/ogdenpm/versionTools) for details.

A caveat on the version tools is that if you are building under *nix then you will need to make sure that the repository is also checked out with *nix line endings, otherwise the line ending differences are identified as uncommitted changes.

------

```
Updated by Mark Ogden 21-Oct-2020
```

