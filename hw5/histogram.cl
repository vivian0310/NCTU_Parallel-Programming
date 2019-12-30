typedef struct tag_RGB
{ 
    uchar R;
    uchar G;
    uchar B;
    uchar align;
} RGB;

__kernel void histogram( 
    __global RGB *data,
    int imgSize,
    __global unsigned int *R,
    __global unsigned int *G,
    __global unsigned int *B)
{
    int globalId = get_global_id(0);

    if (globalId < imgSize){    
       RGB pixel = data[globalId]; 
       atomic_inc(R+pixel.R); 
       atomic_inc(G+pixel.G); 
       atomic_inc(B+pixel.B);
    }
}