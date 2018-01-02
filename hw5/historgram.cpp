#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <fstream>
#include <iostream>

#include <CL/cl.h>

unsigned int * histogram(unsigned int *image_data, unsigned int _size) {

	unsigned int *img = image_data;
	unsigned int *ref_histogram_results;
	unsigned int *ptr;

	ref_histogram_results = (unsigned int *)malloc(256 * 3 * sizeof(unsigned int));
	ptr = ref_histogram_results;
	memset (ref_histogram_results, 0x0, 256 * 3 * sizeof(unsigned int));

	// histogram of R
	for (unsigned int i = 0; i < _size; i += 3)
	{
		unsigned int index = img[i];
		ptr[index]++;
	}

	// histogram of G
	ptr += 256;
	for (unsigned int i = 1; i < _size; i += 3)
	{
		unsigned int index = img[i];
		ptr[index]++;
	}

	// histogram of B
	ptr += 256;
	for (unsigned int i = 2; i < _size; i += 3)
	{
		unsigned int index = img[i];
		ptr[index]++;
	}

	return ref_histogram_results;
}

int main(int argc, char const *argv[])
{

	unsigned int * histogram_results;
	unsigned int i=0, a, input_size;
	std::fstream inFile("input", std::ios_base::in);
	std::ofstream outFile("0556045.out", std::ios_base::out);

	inFile >> input_size;
	unsigned int *image = new unsigned int[input_size];
	while( inFile >> a ) {
		image[i++] = a;
	}

	/* Read Source File */



	/* Setting OpenCL kernel environments */
	cl_platform_id platform_id = NULL;
	cl_device_id device_id = NULL;
	cl_context context = NULL;
	
	cl_uint ret_num_platforms;
	cl_uint ret_num_devices;
	cl_int ret;

	/* Get platform and device info */
	ret = clGetPlatformIDs( 1, &platform_id, &ret_num_platforms );
	ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id , &ret_num_devices );
	if (ret != CL_SUCCESS) {
    	printf("clGetDeviceIDs(): %d\n", ret);
    	return EXIT_FAILURE;
  	}

	/* Create OpenCL context */
	context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret );






	histogram_results = histogram(image, input_size);
	for(unsigned int i = 0; i < 256 * 3; ++i) {
		if (i % 256 == 0 && i != 0)
			outFile << std::endl;
		outFile << histogram_results[i]<< ' ';
	}

	inFile.close();
	outFile.close();

	return 0;
}
