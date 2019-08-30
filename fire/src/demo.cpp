#include "ji.hpp"
#include <string.h>
#include <iostream>
#include <fstream>
#include "ji_util.h"

#include <fstream>  
#include <opencv2/opencv.hpp>			// opencv
#include "opencv2/core/version.hpp"
#include "opencv2/videoio/videoio.hpp"
#include "ji.h"
using namespace std;

int main(int argc,char* argv[]){
	/* init model  */
       //argv[1] 为输入图片 argv[2] 为输入的file图 argv[3] 为输入的video文件 argv[4] 为输入的frame图片 
	ji_init(argc,argv);
	void *det;

	det=ji_create_predictor();
	
	//const char * roiErea = "POLYGON((0.1 0.1, 0.9 0.5,0.35 0.4,0.66 0.66, 0.78 0.78))";
	//const char * roiErea = "POLYGON((0.1 0.1, 0.9  0.1))";
	const char * roiErea = "POLYGON((0.1 0.1, 0.9  0.1, 0.5 0.9))";
	//const char * roiErea = "";
	//buffer=================================================================================
	
	char *json = NULL;
	vector<unsigned char> buffer;
	int ret = ji_file2buffer(argv[1],&buffer);
	if(ret!=0){
		cout<<"convert file to buffer failed."<<endl;
	}
	
	ret = ji_calc(det,buffer.data(),buffer.size(),roiErea,"result_buffer.jpg",&json);
	if(ret!=0){
		cout<<"ji_calc failed."<<endl;
	}
	else cout<<json<<endl;
	
	
	
	//file====================================================================================
	ret=ji_calc_file(det,argv[2],roiErea,"result_file.jpg",&json);
	if(ret!=0){
		cout<<"ji_calc_file failed."<<endl;
	}
	else cout<<json<<endl;
	

	
	//video===================================================================================================
  	JI_EVENT p;
  	 ret=ji_calc_video_file(det,argv[3],roiErea,"result.avi",&p);
  	if(ret!=0){
  		cout<<"ji_calc_video_file failed."<<endl;
  		return 1;
  	}
  	cout<<p.json<<endl;
  	delete[] p.json;
  	cout<<"end"<<endl;
	
	
	//video frame=============================================================================================

	JI_EVENT q;
	JI_CV_FRAME ji_frame;
	JI_CV_FRAME out_frame;
	cv::Mat mat_frame;
	
	mat_frame=cv::imread(argv[4]);
	ji_frame.cols=(int) mat_frame.cols;
	ji_frame.data=(void*)mat_frame.data;
	ji_frame.rows=(int) mat_frame.rows;
	ji_frame.step=mat_frame.step;
	ji_frame.type=mat_frame.type();
	ret=ji_calc_video_frame(det,&ji_frame,roiErea,&out_frame,&q);
	if(ret!=0){
		cout<<"ji_calc_video_frame failed."<<endl;
		return 1; 
	}
	cout<<q.json<<endl;
	delete[] q.json;
 	cv::Mat outfile(out_frame.rows,out_frame.cols,out_frame.type,(unsigned char*)out_frame.data,out_frame.step);
	cv::imwrite("result_frame.jpg",outfile);
	delete [] (uchar*)out_frame.data;
	ji_destory_predictor(det);

	return 0;
}
