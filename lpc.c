#include <math.h>
#include <stdio.h>
#include "lpc.h"

//autocorrelation function
void acf(double *x, long N, long k, short norm, double *rxx)
{
    long i, n;
    double sum = 0,mean = 0;
    
    for (i = 0; i <= N; i++)
    	sum += x[i];
    mean = sum/N;

    for (i = 0; i <= k; i++)
    {
        rxx[i] = 0.0;
        for (n = i; n < N; n++)
            rxx[i] += (x[n] - mean) * (x[n-i] - mean);
    }

    if (norm)
    {
        for (i = 1; i <= k; i++)
            rxx[i] /= rxx[0];
        rxx[0] = 1.0;
    }
}

//levinson recursion algorithm(shamelessly copied from libflake)
void levinson(double *autoc, int max_order, double *ref,double lpc[][MAX_LPC_ORDER])
{
    int i, j, i2;
    double r, err, tmp;
    double lpc_tmp[MAX_LPC_ORDER];

    for(i=0; i<max_order; i++) lpc_tmp[i] = 0;
    err = 1.0;
    if(autoc)
        err = autoc[0];

    for(i=0; i<max_order; i++)
    {
        if(ref)
            r = ref[i];
        else
        {
            r = -autoc[i+1];
            for(j=0; j<i; j++)
                r -= lpc_tmp[j] * autoc[i-j];
            r /= err;
            err *= 1.0 - (r * r);
        }

        i2 = (i >> 1);
        lpc_tmp[i] = r;
        for(j=0; j<i2; j++)
        {
            tmp = lpc_tmp[j];
            lpc_tmp[j] += r * lpc_tmp[i-1-j];
            lpc_tmp[i-1-j] += r * tmp;
        }
        if(i & 1)
            lpc_tmp[j] += lpc_tmp[j] * r;

        for(j=0; j<=i; j++)
            lpc[i][j] = -lpc_tmp[j];
    }
}

int compute_lpc_coefs_est(double *autoc, int max_order, double lpc[][MAX_LPC_ORDER])
{
    int i, j;
    double error;
    double gen[2][MAX_LPC_ORDER];
    double ref[MAX_LPC_ORDER];
    int order_est;

    //Schurr recursion
    for(i=0; i<max_order; i++)
        gen[0][i] = gen[1][i] = autoc[i+1];
    error = autoc[0];
    ref[0] = -gen[1][0] / error;
    error += gen[1][0] * ref[0];
    for(i=1; i<max_order; i++)
    {
        for(j=0; j<max_order-i; j++)
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

    //Estimate optimal order using reflection coefficients
    order_est = 1;
    for(i=max_order-1; i>=0; i--)
    {
        if(fabs(ref[i]) > 0.10)
        {
            order_est = i+1;
            break;
        }
    }

    //Levinson recursion
    levinson(NULL,order_est,ref,lpc);

    return order_est;
}

void calc_residue(const double *samples,int N,double *lpc,short order,double *residue)
{
    int i,j,k;
    double y = 0;
    residue[0] = 0;

    residue[0] = samples[0];
    for (k = 0; k < N; k++)
    {
        for (i = 0; i < order; i++)
        {
            y += lpc[i] * samples[k-i];
            if((k-i)==0)
            	break;
        }
        residue[k+1] = samples[k+1] - y;
        y=0;
    }
}

void calc_original(const double *residue,int N,double *lpc,short order,double *samples)
{
    int i,j,k;
    double y = 0;

    samples[0] = residue[0];
    for (k = 0; k < N; k++)
    {
        for (i = 0; i < order; i++)
        {
            y -= lpc[i]*samples[k-i];
            if((k-i)==0)
            	break;
    	}
        samples[k+1] = residue[k+1] - y;
        y=0;
	}
}
