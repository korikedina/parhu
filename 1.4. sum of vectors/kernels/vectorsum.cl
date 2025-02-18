
__kernel void vecsum_kernel(__global float2* buffer,int n)
{    
    float2 a=(float2)(1.0f,2.0f);
    float2 b=(float2)(3.0f,4.0f);
    float sum1=a.s0+b.s0;
    float sum2=a.s1+b.s1;
    float2 value = (float2)(sum1, sum2);
    printf("%f %f %f %f\n", value.s0, value.s0, value.s1, value.s1);
    printf("%f\n", a.s1);
    if (get_global_id(0) < n) {
        buffer[get_global_id(0)] = value;
    }

}