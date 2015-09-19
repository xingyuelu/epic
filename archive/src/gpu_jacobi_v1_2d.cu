/**
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2014 Kyle Hollins Wray, University of Massachusetts
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of
 *  this software and associated documentation files (the "Software"), to deal in
 *  the Software without restriction, including without limitation the rights to
 *  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 *  the Software, and to permit persons to whom the Software is furnished to do so,
 *  subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 *  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 *  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <iostream>

#include "../include/gpu_jacobi_v1.h"

__global__ void gpu_jacobi_v1_check_2d(unsigned int *m, float *u, float *uPrime, float epsilon, unsigned int *running)
{
	unsigned int di = m[1];

	for (unsigned int i = blockIdx.x; i < m[0]; i += gridDim.x) {
		for (unsigned int j = threadIdx.x; j < m[1]; j += blockDim.x) {
			// Ensure this is not an obstacle, and the difference between iterations is greater than epsilon.
			// If this is true, then we must continue running.
			if (signbit(u[i * di + j]) == 0 &&
					fabsf(uPrime[i * di + j] -
							u[i * di + j]) > epsilon) {
				*running = 1;
			}
		}
	}
}

__global__ void gpu_jacobi_v1_iteration_2d(unsigned int *m, float *u, float *uPrime, float epsilon)
{
	unsigned int di = m[1];

	for (unsigned int i = blockIdx.x; i < m[0]; i += gridDim.x) {
		for (unsigned int j = threadIdx.x; j < m[1]; j += blockDim.x) {
			// Skip this if it is an obstacle. It is better to actually just wastefully compute the
			// equations below, instead of causing branch divergence.
			if (signbit(u[i * di + j]) == 0) {
				// Since this solver assumes the boundary is fixed, we do not need to check min and max.
				// Unless, you decide to merge the if statement into the equations below... then you need these.
	//			unsigned int ip = min(m[0] - 1, i + 1);
	//			unsigned int im = max(0, i - 1);
	//			unsigned int jp = min(m[1] - 1, j + 1);
	//			unsigned int jm = max(0, j - 1);

				float val = 0.25f *
						(fabsf(u[(i + 1) * di + j]) +
						fabsf(u[(i - 1) * di + j]) +
						fabsf(u[i * di + (j + 1)]) +
						fabsf(u[i * di + (j - 1)]));

				// TODO: Convert this into a separate kernel with the first element assigning the boolean running to false.
				// Then sync threads. Then set running to true if fabs(u[] - uPrime[]) > epsilon. Make running an unsigned int...
	//			*running = *running + (unsigned long long int)(fabsf(val - u[i * m[1] + j]) > epsilon);

				uPrime[i * di + j] = val;
			}
		}
	}
}

int gpu_jacobi_v1_alloc_2d(unsigned int *m, float *u,
		unsigned int *&d_m, float *&d_u, float *&d_uPrime)
{
	// Ensure the data is valid.
	if (u == nullptr || m == nullptr || m[0] == 0 || m[1] == 0) {
		std::cerr << "Error[gpu_jacobi_v1_alloc_2d]: Invalid data." << std::endl;
		return 1;
	}

	// Allocate the memory on the device.
	if (cudaMalloc(&d_m, 2 * sizeof(unsigned int)) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_alloc_2d]: Failed to allocate device-side memory for the dimension size values." << std::endl;
		return 2;
	}
	if (cudaMalloc(&d_u, m[0] * m[1] * sizeof(float)) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_alloc_2d]: Failed to allocate device-side memory for the harmonic function values." << std::endl;
		return 2;
	}
	if (cudaMalloc(&d_uPrime, m[0] * m[1] * sizeof(float)) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_alloc_2d]: Failed to allocate device-side memory for the harmonic function values." << std::endl;
		return 2;
	}

	// Copy the data from the host to the device. Note: Even if things like d_uPrime get overwritten,
	// you MUST malloc AND memcpy to use them!
	if (cudaMemcpy(d_m, m, 2 * sizeof(unsigned int), cudaMemcpyHostToDevice) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_alloc_2d]: Failed to copy memory from host to device for the dimension size function." << std::endl;
		return 3;
	}
	if (cudaMemcpy(d_u, u, m[0] * m[1] * sizeof(float), cudaMemcpyHostToDevice) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_alloc_2d]: Failed to copy memory from host to device for the harmonic function." << std::endl;;
		return 3;
	}
	if (cudaMemcpy(d_uPrime, u, m[0] * m[1] * sizeof(float), cudaMemcpyHostToDevice) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_alloc_2d]: Failed to copy memory from host to device for the harmonic function (prime)." << std::endl;
		return 3;
	}

	return 0;
}

int gpu_jacobi_v1_execute_2d(unsigned int *m, float epsilon,
		unsigned int *d_m, float *d_u, float *d_uPrime,
		unsigned int numBlocks, unsigned int numThreads,
		unsigned int stagger)
{
	// Ensure the data is valid.
	if (m == nullptr || epsilon <= 0.0f || d_m == nullptr || d_u == nullptr || numBlocks == 0 || numThreads == 0) {
		std::cerr << "Error[gpu_jacobi_v1_execute_2d]: Invalid data." << std::endl;
		return 1;
	}

	// Also ensure that the number of threads executed are valid.
	if (numThreads % 32 != 0) {
		std::cerr << "Error[gpu_jacobi_v1_execute_2d]: Must specify a number of threads divisible by 32 (the number of threads in a warp)." << std::endl;
		return 1;
	}

	// We must ensure that the stagger for convergence checking is even (i.e., num iterations), so that d_u stores the final result, not d_uPrime.
	if (stagger % 2 == 1) {
		std::cerr << "Error[gpu_jacobi_v1_execute_2d]: Stagger for convergence checking must be even." << std::endl;
		return 1;
	}

	// Create the running value, which keeps the iterations going so long as at least one element needs updating.
	unsigned int *running = new unsigned int;
	*running = 1;

	unsigned int *d_running = nullptr;
	if (cudaMalloc(&d_running, sizeof(unsigned int)) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_execute_2d]: Failed to allocate device-side memory for the running variable." << std::endl;
		return 2;
	}

	if (cudaMemcpy(d_running, running, sizeof(unsigned int), cudaMemcpyHostToDevice) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_execute_2d]: Failed to copy running object from host to device." << std::endl;
		return 3;
	}

	// Iterate until convergence.
	unsigned long long int iterations = 0;

	// Important Note: Must ensure that iterations is even so that d_u stores the final result, not d_uPrime.
	while (*running > 0) {
		// Perform one step of the iteration, either using u and storing in uPrime, or vice versa.
		if (iterations % 2 == 0) {
			gpu_jacobi_v1_iteration_2d<<< numBlocks, numThreads >>>(d_m, d_u, d_uPrime, epsilon);
		} else {
			gpu_jacobi_v1_iteration_2d<<< numBlocks, numThreads >>>(d_m, d_uPrime, d_u, epsilon);
		}
		if (cudaGetLastError() != cudaSuccess) {
			std::cerr << "Error[gpu_jacobi_v1_execute_2d]: Failed to execute the 'iteration' kernel." << std::endl;
			return 3;
		}

		// Wait for the kernel to finish before looping more.
		if (cudaDeviceSynchronize() != cudaSuccess) {
			std::cerr << "Error[gpu_jacobi_v1_execute_2d]: Failed to synchronize the device." << std::endl;
			return 3;
		}

		// Reset the running variable, check for convergence, then copy the running value back to the host.
		if (iterations % stagger == 0) {
			*running = 0;

			if (cudaMemcpy(d_running, running, sizeof(unsigned int), cudaMemcpyHostToDevice) != cudaSuccess) {
				std::cerr << "Error[gpu_jacobi_v1_execute_2d]: Failed to copy running object from host to device." << std::endl;
				return 3;
			}

			gpu_jacobi_v1_check_2d<<< numBlocks, numThreads >>>(d_m, d_u, d_uPrime, epsilon, d_running);
			if (cudaGetLastError() != cudaSuccess) {
				std::cerr << "Error[gpu_jacobi_v1_execute_2d]: Failed to execute the 'check' kernel." << std::endl;
				return 3;
			}

			if (cudaDeviceSynchronize() != cudaSuccess) {
				std::cerr << "Error[gpu_jacobi_v1_execute_2d]: Failed to synchronize the device when checking for convergence." << std::endl;
				return 3;
			}

			if (cudaMemcpy(running, d_running, sizeof(unsigned int), cudaMemcpyDeviceToHost) != cudaSuccess) {
				std::cerr << "Error[gpu_jacobi_v1_execute_2d]: Failed to copy running object from device to host." << std::endl;
				return 3;
			}
		}

		iterations++;
	}

//	std::cout << "GPU Jacobi 2D: Completed in " << iterations << " iterations." << std::endl;

	// Free the memory of the delta value.
	delete running;
	if (cudaFree(d_running) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_execute_2d]: Failed to free memory for the running flag." << std::endl;
		return 4;
	}

	return 0;
}

int gpu_jacobi_v1_get_2d(unsigned int *m, float *d_u, float *u)
{
	if (cudaMemcpy(u, d_u, m[0] * m[1] * sizeof(float), cudaMemcpyDeviceToHost) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_get_2d]: Failed to copy memory from device to host for the entire result." << std::endl;
		return 1;
	}
	return 0;
}

int gpu_jacobi_v1_free_2d(unsigned int *d_m, float *d_u, float *d_uPrime)
{
	if (cudaFree(d_m) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_free_2d]: Failed to free memory for the dimension sizes." << std::endl;
		return 1;
	}
	if (cudaFree(d_u) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_free_2d]: Failed to free memory for the harmonic function." << std::endl;
		return 1;
	}
	if (cudaFree(d_uPrime) != cudaSuccess) {
		std::cerr << "Error[gpu_jacobi_v1_free_2d]: Failed to free memory for the harmonic function (prime)." << std::endl;
		return 1;
	}
	return 0;
}