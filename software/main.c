/******************************************************************************
* Zynq AXI-Stream FFT Accelerator (Software + Hardware Comparison)
*
* Description:
* This program performs an 8-point FFT in two ways:
* 1. Software FFT on ARM (PS)
* 2. Hardware FFT using FPGA (PL) via AXI DMA
*
* It then compares the outputs and measures execution time.
*
* Key Concepts Demonstrated:
* - AXI DMA transfers (MM2S and S2MM)
* - Cache coherency handling
* - Hardware/software co-design
* - Performance comparison (PS vs PL)
******************************************************************************/

#include <stdio.h>
#include <complex.h>
#include <stdlib.h>
#include "platform.h"
#include "xil_printf.h"
#include <xtime_l.h>
#include "xparameters.h"
#include "xaxidma.h"
#include "dma_init.h"
#include "xil_cache.h"

#define N 8   // FFT size

// Bit-reversal index table for 8-point FFT
const int rev8[N] = {0,4,2,6,1,5,3,7};

// Twiddle factors for FFT computation
const float complex W[N/2] = {
    1-0*I,
    0.7071067811865476-0.7071067811865475*I,
    0.0-1*I,
    -0.7071067811865476-0.7071067811865475*I
};

// Bit-reversal reordering of input data
void bitreverse(float complex dataIn[N], float complex dataOut[N]){
	bit_reversal: for(int i=0;i<N;i++){
		dataOut[i]=dataIn[rev8[i]];
	}
}

// Software implementation of 8-point FFT (3 stages)
void FFT_stages(float complex FFT_input[N],float complex FFT_output[N]){
	float complex temp1[N], temp2[N];

	// Stage 1: Butterfly operations
	stage1: for(int i=0;i<N;i=i+2){
			temp1[i] = FFT_input[i]+FFT_input[i+1];
			temp1[i+1] = FFT_input[i]-FFT_input[i+1];
	}

	// Stage 2: Twiddle factor multiplication
	stage2: for(int i=0;i<N;i=i+4){
				for(int j=0;j<2;++j){
					temp2[i+j] = temp1[i+j]+W[2*j]*temp1[i+j+2];
					temp2[i+2+j] =temp1[i+j]-W[2*j]*temp1[i+j+2];
			}
	}

	// Stage 3: Final combination
	stage3: for(int i=0;i<N/2;i=i+1){
				FFT_output[i]=temp2[i]+W[i]*temp2[i+4];
				FFT_output[i+4]=temp2[i]-W[i]*temp2[i+4];
		}
}

int main()
{
    init_platform();   // Initialize platform (UART, caches, etc.)

    // Variables to measure execution time
    XTime PL_start_time, PL_end_time;
    XTime PS_start_time, PS_end_time;

    // Input and output buffers (aligned for cache efficiency)
    volatile float complex FFT_input[N] __attribute__ ((aligned (32))) =
        {11+23*I,32+10*I,91+94*I,15+69*I,47+96*I,44+12*I,96+17*I,49+58*I};

    volatile float complex FFT_output_hw[N] __attribute__ ((aligned (32)));

    float complex FFT_output_sw[N];   // Software FFT output
    float complex FFT_rev_sw[N];      // Bit-reversed input

    // ================= SOFTWARE FFT =================
    printf("\r\nRunning Software FFT...");

    XTime_SetTime(0);                      // Reset timer
    XTime_GetTime(&PS_start_time);         // Start timing

    bitreverse((float complex*)FFT_input, FFT_rev_sw);
    FFT_stages(FFT_rev_sw, FFT_output_sw);

    XTime_GetTime(&PS_end_time);           // End timing
    printf("Done.");

    // ================= HARDWARE FFT =================
    int status;
    XAxiDma AxiDMA;

    // Initialize AXI DMA
    status = DMA_Init(&AxiDMA, XPAR_AXI_DMA_0_DEVICE_ID);
    if(status) {
        printf("\r\nDMA Init Failed\r\n");
        return 1;
    }

    printf("\r\nRunning Hardware FFT (PL)...");

    // -------- Cache Handling --------
    // Flush input buffer to ensure memory consistency
    Xil_DCacheFlushRange((UINTPTR)FFT_input, sizeof(float complex) * N);

    // Invalidate output buffer before transfer
    Xil_DCacheInvalidateRange((UINTPTR)FFT_output_hw, sizeof(float complex) * N);

    XTime_SetTime(0);
    XTime_GetTime(&PL_start_time);

    // -------- DMA Transfers --------
    // Receive (S2MM) - FPGA → Memory
    status = XAxiDma_SimpleTransfer(&AxiDMA,
        (UINTPTR)FFT_output_hw,
        (sizeof(float complex)*N),
        XAXIDMA_DEVICE_TO_DMA);

    if (status != XST_SUCCESS) { printf("RX Fail"); return 1; }

    // Send (MM2S) - Memory → FPGA
    status = XAxiDma_SimpleTransfer(&AxiDMA,
        (UINTPTR)FFT_input,
        (sizeof(float complex)*N),
        XAXIDMA_DMA_TO_DEVICE);

    if (status != XST_SUCCESS) { printf("TX Fail"); return 1; }

    // -------- Polling for Completion --------
    while(XAxiDma_Busy(&AxiDMA, XAXIDMA_DMA_TO_DEVICE));
    while(XAxiDma_Busy(&AxiDMA, XAXIDMA_DEVICE_TO_DMA));

    XTime_GetTime(&PL_end_time);

    // Invalidate output buffer after transfer to fetch updated data
    Xil_DCacheInvalidateRange((UINTPTR)FFT_output_hw, sizeof(float complex) * N);

    printf(" Done.");

    // ================= RESULT VERIFICATION =================
    for(int i=0; i<N; i++){

        // Hardware output is assumed to be reciprocal of FFT result
        float hw_restored_real = 1.0f / crealf(FFT_output_hw[i]);
        float hw_restored_imag = 1.0f / cimagf(FFT_output_hw[i]);

        printf("\n\rPS Output- %f+%fI, PL Output- %f+%fI",
               crealf(FFT_output_sw[i]), cimagf(FFT_output_sw[i]),
               hw_restored_real, hw_restored_imag);

        // Compute difference between software and hardware results
        float diff1 = fabsf(crealf(FFT_output_sw[i]) - hw_restored_real);
        float diff2 = fabsf(cimagf(FFT_output_sw[i]) - hw_restored_imag);

        if(diff1 >= 0.01 || diff2 >= 0.01){
            printf("\n\rData Mismatch found at index %d ! (Diff: %f, %f)", i, diff1, diff2);
        }
        else {
             printf(" DMA Transfer Successful!");
        }
    }

    // ================= PERFORMANCE RESULTS =================
    printf("\n\r------- Execution Time Comparison --------");

    float time_sw = (float)1.0 * (PS_end_time - PS_start_time) / (COUNTS_PER_SECOND/1000000);
    printf("\n\rExecution time for PS in Micro-seconds: %f", time_sw);

    float time_hw = (float)1.0 * (PL_end_time - PL_start_time) / (COUNTS_PER_SECOND/1000000);
    printf("\n\rExecution time for PL in Micro-seconds: %f", time_hw);

    cleanup_platform();
    return 0;
}
