#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include <ios>
#include <vector>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

typedef struct tag_RGB
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t align;
} RGB;

typedef struct tag_Image
{
    bool type;
    uint32_t size;
    uint32_t height;
    uint32_t weight;
    RGB *data;
} Image;

cl_program load_program(cl_context context, const char* filename){
    std::ifstream in(filename, std::ios_base::binary);
    if(!in.good()) {
        return 0;
    }
    // get file length
    in.seekg(0, std::ios_base::end);
    size_t length = in.tellg();
    in.seekg(0, std::ios_base::beg);

    // read program source
    std::vector<char> data(length + 1);
    in.read(&data[0], length);
    data[length] = 0;

    // create and build program 
    const char* source = &data[0];
    cl_program program = clCreateProgramWithSource(context, 1, &source, 0, 0);
    if(program == 0) {
        std::cout << "create program error" << std::endl;
        return 0;
    }

    if(clBuildProgram(program, 0, 0, 0, 0, 0) != CL_SUCCESS) {
        std::cout << "build program error" << std::endl;
        return 0;
    }

    return program;
}

Image *readbmp(const char *filename)
{
    std::ifstream bmp(filename, std::ios::binary);
    char header[54];
    bmp.read(header, 54);
    uint32_t size = *(int *)&header[2];
    uint32_t offset = *(int *)&header[10];
    uint32_t w = *(int *)&header[18];
    uint32_t h = *(int *)&header[22];
    uint16_t depth = *(uint16_t *)&header[28];
    if (depth != 24 && depth != 32)
    {
        printf("we don't suppot depth with %d\n", depth);
        exit(0);
    }
    bmp.seekg(offset, bmp.beg);

    Image *ret = new Image();
    ret->type = 1;
    ret->height = h;
    ret->weight = w;
    ret->size = w * h;
    ret->data = new RGB[w * h]{};
    for (int i = 0; i < ret->size; i++)
    {
        bmp.read((char *)&ret->data[i], depth / 8);
    }
    return ret;
}

int writebmp(const char *filename, Image *img)
{

    uint8_t header[54] = {
        0x42,        // identity : B
        0x4d,        // identity : M
        0, 0, 0, 0,  // file size
        0, 0,        // reserved1
        0, 0,        // reserved2
        54, 0, 0, 0, // RGB data offset
        40, 0, 0, 0, // struct BITMAPINFOHEADER size
        0, 0, 0, 0,  // bmp width
        0, 0, 0, 0,  // bmp height
        1, 0,        // planes
        32, 0,       // bit per pixel
        0, 0, 0, 0,  // compression
        0, 0, 0, 0,  // data size
        0, 0, 0, 0,  // h resolution
        0, 0, 0, 0,  // v resolution
        0, 0, 0, 0,  // used colors
        0, 0, 0, 0   // important colors
    };

    // file size
    uint32_t file_size = img->size * 4 + 54;
    header[2] = (unsigned char)(file_size & 0x000000ff);
    header[3] = (file_size >> 8) & 0x000000ff;
    header[4] = (file_size >> 16) & 0x000000ff;
    header[5] = (file_size >> 24) & 0x000000ff;

    // width
    uint32_t width = img->weight;
    header[18] = width & 0x000000ff;
    header[19] = (width >> 8) & 0x000000ff;
    header[20] = (width >> 16) & 0x000000ff;
    header[21] = (width >> 24) & 0x000000ff;

    // height
    uint32_t height = img->height;
    header[22] = height & 0x000000ff;
    header[23] = (height >> 8) & 0x000000ff;
    header[24] = (height >> 16) & 0x000000ff;
    header[25] = (height >> 24) & 0x000000ff;

    std::ofstream fout;
    fout.open(filename, std::ios::binary);
    fout.write((char *)header, 54);
    fout.write((char *)img->data, img->size * 4);
    fout.close();
}

int main(int argc, char *argv[])
{
    if (argc >= 2)
    {
        cl_int errCode;
        cl_uint numPlatforms;
        cl_platform_id platform_id;
        cl_device_id device_id;
 
        errCode = clGetPlatformIDs (1, &platform_id, NULL);
        // check
        if (errCode != CL_SUCCESS){
            std::cout << "Get platform error" << std::endl;
            return EXIT_FAILURE;
        }

        errCode = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
        // check
        if (errCode != CL_SUCCESS){
            std::cout << "Get device id error" << std::endl;
            return EXIT_FAILURE;
        }
        
        cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &errCode);
        // check
        if (context == 0){
            std::cout << "open context error" << std::endl;
            return EXIT_FAILURE;
        }

        cl_command_queue command = clCreateCommandQueue(context, device_id, 0, &errCode);
        // check
        if (command == 0){
            std::cout << "create command error" << std::endl;
            return EXIT_FAILURE;
        }

        int many_img = argc - 1;
        char *filename;

        for (int i = 0; i < many_img; i++)
        {
            filename = argv[i + 1];
            Image *img = readbmp(filename);
 
            std::cout << img->weight << ":" << img->height << "\n";
            size_t local[2] = {256, 256};
            size_t global[2];  
            unsigned int R[256];
            unsigned int G[256];
            unsigned int B[256];
            cl_mem input = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(RGB)*img->size, NULL, NULL);
            cl_mem output_R = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned int)*256, NULL, NULL);
            cl_mem output_G = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned int)*256, NULL, NULL);
            cl_mem output_B = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned int)*256, NULL, NULL);
            // check        
            if (input == 0 || output_R == 0 || output_G == 0 || output_B == 0){
                std::cout << "I/O error" << std::endl;
                return EXIT_FAILURE;
            }

            cl_program program = load_program(context, "histogram.cl");
            // check
            if (program == 0){
                std::cout << "load program error" << std::endl;
                return EXIT_FAILURE;
            }
            cl_kernel kernel = clCreateKernel(program, "histogram", &errCode);
            // check
            if (kernel == 0){
                std::cout << "create kernel error"<< std::endl;
                return EXIT_FAILURE;
            }
            errCode = clEnqueueWriteBuffer(command, input, CL_TRUE, 0, sizeof(RGB) * img->size, img->data, 0, NULL, NULL);
            // check
            if (errCode != CL_SUCCESS){
                std::cout << "write to input error" << std::endl;
                return EXIT_FAILURE;
            }
            
            errCode = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
            errCode = clSetKernelArg(kernel, 1, sizeof(int), &img->size);
            errCode = clSetKernelArg(kernel, 2, sizeof(cl_mem), &output_R);
            errCode = clSetKernelArg(kernel, 3, sizeof(cl_mem), &output_G);
            errCode = clSetKernelArg(kernel, 4, sizeof(cl_mem), &output_B);
            // check
            if (errCode != CL_SUCCESS){
                std::cout << "set argument error" << std::endl;
                return EXIT_FAILURE;
            }

            for (int i = 0; i < 2; i++){
                global[i] = (img->size % local[i] == 0)? \
                             img->size : img->size - (img->size % local[i]) + local[i];
            }
            errCode = clEnqueueNDRangeKernel(command, kernel, 1, 0, global, local, 0, NULL, NULL);
            // check
            if (errCode != CL_SUCCESS) {
                std::cout << "execute kernel error" << errCode <<std::endl;
                return EXIT_FAILURE;
            }
            errCode = clFinish(command);
            // check
            if (errCode != CL_SUCCESS){
                std::cout << "Finish error" << errCode << std::endl;
                return EXIT_FAILURE;
            }

            errCode = clEnqueueReadBuffer(command, output_R, CL_TRUE, 0, sizeof(unsigned int)*256, R, 0, 0, 0);
            errCode = clEnqueueReadBuffer(command, output_G, CL_TRUE, 0, sizeof(unsigned int)*256, G, 0, 0, 0);
            errCode = clEnqueueReadBuffer(command, output_B, CL_TRUE, 0, sizeof(unsigned int)*256, B, 0, 0, 0);
            if (errCode != CL_SUCCESS) {
                std::cout << "read buffer error" <<" "<< errCode << std::endl;
                return EXIT_FAILURE;
            }
            
            int max = 0;
            for(int i=0;i<256;i++){
                //std::cout << R[i] <<" "<<G[i] <<" "<< B[i] <<std::endl;
                max = R[i] > max ? R[i] : max;
                max = G[i] > max ? G[i] : max;
                max = B[i] > max ? B[i] : max;
            }

            Image *ret = new Image();
            ret->type = 1;
            ret->height = 256;
            ret->weight = 256;
            ret->size = 256 * 256;
            ret->data = new RGB[256 * 256]{};

            for(int i=0;i<ret->height;i++){
                for(int j=0;j<256;j++){
                    if(R[j]*256/max > i)
                        ret->data[256*i+j].R = 255;
                    if(G[j]*256/max > i)
                        ret->data[256*i+j].G = 255;
                    if(B[j]*256/max > i)
                        ret->data[256*i+j].B = 255;
                }
            }

            std::string newfile = "hist_" + std::string(filename); 
            writebmp(newfile.c_str(), ret);

            clReleaseProgram(program);
            clReleaseKernel(kernel);
            clReleaseMemObject(input);
            clReleaseMemObject(output_R);
            clReleaseMemObject(output_G);
            clReleaseMemObject(output_B);
        }
        
        clReleaseCommandQueue(command);
        clReleaseContext(context);
    }
    else{
        printf("Usage: ./hist <img.bmp> [img2.bmp ...]\n");
    }

    return 0;
}