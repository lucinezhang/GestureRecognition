/*
**      File:   baumwelch.cpp
**      ���ܣ����ݸ����Ĺ۲����У���BaumWelch�㷨����HMMģ�Ͳ���
*/

#include <stdio.h> 
#include "nrutil.h"
#include "hmm.h"
#include <math.h>

#define DELTA 0.001

/******************************************************************************
**�������ƣ�BaumWelch
**���ܣ�BaumWelch�㷨
**������phmm��HMMģ��ָ��
**      T���۲����г���
**      O���۲�����
**      alpha��beta��gamma��pniter��Ϊ�м����
**      plogprobinit����ʼ����
**      plogprobfinal: ���ո���
**/ 
void BaumWelch(HMM *phmm, int T, int *O, double **alpha, double **beta,
	double **gamma, int *pniter, 
	double *plogprobinit, double *plogprobfinal)
{
	int	i, j, k;
	int	t, l = 0;

	double	logprobf, logprobb;
	double	numeratorA, denominatorA;
	double	numeratorB, denominatorB;

	double ***xi, *scale;
	double delta, deltaprev, logprobprev;

	deltaprev = 10e-70;

	xi = AllocXi(T, phmm->N);
	scale = dvector(1, T);

	ForwardWithScale(phmm, T, O, alpha, scale, &logprobf);
	*plogprobinit = logprobf; /* log P(O |��ʼ״̬) */
	BackwardWithScale(phmm, T, O, beta, scale, &logprobb);
	ComputeGamma(phmm, T, alpha, beta, gamma);
	ComputeXi(phmm, T, O, alpha, beta, xi);
	logprobprev = logprobf;

	do  {	

		/* ���¹��� t=1 ʱ��״̬Ϊi ��Ƶ�� */
		for (i = 1; i <= phmm->N; i++) 
			phmm->pi[i] = .001 + .999*gamma[1][i];

		/* ���¹���ת�ƾ���͹۲���� */
		for (i = 1; i <= phmm->N; i++) 
		{ 
			denominatorA = 0.0;
			for (t = 1; t <= T - 1; t++) 
				denominatorA += gamma[t][i];

			for (j = 1; j <= phmm->N; j++) 
			{
				numeratorA = 0.0;
				for (t = 1; t <= T - 1; t++) 
					numeratorA += xi[t][i][j];
				phmm->A[i][j] = .001 + 
						.999*numeratorA/denominatorA;
			}

			denominatorB = denominatorA + gamma[T][i]; 
			for (k = 1; k <= phmm->M; k++) 
			{
				numeratorB = 0.0;
				for (t = 1; t <= T; t++) 
				{
					if (O[t] == k) 
						numeratorB += gamma[t][i];
				}

				phmm->B[i][k] = .001 +
						.999*numeratorB/denominatorB;
			}
		}

		ForwardWithScale(phmm, T, O, alpha, scale, &logprobf);
		BackwardWithScale(phmm, T, O, beta, scale, &logprobb);
		ComputeGamma(phmm, T, alpha, beta, gamma);
		ComputeXi(phmm, T, O, alpha, beta, xi);

		/* ��������ֱ�ӵĸ��ʲ� */
		delta = logprobf - logprobprev; 
		logprobprev = logprobf;
		l++;
	}
	while (delta > DELTA); /* �����Ĳ�̫�󣬱����������˳� */ 
 
	*pniter = l;
	*plogprobfinal = logprobf; /* log P(O|estimated model) */
	FreeXi(xi, T, phmm->N);
	free_dvector(scale, 1, T);
}

/************************************************************************
**���û��ӿں���
**/
void ComputeGamma(HMM *phmm, int T, double **alpha, double **beta, 
	double **gamma)
{

	int 	i, j;
	int	t;
	double	denominator;

	for (t = 1; t <= T; t++) 
	{
		denominator = 0.0;
		for (j = 1; j <= phmm->N; j++) 
		{
			gamma[t][j] = alpha[t][j]*beta[t][j];
			denominator += gamma[t][j];
		}

		for (i = 1; i <= phmm->N; i++) 
			gamma[t][i] = gamma[t][i]/denominator;
	}
}

/************************************************************************
**���û��ӿں���
**/
void ComputeXi(HMM* phmm, int T, int *O, double **alpha, double **beta, 
	double ***xi)
{
	int i, j;
	int t;
	double sum;

	for (t = 1; t <= T - 1; t++) {
		sum = 0.0;	
		for (i = 1; i <= phmm->N; i++) 
			for (j = 1; j <= phmm->N; j++) 
			{
				xi[t][i][j] = alpha[t][i]*beta[t+1][j]
					*(phmm->A[i][j])
					*(phmm->B[j][O[t+1]]);
				sum += xi[t][i][j];
			}

		for (i = 1; i <= phmm->N; i++) 
			for (j = 1; j <= phmm->N; j++) 
				xi[t][i][j]  /= sum;
	}
}

/************************************************************************
**���û��ӿں���
**/
double *** AllocXi(int T, int N)
{
	int t;
	double ***xi;

	xi = (double ***) malloc(T*sizeof(double **));

	xi --;

	for (t = 1; t <= T; t++)
		xi[t] = dmatrix(1, N, 1, N);
	return xi;
}

/************************************************************************
**���û��ӿں���
**/
void FreeXi(double *** xi, int T, int N)
{
	int t;

	for (t = 1; t <= T; t++)
		free_dmatrix(xi[t], 1, N, 1, N);

	xi ++;
	free(xi);
}