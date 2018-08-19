#include <Windows.h>
#include <iostream> 
#include <fstream>
#include <NuiApi.h>
#include <conio.h>
#include <cmath>
#include <vector>
#include <list>
#include <opencv2/opencv.hpp> 
#include "hmm.h"

using namespace std;
using namespace cv;

#define MIN 1000000.0

#define stational_x_l 0.02 
#define stational_y_l 0.02 
#define stational_var_l 0.0015
#define stational_v_l

#define stational_x_r 0.02 
#define stational_y_r 0.02 
#define stational_var_r 0.0015
#define stational_v_r

#define FrameNumLimit 100

struct position{
	double x;
	double y;
	double z;
};

struct f_skeleton{
	position pos[20]; //save 20 skeleton points
	long time; //time stamp (ms)
};

//���ڴ��б���������k֡������Ϣ���䳤������ΪFrameNumLimit�������ȴ���FrameNumLimitʱ�������ϵ����ݵ���
list<f_skeleton> past_frames; 

int hand_r = NUI_SKELETON_POSITION_HAND_RIGHT;
int hand_l = NUI_SKELETON_POSITION_HAND_LEFT;

bool mode = true; //ȫ�ֱ�������ǵ�ǰ��ѵ��ģʽ���ǲ���ģʽ��true-���ԣ�false����ѵ��
bool write_on = false; //ȫ�ֱ�������ǵ�ǰ�Ƿ�����������д��״̬

int gesture = 1;

//ͨ������ؽڵ��λ�ã��ѹ���������  
void drawSkeleton(Mat &image, CvPoint pointSet[], int whichone);

//������
int main(int argc, char *argv[])
{
	cin>>mode;

	int train_samp_num = 0;
	if (!mode)
		cin >> gesture; //gesture = 1,2,3,��Ϊ������Ҫ�����һ�������������ʵֵ

	Mat skeletonImage;
	skeletonImage.create(240, 320, CV_8UC3);
	CvPoint skeletonPoint[NUI_SKELETON_COUNT][NUI_SKELETON_POSITION_COUNT] = { cvPoint(0, 0) };
	bool tracked[NUI_SKELETON_COUNT]={FALSE}; //����Ƿ���ٵ���
 
	//�������ļ�����¼�ֵ�λ�ã�x_l,y_l,x_r,y_r��
	char * filename[10] = {"world.txt","country.txt","sground.txt","home.txt","strong.txt","rich.txt"};
	//��Ϊѵ��ģʽ��������������ļ�
	ofstream file (filename[gesture], ios::out);

	if(!file)
		cout<<"Err"<<endl;

    //��ʼ��NUI��ע��������USES_SKELETON
    HRESULT hr = NuiInitialize(NUI_INITIALIZE_FLAG_USES_SKELETON); 
    if (FAILED(hr)) 
    { 
        cout<<"NuiInitialize failed"<<endl; 
        return hr; 
    } 

    //��������ź��¼���� 
    HANDLE skeletonEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
 
	//�򿪹��������¼�
    hr = NuiSkeletonTrackingEnable( skeletonEvent, 0 ); 
    if( FAILED( hr ) )  
    { 
        cout<<"Could not open color image stream video"<<endl; 
        NuiShutdown(); 
        return hr; 
    }

	bool writing = false; //����Ƿ�����Ч��д��״̬��ֻ�д�������״̬�У����ǲ����ļ���д������
	int hand_r = NUI_SKELETON_POSITION_HAND_RIGHT; 
	int hand_l = NUI_SKELETON_POSITION_HAND_LEFT;

	int k = 0;
	long t[5000] = {0};
	double x_l[5000], y_l[5000], z_l[5000];
	double x_r[5000], y_r[5000], z_r[5000];

	namedWindow("skeletonImage", CV_WINDOW_AUTOSIZE);

    //��ʼ��ȡ������������ 
	/*****************************�������������ѭ��**********************************/
    while(!kbhit()) //��ȡkinect�������е�һ֡��ֱ����׽��ĳ������ļ����¼�������ѭ��
    { 
        NUI_SKELETON_FRAME skeletonFrame = {0};  //����֡�Ķ��� 
		bool bFoundSkeleton = false; 

		//���޵ȴ��µ����ݣ��ȵ��󷵻�
        if (WaitForSingleObject(skeletonEvent, INFINITE)==0) //����ȵ�������
        { 
			//�ӸղŴ���������������еõ���֡���ݣ���ȡ�������ݵ�ַ����skeletonFrame
            hr = NuiSkeletonGetNextFrame( 0, &skeletonFrame); 
			if (SUCCEEDED(hr))
			{
				//NUI_SKELETON_COUNT�Ǽ�⵽�Ĺ��������������ٵ���������
				for( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ ) 
                { 
					NUI_SKELETON_TRACKING_STATE trackingState = skeletonFrame.SkeletonData[i].eTrackingState;
					//�Ƿ���ٵ��� 
                    if( trackingState == NUI_SKELETON_TRACKED )
                    { 
						//cout<<"found it"<<endl;
                        bFoundSkeleton = true; 	
                    } 
                }
			}
			if( !bFoundSkeleton )  //��һ���˶�û�в�׽���������¿�ʼһ�ֲ�׽����
            { 
                continue; 
            } 

			//ƽ������֡����������
			NuiTransformSmooth(&skeletonFrame, NULL);
			skeletonImage.setTo(0);

			float min_z = MIN;
			int object = 0;

			//�ҵ���ǰ���һ���ˣ������С����������Ϊ������object�������ǲ�׽����������
			for(int i = 0; i < NUI_SKELETON_COUNT; i++)
			{
				//�϶��Ƿ���һ����ȷ���������������������ٵ����Ҽ粿���ģ�����λ�ã�������ٵ��� 
				if( skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED &&   
					skeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED)   
				{
					if(skeletonFrame.SkeletonData[i].Position.z < min_z)
					{
						min_z = skeletonFrame.SkeletonData[i].Position.z;
						object = i;
					}

					float fx, fy;
					//�õ����и��ٵ��Ĺؽڵ�����꣬��ת��Ϊ���ǵ���ȿռ�����꣬��Ϊ�����������ͼ���а���Щ�ؽڵ��ǳ�����  
					//NUI_SKELETON_POSITION_COUNTΪ���ٵ���һ�������Ĺؽڵ����Ŀ��Ϊ20  
					for (int j = 0; j < NUI_SKELETON_POSITION_COUNT; j++)
					{
						NuiTransformSkeletonToDepthImage(skeletonFrame.SkeletonData[i].SkeletonPositions[j], &fx, &fy);
						skeletonPoint[i][j].x = (int)fx;
						skeletonPoint[i][j].y = (int)fy;
					}

					for (int j = 0; j<NUI_SKELETON_POSITION_COUNT; j++)
					{
						if (skeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[j] != NUI_SKELETON_POSITION_NOT_TRACKED)//���ٵ�һ��������״̬��1û�б����ٵ���2���ٵ���3���ݸ��ٵ��Ĺ��Ƶ�     
						{
							circle(skeletonImage, skeletonPoint[i][j], 3, cvScalar(0, 255, 255), 1, 8, 0);
							tracked[i] = TRUE;
						}
					}
					drawSkeleton(skeletonImage, skeletonPoint[i], i);
				}
			}
			imshow("skeletonImage", skeletonImage); //��ʾͼ�� 

			
			f_skeleton skeleton_user; //�����֡��user������Ϣ
			//�ѹ��������Լ�ʱ����������ݽṹ
			skeleton_user.time = (long)(skeletonFrame.liTimeStamp.LowPart +(((long)skeletonFrame.liTimeStamp.HighPart) << 32 ));
			for (int j = 0 ; j < NUI_SKELETON_POSITION_COUNT; j++)
			{
				skeleton_user.pos[j].x = skeletonFrame.SkeletonData[object].SkeletonPositions[j].x;
				skeleton_user.pos[j].y = skeletonFrame.SkeletonData[object].SkeletonPositions[j].y;
				skeleton_user.pos[j].z = skeletonFrame.SkeletonData[object].SkeletonPositions[j].z;
			}
			//���µ�����֡����list��
			past_frames.push_back(skeleton_user);
			if(past_frames.size() > FrameNumLimit)
				past_frames.erase(past_frames.begin()); //ɾ�����������֡

			if(write_on)//���Ŀǰ������д�׶�
			{
				/*
				 * �����ж��Ƿ��˳���д�׶Σ�
				 * ������д�ֺ�����������λ����������ϵ����Ҫ����yֵ�� 
				 */
				if((skeleton_user.pos[NUI_SKELETON_POSITION_HEAD].y - skeleton_user.pos[hand_r].y > 0.65) && (skeleton_user.pos[NUI_SKELETON_POSITION_HEAD].y - skeleton_user.pos[hand_l].y > 0.65))
				{
					write_on = false;
						
					if(!mode)//ѵ��ģʽ
					{
						file<<"0"<<endl; //��ǹ켣����ֹ��
						train_samp_num++;
						cout<<"end writing"<<endl;
					}
					else
					{
						t[k] = 0;
						k = 0;
						int result = hmm_test(t, x_l, y_l, z_l, x_r, y_r, z_r);

						if (result == 0)
							cout << "�������";
						else if (result == 1)
							cout << "�� " << endl;
						else if (result == 2)
							cout << "����ص�";
						else if (result == 3)
							cout << "��" << endl;
						else if (result == 4)
							cout << "����ǿ��";
						else if (result == 5)
							cout << "���и���";

						//system("pause");

						//printf(" "); //getch();
					}
					continue;
				}

				t[k]=skeleton_user.time;
				x_l[k]=skeleton_user.pos[hand_l].x;
				y_l[k]=skeleton_user.pos[hand_l].y;
				z_l[k]=skeleton_user.pos[hand_l].z;
				x_r[k]=skeleton_user.pos[hand_r].x;
				y_r[k]=skeleton_user.pos[hand_r].y;
				z_r[k]=skeleton_user.pos[hand_r].z;
				
				if(!mode)
					file<<t[k]<<" "<<x_l[k]<<" "<<y_l[k]<<" "<<z_l[k]<<" "<<x_r[k]<<" "<<y_r[k]<<" "<<z_r[k]<<endl;

				++k;
				//cout << k << endl;
			}
			else //��δ������д״̬
			{
				/*
				 * �ж��Ƿ�Ӧ����ʼ��������:
				 */
				if(past_frames.size()>=FrameNumLimit) //����ʲô���жϹ켣����ʼ��
				{
					if( (abs(skeleton_user.pos[hand_l].z - skeleton_user.pos[NUI_SKELETON_POSITION_HEAD].z) > 0.1) 
						&& (abs(skeleton_user.pos[hand_l].y - skeleton_user.pos[NUI_SKELETON_POSITION_HEAD].y)) < 0.65 
						|| (abs(skeleton_user.pos[hand_r].z - skeleton_user.pos[NUI_SKELETON_POSITION_HEAD].z) > 0.1) 
						&& (abs(skeleton_user.pos[hand_r].y - skeleton_user.pos[NUI_SKELETON_POSITION_HEAD].y) < 0.65) ) 
					{
						write_on = true;
						//cout <<"start writing"<<endl;
					}
				}
			}
        } 
        else 
        { 
            cout<<"Buffer length of received texture is bogus\r\n"<<endl; 
        }
    } 

    //�ر�NUI���� 
    NuiShutdown(); 
	
	//ѵ������
	char * outputfile[10] = {"hmm_world.txt","hmm_country.txt","hmm_sground.txt", "hmm_home.txt", "hmm_strong.txt", "hmm_rich.txt"};
	if(!mode)
	{
		hmm_train(filename[gesture],outputfile[gesture],train_samp_num);
	}
	
	//�ر��ļ�
	file.close();

	system("pause");
	return 0;

}

/*****************************����Ϊ��������****************************/

bool ifStartInput()
{
	return 0;
}
//��LARGE_INTEGERת��Ϊlong
long convert_large_integer(LARGE_INTEGER num)
{
	long new_num = (long)(num.LowPart +(((long)num.HighPart) << 32 ));  
	return new_num; 
}

//����ؽڵ���˶����ʣ�����dimָ�����ٶȵĿռ�ά�ȣ�Ĭ��Ϊ2ά�ռ��е�����
double compute_verocity(position p1, position p2, long time1, long time2, int dim = 2)
{
	double v = (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
	if(dim == 3)
		v += (p1.z  -p2.z) * (p1.z - p2.z);
	long dur = abs(time1 - time2);
	v /= dur;
	return v;
}

double Variance(list<f_skeleton> frameList)
{
	double meanx_l = 0, meany_l = 0, meanx_r = 0, meany_r = 0, var = 0;
	int len = frameList.size();
	list<f_skeleton>::iterator it;
	for(it = frameList.begin(); it != frameList.end(); ++it)
	{
		meanx_l += it->pos[hand_l].x;
		meany_l += it->pos[hand_l].y;
		meanx_r += it->pos[hand_r].x;
		meany_r += it->pos[hand_r].y;
	}
	meanx_l /= (float)len;
	meany_l /= (float)len;
	meanx_r /= (float)len;
	meany_r /= (float)len;

	for(it = frameList.begin(); it != frameList.end(); ++it)
	{
		var += (it->pos[hand_l].x - meanx_l) * (it->pos[hand_l].x - meanx_l);
		var += (it->pos[hand_l].y - meany_l) * (it->pos[hand_l].y - meany_l);
		var += (it->pos[hand_r].x - meanx_r) * (it->pos[hand_r].x - meanx_r);
		var += (it->pos[hand_r].y - meany_r) * (it->pos[hand_r].y - meany_r);
	}	
	return var;
}

//ͨ������ؽڵ��λ�ã��ѹ���������  
void drawSkeleton(Mat &image, CvPoint pointSet[], int whichone)
{
	CvScalar color;
	switch (whichone) //���ٲ�ͬ������ʾ��ͬ����ɫ   
	{
	case 0:
		color = cvScalar(255);
		break;
	case 1:
		color = cvScalar(0, 255);
		break;
	case 2:
		color = cvScalar(0, 0, 255);
		break;
	case 3:
		color = cvScalar(255, 255, 0);
		break;
	case 4:
		color = cvScalar(255, 0, 255);
		break;
	case 5:
		color = cvScalar(0, 255, 255);
		break;
	}

	if ((pointSet[NUI_SKELETON_POSITION_HEAD].x != 0 || pointSet[NUI_SKELETON_POSITION_HEAD].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_HEAD], pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_SPINE].x != 0 || pointSet[NUI_SKELETON_POSITION_SPINE].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER], pointSet[NUI_SKELETON_POSITION_SPINE], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_SPINE].x != 0 || pointSet[NUI_SKELETON_POSITION_SPINE].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_HIP_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_CENTER].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_SPINE], pointSet[NUI_SKELETON_POSITION_HIP_CENTER], color, 2);

	//����֫   
	if ((pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_SHOULDER_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER], pointSet[NUI_SKELETON_POSITION_SHOULDER_LEFT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_SHOULDER_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_LEFT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_ELBOW_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_ELBOW_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_SHOULDER_LEFT], pointSet[NUI_SKELETON_POSITION_ELBOW_LEFT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_ELBOW_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_ELBOW_LEFT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_WRIST_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_WRIST_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_ELBOW_LEFT], pointSet[NUI_SKELETON_POSITION_WRIST_LEFT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_WRIST_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_WRIST_LEFT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_HAND_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_HAND_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_WRIST_LEFT], pointSet[NUI_SKELETON_POSITION_HAND_LEFT], color, 2);

	//����֫   
	if ((pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_SHOULDER_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_SHOULDER_CENTER], pointSet[NUI_SKELETON_POSITION_SHOULDER_RIGHT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_SHOULDER_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_SHOULDER_RIGHT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_ELBOW_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_ELBOW_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_SHOULDER_RIGHT], pointSet[NUI_SKELETON_POSITION_ELBOW_RIGHT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_ELBOW_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_ELBOW_RIGHT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_WRIST_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_WRIST_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_ELBOW_RIGHT], pointSet[NUI_SKELETON_POSITION_WRIST_RIGHT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_WRIST_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_WRIST_RIGHT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_HAND_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_HAND_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_WRIST_RIGHT], pointSet[NUI_SKELETON_POSITION_HAND_RIGHT], color, 2);

	//����֫   
	if ((pointSet[NUI_SKELETON_POSITION_HIP_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_CENTER].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_HIP_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_HIP_CENTER], pointSet[NUI_SKELETON_POSITION_HIP_LEFT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_HIP_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_LEFT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_KNEE_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_KNEE_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_HIP_LEFT], pointSet[NUI_SKELETON_POSITION_KNEE_LEFT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_KNEE_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_KNEE_LEFT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_ANKLE_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_ANKLE_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_KNEE_LEFT], pointSet[NUI_SKELETON_POSITION_ANKLE_LEFT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_ANKLE_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_ANKLE_LEFT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_FOOT_LEFT].x != 0 || pointSet[NUI_SKELETON_POSITION_FOOT_LEFT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_ANKLE_LEFT], pointSet[NUI_SKELETON_POSITION_FOOT_LEFT], color, 2);

	//����֫   
	if ((pointSet[NUI_SKELETON_POSITION_HIP_CENTER].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_CENTER].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_HIP_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_HIP_CENTER], pointSet[NUI_SKELETON_POSITION_HIP_RIGHT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_HIP_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_HIP_RIGHT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_KNEE_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_KNEE_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_HIP_RIGHT], pointSet[NUI_SKELETON_POSITION_KNEE_RIGHT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_KNEE_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_KNEE_RIGHT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_ANKLE_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_ANKLE_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_KNEE_RIGHT], pointSet[NUI_SKELETON_POSITION_ANKLE_RIGHT], color, 2);
	if ((pointSet[NUI_SKELETON_POSITION_ANKLE_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_ANKLE_RIGHT].y != 0) &&
		(pointSet[NUI_SKELETON_POSITION_FOOT_RIGHT].x != 0 || pointSet[NUI_SKELETON_POSITION_FOOT_RIGHT].y != 0))
		line(image, pointSet[NUI_SKELETON_POSITION_ANKLE_RIGHT], pointSet[NUI_SKELETON_POSITION_FOOT_RIGHT], color, 2);
}