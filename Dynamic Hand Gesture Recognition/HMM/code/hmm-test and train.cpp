#include"hmm.h"
#include<iostream>
#include<cmath>
#include<fstream>
using namespace std;

#define PI 3.14159
#define sin_A1  sin(PI / 12.0)
#define cos_A1  cos(PI / 12.0)
#define sin_A2  sin(5 * PI / 12.0)
#define cos_A2  cos(5 * PI / 12.0)
#define sin_A3  sin(7 * PI / 12.0)
#define cos_A3  cos(7 * PI / 12.0)
#define sin_A4  sin(11 * PI / 12.0)
#define cos_A4  cos(11 * PI / 12.0)
#define sin_A5  sin(13 * PI / 12.0)
#define cos_A5  cos(13 * PI / 12.0)
#define sin_A6  sin(17 * PI / 12.0)
#define cos_A6  cos(17 * PI / 12.0)
#define sin_A7  sin(19 * PI / 12.0)
#define cos_A7  cos(19 * PI / 12.0)
#define sin_A8  sin(23 * PI / 12.0)
#define cos_A8  cos(23 * PI / 12.0)

#define N 12//隐藏状态
#define M 64//可观测状态

int hmm_test(long t[], double x_l[], double y_l[], double z_l[], double x_r[], double y_r[], double z_r[])
{

	int O[5000];

	HMM * hmm[10];
	char * hmmf[10] = { "hmm_world.txt", "hmm_country.txt", "hmm_sground.txt", "hmm_home.txt", "hmm_strong.txt", "hmm_rich.txt" };

	for(int f = 0; f < 6; ++f)
	{
		freopen(hmmf[f],"r",stdin);
		hmm[f] = new HMM;
		ReadHMM(stdin,hmm[f]);
	}

	//读入测试样例
	int i = 1;
	while(t[i] != 0)
	{
		double tempx_l = x_l[i] - x_l[i-1];
		double tempy_l = y_l[i] - y_l[i-1];
		double tempz_l = z_l[i] - z_l[i-1];

		double tempx_r = x_r[i] - x_r[i-1];
		double tempy_r = y_r[i] - y_r[i-1];
		double tempz_r = z_r[i] - z_r[i-1];

		double dist_l = sqrt(tempx_l * tempx_l + tempy_l * tempy_l);
		double dist_r = sqrt(tempx_r * tempx_r + tempy_r * tempy_r);

		double tempsin_l = tempx_l / dist_l;
		double tempcos_l = tempy_l / dist_l;

		double tempsin_r = tempx_r / dist_r;
		double tempcos_r = tempy_r / dist_r;


		int left_ob = 0, right_ob = 0;

		/*if ((sin_A8 <= tempsin_l) && (tempsin_l < sin_A1) && tempcos_l > 0)
			left_ob = 1;
		if ((sin_A1 <= tempsin_l) && (tempsin_l < sin_A2) && tempcos_l > 0)
			left_ob = 2;
		if (sin_A2 <= tempsin_l)
			left_ob = 3;
		if ((sin_A4 <= tempsin_l) && (tempsin_l < sin_A3) && tempcos_l < 0)
			left_ob = 4;
		if ((sin_A5 <= tempsin_l) && (tempsin_l < sin_A4) && tempcos_l < 0)
			left_ob = 5;
		if ((sin_A6 <= tempsin_l) && (tempsin_l < sin_A5) && tempcos_l < 0)
			left_ob = 6;
		if ((tempsin_l <= sin_A7) && tempsin_l < 0)
			left_ob = 7;
		if ((sin_A7 <= tempsin_l) && (tempsin_l < sin_A8) && tempcos_l > 0)
			left_ob = 8;

		if ((sin_A8 <= tempsin_r) && (tempsin_r < sin_A1) && tempcos_r > 0)
			right_ob = 1;
		if ((sin_A1 <= tempsin_r) && (tempsin_r < sin_A2) && tempcos_r > 0)
			right_ob = 2;
		if (sin_A2 <= tempsin_r)
			right_ob = 3;
		if ((sin_A4 <= tempsin_r) && (tempsin_r < sin_A3) && tempcos_r < 0)
			right_ob = 4;
		if ((sin_A5 <= tempsin_r) && (tempsin_r < sin_A4) && tempcos_r < 0)
			right_ob = 5;
		if ((sin_A6 <= tempsin_r) && (tempsin_r < sin_A5) && tempcos_r < 0)
			right_ob = 6;
		if ((tempsin_r <= sin_A7) && tempsin_r < 0)
			right_ob = 7;
		if ((sin_A7 <= tempsin_r) && (tempsin_r < sin_A8) && tempcos_r > 0)
			right_ob = 8;*/
		if(tempz_l >= 0)
		{
			if(tempy_l >= 0)
			{
				if(tempx_l >= 0)
				{
					left_ob = 1;
				}
				else
					left_ob = 2;
			}
			else
			{
				if(tempx_l >= 0)
				{
					left_ob = 3;
				}
				else
					left_ob = 4;
			}
		}
		else
		{
			if(tempy_l >= 0)
			{
				if(tempx_l >= 0)
				{
					left_ob = 5;
				}
				else
					left_ob = 6;
			}
			else
			{
				if(tempx_l >= 0)
				{
					left_ob = 7;
				}
				else
					left_ob = 8;
			}
		}
		if(tempz_r >= 0)
		{
			if(tempy_r >= 0)
			{
				if(tempx_r >= 0)
				{
					right_ob = 1;
				}
				else
					right_ob = 2;
			}
			else
			{
				if(tempx_r >= 0)
				{
					right_ob = 3;
				}
				else
					right_ob = 4;
			}
		}
		else
		{
			if(tempy_r >= 0)
			{
				if(tempx_r >= 0)
				{
					right_ob = 5;
				}
				else
					right_ob = 6;
			}
			else
			{
				if(tempx_r >= 0)
				{
					right_ob = 7;
				}
				else
					right_ob = 8;
			}
		}
		O[i] = left_ob * 8 - 8 + right_ob;
		i ++;
	}
	
	int T_test = i;
	int q[5000] = {0};
	int ** psi = new int* [T_test+1];
	for(int j = 0; j < T_test+1; j++)
		psi[j] = new int[T_test+1];
	double pf;
	double ** delta = new double* [T_test+1];
	for(int j = 0; j < T_test+1; j++)
		delta[j] = new double[T_test+1];

	//找到最大的概率以及对应的最可能手势轨迹
	double pmax = 0;
	int answer = -1;
	for(int f = 0; f < 6; f++)
	{
		Viterbi(hmm[f],T_test-1,O,delta,psi,q,&pf);
		if(pf > pmax)
		{
			pmax = pf;
			answer = f;
		}
	}

	fclose(stdin);
	return answer;

}

void hmm_train(char * infile, char* hmmfile,int train_samp_num)
{
	int T;
	int t[5000];
	int O[5000];
	double x_l[5000], y_l[5000], z_l[5000], x_r[5000], y_r[5000], z_r[5000];

	HMM * hmm[2];
	hmm[0] = new HMM;
	hmm[1] = new HMM;

	freopen(infile,"r",stdin);

	int sample_num = 0;

	for(sample_num = 0;sample_num < train_samp_num; sample_num ++)
	{
		cin >> t[0] >> x_l[0] >> y_l[0] >>z_l[0]>> x_r[0] >> y_r[0]>>z_r[0];
		int i = 1;
		while(cin >> t[i])
		{
			if(t[i] == 0)
				break;
			cin >> x_l[i] >> y_l[i] >>z_l[i]>> x_r[i] >> y_r[i]>>z_r[i];
			double tempx_l = x_l[i] - x_l[i-1];
			double tempy_l = y_l[i] - y_l[i-1];
			double tempz_l = z_l[i] - z_l[i-1];

			double tempx_r = x_r[i] - x_r[i-1];
			double tempy_r = y_r[i] - y_r[i-1];
			double tempz_r = z_r[i] - z_r[i-1];

			double dist_l = sqrt(tempx_l * tempx_l + tempy_l * tempy_l);
			double dist_r = sqrt(tempx_r * tempx_r + tempy_r * tempy_r);

			double tempsin_l = tempx_l / dist_l;
			double tempcos_l = tempy_l / dist_l;

			double tempsin_r = tempx_r / dist_r;
			double tempcos_r = tempy_r / dist_r;

			int left_ob = 0, right_ob = 0;

			if ((sin_A8 <= tempsin_l) && (tempsin_l < sin_A1) && tempcos_l > 0)
				left_ob = 1;
			if ((sin_A1 <= tempsin_l) && (tempsin_l < sin_A2) && tempcos_l > 0)
				left_ob = 2;
			if (sin_A2 <= tempsin_l)
				left_ob = 3;
			if ((sin_A4 <= tempsin_l) && (tempsin_l < sin_A3) && tempcos_l < 0)
				left_ob = 4;
			if ((sin_A5 <= tempsin_l) && (tempsin_l < sin_A4) && tempcos_l < 0)
				left_ob = 5;
			if ((sin_A6 <= tempsin_l) && (tempsin_l < sin_A5) && tempcos_l < 0)
				left_ob = 6;
			if ((tempsin_l <= sin_A7) && tempsin_l < 0)
				left_ob = 7;
			if ((sin_A7 <= tempsin_l) && (tempsin_l < sin_A8) && tempcos_l > 0)
				left_ob = 8;

			if ((sin_A8 <= tempsin_r) && (tempsin_r < sin_A1) && tempcos_r > 0)
				right_ob = 1;
			if ((sin_A1 <= tempsin_r) && (tempsin_r < sin_A2) && tempcos_r > 0)
				right_ob = 2;
			if (sin_A2 <= tempsin_r)
				right_ob = 3;
			if ((sin_A4 <= tempsin_r) && (tempsin_r < sin_A3) && tempcos_r < 0)
				right_ob = 4;
			if ((sin_A5 <= tempsin_r) && (tempsin_r < sin_A4) && tempcos_r < 0)
				right_ob = 5;
			if ((sin_A6 <= tempsin_r) && (tempsin_r < sin_A5) && tempcos_r < 0)
				right_ob = 6;
			if ((tempsin_r <= sin_A7) && tempsin_r < 0)
				right_ob = 7;
			if ((sin_A7 <= tempsin_r) && (tempsin_r < sin_A8) && tempcos_r > 0)
				right_ob = 8;
			/*if(tempz_l >= 0)
			{
				if(tempy_l >= 0)
				{
					if(tempx_l >= 0)
					{
						left_ob = 1;
					}
					else
						left_ob = 2;
				}
				else
				{
					if(tempx_l >= 0)
					{
						left_ob = 3;
					}
					else
						left_ob = 4;
				}
			}
			else
			{
				if(tempy_l >= 0)
				{
					if(tempx_l >= 0)
					{
						left_ob = 5;
					}
					else
						left_ob = 6;
				}
				else
				{
					if(tempx_l >= 0)
					{
						left_ob = 7;
					}
					else
						left_ob = 8;
				}
			}
			if(tempz_r >= 0)
			{
				if(tempy_r >= 0)
				{
					if(tempx_r >= 0)
					{
						right_ob = 1;
					}
					else
						right_ob = 2;
				}	
				else
				{
					if(tempx_r >= 0)
					{
						right_ob = 3;
					}
					else
						right_ob = 4;
				}
			}
			else
			{
				if(tempy_r >= 0)
				{
					if(tempx_r >= 0)
					{
						right_ob = 5;
					}
					else
						right_ob = 6;
				}
				else
				{
					if(tempx_r >= 0)
					{
						right_ob = 7;
					}
					else
						right_ob = 8;
				}
			}*/
			O[i] = left_ob * 8 - 8 + right_ob;
			i ++;
		}
		T = i;//可观察状态序列中状态数

		double ** alpha = new double* [T+1];
		for(int j = 0; j < T+1; j++)
			alpha[j] = new double[T+1];
		double ** beta = new double* [T+1];
		for(int j = 0; j < T+1; j++)
			beta[j] = new double[T+1];
		double ** gamma = new double* [T+1];
		for(int j = 0; j < T+1; j++)
			gamma[j] = new double[T+1];

		double pi, pf;
		int pniter;
		
		int f = sample_num == 0 ? 0 : 1; 
		InitHMM(hmm[f], N, M, 1);
		BaumWelch(hmm[f], T-1, O, alpha, beta, gamma, &pniter, &pi, &pf);
		delete alpha;
		delete beta;
		delete gamma;

		//将新hmm中的A,B,pi值加到chmm的对应位置上
		if(f == 0)
			continue;

		for(int n = 1; n <= N; ++n)
		{
			//add A
			for(int n1 = 1; n1 <= N; ++n1)
			{
				hmm[0]->A[n][n1] += hmm[1]->A[n][n1];
			}
			//add B
			for(int m = 1; m <= M; ++m)
			{
				hmm[0]->B[n][m] += hmm[1]->B[n][m];
			}
			//add pi
			hmm[0]->pi[n] += hmm[1]->pi[n];
		}

	}
	fclose(stdin);
	
	//求出平均的转移矩阵和概率分布向量
	for(int n = 1; n <= N; ++n)
	{
		//avg A
		for(int n1 = 1; n1 <= N; ++n1)
		{
			hmm[0]->A[n][n1] /= sample_num;
		}
		//avg B
		for(int m = 1; m <= M; ++m)
		{
			hmm[0]->B[n][m] /= sample_num;
		}
		//avg pi
		hmm[0]->pi[n] /= sample_num;
	}

	freopen(hmmfile,"w",stdout);
	PrintHMM(stdout, hmm[0]);

	
	fclose(stdout);
	
}