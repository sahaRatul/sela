#ifndef _LPC_H_
#define _LPC_H_

#define MAX_LPC_ORDER 32

int compute_lpc_coefs_est(double *autoc, int max_order, double lpc[][MAX_LPC_ORDER]);
void levinson(double *autoc, int max_order, double *ref,double lpc[][MAX_LPC_ORDER]);
void acf(double *x, long N, long k, short norm, double *rxx);
void calc_residue(const double *samples,int N,double *lpc,short order,double *residue);
void calc_original(const double *residue,int N,double *lpc,short order,double *estimate);
short GetSignalRA(double *d, long N, double *cof, short P, double *x);

#endif
