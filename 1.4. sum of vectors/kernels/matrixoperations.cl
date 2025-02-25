__kernel void trans(__global float2* buffer1, __global float2* buffer2, __global float2* result, int n)
{    
    float2 a = buffer1[get_global_id(0)]; 
    float2 b = buffer2[get_global_id(0)];
    float sum1=a.s0+b.s0;
    float sum2=a.s1+b.s1;
    float2 value = (float2)(sum1, sum2);
    if (get_global_id(0) < n) {
        result[get_global_id(0)] = value;
    }

}