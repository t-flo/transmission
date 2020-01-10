# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
- Fix split file bug.

##  [0.2.1] - 2020-01-09
### Fixed
- Fixed split file size, and make sure the split size can be divided by chunk size, 32 768 bytes.

##  [0.2.0] - 2020-01-08
### Added
- Support for files bigger than 4GB on FAT32 by splitting and applying the file concatenation attribute.

### Fixed
- Fix cli build on linux
- Performance adjustment

##  [0.1.0]

- First release.