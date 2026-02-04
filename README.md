# gpu-burn

Multi-GPU CUDA stress test for production quality assurance
<http://wili.cc/blog/gpu-burn.html>

- [gpu-burn](#gpu-burn)
  - [Features](#features)
  - [Easy docker build and run](#easy-docker-build-and-run)
  - [Binary packages](#binary-packages)
  - [Building](#building)
  - [Usage](#usage)
  - [Production Q/A Features](#production-qa-features)
  - [Documentation](#documentation)

## Features

- Multi-GPU stress testing with matrix multiplication workloads
- Single and double precision floating-point support
- Tensor Core acceleration (when available)
- Temperature monitoring during test execution
- **Structured JSON output** for automated testing and CI/CD integration
- **Configuration file support** for reproducible test configurations
- **Standardized exit codes** for scripting and automation
- **Standalone binary** with embedded CUDA kernel (no external files required)

## Easy docker build and run

```plain
git clone https://github.com/wilicc/gpu-burn
cd gpu-burn
docker build -t gpu_burn .
docker run --rm --gpus all gpu_burn
```

## Binary packages

<https://repology.org/project/gpu-burn/versions>

## Building

### Quick Start

To build GPU Burn:

```bash
make
```

This creates a **standalone executable** with the CUDA kernel embedded directly in the binary.
The resulting `gpu_burn` executable can be copied anywhere and run without additional files.

To remove artifacts built by GPU Burn:

```bash
make clean
```

### Build Options

GPU Burn builds with a default Compute Capability of 7.5 as specified on NVIDIA's [CUDA GPU Compute Capability](https://developer.nvidia.com/cuda-gpus).
To override this with a different value:

```bash
make COMPUTE=<compute capability value>
```

Additional compiler flags:

```bash
make CFLAGS=-Wall
```

Additional linker flags:

```bash
make LDFLAGS=-lmylib
```

Additional nvcc flags:

```bash
make NVCCFLAGS="-ccbin <path to host compiler>"
```

Custom CUDA toolkit path (default is /usr/local/cuda):

```bash
make CUDAPATH=/usr/local/cuda-<version>
```

Custom gcc path (default is /usr/bin):

```bash
make CCPATH=/usr/local/bin
```

Docker build options (IMAGE_NAME, CUDA_VERSION, IMAGE_DISTRO):

```bash
make IMAGE_NAME=myregistry.private.com/gpu-burn CUDA_VERSION=12.0.1 IMAGE_DISTRO=ubuntu22.04 image
```

### Building with External PTX File (Legacy)

By default, the CUDA kernel (PTX) is embedded in the executable. To build with an external PTX file instead:

1. Remove the `-DEMBED_PTX` flag from Makefile line 17
2. Run:
   ```bash
   make clean
   make
   ```

Now `gpu_burn` will require `compare.ptx` in the working directory (old behavior).

### Installation

To install globally (optional):

```bash
sudo cp gpu_burn /usr/local/bin/
```

The executable is standalone and can be run from any location.

## Usage

### Command-Line Options

```
GPU Burn
Usage: gpu-burn [OPTIONS] [TIME]

OPTIONS:
  -m X          Use X MB of memory
  -m N%         Use N% of available GPU memory (default 90%)
  -d            Use double precision
  -tc           Try to use Tensor Cores (if available)
  -l            List all GPUs in the system
  -i N          Execute only on GPU N
  -o FILE       Write structured JSON results to FILE
  -cfg FILE     Load configuration from JSON FILE
  -c FILE       Use FILE as compare kernel (advanced)
  -stts T       Set SIGTERM timeout threshold to T seconds
  -h            Show this help message

TIME (optional): Test duration in seconds (default: 10)

EXIT CODES:
  0 - Success (all GPUs passed)
  1 - No CUDA GPUs found
  2 - CUDA initialization failed
  3 - All client processes died
  4 - One or more GPUs are faulty
  5 - Invalid command-line arguments
  6 - Configuration file error
  7 - Kernel loading error
```

### Basic Examples

List available GPUs:

```bash
gpu-burn -l
```

Run basic 10-minute stress test on all GPUs:

```bash
gpu-burn 600
```

Test with double precision for 1 hour:

```bash
gpu-burn -d 3600
```

Test specific GPU with Tensor Cores:

```bash
gpu-burn -i 0 -tc 1800
```

Use 50% of GPU memory for testing:

```bash
gpu-burn -m 50% 300
```

### Production Q/A Examples

Save results to JSON file for automated analysis:

```bash
gpu-burn -o results.json 600
```

Load test configuration from file:

```bash
gpu-burn -cfg production_test.json
```

Example configuration file (`test_config.json`):

```json
{
  "duration": 3600,
  "use_doubles": false,
  "use_tensor_cores": true,
  "device_id": -1,
  "memory_percent": 90,
  "output_file": "gpu_burn_results.json"
}
```

Use in automated testing scripts:

```bash
#!/bin/bash
gpu-burn -o results.json 600
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo "✓ All GPUs passed testing"
elif [ $EXIT_CODE -eq 4 ]; then
    echo "✗ GPU failure detected"
    exit 1
else
    echo "✗ Test error (exit code: $EXIT_CODE)"
    exit 1
fi
```

## Production Q/A Features

### Structured JSON Output

GPU-burn can generate structured JSON output for integration with automated testing frameworks and CI/CD pipelines:

```bash
gpu-burn -o results.json 600
```

The JSON output includes:
- System information (hostname, OS, architecture)
- GPU details (model, memory, driver info)
- Test configuration and parameters
- Per-GPU performance metrics and errors
- Overall test result (PASS/FAIL)
- Error classification and timestamps

### Configuration Files

Complex test configurations can be saved in JSON files and reused:

```bash
# Create configuration
cat > qa_test.json <<EOF
{
  "duration": 1800,
  "use_tensor_cores": true,
  "memory_percent": 95,
  "output_file": "qa_results.json"
}
EOF

# Run test
gpu-burn -cfg qa_test.json
```

Command-line arguments override configuration file settings, allowing flexible test variations:

```bash
# Override duration from config file
gpu-burn -cfg base_config.json 300
```

### Error Classification

Errors are automatically categorized for better diagnostics:
- **COMPUTE**: Computation mismatch errors
- **MEMORY**: Memory allocation/access errors
- **THERMAL**: Temperature-related issues
- **TIMEOUT**: Process timeout errors
- **CUDA_API**: CUDA API call failures
- **PROCESS**: Process management errors

### Standardized Exit Codes

Exit codes follow a consistent scheme for easy scripting and automation. See the exit codes table in the usage section above.

## Documentation

- **[PHASE1_FEATURES.md](PHASE1_FEATURES.md)** - Complete documentation of production Q/A features
- **[IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)** - Technical implementation details
- **[example_config.json](example_config.json)** - Example configuration file
- **gpu-burn.8** - Man page (install with `man ./gpu-burn.8`)
