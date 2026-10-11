# ForenC

**Forensic Evidence Collection and Integrity Verification Tool**

ForenC is a command-line tool written in C for digital forensics and incident response (DFIR). It helps investigators generate file manifests, calculate SHA-256 hashes, verify file integrity, and compare evidence snapshots.

## Features

- **SHA-256 hashing** of individual files.
- **Recursive scanning** of directories.
- **JSON evidence manifests** containing file paths, sizes, and hashes.
- **Integrity verification** against an existing manifest.
- **Manifest comparison** to identify added, deleted, modified, and unchanged files.
- **Automated tests** for core functionality.

## Requirements

- GCC or a compatible C compiler
- GNU Make
- OpenSSL development libraries
- json-c development libraries

On Kali Linux or Debian:

```bash
sudo apt update
sudo apt install build-essential libssl-dev libjson-c-dev
```

## Installation

Clone the repository:

```bash
git clone git@github.com:makeu05/ForenC.git
cd ForenC
```

Build the project:

```bash
make
```

## Usage

Display help:

```bash
./forenc --help
```

Display the version:

```bash
./forenc --version
```

Calculate a file's SHA-256 hash:

```bash
./forenc hash /path/to/file
```

Scan a directory and generate a JSON manifest:

```bash
./forenc scan /path/to/evidence -o manifest.json
```

Verify the integrity of files listed in a manifest:

```bash
./forenc verify manifest.json
```

Compare two manifests:

```bash
./forenc diff baseline.json followup.json
```

Run the automated tests:

```bash
make test
```

## Integrity and Forensic Considerations

ForenC is designed to support evidence integrity verification and file-system snapshot comparison.

- Preserve original evidence and work on forensic copies whenever possible.
- Record acquisition details and maintain a documented chain of custody.
- Store manifests securely and independently from the evidence being examined.
- Remember that matching hashes support integrity verification but do not, by themselves, establish authenticity or prove who created a file.

ForenC is an educational and developmental project. Validate its behavior in your own forensic workflow before relying on it in a real investigation.

## License

No license has been specified yet. All rights are reserved by default until a license is added to this repository.
