#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>

#include "lpc.h"

int32_t check_if_constant(const int16_t *data,int32_t num_elements)
{
	int16_t temp = data[0];

	for(int32_t i = 1; i < num_elements; i++)
	{
		if(temp != data[i])
			return -1;
	}

	return 0;
}

/*autocorrelation function*/
void acf(double *x,int32_t N,int64_t k,int16_t norm,double *rxx)
{
	int64_t i, n;
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
void levinson(double *autoc,uint8_t max_order,double *ref,double lpc[][MAX_LPC_ORDER])
{
	int32_t i, j, i2;
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
uint8_t compute_ref_coefs(double *autoc,uint8_t max_order,double *ref)
{
	int32_t i, j;
	double error;
	double gen[2][MAX_LPC_ORDER];
	uint8_t order_est;

	/*Schurr recursion*/
	for(i=0; i < max_order; i++)
		gen[0][i] = gen[1][i] = autoc[i+1];

	error = autoc[0];
	ref[0] = -gen[1][0] / error;
	error += gen[1][0] * ref[0];
	for(i=1; i<max_order; i++)
	{
		for(j = 0; j < max_order - i; j++)
		{
			gen[1][j] = gen[1][j+1] + ref[i-1] * gen[0][j];
			gen[0][j] = gen[1][j+1] * ref[i-1] + gen[0][j];
		}
		ref[i] = -gen[1][0] / error;
		error += gen[1][0] * ref[i];
	}

	/*Estimate optimal order using reflection coefficients*/
	order_est = 1;
	for(i = max_order - 1; i >= 0; i--)
	{
		if(fabs(ref[i]) > 0.05)
		{
			order_est = i+1;
			break;
		}
	}

	return(order_est);
}

/*Quantize reflection coeffs*/
int32_t qtz_ref_cof(double *par,uint8_t ord,int32_t *q_ref)
{
	for(int32_t i = 0; i < ord; i++)
	{
		if(i == 0)
			q_ref[i] = floor(64 * (-1 + (SQRT2 * sqrt(par[i]  + 1))));
		else if(i == 1)
			q_ref[i] = floor(64 * (-1 + (SQRT2 * sqrt(-par[i] + 1))));
		else
			q_ref[i] = floor(64 * par[i]);
	}
	return(0);
}

/*Dequantize reflection coefficients*/
int32_t dqtz_ref_cof(const int32_t *q_ref,uint8_t ord,double *ref)
{
	int32_t spar[MAX_LPC_ORDER];
	double temp;

	if(ord <= 1)
	{
		ref[0] = 0;
		return(0);
	}

	for(int32_t i = 0; i < ord; i++)
	{
		if(i == 0)
		{
			temp = (((double)q_ref[i]/64) + 1)/SQRT2;
			temp *= temp;
			ref[i] = temp - 1;
		}
		else if(i == 1)
		{
			temp = (((double)q_ref[i]/64) + 1)/SQRT2;
			temp *= temp;
			ref[i] = 1 - temp;
		}
		else
			ref[i] = (double)q_ref[i]/64;
	}

	return(0);
}

/*Calculate residues from samples and lpc coefficients*/
void calc_residue(const int32_t *samples,int64_t N,int16_t ord,int16_t Q,int64_t *coff,int32_t *residues)
{
	int64_t k, i;
	int64_t corr;
	int64_t y;

	corr = ((int64_t)1) << (Q - 1);//Correction term

	residues[0] = samples[0];
	for(k = 1; k <= ord; k++)
	{
		y = corr;
		for(i = 1; i <= k; i++)
		{
			y += (int64_t)coff[i] * samples[k-i];
			if((k - i) == 0)
				break;
		}
		residues[k] = samples[k] - (int32_t)(y >> Q);
	}

	for(k = ord + 1; k < N; k++)
	{
		y = corr;
		for(i = 0; i <= ord; i++)
			y += coff[i] * samples[k-i];
		residues[k] = samples[k] - (int32_t)(y >> Q);
	}
}

/*Calculate samples from residues and lpc coefficients*/
void calc_signal(const int32_t *residues,int64_t N,int16_t ord,int16_t Q,int64_t *coff,int32_t *samples)
{
	int64_t k, i;
	int64_t corr;
	int64_t y;

	corr = ((int64_t)1) << (Q - 1);//Correction term

	samples[0] = residues[0];
	for(k = 1; k <= ord; k++)
	{
		y = corr;
		for(i = 1; i <= k; i++)
		{
			y -= (int64_t)(coff[i] * samples[k-i]);
			if((k - i) == 0)
				break;
		}
		samples[k] = residues[k] - (int32_t)(y >> Q);
	}

	for(k = ord + 1; k < N; k++)
	{
		y = corr;
		for(i = 0; i <= ord; i++)
			y -= coff[i] * samples[k-i];
		samples[k] = residues[k] - (int32_t)(y >> Q);
	}
}
