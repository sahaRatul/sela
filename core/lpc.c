#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>

#include "lpc.h"

//Lookup Tables for 1st, 2nd and higher order quantized reflection coeffs
static const double
lookup_1st_order_coeffs[128] =
{
	-1, -0.9998779296875, -0.99951171875, -0.9989013671875, -0.998046875, -0.9969482421875, -0.99560546875,
	-0.9940185546875, -0.9921875, -0.9901123046875, -0.98779296875, -0.9852294921875, -0.982421875, -0.9793701171875,
	-0.97607421875, -0.9725341796875, -0.96875, -0.9647216796875, -0.96044921875, -0.9559326171875, -0.951171875,
	-0.9461669921875, -0.94091796875, -0.9354248046875, -0.9296875, -0.9237060546875, -0.91748046875,
	-0.9110107421875, -0.904296875, -0.8973388671875, -0.89013671875, -0.8826904296875, -0.875, -0.8670654296875,
	-0.85888671875, -0.8504638671875, -0.841796875, -0.8328857421875, -0.82373046875, -0.8143310546875, -0.8046875,
	-0.7947998046875, -0.78466796875, -0.7742919921875, -0.763671875, -0.7528076171875, -0.74169921875,
	-0.7303466796875, -0.71875, -0.7069091796875, -0.69482421875000011, -0.6824951171875, -0.669921875,
	-0.6571044921875, -0.64404296875, -0.63073730468750011, -0.6171875, -0.6033935546875, -0.58935546875,
	-0.57507324218750011, -0.56054687500000011, -0.5457763671875, -0.53076171875, -0.5155029296875,
	-0.50000000000000011, -0.48425292968750011, -0.46826171875, -0.4520263671875, -0.43554687500000011,
	-0.41882324218750011, -0.40185546875000011, -0.38464355468750011, -0.3671875, -0.34948730468750011,
	-0.33154296875000011, -0.31335449218750011, -0.29492187500000022, -0.2762451171875, -0.25732421875000011,
	-0.23815917968750011, -0.21875000000000011, -0.19909667968750022, -0.17919921875, -0.15905761718750011,
	-0.13867187500000011, -0.11804199218750011, -0.097167968750000222, -0.076049804687500222, -0.054687500000000111,
	-0.033081054687500111, -0.011230468750000111, 0.010864257812499778, 0.033203125, 0.055786132812499778,
	0.07861328125, 0.1016845703125, 0.12499999999999978, 0.1485595703125, 0.17236328124999978, 0.1964111328125,
	0.22070312499999956, 0.24523925781249978, 0.27001953125, 0.29504394531249978, 0.3203125, 0.34582519531249956,
	0.37158203124999978, 0.3975830078125, 0.42382812499999978, 0.4503173828125, 0.47705078124999956,
	0.50402832031249978, 0.53125, 0.55871582031249978, 0.58642578125, 0.61437988281249956, 0.64257812499999978,
	0.6710205078125, 0.69970703124999956, 0.7286376953125, 0.75781249999999956, 0.78723144531249978, 0.81689453125,
	0.84680175781249956, 0.876953125, 0.90734863281249956, 0.93798828124999978, 0.9688720703125
};

static const double
lookup_2nd_order_coeffs[128] =
{
	-0.50000000000000011, 0.9998779296875, 0.99951171875, 0.9989013671875, 0.998046875, 0.9969482421875,
	0.99560546875, 0.9940185546875, 0.9921875, 0.9901123046875, 0.98779296875, 0.9852294921875, 0.982421875,
	0.9793701171875, 0.97607421875, 0.9725341796875, 0.96875, 0.9647216796875, 0.96044921875, 0.9559326171875,
	0.951171875, 0.9461669921875, 0.94091796875, 0.9354248046875, 0.9296875, 0.9237060546875, 0.91748046875,
	0.9110107421875, 0.904296875, 0.8973388671875, 0.89013671875, 0.8826904296875, 0.875, 0.8670654296875,
	0.85888671875, 0.8504638671875, 0.841796875, 0.8328857421875, 0.82373046875, 0.8143310546875, 0.8046875,
	0.7947998046875, 0.78466796875, 0.7742919921875, 0.763671875, 0.7528076171875, 0.74169921875, 0.7303466796875,
	0.71875, 0.7069091796875, 0.69482421875000011, 0.6824951171875, 0.669921875, 0.6571044921875, 0.64404296875,
	0.63073730468750011, 0.6171875, 0.6033935546875, 0.58935546875, 0.57507324218750011, 0.56054687500000011,
	0.5457763671875, 0.53076171875, 0.5155029296875, 0.50000000000000011, 0.48425292968750011, 0.46826171875,
	0.4520263671875, 0.43554687500000011, 0.41882324218750011, 0.40185546875000011, 0.38464355468750011, 0.3671875,
	0.34948730468750011, 0.33154296875000011, 0.31335449218750011, 0.29492187500000022, 0.2762451171875,
	0.25732421875000011, 0.23815917968750011, 0.21875000000000011, 0.19909667968750022, 0.17919921875,
	0.15905761718750011, 0.13867187500000011, 0.11804199218750011, 0.097167968750000222, 0.076049804687500222,
	0.054687500000000111, 0.033081054687500111, 0.011230468750000111, -0.010864257812499778, -0.033203125,
	-0.055786132812499778, -0.07861328125, -0.1016845703125, -0.12499999999999978, -0.1485595703125,
	-0.17236328124999978, -0.1964111328125, -0.22070312499999956, -0.24523925781249978, -0.27001953125,
	-0.29504394531249978, -0.3203125, -0.34582519531249956, -0.37158203124999978, -0.3975830078125,
	-0.42382812499999978, -0.4503173828125, -0.47705078124999956, -0.50402832031249978, -0.53125,
	-0.55871582031249978, -0.58642578125, -0.61437988281249956, -0.64257812499999978, -0.6710205078125,
	-0.69970703124999956, -0.7286376953125, -0.75781249999999956, -0.78723144531249978, -0.81689453125,
	-0.84680175781249956, -0.876953125, -0.90734863281249956, -0.93798828124999978, -0.9688720703125
};

static const double
lookup_higher_order_coeffs[128] =
{
	-1, -0.984375, -0.96875, -0.953125, -0.9375, -0.921875, -0.90625, -0.890625, -0.875, -0.859375, -0.84375,
	-0.828125, -0.8125, -0.796875, -0.78125, -0.765625, -0.75, -0.734375, -0.71875, -0.703125, -0.6875, -0.671875,
	-0.65625, -0.640625, -0.625, -0.609375, -0.59375, -0.578125, -0.5625, -0.546875, -0.53125, -0.515625, -0.5,
	-0.484375, -0.46875, -0.453125, -0.4375, -0.421875, -0.40625, -0.390625, -0.375, -0.359375, -0.34375, -0.328125,
	-0.3125, -0.296875, -0.28125, -0.265625, -0.25, -0.234375, -0.21875, -0.203125, -0.1875, -0.171875, -0.15625,
	-0.140625, -0.125, -0.109375, -0.09375, -0.078125, -0.0625, -0.046875, -0.03125, -0.015625, 0, 0.015625, 0.03125,
	0.046875, 0.0625, 0.078125, 0.09375, 0.109375, 0.125, 0.140625, 0.15625, 0.171875, 0.1875, 0.203125, 0.21875,
	0.234375, 0.25, 0.265625, 0.28125, 0.296875, 0.3125, 0.328125, 0.34375, 0.359375, 0.375, 0.390625, 0.40625,
	0.421875, 0.4375, 0.453125, 0.46875, 0.484375, 0.5, 0.515625, 0.53125, 0.546875, 0.5625, 0.578125, 0.59375,
	0.609375, 0.625, 0.640625, 0.65625, 0.671875, 0.6875, 0.703125, 0.71875, 0.734375, 0.75, 0.765625, 0.78125,
	0.796875, 0.8125, 0.828125, 0.84375, 0.859375, 0.875, 0.890625, 0.90625, 0.921875, 0.9375, 0.953125, 0.96875,0.984375
};

/*Checks if every sample of a block is equal or not*/
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
void auto_corr_fun(double *x,int32_t N,int64_t k,int16_t norm,double *rxx)
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

	return;
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
			for(j = 0; j < i; j++)
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

	return;
}

/*Fixed point levinson function*/
void levinson_fixed(int64_t *autocorr,uint8_t max_order,int64_t *ref,int64_t lpc[][MAX_LPC_ORDER])
{
	int32_t i, j, i2;
	int64_t r, err, tmp;
	int64_t lpc_tmp[MAX_LPC_ORDER];

	for(i = 0; i < max_order; i++)
		lpc_tmp[i] = 0;
	err = 1.0;
	if(autocorr)
		err = autocorr[0];

	for(i = 0; i < max_order; i++)
	{
		if(ref)
			r = ref[i];
		else
		{
			r = -autocorr[i+1];
			for(j = 0; j < i; j++)
				r -= lpc_tmp[j] * autocorr[i-j];
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

	return;
}

/*Calculate reflection coefficients*/
uint8_t compute_ref_coefs(double *autoc,uint8_t max_order,double *ref)
{
	int32_t i, j;
	double error;
	double gen[2][MAX_LPC_ORDER];
	uint8_t order_est;

	//Schurr recursion
	for(i = 0; i < max_order; i++)
		gen[0][i] = gen[1][i] = autoc[i+1];

	error = autoc[0];
	ref[0] = -gen[1][0] / error;
	error += gen[1][0] * ref[0];
	for(i = 1; i < max_order; i++)
	{
		for(j = 0; j < max_order - i; j++)
		{
			gen[1][j] = gen[1][j+1] + ref[i-1] * gen[0][j];
			gen[0][j] = gen[1][j+1] * ref[i-1] + gen[0][j];
		}
		ref[i] = -gen[1][0] / error;
		error += gen[1][0] * ref[i];
	}

	//Estimate optimal order using reflection coefficients
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
			ref[i] = lookup_1st_order_coeffs[q_ref[i] + 64];
		else if(i == 1)
			ref[i] = lookup_2nd_order_coeffs[q_ref[i] + 64];
		else
			ref[i] = lookup_higher_order_coeffs[q_ref[i] + 64];
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

		y += (int64_t)(coff[0] * samples[k - 0]);
		y += (int64_t)(coff[1] * samples[k - 1]);
		y += (int64_t)(coff[2] * samples[k - 2]);
		y += (int64_t)(coff[3] * samples[k - 3]);
		y += (int64_t)(coff[4] * samples[k - 4]);
		y += (int64_t)(coff[5] * samples[k - 5]);
		y += (int64_t)(coff[6] * samples[k - 6]);
		y += (int64_t)(coff[7] * samples[k - 7]);
		y += (int64_t)(coff[8] * samples[k - 8]);
		y += (int64_t)(coff[9] * samples[k - 9]);
		y += (int64_t)(coff[10] * samples[k - 10]);
		y += (int64_t)(coff[11] * samples[k - 11]);
		y += (int64_t)(coff[12] * samples[k - 12]);
		y += (int64_t)(coff[13] * samples[k - 13]);
		y += (int64_t)(coff[14] * samples[k - 14]);
		y += (int64_t)(coff[15] * samples[k - 15]);
		y += (int64_t)(coff[16] * samples[k - 16]);
		y += (int64_t)(coff[17] * samples[k - 17]);
		y += (int64_t)(coff[18] * samples[k - 18]);
		y += (int64_t)(coff[19] * samples[k - 19]);
		y += (int64_t)(coff[20] * samples[k - 20]);
		y += (int64_t)(coff[21] * samples[k - 21]);
		y += (int64_t)(coff[22] * samples[k - 22]);
		y += (int64_t)(coff[23] * samples[k - 23]);
		y += (int64_t)(coff[24] * samples[k - 24]);
		y += (int64_t)(coff[25] * samples[k - 25]);
		y += (int64_t)(coff[26] * samples[k - 26]);
		y += (int64_t)(coff[27] * samples[k - 27]);
		y += (int64_t)(coff[28] * samples[k - 28]);
		y += (int64_t)(coff[29] * samples[k - 29]);
		y += (int64_t)(coff[30] * samples[k - 30]);
		y += (int64_t)(coff[31] * samples[k - 31]);
		y += (int64_t)(coff[32] * samples[k - 32]);
		y += (int64_t)(coff[33] * samples[k - 33]);
		y += (int64_t)(coff[34] * samples[k - 34]);
		y += (int64_t)(coff[35] * samples[k - 35]);
		y += (int64_t)(coff[36] * samples[k - 36]);
		y += (int64_t)(coff[37] * samples[k - 37]);
		y += (int64_t)(coff[38] * samples[k - 38]);
		y += (int64_t)(coff[39] * samples[k - 39]);
		y += (int64_t)(coff[40] * samples[k - 40]);
		y += (int64_t)(coff[41] * samples[k - 41]);
		y += (int64_t)(coff[42] * samples[k - 42]);
		y += (int64_t)(coff[43] * samples[k - 43]);
		y += (int64_t)(coff[44] * samples[k - 44]);
		y += (int64_t)(coff[45] * samples[k - 45]);
		y += (int64_t)(coff[46] * samples[k - 46]);
		y += (int64_t)(coff[47] * samples[k - 47]);
		y += (int64_t)(coff[48] * samples[k - 48]);
		y += (int64_t)(coff[49] * samples[k - 49]);
		y += (int64_t)(coff[50] * samples[k - 50]);
		y += (int64_t)(coff[51] * samples[k - 51]);
		y += (int64_t)(coff[52] * samples[k - 52]);
		y += (int64_t)(coff[53] * samples[k - 53]);
		y += (int64_t)(coff[54] * samples[k - 54]);
		y += (int64_t)(coff[55] * samples[k - 55]);
		y += (int64_t)(coff[56] * samples[k - 56]);
		y += (int64_t)(coff[57] * samples[k - 57]);
		y += (int64_t)(coff[58] * samples[k - 58]);
		y += (int64_t)(coff[59] * samples[k - 59]);
		y += (int64_t)(coff[60] * samples[k - 60]);
		y += (int64_t)(coff[61] * samples[k - 61]);
		y += (int64_t)(coff[62] * samples[k - 62]);
		y += (int64_t)(coff[63] * samples[k - 63]);
		y += (int64_t)(coff[64] * samples[k - 64]);
		y += (int64_t)(coff[65] * samples[k - 65]);
		y += (int64_t)(coff[66] * samples[k - 66]);
		y += (int64_t)(coff[67] * samples[k - 67]);
		y += (int64_t)(coff[68] * samples[k - 68]);
		y += (int64_t)(coff[69] * samples[k - 69]);
		y += (int64_t)(coff[70] * samples[k - 70]);
		y += (int64_t)(coff[71] * samples[k - 71]);
		y += (int64_t)(coff[72] * samples[k - 72]);
		y += (int64_t)(coff[73] * samples[k - 73]);
		y += (int64_t)(coff[74] * samples[k - 74]);
		y += (int64_t)(coff[75] * samples[k - 75]);
		y += (int64_t)(coff[76] * samples[k - 76]);
		y += (int64_t)(coff[77] * samples[k - 77]);
		y += (int64_t)(coff[78] * samples[k - 78]);
		y += (int64_t)(coff[79] * samples[k - 79]);
		y += (int64_t)(coff[80] * samples[k - 80]);
		y += (int64_t)(coff[81] * samples[k - 81]);
		y += (int64_t)(coff[82] * samples[k - 82]);
		y += (int64_t)(coff[83] * samples[k - 83]);
		y += (int64_t)(coff[84] * samples[k - 84]);
		y += (int64_t)(coff[85] * samples[k - 85]);
		y += (int64_t)(coff[86] * samples[k - 86]);
		y += (int64_t)(coff[87] * samples[k - 87]);
		y += (int64_t)(coff[88] * samples[k - 88]);
		y += (int64_t)(coff[89] * samples[k - 89]);
		y += (int64_t)(coff[90] * samples[k - 90]);
		y += (int64_t)(coff[91] * samples[k - 91]);
		y += (int64_t)(coff[92] * samples[k - 92]);
		y += (int64_t)(coff[93] * samples[k - 93]);
		y += (int64_t)(coff[94] * samples[k - 94]);
		y += (int64_t)(coff[95] * samples[k - 95]);
		y += (int64_t)(coff[96] * samples[k - 96]);
		y += (int64_t)(coff[97] * samples[k - 97]);
		y += (int64_t)(coff[98] * samples[k - 98]);
		y += (int64_t)(coff[99] * samples[k - 99]);
		y += (int64_t)(coff[100] * samples[k - 100]);
		
		//for(i = 0; i <= ord; i++)
			//y += (int64_t)(coff[i] * samples[k-i]);
		residues[k] = samples[k] - (int32_t)(y >> Q);
	}

	return;
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

		y -= (int64_t)(coff[0] * samples[k - 0]);
		y -= (int64_t)(coff[1] * samples[k - 1]);
		y -= (int64_t)(coff[2] * samples[k - 2]);
		y -= (int64_t)(coff[3] * samples[k - 3]);
		y -= (int64_t)(coff[4] * samples[k - 4]);
		y -= (int64_t)(coff[5] * samples[k - 5]);
		y -= (int64_t)(coff[6] * samples[k - 6]);
		y -= (int64_t)(coff[7] * samples[k - 7]);
		y -= (int64_t)(coff[8] * samples[k - 8]);
		y -= (int64_t)(coff[9] * samples[k - 9]);
		y -= (int64_t)(coff[10] * samples[k - 10]);
		y -= (int64_t)(coff[11] * samples[k - 11]);
		y -= (int64_t)(coff[12] * samples[k - 12]);
		y -= (int64_t)(coff[13] * samples[k - 13]);
		y -= (int64_t)(coff[14] * samples[k - 14]);
		y -= (int64_t)(coff[15] * samples[k - 15]);
		y -= (int64_t)(coff[16] * samples[k - 16]);
		y -= (int64_t)(coff[17] * samples[k - 17]);
		y -= (int64_t)(coff[18] * samples[k - 18]);
		y -= (int64_t)(coff[19] * samples[k - 19]);
		y -= (int64_t)(coff[20] * samples[k - 20]);
		y -= (int64_t)(coff[21] * samples[k - 21]);
		y -= (int64_t)(coff[22] * samples[k - 22]);
		y -= (int64_t)(coff[23] * samples[k - 23]);
		y -= (int64_t)(coff[24] * samples[k - 24]);
		y -= (int64_t)(coff[25] * samples[k - 25]);
		y -= (int64_t)(coff[26] * samples[k - 26]);
		y -= (int64_t)(coff[27] * samples[k - 27]);
		y -= (int64_t)(coff[28] * samples[k - 28]);
		y -= (int64_t)(coff[29] * samples[k - 29]);
		y -= (int64_t)(coff[30] * samples[k - 30]);
		y -= (int64_t)(coff[31] * samples[k - 31]);
		y -= (int64_t)(coff[32] * samples[k - 32]);
		y -= (int64_t)(coff[33] * samples[k - 33]);
		y -= (int64_t)(coff[34] * samples[k - 34]);
		y -= (int64_t)(coff[35] * samples[k - 35]);
		y -= (int64_t)(coff[36] * samples[k - 36]);
		y -= (int64_t)(coff[37] * samples[k - 37]);
		y -= (int64_t)(coff[38] * samples[k - 38]);
		y -= (int64_t)(coff[39] * samples[k - 39]);
		y -= (int64_t)(coff[40] * samples[k - 40]);
		y -= (int64_t)(coff[41] * samples[k - 41]);
		y -= (int64_t)(coff[42] * samples[k - 42]);
		y -= (int64_t)(coff[43] * samples[k - 43]);
		y -= (int64_t)(coff[44] * samples[k - 44]);
		y -= (int64_t)(coff[45] * samples[k - 45]);
		y -= (int64_t)(coff[46] * samples[k - 46]);
		y -= (int64_t)(coff[47] * samples[k - 47]);
		y -= (int64_t)(coff[48] * samples[k - 48]);
		y -= (int64_t)(coff[49] * samples[k - 49]);
		y -= (int64_t)(coff[50] * samples[k - 50]);
		y -= (int64_t)(coff[51] * samples[k - 51]);
		y -= (int64_t)(coff[52] * samples[k - 52]);
		y -= (int64_t)(coff[53] * samples[k - 53]);
		y -= (int64_t)(coff[54] * samples[k - 54]);
		y -= (int64_t)(coff[55] * samples[k - 55]);
		y -= (int64_t)(coff[56] * samples[k - 56]);
		y -= (int64_t)(coff[57] * samples[k - 57]);
		y -= (int64_t)(coff[58] * samples[k - 58]);
		y -= (int64_t)(coff[59] * samples[k - 59]);
		y -= (int64_t)(coff[60] * samples[k - 60]);
		y -= (int64_t)(coff[61] * samples[k - 61]);
		y -= (int64_t)(coff[62] * samples[k - 62]);
		y -= (int64_t)(coff[63] * samples[k - 63]);
		y -= (int64_t)(coff[64] * samples[k - 64]);
		y -= (int64_t)(coff[65] * samples[k - 65]);
		y -= (int64_t)(coff[66] * samples[k - 66]);
		y -= (int64_t)(coff[67] * samples[k - 67]);
		y -= (int64_t)(coff[68] * samples[k - 68]);
		y -= (int64_t)(coff[69] * samples[k - 69]);
		y -= (int64_t)(coff[70] * samples[k - 70]);
		y -= (int64_t)(coff[71] * samples[k - 71]);
		y -= (int64_t)(coff[72] * samples[k - 72]);
		y -= (int64_t)(coff[73] * samples[k - 73]);
		y -= (int64_t)(coff[74] * samples[k - 74]);
		y -= (int64_t)(coff[75] * samples[k - 75]);
		y -= (int64_t)(coff[76] * samples[k - 76]);
		y -= (int64_t)(coff[77] * samples[k - 77]);
		y -= (int64_t)(coff[78] * samples[k - 78]);
		y -= (int64_t)(coff[79] * samples[k - 79]);
		y -= (int64_t)(coff[80] * samples[k - 80]);
		y -= (int64_t)(coff[81] * samples[k - 81]);
		y -= (int64_t)(coff[82] * samples[k - 82]);
		y -= (int64_t)(coff[83] * samples[k - 83]);
		y -= (int64_t)(coff[84] * samples[k - 84]);
		y -= (int64_t)(coff[85] * samples[k - 85]);
		y -= (int64_t)(coff[86] * samples[k - 86]);
		y -= (int64_t)(coff[87] * samples[k - 87]);
		y -= (int64_t)(coff[88] * samples[k - 88]);
		y -= (int64_t)(coff[89] * samples[k - 89]);
		y -= (int64_t)(coff[90] * samples[k - 90]);
		y -= (int64_t)(coff[91] * samples[k - 91]);
		y -= (int64_t)(coff[92] * samples[k - 92]);
		y -= (int64_t)(coff[93] * samples[k - 93]);
		y -= (int64_t)(coff[94] * samples[k - 94]);
		y -= (int64_t)(coff[95] * samples[k - 95]);
		y -= (int64_t)(coff[96] * samples[k - 96]);
		y -= (int64_t)(coff[97] * samples[k - 97]);
		y -= (int64_t)(coff[98] * samples[k - 98]);
		y -= (int64_t)(coff[99] * samples[k - 99]);
		y -= (int64_t)(coff[100] * samples[k - 100]);
		
		//for(i = 0; i <= ord; i++)
			//y -= (int64_t)(coff[i] * samples[k-i]);
		samples[k] = residues[k] - (int32_t)(y >> Q);
	}

	return;
}
