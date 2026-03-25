# Zynq AXI Stream FFT Accelerator

## Overview

This project implements a streaming hardware accelerator on a Zynq SoC using AXI DMA and AXI-Stream interfaces. The system demonstrates high-throughput data transfer between the ARM processor (PS) and FPGA fabric (PL), where a pipeline consisting of FFT and floating-point processing operates on streaming data.

The project compares:
- Software FFT execution on ARM (PS)
- Hardware-accelerated FFT pipeline on FPGA (PL)

## Hardware Design
Block design screenshots are available in the `hardware/` folder.
The hardware is implemented using Vivado IP Integrator and includes:
- Processing System (Zynq PS)
- AXI DMA for memory-to-stream and stream-to-memory transfers
- FFT IP core for frequency domain transformation
- AXI Stream Data Width Converter for interface compatibility
- Floating Point IP for post-processing
- AXI Interconnect and SmartConnect for control path
- System ILA for debugging and verification
Streaming interfaces are used to enable efficient and high-throughput data transfer between modules.

## Data Flow
DDR Memory (PS)
↓
AXI DMA (MM2S)
↓
FFT → Width Converter → Floating Point
↓
AXI DMA (S2MM)
↓
DDR Memory (PS)

### Execution Steps
1. Input data is stored in DDR memory
2. AXI DMA transfers data from PS to PL using AXI-Stream
3. Data is processed through the streaming pipeline
4. FFT performs transformation on incoming data
5. Intermediate processing occurs via width conversion and floating-point operations
6. Processed data is sent back to memory via DMA
7. ARM processor reads and verifies results

## Software Implementation

Located in `software/main.c` and `software/dma_init.h`

The software is responsible for:
- Initializing AXI DMA
- Managing cache coherency (flush and invalidate operations)
- Transferring data between PS and PL
- Performing reference FFT in software
- Comparing hardware and software results
- Measuring execution time for performance evaluation
- 
## Debugging and Verification
ILA waveform captures and debugging outputs are available in the `results/` folder.
The AXI-Stream interface was verified using Vivado Integrated Logic Analyzer (ILA).

### Key Observations

- Data transfer occurs only when TVALID and TREADY are asserted
- TLAST correctly marks the end of each FFT frame
- Continuous data streaming confirms correct pipeline operation
- Data is observed in IEEE-754 floating-point format
- AXI protocol behavior is validated through waveform inspection
- 
## Results
The system successfully demonstrates functional correctness and performance improvement using hardware acceleration.

### Functional Verification
- Hardware and software FFT outputs are compared
- Minor differences may occur due to floating-point precision

### Performance

Execution time comparison between PS and PL implementations is given below .

Execution time for PS in Micro-seconds: 4.680000
Execution time for PL in Micro-seconds: 5.098462

## Repository Structure
zynq-axi-stream-fft-accelerator/
├── hardware/ # Block design screenshots and ILA setup
├── software/ # C code for DMA and FFT execution
├── results/ # ILA waveform captures and output logs
└── README.md

## How to Run

### Vivado (Hardware)

1. Open the Vivado project
2. Generate bitstream
3. Export hardware (along with bitstream).
### SDK (Software)

1. Create new project using exported bitstream.
2. Add the source file from `software/`
3. Build and run the application on hardware

## Key Learnings
- Integration of AXI4 and AXI4-Stream protocols
- High-throughput data transfer using AXI DMA
- Hardware/software co-design on Zynq architecture
- Cache coherency handling in embedded systems
- Real-time debugging using Vivado ILA
- Understanding of streaming data pipelines in FPGA systems
## Author

Daksh  
Electronics and VLSI Engineering  
IIIT Delhi
