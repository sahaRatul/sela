#ifndef _LPC_H_
#define _LPC_H_

#define SQRT2 1.41421356237
#define MAX_LPC_ORDER 60

void acf(double *x,int N,long k,short norm,double *rxx);
void levinson(double *autoc,unsigned char max_order,double *ref,double lpc[][MAX_LPC_ORDER]);
unsigned char compute_ref_coefs(double *autoc,unsigned char max_order,double *ref);
int qtz_ref_cof(double *par,unsigned char ord,short *q_ref);
int dqtz_ref_cof(const short *q_ref,unsigned char ord,short Q,double *ref);
void calc_residue(const int *samples,long N,short ord,short Q,int *coff,short *residues);
void calc_signal(const short *residues,long N,short ord,short Q,int *coff,int *samples);

#endif
