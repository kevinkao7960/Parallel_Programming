#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <fstream>
#include <iostream>

#include <CL/cl.h>

#define MAX_SOURCE_SIZE (0x100000)

// unsigned int * histogram(unsigned int *image_data, unsigned int _size) {

// 	unsigned int *img = image_data;
// 	unsigned int *ref_histogram_results;
// 	unsigned int *ptr;

// 	ref_histogram_results = (unsigned int *)malloc(256 * 3 * sizeof(unsigned int));
// 	ptr = ref_histogram_results;
// 	memset (ref_histogram_results, 0x0, 256 * 3 * sizeof(unsigned int));

// 	// histogram of R
// 	for (unsigned int i = 0; i < _size; i += 3)
// 	{
// 		unsigned int index = img[i];
// 		ptr[index]++;
// 	}

// 	// histogram of G
// 	ptr += 256;
// 	for (unsigned int i = 1; i < _size; i += 3)
// 	{
// 		unsigned int index = img[i];
// 		ptr[index]++;
// 	}

// 	// histogram of B
// 	ptr += 256;
// 	for (unsigned int i = 2; i < _size; i += 3)
// 	{
// 		unsigned int index = img[i];
// 		ptr[index]++;
// 	}

// 	return ref_histogram_results;
// }

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
	FILE *fp;
	char fileName[] = "./histogram.cl";
	char *source_str;
	size_t source_size;

	/* Load the source code containing the kernel function */
	fp = fopen( fileName, "r" );
	if(!fp){
		fprintf( stderr, "Load kernel function error!\n");
		exit(1);
	}
	source_str = (char*)malloc( MAX_SOURCE_SIZE*sizeof(char) );
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);

	/* Setting OpenCL kernel environments */
	cl_platform_id platform_id = NULL;
	cl_device_id device_id = NULL;
	cl_context context = NULL;
	cl_command_queue command_queue = NULL;
	cl_program program = NULL;
	cl_kernel kernel = NULL;
	cl_mem input = NULL;
	cl_mem output = NULL;
	cl_uint ret_num_platforms;
	cl_uint ret_num_devices;
	cl_int ret;

	size_t max_items;
	size_t max_work[3];
	size_t local_work_size;
	size_t global_work_size;
	unsigned int task_per_thread;
	unsigned int total_tasks;

	/* Get platform and device info */
	ret = clGetPlatformIDs( 1, &platform_id, &ret_num_platforms );
	ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id , &ret_num_devices );
	if (ret != CL_SUCCESS) {
    	printf("clGetDeviceIDs(): %d\n", ret);
    	return EXIT_FAILURE;
  	}

	ret = clGetDeviceInfo( device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(max_work), &max_work, NULL );
	if( ret!= CL_SUCCESS ){
		printf("clGetDeviceInfo(): %d\n", ret);
		return EXIT_FAILURE;
	}
	max_items = max_work[0]*max_work[1]*max_work[2];
	total_tasks = input_size / 3;
	task_per_thread = total_tasks / max_items + 1;

	/* Create OpenCL context */
	context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret );

	/* Create command queue */
	command_queue = clCreateCommandQueue( context, device_id, 0, &ret );
	if( !command_queue ){
		printf("clCreateCommandQueue(): %d\n", ret );
		return EXIT_FAILURE;
	}
	/**
	 * Create OpenCL buffer
	 * read-only: input data
	 * 	size: input_size 
	 * write-only: output data
	 * 	size: 256 * 3
	 */
	input = clCreateBuffer( context, CL_MEM_READ_ONLY, input_size * sizeof(unsigned int), NULL, &ret );
	output = clCreateBuffer( context, CL_MEM_WRITE_ONLY, 256 * 3 * sizeof(unsigned int), NULL, &ret );
	if (!input || !output) {
    	printf("ERROR: clCreateBuffer()\n");
    	return EXIT_FAILURE;
  	}
	/* Copy input data into the memory buffer */
	ret = clEnqueueWriteBuffer( command_queue, input, CL_TRUE, 0, input_size * sizeof(unsigned int), image, 0, NULL, NULL );

	/* Create Kernel program from the source */
	program = clCreateProgramWithSource( context, 1, (const char **)&source_str, (const size_t*)&source_size, &ret );

	/* Build Kernel Program */
	ret = clBuildProgram( program, 1, &device_id, NULL, NULL, NULL );

	/* Create OpenCL Kernel */
	kernel = clCreateKernel( program, "histogram", &ret );
	if (!kernel || ret != CL_SUCCESS) {
    	printf("ERROR: clCreateKernel(): %d, %d\n", kernel, ret);
    	return EXIT_FAILURE;
  	}

	/* Set OpenCL Kernel Parameters */
	ret = 0;
	ret = clSetKernelArg( kernel, 0, sizeof(cl_mem), (void*)&input);
	ret = clSetKernelArg( kernel, 1, sizeof(cl_mem), (void*)&output);
	ret = clSetKernelArg( kernel, 2, sizeof(unsigned int), &total_tasks );
	ret = clSetKernelArg( kernel, 3, sizeof(unsigned int), &task_per_thread );
	
	if( ret != CL_SUCCESS ){
		printf("ERROR: clSetKernelArg(): %d, %d\n", kernel, ret);
    	return EXIT_FAILURE;
	}

	/* get global_work_size and local_work_size */
	ret = clGetKernelWorkGroupInfo( kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local_work_size), &local_work_size, NULL );
	if( ret != CL_SUCCESS ){
		printf( "clGetKernelWorkGroupInfo(): %d\n", ret );
		return EXIT_FAILURE;
	}
	global_work_size = max_items;

	/* execute kernel */
	ret = clEnqueueNDRangeKernel( command_queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL );
	if( ret != CL_SUCCESS ){
		printf( "clEnqueueNDRangeKernel(): %d\n", ret );
		return EXIT_FAILURE;
	}
	clFinish( command_queue );

	/* Read the result from the device */
	ret = clEnqueueReadBuffer( command_queue, output, CL_TRUE, 0, 256*3*sizeof(unsigned int), histogram_results, 0, NULL, NULL );
	if( ret != CL_SUCCESS ){
		printf( "clEnqueueReadBuffer(): %d\n", ret );
		return EXIT_FAILURE;
	}

	// histogram_results = histogram(image, input_size);
	for(unsigned int i = 0; i < 256 * 3; ++i) {
		if (i % 256 == 0 && i != 0)
			outFile << std::endl;
		outFile << histogram_results[i]<< ' ';
	}

	inFile.close();
	outFile.close();

	ret = clReleaseKernel(kernel);
	ret = clReleaseMemObject(input);
	ret = clReleaseMemObject(output);
	ret = clReleaseProgram(program);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	return 0;
}
