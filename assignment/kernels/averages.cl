__kernel void avg(__global short4* buffer1, __global short4* buffer2, __global short4* result)
{    
    short4 left = buffer1[get_global_id(0)]; 
    short4 right = buffer2[get_global_id(0)];
    short4 subs;
    short value;
    for(int i=0; i<4; i++){
        subs[i]=left[i]-right[i];
    }
    for(int i=0; i<4; i++){
        value+=subs[i];
    }
    value/=4;
    if (get_global_id(0) < 4) {
        result[get_global_id(0)] = value;
    }

}
