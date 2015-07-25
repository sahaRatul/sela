#ifndef _LPC_H_
#define _LPC_H_

#define SQRT2 1.41421356237
#define MAX_LPC_ORDER 60

void acf(double *x,int32_t N,int64_t k,int16_t norm,double *rxx);
void levinson(double *autoc,uint8_t max_order,double *ref,double lpc[][MAX_LPC_ORDER]);
uint8_t compute_ref_coefs(double *autoc,uint8_t max_order,double *ref);
int32_t qtz_ref_cof(double *par,uint8_t ord,int32_t *q_ref);
int32_t dqtz_ref_cof(const int32_t *q_ref,uint8_t ord,int16_t Q,double *ref);
void calc_residue(const int32_t *samples,int64_t N,int16_t ord,int16_t Q,int32_t *coff,int32_t *residues);
void calc_signal(const int32_t *residues,int64_t N,int16_t ord,int16_t Q,int32_t *coff,int32_t *samples);

#endif
