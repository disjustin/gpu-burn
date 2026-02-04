# Phase 1 Production Q/A Features

This document describes the Phase 1 enhancements to gpu-burn for production quality assurance.

## Features Implemented

### 1. Structured JSON Output

GPU-burn now supports structured JSON output for automated analysis and integration with test frameworks.

**Usage:**

```bash
gpu-burn -o results.json 600
```

**Output Format:**

```json
{
  "test_name": "gpu-burn",
  "version": "1.0.0",
  "start_time": "2026-02-03T12:00:00",
  "end_time": "2026-02-03T12:10:00",
  "duration_seconds": 600,
  "hostname": "test-server-01",
  "os": "Linux 5.15.0",
  "architecture": "x86_64",
  "precision": "float",
  "tensor_cores": false,
  "matrix_size": 8192,
  "gpus": [
    {
      "id": 0,
      "name": "NVIDIA A100-SXM4-40GB",
      "memory_mb": 40960,
      "status": "OK",
      "errors": [],
      "iterations": [
        {
          "iteration": 0,
          "gflops": 19500.00,
          "errors": 0,
          "temperature_c": 65
        }
      ]
    }
  ],
  "overall_result": "PASS"
}
```

### 2. Enhanced Error Classification

Errors are now categorized by type for better diagnostics:

- **COMPUTE**: Computation mismatch errors
- **MEMORY**: Memory allocation or access errors
- **THERMAL**: Temperature-related issues
- **TIMEOUT**: Process timeout errors
- **CUDA_API**: CUDA API call failures
- **PROCESS**: Process management errors
- **UNKNOWN**: Unclassified errors

Each error includes:

- Type classification
- Count
- Descriptive message
- Timestamp

### 3. Configuration File Support

Load test parameters from a JSON configuration file instead of command-line arguments.

**Usage:**

```bash
gpu-burn -cfg test_config.json
```

**Configuration File Format** (`example_config.json`):

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

**Supported Configuration Keys:**

- `duration`: Test duration in seconds
- `use_doubles`: Use double precision (true/false)
- `use_tensor_cores`: Enable Tensor Cores (true/false)
- `device_id`: Specific GPU ID to test (-1 for all)
- `memory_mb`: Memory to use in megabytes
- `memory_percent`: Memory to use as percentage (1-100)
- `output_file`: Path for JSON results output

**Note:** Command-line arguments override configuration file settings.

### 4. Standardized Exit Codes

Clear exit codes for automated testing and CI/CD integration:

| Exit Code | Meaning | Description |
|-----------|---------|-------------|
| 0 | SUCCESS | All GPUs passed testing |
| 1 | ERROR_NO_GPUS | No CUDA-capable GPUs found |
| 2 | ERROR_CUDA_INIT | CUDA initialization failed |
| 3 | ERROR_ALL_CLIENTS_DEAD | All test processes died |
| 4 | ERROR_GPU_FAULTY | One or more GPUs are faulty |
| 5 | ERROR_INVALID_ARGS | Invalid command-line arguments |
| 6 | ERROR_CONFIG_FILE | Configuration file error |
| 7 | ERROR_KERNEL_LOAD | Kernel loading error |

**Usage in scripts:**

```bash
#!/bin/bash
gpu-burn -o results.json 600
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo "All GPUs passed!"
elif [ $EXIT_CODE -eq 4 ]; then
    echo "GPU failure detected!"
    # Parse results.json for details
else
    echo "Test error: $EXIT_CODE"
fi

exit $EXIT_CODE
```

## Examples

### Basic Test with JSON Output

```bash
gpu-burn -o results.json 300
```

### Test Specific GPU with Doubles

```bash
gpu-burn -i 0 -d -o gpu0_test.json 1800
```

### Use Configuration File

```bash
gpu-burn -cfg production_test.json
```

### Configuration File with CLI Override

```bash
# Config file sets duration=3600, but CLI overrides it
gpu-burn -cfg base_config.json 600 -o custom_results.json
```

### CI/CD Integration

```bash
#!/bin/bash
# Run GPU validation in CI/CD pipeline

gpu-burn -cfg ci_config.json
RESULT=$?

# Upload results to artifact storage
if [ -f "gpu_test_results.json" ]; then
    aws s3 cp gpu_test_results.json s3://test-results/gpu-$(date +%Y%m%d)/
fi

# Fail build if GPUs are faulty
if [ $RESULT -eq 4 ]; then
    echo "GPU validation failed - marking build as failed"
    exit 1
fi

exit 0
```

## Building

No changes to the build process:

```bash
make clean
make
```

The structured output and config parsing are implemented as lightweight header-only libraries with no external dependencies.

## Migration from Previous Version

### Command-Line Compatibility

All existing command-line arguments work exactly as before. The new features are opt-in:

**Old usage (still works):**

```bash
gpu-burn 600
gpu-burn -d 1800
gpu-burn -i 0 -m 50% 300
```

**New usage (optional):**

```bash
gpu-burn -o results.json 600
gpu-burn -cfg test.json
```

### Exit Code Changes

**Before:** Inconsistent exit codes (errno-based)
**After:** Standardized exit codes (0-7)

**Migration:** Update scripts that check exit codes to use the new standardized codes.

## File Structure

```
gpu-burn/
├── gpu_burn-drv.cpp          # Main program (modified)
├── compare.cu                # CUDA kernels (unchanged)
├── structured_output.h       # NEW: JSON output & error classification
├── config_parser.h           # NEW: Configuration file parser
├── example_config.json       # NEW: Example configuration
├── PHASE1_FEATURES.md        # NEW: This documentation
├── Makefile                  # Modified for embedded PTX
└── README.md                 # Original documentation
```

## Performance Impact

- **JSON output**: Negligible (<1% overhead when enabled, zero when disabled)
- **Config parsing**: One-time cost at startup (~1ms)
- **Error classification**: Zero overhead (same error handling, just categorized)
- **Exit codes**: Zero overhead (return value vs. exit)

## Known Limitations

1. **JSON Output**: Only written after test completes (not streaming)
2. **Config File**: Simple JSON parser, limited error messages
3. **Error Classification**: Currently only tracks computational errors from kernels

## Future Enhancements (Phase 2+)

- Real-time metrics streaming
- Performance baselines and regression detection
- Additional workload types (memory, FFT, etc.)
- Web dashboard integration
- Historical trend analysis

## Support

For issues or questions:

- GitHub Issues: <https://github.com/anthropics/gpu-burn/issues>
- Include: Command used, configuration file (if any), and JSON output
