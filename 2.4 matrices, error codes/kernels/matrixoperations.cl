__kernel void multiply(__global float4* buffer1, __global float4* buffer2, __global float4* result, int n)
{    
    float4 a = buffer1[get_global_id(0)]; 
    float4 b = buffer2[get_global_id(0)];
    float4 value;
    for(int i=0; i<4; i++){
        value[i]=a[i]*b[i];
    }
    if (get_global_id(0) < n) {
        result[get_global_id(0)] = value;
    }

}

__kernel void trans(__global float4* buffer1, __global float4* buffer2, __global float4* result, int n)
{    
    int ncols=2;
    float4 a = buffer1[get_global_id(0)]; 
    float4 b = buffer2[get_global_id(0)];
    float4 value = (float4)(0.0f); 
    for(int i=0; i<length(a); i++){
        int row=i/ncols;
        int col=i%ncols;
        value[i]=a[col*ncols+row];
    }
    if (get_global_id(0) < n) {
        result[get_global_id(0)] = value;
    }

}