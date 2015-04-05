#include <iostream>
#include <highgui.h>
#include <cmath>
#include <cv.h>
#include <cxcore.h>
#include <string>
#include <iostream>
#include <fstream>
#include <strngs.h>
#include <baseapi.h>
#include "stdafx.h"
#pragma comment(lib,"libtesseract302d.lib")
#pragma comment(lib,"liblept168.lib")

using namespace std;
using namespace cv;

int CompareHist(IplImage *image1,IplImage *image2,int thisCount);
vector <IplImage*> wordstodetect;//存储用来识别的字幕图片
int countOfCharacter;//字符个数
int countOfFrame;
int framecount = 0;

int main(int argc, char** argv) {
	
	cvNamedWindow("video");
	IplImage* frame = NULL;
	IplImage* words = NULL;
	IplImage* wordscopy = NULL;
	vector <IplImage*> wordsprepare;//存储一条字幕的容器
	vector <IplImage*> wordsvector;//存储每帧中单个字的容器
	
	CvCapture* Capture = cvCreateFileCapture("E:\\father.mkv");
	if(!Capture)
		cout<<"Capture Video failed!"<<endl;
	while(1) {
		frame = cvQueryFrame(Capture);
		if(!frame) {
			cout<<"Query frame failed!"<<endl;
			break;
		}
		framecount ++;
		cvSetImageROI(frame,cvRect(frame->width*0.1, frame->height*0.8, frame->width*0.5, frame->height*0.2));
		words = cvCreateImage(cvGetSize(frame),8,1);
		wordscopy = cvCreateImage(cvGetSize(frame),8,1);
		cvCvtColor(frame,words,CV_RGB2GRAY);
		cvResetImageROI(frame);
		cvThreshold(words,words,240,255,CV_THRESH_BINARY);//二值化

		IplImage* wordsdilate = cvCreateImage(cvGetSize(words),8,1);
		cvDilate( words,wordsdilate, NULL,1); //膨胀 
		cvDilate( words,words, NULL,1); //膨胀 
		cvErode( words,words, NULL,1); //腐蚀  
		cvCopy(words,wordscopy);
		//cvDilate( words,words, NULL,1); //膨胀  

		//cvNot(words, words);
		Mat wordsmat(wordsdilate);

		vector< vector<Point> >  contours;
		findContours(wordsmat,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE);//提取边缘

		int cmin= 150;  // 滤除边缘过长或过短的 ,其余的视为字幕边缘
		int cmax= 500; //  
		vector<vector<Point>>::const_iterator itc= contours.begin();  
		while (itc!=contours.end()) {  
  
			if (itc->size() < cmin || itc->size() > cmax)  
				itc= contours.erase(itc);  
			else   
				++itc;  
		} 
		cout<<"轮廓数："<<contours.size()<<endl;

		wordsmat.setTo(Scalar(255));  
		drawContours(wordsmat,contours,-1,Scalar(0),2);

		int startx = 500,starty = 500,endx = 0,endy = 0;
		vector<vector<Point>>::const_iterator itc_rec= contours.begin();  
		while (itc_rec!=contours.end())  
		{  
			Rect r0= boundingRect(Mat(*(itc_rec))); 
			CvRect rec = cvRect(r0.x,r0.y,r0.width,r0.height);
			//每个字的大小在50*50左右，求出字幕左上角与右下角
			if(r0.x < startx) {
				startx = r0.x;
			}
			if(r0.y < starty) {
				starty = r0.y;
			}
			if((r0.x+r0.width) > endx) {
				endx = r0.x+r0.width;
			}
			if((r0.y+r0.height) > endy) {
				endy = r0.y+r0.height;
			}
			cvRectangle(words,cvPoint(r0.x,r0.y),cvPoint(r0.x+r0.width,r0.y+r0.height),cvScalar(255,255,255));
			IplImage* oneword = cvCreateImage(cvSize(r0.width,r0.height),8,1);//每个单个字都截取出来并保存
			cvSetImageROI(words,rec);
			cvCopy(words,oneword);
			cvNot(oneword, oneword);
			wordsvector.push_back(oneword);
			cvResetImageROI(words);
			rectangle(wordsmat,r0,Scalar(255,255,255),1);  
			++itc_rec;  
		}  

		char c[50];
		char s[50];
		for(int i=0; i<wordsvector.size(); i++) {
			sprintf(c,"%d",i);
			sprintf(s,"%d.jpg",i);
			//cvShowImage(c,wordsvector[i]);//显示单个字
			//cvSaveImage(s,wordsvector[i]);
			cvReleaseImage(&wordsvector[i]);
		}
		//imshow("Contours",wordsmat); 

		cvNot(words, words);
		cvCircle(words,cvPoint(startx,starty),3,cvScalar(0,0,0),3);//画出字幕左上角与右下角
		cvCircle(words,cvPoint(endx,endy),3,cvScalar(0,0,0),3);

		if((endx-startx) > 0 && abs((int)(contours.size()*55) - (endx-startx)) < 100 ) {
			IplImage* wordscut = cvCreateImage(cvSize(endx-startx, 55),8,1);

			cvSetImageROI(wordscopy,cvRect(startx,starty,endx-startx,55));
			cvCopy(wordscopy,wordscut);
			cvNot(wordscut, wordscut);
			cvShowImage("wordscut",wordscut);
			if(framecount == 1) {
				wordsprepare.push_back(wordscut);
			} 
			else {
				CompareHist(wordsprepare[0],wordscut,contours.size());//比较前后两帧的白色像素点个数
				cvReleaseImage(&wordsprepare[0]);
				wordsprepare.clear();
				wordsprepare.push_back(wordscut);
			}
			//cvReleaseImage(&wordscut);
		}
		char z[50];
		for(int i=0; i<wordstodetect.size(); i++) {
			sprintf(z,"wordstodetect%d",i);
			cvShowImage(z,wordstodetect[i]);
		}

		cvShowImage("video",words);
		cvSaveImage("E:\\words.jpg",words);
		IplImage* frameresize = cvCreateImage(cvSize(640,640),8,3);
		cvResize(frame,frameresize);
		cvShowImage("frame",frameresize);
		cvReleaseImage(&frameresize);
		//cvReleaseImage(&frame);

		cvReleaseImage(&words);
		cvReleaseImage(&wordsdilate);
		cvWaitKey(10);
	}

	//OCR识别部分
	tesseract::PageSegMode pagesegmode = tesseract:: PSM_SINGLE_LINE;
    tesseract::TessBaseAPI api;
	api.Init(NULL, "chi_sim", tesseract::OEM_DEFAULT);//识别汉字
	char a[100];
	ofstream out("adc.txt");
	for(int i = 0; i < wordstodetect.size(); i++) {
		sprintf(a,"E:\\wordstodetect%d.png",i);//每一行字幕的保存路径
		cvSaveImage(a,wordstodetect[i]);
		STRING text_out;  
		if (!api.ProcessPages(a, NULL, 0, &text_out))  
		{  
			return 0;  
		}   
		out<<text_out.string()<<endl;
	}

}

int CompareHist(IplImage *image1,IplImage *image2,int thisCount) { 
	int compares;
	int comparesValue;
	compares = abs(cvCountNonZero(image1) - cvCountNonZero(image2));
    printf("Compares : %d\n",compares );
	cout<<"abscount"<<abs(countOfCharacter-thisCount)*2500<<endl;
	if(abs(countOfCharacter-thisCount)<=2) {
		comparesValue = 5000;
	}
	else {
		comparesValue = abs(countOfCharacter-thisCount)*2500;
	}
	if(compares < 500 && framecount == 2) {
		IplImage* wordscopy = cvCreateImage(cvGetSize(image1),8,1);
		cvCopy(image1,wordscopy);
		wordstodetect.push_back(wordscopy);
		countOfFrame = framecount;
		countOfCharacter = thisCount;
		cout<<"firstimage pushback!!"<<endl;
	}
	else if(800 < compares && compares < comparesValue) {
		if((framecount - countOfFrame) > 6*countOfCharacter)
		{
			IplImage* wordscopy = cvCreateImage(cvGetSize(image2),8,1);
			cvCopy(image2,wordscopy);
			wordstodetect.push_back(wordscopy);
			countOfCharacter = thisCount;
			countOfFrame = framecount;
			cout<<"Change words to pushback!!"<<endl;
		}
	}
	else {
		cout<<"No use wordsimage!"<<endl;
	}
//	cvShowImage("image1",image1);
//	cvShowImage("image2",image2);
    return 0;  
}  
