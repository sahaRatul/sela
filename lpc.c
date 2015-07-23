#include <stdio.h>
#include <malloc.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>

#include "lpc.h"

const int scale_table[]={
                            -1048544,-1048288,-1047776,-1047008,-1045984,-1044704,
                            -1043168,-1041376,-1039328,-1037024,-1034464,-1031648,
                            -1028576,-1025248,-1021664,-1017824,-1013728,-1009376,
                            -1004768,-999904,-994784,-989408,-983776,-977888,
                            -971744,-965344,-958688,-951776,-944608,-937184,
                            -929504,-921568,-913376,-904928,-896224,-887264,
                            -878048,-868576,-858848,-848864,-838624,-828128,
                            -817376,-806368,-795104,-783584,-771808,-759776,
                            -747488,-734944,-722144,-709088,-695776,-682208,
                            -668384,-654304,-639968,-625376,-610528,-595424,
                            -580064,-564448,-548576,-532448,-516064,-499424,
                            -482528,-465376,-447968,-430304,-412384,-394208,
                            -375776,-357088,-338144,-318944,-299488,-279776,
                            -259808,-239584,-219104,-198368,-177376,-156128,
                            -134624,-112864,-90848,-68576,-46048,-23268,-224,
                            23072,46624,70432,94496,118816,143392,168224,
                            193312,218656,244256,270112,296224,322592,349216,
                            376096,403232,430624,458272,486176,514336,542752,
                            571424,600352,629536,658976,688672,718624,748832,
                            779296,810016,840992,872224,903712,935456,967456,
                            999712,1032224
                        };

/*autocorrelation function*/
void acf(double *x,int N,long k,short norm,double *rxx)
{
    long i, n;
    double sum = 0,mean = 0;
    
    for (i = 0; i < N; i++)
    	sum += x[i];
    mean = sum/N;

    for(i = 0; i <= k; i++)
    {
        rxx[i] = 0.0;
        for (n = i; n < N; n++)
            rxx[i] += (x[n] - mean) * (x[n-i] - mean);
    }

    if(norm)
    {
        for (i = 1; i <= k; i++)
            rxx[i] /= rxx[0];
        rxx[0] = 1.0;
    }
}

/*Levinson recursion algorithm*/
void levinson(double *autoc,unsigned char max_order,double *ref,double lpc[][MAX_LPC_ORDER])
{
    int i, j, i2;
    double r, err, tmp;
    double lpc_tmp[MAX_LPC_ORDER];

    for(i = 0; i < max_order; i++)
        lpc_tmp[i] = 0;
    err = 1.0;
    if(autoc)
        err = autoc[0];

    for(i = 0; i < max_order; i++)
    {
        if(ref)
            r = ref[i];
        else
        {
            r = -autoc[i+1];
            for(j = 0; j<i; j++)
                r -= lpc_tmp[j] * autoc[i-j];
            r /= err;
            err *= 1.0 - (r * r);
        }

        i2 = i >> 1;
        lpc_tmp[i] = r;
        for(j = 0; j < i2; j++)
        {
            tmp = lpc_tmp[j];
            lpc_tmp[j] += r * lpc_tmp[i-1-j];
            lpc_tmp[i-1-j] += r * tmp;
        }
        if(i & 1)
            lpc_tmp[j] += lpc_tmp[j] * r;

        for(j = 0; j <= i; j++)
            lpc[i][j] = -lpc_tmp[j];
    }
}

/*Calculate reflection coefficients*/
unsigned char compute_ref_coefs(double *autoc,unsigned char max_order,double *ref)
{
    int i, j;
    double error;
    double gen[2][MAX_LPC_ORDER];
    unsigned char order_est;

    /*Schurr recursion*/
    for(i=0; i < max_order; i++)
        gen[0][i] = gen[1][i] = autoc[i+1];

    error = autoc[0];
    ref[0] = -gen[1][0] / error;
    error += gen[1][0] * ref[0];
    for(i=1; i<max_order; i++)
    {
        for(j=0; j < max_order - i; j++)
        {
            gen[1][j] = gen[1][j+1] + ref[i-1] * gen[0][j];
            gen[0][j] = gen[1][j+1] * ref[i-1] + gen[0][j];
        }
        ref[i] = -gen[1][0] / error;
        error += gen[1][0] * ref[i];
    }
    
	#ifdef __DEBUG__
		for(i = 0;i < max_order; i++)
			fprintf(stderr,"REF[%d] = %f\n",i,ref[i]);
	#endif

    /*Estimate optimal order using reflection coefficients*/
    order_est = 1;
    for(i = max_order-1; i >= 0; i--)
    {
        if(fabs(ref[i]) > 0.10)
        {
            order_est = i+1;
            break;
        }
    }

    return(order_est);
}

/*Quantize reflection coeffs*/
int qtz_ref_cof(double *ref,unsigned char ord,short *q_ref)
{
	for(int i = 0; i < ord; i++)
	{
		if(i == 0)
			q_ref[i] = 64 * (-1 + SQRT2 * sqrt(ref[i]  + 1));
		else if(i == 1)
			q_ref[i] = 64 * (-1 + SQRT2 * sqrt(-ref[i] + 1));
		else
			q_ref[i] = 64 * ref[i];
	}
	return(0);
}

/*Dequantize reflection coefficients*/
int dqtz_ref_cof(const short *q_ref,unsigned char ord,short Q,double *ref)
{
    int spar[MAX_LPC_ORDER];
    int corr = 1 << 20;
    for(int i = 0; i < ord; i++)
    {
        if(i == 0)
            spar[i] = scale_table[q_ref[i] + 64];
        else if(i == 1)
            spar[i] = -scale_table[q_ref[i] + 64];
        else
            spar[i] = q_ref[i] * (1 << (Q - 6)) + (1 << (Q - 7));
    }

    for(int i = 0; i < ord; i++)
        ref[i] = ((double)spar[i])/corr;

    return(0);
}

void calc_residue(const int *samples,long N,short ord,short Q,int *coff,short *residues)
{
    long k, i;
    int corr;
    int64_t y;

    corr = 1 << (Q - 1);
    residues[0] = samples[0];

    for(k = 0; k < N; k++)
    {
        y = corr;
        for(i = 0; i <= ord; i++)
        {
            y += (int64_t)coff[i] * samples[k-i];
            if((k - i) == 0)
                break;
        }
        residues[k] = samples[k] - (int)(y >> Q);
    }
}

void calc_signal(const short *residues,long N,short ord,short Q,int *coff,int *samples)
{
    long k, i;
    int corr;
    int64_t y;

    corr = 1 << (Q - 1);
    samples[0] = residues[0];

    for(k = 0; k < N; k++)
    {
        y = corr;
        for(i = 0; i <= ord; i++)
        {
            y -= (int64_t)coff[i] * samples[k-i];
            if((k - i) == 0)
                break;
        }
        samples[k] = residues[k] - (int)(y >> Q);
    }
}
