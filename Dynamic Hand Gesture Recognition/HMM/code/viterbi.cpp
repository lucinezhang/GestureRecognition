/*
**   File��Viterbi.cpp
**   ���ܣ�����HMM�͹۲����У�������ܵ�״̬
*/

#include <math.h>
#include "hmm.h"
#include "nrutil.h"

#define VITHUGE  100000000000.0

/**************************************************************************
** �������ƣ�Viterbi
** ���ܣ�Viterbi�㷨
** ������phmm��HMM�ṹָ��
**       T���۲�ֵ�ĸ���
**       O���۲�����
**       delta��psiΪ�м����
**       q����õ����״̬����
**       pprob������
****************************************************************************/
void Viterbi(HMM *phmm, int T, int *O, double **delta, int **psi, 
	int *q, double *pprob)
{
	int 	i, j;	/* ״̬�±� */
	int  	t;	/* ʱ���±� */	

	int	maxvalind;
	double	maxval, val;

	/* 1. ��ʼ��  */
	
	for (i = 1; i <= phmm->N; i++) 
	{
		delta[1][i] = phmm->pi[i] * (phmm->B[i][O[1]]);
		psi[1][i] = 0;
	}	

	/* 2. �ݹ� */
	
	for (t = 2; t <= T; t++) 
	{
		for (j = 1; j <= phmm->N; j++) 
		{
			maxval = 0.0;
			maxvalind = 1;	
			for (i = 1; i <= phmm->N; i++) 
			{
				val = delta[t-1][i]*(phmm->A[i][j]);
				if (val > maxval) {
					maxval = val;	
					maxvalind = i;	
				}
			}
			
			delta[t][j] = maxval*(phmm->B[j][O[t]]);
			psi[t][j] = maxvalind; 

		}
	}

	/* 3. ��ֹ */

	*pprob = 0.0;
	q[T] = 1;
	for (i = 1; i <= phmm->N; i++) 
	{
		if (delta[T][i] > *pprob) 
		{
			*pprob = delta[T][i];	
			q[T] = i;
		}
	}

	/* 4. Path (state sequence) backtracking */

	for (t = T - 1; t >= 1; t--)
		q[t] = psi[t+1][q[t+1]];

}

