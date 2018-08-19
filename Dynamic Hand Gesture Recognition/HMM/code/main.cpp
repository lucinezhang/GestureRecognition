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

//ÔÚÄÚ´æÖÐ±£´æÁ¬ÐøµÄkÖ¡¹Ç÷ÀÐÅÏ¢£¬Æä³¤¶ÈÉÏÏÞÎªFrameNumLimit£¬µ±³¤¶È´óÓÚFrameNumLimitÊ±£¬°Ñ×îÀÏµÄÊý¾Ýµ¯³ö
list<f_skeleton> past_frames; 

int hand_r = NUI_SKELETON_POSITION_HAND_RIGHT;
int hand_l = NUI_SKELETON_POSITION_HAND_LEFT;

bool mode = true; //È«¾Ö±äÁ¿£¬±ê¼Çµ±Ç°ÊÇÑµÁ·Ä£Ê½»¹ÊÇ²âÊÔÄ£Ê½£ºtrue-²âÊÔ£¬false¡ª¡ªÑµÁ·
bool write_on = false; //È«¾Ö±äÁ¿£¬±ê¼Çµ±Ç°ÊÇ·ñÊôÓÚÕýÔÚÊéÐ´µÄ×´Ì¬

int gesture = 1;

//Í¨¹ý´«Èë¹Ø½ÚµãµÄÎ»ÖÃ£¬°Ñ¹Ç÷À»­³öÀ´  
void drawSkeleton(Mat &image, CvPoint pointSet[], int whichone);

//Ö÷º¯Êý
int main(int argc, char *argv[])
{
	cin>>mode;

	int train_samp_num = 0;
	if (!mode)
		cin >> gesture; //gesture = 1,2,3,ÿÿÎª½ÓÏÂÀ´ÒªÊä³öµÄÒ»×é²âÊÔÑùÀýµÄÕæÊµÖµ

	Mat skeletonImage;
	skeletonImage.create(240, 320, CV_8UC3);
	CvPoint skeletonPoint[NUI_SKELETON_COUNT][NUI_SKELETON_POSITION_COUNT] = { cvPoint(0, 0) };
	bool tracked[NUI_SKELETON_COUNT]={FALSE}; //±ê¼ÇÊÇ·ñ¸ú×Ùµ½ÈË
 
	//´´½¨ÐÂÎÄ¼þ£¬¼ÇÂ¼ÊÖµÄÎ»ÖÃ£¨x_l,y_l,x_r,y_r£©
	char * filename[10] = {"world.txt","country.txt","sground.txt","home.txt","strong.txt","rich.txt"};
	//ÈôÎªÑµÁ·Ä£Ê½£¬Ôò½«Ñù±¾Êä³öµ½ÎÄ¼þ
	ofstream file (filename[gesture], ios::out);

	if(!file)
		cout<<"Err"<<endl;

    //³õÊ¼»¯NUI£¬×¢ÒâÕâÀïÊÇUSES_SKELETON
    HRESULT hr = NuiInitialize(NUI_INITIALIZE_FLAG_USES_SKELETON); 
    if (FAILED(hr)) 
    { 
        cout<<"NuiInitialize failed"<<endl; 
        return hr; 
    } 

    //¶¨Òå¹Ç÷ÀÐÅºÅÊÂ¼þ¾ä±ú 
    HANDLE skeletonEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
 
	//´ò¿ª¹Ç÷À¸ú×ÙÊÂ¼þ
    hr = NuiSkeletonTrackingEnable( skeletonEvent, 0 ); 
    if( FAILED( hr ) )  
    { 
        cout<<"Could not open color image stream video"<<endl; 
        NuiShutdown(); 
        return hr; 
    }

	bool writing = false; //±ê¼ÇÊÇ·ñ´¦ÓÚÓÐÐ§ÊéÐ´µÄ×´Ì¬£¬Ö»ÓÐ´¦ÓÚÕâÖÖ×´Ì¬ÖÐ£¬ÎÒÃÇ²ÅÏòÎÄ¼þÖÐÐ´ÈëÊý¾Ý
	int hand_r = NUI_SKELETON_POSITION_HAND_RIGHT; 
	int hand_l = NUI_SKELETON_POSITION_HAND_LEFT;

	int k = 0;
	long t[5000] = {0};
	double x_l[5000], y_l[5000], z_l[5000];
	double x_r[5000], y_r[5000], z_r[5000];

	namedWindow("skeletonImage", CV_WINDOW_AUTOSIZE);

    //¿ªÊ¼¶ÁÈ¡¹Ç÷À¸ú×ÙÊý¾Ý 
	/*****************************Êý¾ÝÁ÷´¦ÀíºËÐÄÑ­»·**********************************/
    while(!kbhit()) //¶ÁÈ¡kinectÊý¾ÝÁ÷ÖÐµÄÒ»Ö¡£¬Ö±µ½²¶×½µ½Ä³¸öÈÎÒâµÄ¼üÅÌÊÂ¼þ£¬Ìø³öÑ­»·
    { 
        NUI_SKELETON_FRAME skeletonFrame = {0};  //¹Ç÷ÀÖ¡µÄ¶¨Òå 
		bool bFoundSkeleton = false; 

		//ÎÞÏÞµÈ´ýÐÂµÄÊý¾Ý£¬µÈµ½ºó·µ»Ø
        if (WaitForSingleObject(skeletonEvent, INFINITE)==0) //Èç¹ûµÈµ½ÁËÊý¾Ý
        { 
			//´Ó¸Õ²Å´ò¿ªÊý¾ÝÁ÷µÄÁ÷¾ä±úÖÐµÃµ½¸ÃÖ¡Êý¾Ý£¬¶ÁÈ¡µ½µÄÊý¾ÝµØÖ·´æÓÚskeletonFrame
            hr = NuiSkeletonGetNextFrame( 0, &skeletonFrame); 
			if (SUCCEEDED(hr))
			{
				//NUI_SKELETON_COUNTÊÇ¼ì²âµ½µÄ¹Ç÷ÀÊý£¨¼´£¬¸ú×Ùµ½µÄÈËÊý£©
				for( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ ) 
                { 
					NUI_SKELETON_TRACKING_STATE trackingState = skeletonFrame.SkeletonData[i].eTrackingState;
					//ÊÇ·ñ¸ú×Ùµ½ÁË 
                    if( trackingState == NUI_SKELETON_TRACKED )
                    { 
						//cout<<"found it"<<endl;
                        bFoundSkeleton = true; 	
                    } 
                }
			}
			if( !bFoundSkeleton )  //ÈôÒ»¸öÈË¶¼Ã»ÓÐ²¶×½µ½£¬ÔòÖØÐÂ¿ªÊ¼Ò»ÂÖ²¶×½¹ý³Ì
            { 
                continue; 
            } 

			//Æ½»¬¹Ç÷ÀÖ¡£¬Ïû³ý¶¶¶¯
			NuiTransformSmooth(&skeletonFrame, NULL);
			skeletonImage.setTo(0);

			float min_z = MIN;
			int object = 0;

			//ÕÒµ½×îÇ°ÃæµÄÒ»¸öÈË£¨Éî¶È×îÐ¡£©£¬½«Ëû¶¨ÎªÊäÈëÕßobject£¬²»¿¼ÂÇ²¶×½µ½µÄÆäËûÈË
			for(int i = 0; i < NUI_SKELETON_COUNT; i++)
			{
				//¶Ï¶¨ÊÇ·ñÊÇÒ»¸öÕýÈ·¹Ç÷ÀµÄÌõ¼þ£º¹Ç÷À±»¸ú×Ùµ½²¢ÇÒ¼ç²¿ÖÐÐÄ£¨¾±²¿Î»ÖÃ£©±ØÐë¸ú×Ùµ½¡£ 
				if( skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED &&   
					skeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED)   
				{
					if(skeletonFrame.SkeletonData[i].Position.z < min_z)
					{
						min_z = skeletonFrame.SkeletonData[i].Position.z;
						object = i;
					}

					float fx, fy;
					//ÄÃµ½ËùÓÐ¸ú×Ùµ½µÄ¹Ø½ÚµãµÄ×ø±ê£¬²¢×ª»»ÎªÎÒÃÇµÄÉî¶È¿Õ¼äµÄ×ø±ê£¬ÒòÎªÎÒÃÇÊÇÔÚÉî¶ÈÍ¼ÏñÖÐ°ÑÕâÐ©¹Ø½Úµã±ê¼Ç³öÀ´µÄ  
					//NUI_SKELETON_POSITION_COUNTÎª¸ú×Ùµ½µÄÒ»¸ö¹Ç÷ÀµÄ¹Ø½ÚµãµÄÊýÄ¿£¬Îª20  
					for (int j = 0; j < NUI_SKELETON_POSITION_COUNT; j++)
					{
						NuiTransformSkeletonToDepthImage(skeletonFrame.SkeletonData[i].SkeletonPositions[j], &fx, &fy);
						skeletonPoint[i][j].x = (int)fx;
						skeletonPoint[i][j].y = (int)fy;
					}

					for (int j = 0; j<NUI_SKELETON_POSITION_COUNT; j++)
					{
						if (skeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[j] != NUI_SKELETON_POSITION_NOT_TRACKED)//¸ú×ÙµãÒ»ÓÃÓÐÈýÖÖ×´Ì¬£º1Ã»ÓÐ±»¸ú×Ùµ½£¬2¸ú×Ùµ½£¬3¸ù¾Ý¸ú×Ùµ½µÄ¹À¼Æµ½     
						{
							circle(skeletonImage, skeletonPoint[i][j], 3, cvScalar(0, 255, 255), 1, 8, 0);
							tracked[i] = TRUE;
						}
					}
					drawSkeleton(skeletonImage, skeletonPoint[i], i);
				}
			}
			imshow("skeletonImage", skeletonImage); //ÏÔÊ¾Í¼Ïñ 

			
			f_skeleton skeleton_user; //¶¨Òå´ËÖ¡µÄuser¹Ç÷ÀÐÅÏ¢
			//°Ñ¹Ç÷À×ø±êÒÔ¼°Ê±¼ä´Á´æÈëÊý¾Ý½á¹¹
			skeleton_user.time = (long)(skeletonFrame.liTimeStamp.LowPart +(((long)skeletonFrame.liTimeStamp.HighPart) << 32 ));
			for (int j = 0 ; j < NUI_SKELETON_POSITION_COUNT; j++)
			{
				skeleton_user.pos[j].x = skeletonFrame.SkeletonData[object].SkeletonPositions[j].x;
				skeleton_user.pos[j].y = skeletonFrame.SkeletonData[object].SkeletonPositions[j].y;
				skeleton_user.pos[j].z = skeletonFrame.SkeletonData[object].SkeletonPositions[j].z;
			}
			//½«ÐÂµÄÊý¾ÝÖ¡¼ÓÈëlistÖÐ
			past_frames.push_back(skeleton_user);
			if(past_frames.size() > FrameNumLimit)
				past_frames.erase(past_frames.begin()); //É¾³ý×îÔçµÄÊý¾ÝÖ¡

			if(write_on)//Èç¹ûÄ¿Ç°´¦ÓÚÊéÐ´½×¶Î
			{
				/*
				 * Ê×ÏÈÅÐ¶ÏÊÇ·ñÍË³öÊéÐ´½×¶Î£º
				 * ÀûÓÃÊéÐ´ÊÖºÍÉíÌåÆäËû²¿Î»µÄÏà¶Ô×ø±ê¹ØÏµ£¨Ö÷ÒªÀûÓÃyÖµ£© 
				 */
				if((skeleton_user.pos[NUI_SKELETON_POSITION_HEAD].y - skeleton_user.pos[hand_r].y > 0.65) && (skeleton_user.pos[NUI_SKELETON_POSITION_HEAD].y - skeleton_user.pos[hand_l].y > 0.65))
				{
					write_on = false;
						
					if(!mode)//ÑµÁ·Ä£Ê½
					{
						file<<"0"<<endl; //±ê¼Ç¹ì¼£µÄÖÕÖ¹µã
						train_samp_num++;
						cout<<"end writing"<<endl;
					}
					else
					{
						t[k] = 0;
						k = 0;
						int result = hmm_test(t, x_l, y_l, z_l, x_r, y_r, z_r);

						if (result == 0)
							cout << "ÔÚÊÀ½çµÄ";
						else if (result == 1)
							cout << "¹ú " << endl;
						else if (result == 2)
							cout << "ÔÚÌìµØµÄ";
						else if (result == 3)
							cout << "¼Ò" << endl;
						else if (result == 4)
							cout << "ÓÐÁËÇ¿µÄ";
						else if (result == 5)
							cout << "²ÅÓÐ¸»µÄ";

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
			else //²¢Î´´¦ÓÚÊéÐ´×´Ì¬
			{
				/*
				 * ÅÐ¶ÏÊÇ·ñÓ¦µ±¿ªÊ¼¶ÁÈëÊý¾Ý:
				 */
				if(past_frames.size()>=FrameNumLimit) //¸ù¾ÝÊ²Ã´À´ÅÐ¶Ï¹ì¼£µÄÆðÊ¼¡ù
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

    //¹Ø±ÕNUIÁ´½Ó 
    NuiShutdown(); 
	
	//ÑµÁ·Ñù±¾
	char * outputfile[10] = {"hmm_world.txt","hmm_country.txt","hmm_sground.txt", "hmm_home.txt", "hmm_strong.txt", "hmm_rich.txt"};
	if(!mode)
	{
		hmm_train(filename[gesture],outputfile[gesture],train_samp_num);
	}
	
	//¹Ø±ÕÎÄ¼þ
	file.close();

	system("pause");
	return 0;

}

/*****************************ÏÂÃæÎª¸¨Öúº¯Êý****************************/

bool ifStartInput()
{
	return 0;
}
//½«LARGE_INTEGER×ª»»Îªlong
long convert_large_integer(LARGE_INTEGER num)
{
	long new_num = (long)(num.LowPart +(((long)num.HighPart) << 32 ));  
	return new_num; 
}

//¼ÆËã¹Ø½ÚµãµÄÔË¶¯ËÙÂÊ£¬ÆäÖÐdimÖ¸¼ÆËãËÙ¶ÈµÄ¿Õ¼äÎ¬¶È£¬Ä¬ÈÏÎª2Î¬¿Õ¼äÖÐµÄËÙÂÊ
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

//Í¨¹ý´«Èë¹Ø½ÚµãµÄÎ»ÖÃ£¬°Ñ¹Ç÷À»­³öÀ´  
void drawSkeleton(Mat &image, CvPoint pointSet[], int whichone)
{
	CvScalar color;
	switch (whichone) //¸ú×Ù²»Í¬µÄÈËÏÔÊ¾²»Í¬µÄÑÕÉ«   
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

	//×óÉÏÖ«   
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

	//ÓÒÉÏÖ«   
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

	//×óÏÂÖ«   
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

	//ÓÒÏÂÖ«   
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