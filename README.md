# SELA
### SimplE Lossless Audio
A simplified lossless audio codec written for my final year project.

## NOTE: THIS code is currently being rewritten. You can get the legacy code by switching to `legacy` branch.
You can also check out the Java version of this codec at [https://github.com/sahaRatul/sela-java](https://github.com/sahaRatul/sela-java)

[![Build Status](https://travis-ci.org/sahaRatul/sela.svg?branch=master)](https://travis-ci.org/sahaRatul/sela)
[![Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=sahaRatul_sela&metric=ncloc)](https://sonarcloud.io/dashboard?id=sahaRatul_sela)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=sahaRatul_sela&metric=alert_status)](https://sonarcloud.io/dashboard?id=sahaRatul_sela)
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=sahaRatul_sela&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=sahaRatul_sela)
[![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=sahaRatul_sela&metric=security_rating)](https://sonarcloud.io/dashboard?id=sahaRatul_sela)
[![codecov](https://codecov.io/gh/sahaRatul/sela/branch/latest/graph/badge.svg)](https://codecov.io/gh/sahaRatul/sela)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

### Block Diagrams
![Encoder](https://cloud.githubusercontent.com/assets/12273725/8868411/c24585e6-31f5-11e5-937a-e3c11c632704.png)
![Decoder](https://cloud.githubusercontent.com/assets/12273725/8868418/cbb6a1dc-31f5-11e5-91f6-8290766baa34.png)

### References
- Linear Prediction
  - [Wikipedia](https://en.wikipedia.org/wiki/Linear_prediction)
  - [Digital Signal Processing by John G. Proakis & Dimitris G. Monolakis](http://www.amazon.com/Digital-Signal-Processing-4th-Edition/dp/0131873741)
  - [A detailed pdf](http://www.ece.ucsb.edu/Faculty/Rabiner/ece259/digital%20speech%20processing%20course/lectures_new/Lecture%2013_winter_2012_6tp.pdf)
- Golomb-Rice lossless compression algorithm
  - [Wikipedia](https://en.wikipedia.org/wiki/Golomb_coding)
  - [Here is an implementation](http://michael.dipperstein.com/rice/index.html)
- [FLAC overview](https://xiph.org/flac/documentation_format_overview.html)
- [Paper on shorten, the original open source lossless codec](ftp://svr-ftp.eng.cam.ac.uk/pub/reports/robinson_tr156.ps.Z)
- ISO/IEC 14496 Part 3, Subpart 11 (Audio Lossless Coding)

### License : MIT
