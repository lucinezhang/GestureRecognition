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

//在内存中保存连续的k帧骨骼信息，其长度上限为FrameNumLimit，当长度大于FrameNumLimit时，把最老的数据弹出
list<f_skeleton> past_frames; 

int hand_r = NUI_SKELETON_POSITION_HAND_RIGHT;
int hand_l = NUI_SKELETON_POSITION_HAND_LEFT;

bool mode = true; //全局变量，标记当前是训练模式还是测试模式：true-测试，false——训练
bool write_on = false; //全局变量，标记当前是否属于正在书写的状态

int gesture = 1;

//通过传入关节点的位置，把骨骼画出来  
void drawSkeleton(Mat &image, CvPoint pointSet[], int whichone);

//主函数
int main(int argc, char *argv[])
{
	cin>>mode;

	int train_samp_num = 0;
	if (!mode)
		cin >> gesture; //gesture = 1,2,3,��为接下来要输出的一组测试样例的真实值

	Mat skeletonImage;
	skeletonImage.create(240, 320, CV_8UC3);
	CvPoint skeletonPoint[NUI_SKELETON_COUNT][NUI_SKELETON_POSITION_COUNT] = { cvPoint(0, 0) };
	bool tracked[NUI_SKELETON_COUNT]={FALSE}; //标记是否跟踪到人
 
	//创建新文件，记录手的位置（x_l,y_l,x_r,y_r）
	char * filename[10] = {"world.txt","country.txt","sground.txt","home.txt","strong.txt","rich.txt"};
	//若为训练模式，则将样本输出到文件
	ofstream file (filename[gesture], ios::out);

	if(!file)
		cout<<"Err"<<endl;

    //初始化NUI，注意这里是USES_SKELETON
    HRESULT hr = NuiInitialize(NUI_INITIALIZE_FLAG_USES_SKELETON); 
    if (FAILED(hr)) 
    { 
        cout<<"NuiInitialize failed"<<endl; 
        return hr; 
    } 

    //定义骨骼信号事件句柄 
    HANDLE skeletonEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
 
	//打开骨骼跟踪事件
    hr = NuiSkeletonTrackingEnable( skeletonEvent, 0 ); 
    if( FAILED( hr ) )  
    { 
        cout<<"Could not open color image stream video"<<endl; 
        NuiShutdown(); 
        return hr; 
    }

	bool writing = false; //标记是否处于有效书写的状态，只有处于这种状态中，我们才向文件中写入数据
	int hand_r = NUI_SKELETON_POSITION_HAND_RIGHT; 
	int hand_l = NUI_SKELETON_POSITION_HAND_LEFT;

	int k = 0;
	long t[5000] = {0};
	double x_l[5000], y_l[5000], z_l[5000];
	double x_r[5000], y_r[5000], z_r[5000];

	namedWindow("skeletonImage", CV_WINDOW_AUTOSIZE);

    //开始读取骨骼跟踪数据 
	/*****************************数据流处理核心循环**********************************/
    while(!kbhit()) //读取kinect数据流中的一帧，直到捕捉到某个任意的键盘事件，跳出循环
    { 
        NUI_SKELETON_FRAME skeletonFrame = {0};  //骨骼帧的定义 
		bool bFoundSkeleton = false; 

		//无限等待新的数据，等到后返回
        if (WaitForSingleObject(skeletonEvent, INFINITE)==0) //如果等到了数据
        { 
			//从刚才打开数据流的流句柄中得到该帧数据，读取到的数据地址存于skeletonFrame
            hr = NuiSkeletonGetNextFrame( 0, &skeletonFrame); 
			if (SUCCEEDED(hr))
			{
				//NUI_SKELETON_COUNT是检测到的骨骼数（即，跟踪到的人数）
				for( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ ) 
                { 
					NUI_SKELETON_TRACKING_STATE trackingState = skeletonFrame.SkeletonData[i].eTrackingState;
					//是否跟踪到了 
                    if( trackingState == NUI_SKELETON_TRACKED )
                    { 
						//cout<<"found it"<<endl;
                        bFoundSkeleton = true; 	
                    } 
                }
			}
			if( !bFoundSkeleton )  //若一个人都没有捕捉到，则重新开始一轮捕捉过程
            { 
                continue; 
            } 

			//平滑骨骼帧，消除抖动
			NuiTransformSmooth(&skeletonFrame, NULL);
			skeletonImage.setTo(0);

			float min_z = MIN;
			int object = 0;

			//找到最前面的一个人（深度最小），将他定为输入者object，不考虑捕捉到的其他人
			for(int i = 0; i < NUI_SKELETON_COUNT; i++)
			{
				//断定是否是一个正确骨骼的条件：骨骼被跟踪到并且肩部中心（颈部位置）必须跟踪到。 
				if( skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED &&   
					skeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED)   
				{
					if(skeletonFrame.SkeletonData[i].Position.z < min_z)
					{
						min_z = skeletonFrame.SkeletonData[i].Position.z;
						object = i;
					}

					float fx, fy;
					//拿到所有跟踪到的关节点的坐标，并转换为我们的深度空间的坐标，因为我们是在深度图像中把这些关节点标记出来的  
					//NUI_SKELETON_POSITION_COUNT为跟踪到的一个骨骼的关节点的数目，为20  
					for (int j = 0; j < NUI_SKELETON_POSITION_COUNT; j++)
					{
						NuiTransformSkeletonToDepthImage(skeletonFrame.SkeletonData[i].SkeletonPositions[j], &fx, &fy);
						skeletonPoint[i][j].x = (int)fx;
						skeletonPoint[i][j].y = (int)fy;
					}

					for (int j = 0; j<NUI_SKELETON_POSITION_COUNT; j++)
					{
						if (skeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[j] != NUI_SKELETON_POSITION_NOT_TRACKED)//跟踪点一用有三种状态：1没有被跟踪到，2跟踪到，3根据跟踪到的估计到     
						{
							circle(skeletonImage, skeletonPoint[i][j], 3, cvScalar(0, 255, 255), 1, 8, 0);
							tracked[i] = TRUE;
						}
					}
					drawSkeleton(skeletonImage, skeletonPoint[i], i);
				}
			}
			imshow("skeletonImage", skeletonImage); //显示图像 

			
			f_skeleton skeleton_user; //定义此帧的user骨骼信息
			//把骨骼坐标以及时间戳存入数据结构
			skeleton_user.time = (long)(skeletonFrame.liTimeStamp.LowPart +(((long)skeletonFrame.liTimeStamp.HighPart) << 32 ));
			for (int j = 0 ; j < NUI_SKELETON_POSITION_COUNT; j++)
			{
				skeleton_user.pos[j].x = skeletonFrame.SkeletonData[object].SkeletonPositions[j].x;
				skeleton_user.pos[j].y = skeletonFrame.SkeletonData[object].SkeletonPositions[j].y;
				skeleton_user.pos[j].z = skeletonFrame.SkeletonData[object].SkeletonPositions[j].z;
			}
			//将新的数据帧加入list中
			past_frames.push_back(skeleton_user);
			if(past_frames.size() > FrameNumLimit)
				past_frames.erase(past_frames.begin()); //删除最早的数据帧

			if(write_on)//如果目前处于书写阶段
			{
				/*
				 * 首先判断是否退出书写阶段：
				 * 利用书写手和身体其他部位的相对坐标关系（主要利用y值） 
				 */
				if((skeleton_user.pos[NUI_SKELETON_POSITION_HEAD].y - skeleton_user.pos[hand_r].y > 0.65) && (skeleton_user.pos[NUI_SKELETON_POSITION_HEAD].y - skeleton_user.pos[hand_l].y > 0.65))
				{
					write_on = false;
						
					if(!mode)//训练模式
					{
						file<<"0"<<endl; //标记轨迹的终止点
						train_samp_num++;
						cout<<"end writing"<<endl;
					}
					else
					{
						t[k] = 0;
						k = 0;
						int result = hmm_test(t, x_l, y_l, z_l, x_r, y_r, z_r);

						if (result == 0)
							cout << "在世界的";
						else if (result == 1)
							cout << "国 " << endl;
						else if (result == 2)
							cout << "在天地的";
						else if (result == 3)
							cout << "家" << endl;
						else if (result == 4)
							cout << "有了强的";
						else if (result == 5)
							cout << "才有富的";

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
			else //并未处于书写状态
			{
				/*
				 * 判断是否应当开始读入数据:
				 */
				if(past_frames.size()>=FrameNumLimit) //根据什么来判断轨迹的起始※
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

    //关闭NUI链接 
    NuiShutdown(); 
	
	//训练样本
	char * outputfile[10] = {"hmm_world.txt","hmm_country.txt","hmm_sground.txt", "hmm_home.txt", "hmm_strong.txt", "hmm_rich.txt"};
	if(!mode)
	{
		hmm_train(filename[gesture],outputfile[gesture],train_samp_num);
	}
	
	//关闭文件
	file.close();

	system("pause");
	return 0;

}

/*****************************下面为辅助函数****************************/

bool ifStartInput()
{
	return 0;
}
//将LARGE_INTEGER转换为long
long convert_large_integer(LARGE_INTEGER num)
{
	long new_num = (long)(num.LowPart +(((long)num.HighPart) << 32 ));  
	return new_num; 
}

//计算关节点的运动速率，其中dim指计算速度的空间维度，默认为2维空间中的速率
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

//通过传入关节点的位置，把骨骼画出来  
void drawSkeleton(Mat &image, CvPoint pointSet[], int whichone)
{
	CvScalar color;
	switch (whichone) //跟踪不同的人显示不同的颜色   
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

	//左上肢   
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

	//右上肢   
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

	//左下肢   
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

	//右下肢   
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