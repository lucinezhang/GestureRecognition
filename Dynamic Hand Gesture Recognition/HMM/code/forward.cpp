/*
**      File:   backward.cpp
**      ���ܣ������۲�ֵ���к�HMMģ�ͣ�����ǰ������㷨
**            ��ȡ�����
*/

#include <stdio.h>
#include "hmm.h"

/***************************************************************************
** �������ƣ�ForwardWithScale
** ���ܣ���ǰ�㷨���Ʋ���������������������
** ������phmm��ָ��HMM��ָ��
**       T���۲�ֵ���еĳ���
**       O���۲�ֵ����
**       beta���������õ�����ʱ����
**       scale��������������
**       pprob������ֵ����Ҫ��ĸ���
*****************************************************************************/
void ForwardWithScale(HMM *phmm, int T, int *O, double **alpha, 
	double *scale, double *pprob)
{
	int     i, j;   /* ״ָ̬ʾ */
	int     t;      /* ʱ���±� */
	double sum;
 
 
	/* 1. ��ʼ�� */
	scale[1] = 0.0;
	for(i = 1; i <= phmm->N; i ++)
	{
		alpha[1][i] = phmm->pi[i]*(phmm->B[i][O[1]]);
		scale[i] += alpha[1][i];
	}
	for (i = 1; i <= phmm->N; i++)
		alpha[1][i] = 1.0/scale[1]; 
 
	/* 2. �ݹ� */
	for (t = 1; t < T; t++) 
	{
		scale[t+1] = 0.0;
		for (j = 1; j <= phmm->N; j++) 
		{
			sum = 0.0;
			for (i = 1; i <= phmm->N; i++)
				sum += phmm->A[i][j] * alpha[t][i];
			alpha[t+1][j] = sum*(phmm->B[j][O[t+1]]);
			scale[t+1]+=alpha[t+1][j];
		}
		for(j = 1; j <= phmm->N; j ++)
			alpha[t+1][j]/=scale[t+1];
	}
		/* 3. ��ֹ */
	*pprob = 0.0;
	for (t = 1; t<T; t++)
		*pprob += log(scale[t]);

}
