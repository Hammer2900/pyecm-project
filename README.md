# PyECM Tools

[![PyPI Version](https://img.shields.io/pypi/v/pyecm-tools.svg)](https://pypi.org/project/pyecm-tools/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Python Versions](https://img.shields.io/pypi/pyversions/pyecm-tools.svg)](https://pypi.org/project/pyecm-tools/)

A high-performance Python wrapper for Neill Corlett's classic ECM (Error Code Modeler) tools, designed for compressing and decompressing PlayStation (PSX) and other CD-based disc images.

## What is ECM and Why Use It for PSX Games?

PlayStation disc images (usually in `.bin`/`.cue` format) are large. While you can compress them with standard tools like `.zip` or `.7z`, the results are often not as good as they could be.

This is because CD-ROM data includes **Error Detection Code (EDC)** and **Error Correction Code (ECC)**. This data is mathematically generated and behaves much like random noise, which is notoriously difficult to compress.

The **ECM tool** works by stripping out this redundant ECC/EDC data from the disc image. The resulting `.ecm` file is much smaller and contains more uniform data, which can then be compressed *extremely* well by other tools like 7-Zip or RAR.

When you need to use the image again, the `unecm` process perfectly regenerates the original ECC/EDC data, restoring the file to its original, bit-perfect state.

**This package makes that powerful C code available with a simple, Pythonic interface.**

## Features

- **Faithful Wrapper:** Uses the original, battle-tested C source code from Neill Corlett for 100% compatibility.
- **High Performance:** All heavy lifting is done in compiled C code, offering native speed for encoding and decoding.
- **Simple Pythonic API:** Provides easy-to-use `encode()` and `decode()` functions.
- **Progress Reporting:** Includes a callback mechanism to easily track the progress of long operations, perfect for GUIs or logging.
- **Cross-Platform:** Compiles on Windows, macOS, and Linux.

## Installation

You can install the package directly from PyPI using `uv` or `pip`:

```bash
uv pip install pyecm-tools
```

## Quick Start: Compressing Civilization II

Let's say you have a disc image of *Civilization II* for the PlayStation and you want to archive it efficiently.

```python
import pyecm
import os

# --- Define our file paths ---
input_file = "Civilization II (USA).bin"
ecm_file = "Civilization II (USA).ecm"
restored_file = "Civilization II (USA).restored.bin"

# --- Create a dummy file for the example if it doesn't exist ---
if not os.path.exists(input_file):
    print(f"Creating a dummy file for '{input_file}'...")
    with open(input_file, "wb") as f:
        # A typical PSX game is ~650MB. We'll make a smaller 50MB file.
        f.write(os.urandom(50 * 1024 * 1024))

# 1. Encode the .bin file to .ecm
print(f"Encoding '{input_file}' to '{ecm_file}'...")
try:
    pyecm.encode(input_file, ecm_file)
    print("Encoding successful!")
except Exception as e:
    print(f"An error occurred: {e}")

# 2. Decode the .ecm file back to a .bin
print(f"\nDecoding '{ecm_file}' to '{restored_file}'...")
try:
    pyecm.decode(ecm_file, restored_file)
    print("Decoding successful!")
except Exception as e:
    print(f"An error occurred: {e}")

# You can now verify that 'input_file' and 'restored_file' are identical.
# For even better compression, you can now compress the .ecm file with 7-Zip.
```

## Advanced Usage: Progress Reporting

For large files, you'll want to see the progress. You can pass any Python function as a callback to the `progress` argument.

```python
import pyecm

def my_progress_reporter(current_bytes, total_bytes, operation_type):
    """A callback function to display progress."""
    if total_bytes == 0:
        percent = 100
    else:
        percent = (current_bytes * 100) / total_bytes

    # The 'type' argument is 0 for the initial analysis phase and 1 for processing
    stage = "Analyzing" if operation_type == 0 else "Processing"

    print(f"\r{stage}: {percent:.2f}% complete ({current_bytes}/{total_bytes})", end="")
    if current_bytes == total_bytes:
        print() # Newline at the end

# --- Use the callback with encode ---
print("Encoding with progress reporting:")
pyecm.encode(
    "Civilization II (USA).bin",
    "Civilization II (USA).ecm",
    progress=my_progress_reporter
)

# --- Use the callback with decode ---
print("\nDecoding with progress reporting:")
pyecm.decode(
    "Civilization II (USA).ecm",
    "Civilization II (USA).restored.bin",
    progress=my_progress_reporter
)
```

## License

This Python wrapper (`pyecm-tools`) is distributed under the **MIT License**.

The original C code for ECM and UNECM is copyrighted by Neill Corlett and is distributed under the **GNU General Public License v2 (GPLv2)**. The source code is included in this package.

## Acknowledgements

- All credit for the brilliant ECM algorithm and its implementation goes to **Neill Corlett**.