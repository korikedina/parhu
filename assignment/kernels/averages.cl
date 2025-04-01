__kernel void avg(__global short4* buffer1, __global short4* buffer2, __global short4* result)
{    
    short4 left = buffer1[get_global_id(0)]; 
    printf("l %u %u", left[0], left[0]);
    short4 right = buffer2[get_global_id(0)];
    printf("r %u %u", right[0], right[0]);
    short4 subs;
    short value;
    for(int i=0; i<4; i++){
        subs[i]=left[i]-right[i];
        printf("h %d %d. %u %u\n", i, i, subs[i], subs[i]);
    }
    for(int i=0; i<4; i++){
        value+=subs[i];
    }
    value/=4;
    if (get_global_id(0) < 4) {
        result[get_global_id(0)] = value;
    }

}
