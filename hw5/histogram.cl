__kernel void histogram( __global unsigned int *image_data, __global unsigned int* histogram_results, unsigned int total_tasks, unsigned int task_per_thread){
    int global_id = get_global_id(0);
    size_t width = get_global_size(0);

    if( global_id < 256 * 3 ){
        histogram_results[global_id] = 0;
    }

    for( unsigned int i = 0; i < task_per_thread; i++ ){
        if( (global_id + i * width) < total_tasks ){
            for( unsigned int j = 0; j < 3; j++ ){
                unsigned int index = image_data[(global_id + (i * width)) * 3 + j];
                atomic_inc(&histogram_results[index + j * 256]);
            }
        }
    }
	
}