#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <thread>
#include <atomic>
#include <mutex>  
#include <condition_variable> // std::condition_variable


#include <opencv2/opencv.hpp>			// opencv
#include "opencv2/core/version.hpp"
#include "opencv2/videoio/videoio.hpp"

#include "ji.h"
#include "json/json.h"        //jsoncpp
#include "ji.hpp"
#include<boost/geometry.hpp>
#include<boost/geometry/geometries/point_xy.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include "roi/BoostInterface.h"

//#include "ji_util.h"
using namespace std;
int WIDTH=416;
int HEIGHT=416;
bool draw_analysis_result =false;
bool show_video =false;
bool show_roi = false;
float thresh=0.2;
int cur_gpu_id = 0;
void getpoint(const char* args,cv::Size cur_size,vector<cv::Point> &points){
    std::string str;
    vector<string> vStr;
    str=args;
    boost::split( vStr, str, boost::is_any_of( "-" ), boost::token_compress_on );
    for( vector<string>::iterator it = vStr.begin(); it != vStr.end(); ++ it ){
	boost::geometry::model::d2::point_xy<double> a;
	boost::geometry::read_wkt(*it,a);
	cv::Point pointcv=cv::Point((a.x()*cur_size.width),(a.y()*cur_size.height));
	points.push_back(pointcv);
    }
}
cv::Mat getRoi(cv::Mat roi_mat,float f_rate){
    cv::Mat back(WIDTH, HEIGHT, CV_8UC3, Scalar(0, 0, 0));

    cv::resize(roi_mat,roi_mat,cv::Size(0,0),f_rate,f_rate);

    cv::Mat backroi=back(cv::Rect(0,0,roi_mat.cols,roi_mat.rows));
    cv::addWeighted(backroi,0.0,roi_mat,1.0,0,backroi);
    return back;
}

void goback(std::vector<bbox_t> &result_vec,cv::Rect rec,float f_rate){
      for (auto &i : result_vec){
	  i.x=rec.x+i.x/f_rate;
	  i.y=rec.y+i.y/f_rate;
	  i.w=i.w/f_rate;
	  i.h=i.h/f_rate;
      }
}
  
int write_result(std::vector<bbox_t> result_vec,Json::Value &root){
    
    //Json::Value obj_img;
    int fire_num = 0;
    for (auto &i : result_vec) {
        if(i.obj_id==0){
            root["flameInfo"][fire_num]["x"]=i.x;
            root["flameInfo"][fire_num]["y"]=i.y;
            root["flameInfo"][fire_num]["width"]=i.w;
            root["flameInfo"][fire_num]["height"]=i.h;
            fire_num++;
        }
    }

    if(fire_num>0){
        root["alertFlag"]=1;
        root["numOfFlameRects"]=fire_num;
        root["message"]="Waning! Flame is being detected";
    }
    else{
        root["flameInfo"]={};
        root["alertFlag"]=0;
        root["numOfFlameRects"]=fire_num;
        root["message"]="Safely! No Flame";
    }
    return 0;
}

void draw_boxes(cv::Mat mat_img, std::vector<bbox_t> result_vec,cv::Rect *rec = NULL, int current_det_fps = -1, int current_cap_fps = -1)
{
	if(show_roi){
	  if(rec!=NULL){
	    cv::rectangle(mat_img, *rec,cv::Scalar(255, 0, 0), 3, 8, 0);
	  }
	}
	if(draw_analysis_result){
	for (auto &i : result_vec) {
		cv::Scalar color = obj_id_to_color(i.obj_id);
		
		if (i.obj_id==0) {
			std::string obj_name = "fire";
				cv::rectangle(mat_img, cv::Rect(i.x, i.y, i.w, i.h), color, 2);
				//if (i.track_id > 0) obj_name += " - " + std::to_string(i.track_id);
				cv::Size const text_size = getTextSize(obj_name, cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, 2, 0);
				//int const max_width = (text_size.width > i.w + 2) ? text_size.width : (i.w + 2);
				int const max_width = text_size.width;
				cv::rectangle(mat_img, cv::Point2f(std::max((int)i.x - 1, 0), std::max((int)i.y - 22, 0)),
					cv::Point2f(std::min((int)i.x + max_width, mat_img.cols - 1), std::min((int)i.y, mat_img.rows - 1)),
					color, CV_FILLED, 8, 0);
				putText(mat_img, obj_name, cv::Point2f(i.x, i.y - 8), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, cv::Scalar(0, 0, 0), 2);
			}
	}
	}
	// if(current_det_fps >= 0 && current_cap_fps >= 0) {
	// 	std::string fps_str = "FPS detection: " + std::to_string(current_det_fps) + "   FPS capture: " + std::to_string(current_cap_fps);
	// 	putText(mat_img, fps_str, cv::Point2f(10, 20), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(50, 200, 0), 2);
	// }
}

void write_video_result(std::vector<bbox_t> result_vec, double ttime,int id,Json::Value &root_out){
    int fire_num = 0;
    string id_string=to_string(id);
    //int id_string=id;
    //int id_string=id;
    Json::Value root;
	 for (auto &i : result_vec) {
       
         if(i.obj_id==0){
            
            root[id_string]["flameInfo"][fire_num]["x"]=i.x;
            root[id_string]["flameInfo"][fire_num]["y"]=i.y;
            root[id_string]["flameInfo"][fire_num]["width"]=i.w;
            root[id_string]["flameInfo"][fire_num]["height"]=i.h;
            fire_num++;
        }      

	}
    if(fire_num>0){
        root[id_string]["alertFlag"]=1;
        root[id_string]["timeStamp"]=int(ttime);
        root[id_string]["numOfFlameRects"]=fire_num;
        root[id_string]["message"]="Waning! Flame is being detected";
	root_out.append(root);
    }
    else{
        root[id_string]["flameInfo"]={};
        root[id_string]["alertFlag"]=0;
        root[id_string]["timeStamp"]=int(ttime);
        root[id_string]["numOfFlameRects"]=fire_num;
        root[id_string]["message"]="Safely! No Flame";
    }
    //root_out.append(root);
}

void write_frame_result(std::vector<bbox_t> result_vec, int id,Json::Value &root){
    int fire_num = 0;
    string id_string=to_string(id);
    //int id_string=id;
	 for (auto &i : result_vec) {
       
         if(i.obj_id==0){
            
            root[id_string]["flameInfo"][fire_num]["x"]=i.x;
            root[id_string]["flameInfo"][fire_num]["y"]=i.y;
            root[id_string]["flameInfo"][fire_num]["width"]=i.w;
            root[id_string]["flameInfo"][fire_num]["height"]=i.h;
            fire_num++;
        }      

	}
    if(fire_num>0){
        root[id_string]["alertFlag"]=1;
        root[id_string]["numOfFlameRects"]=fire_num;
        root[id_string]["message"]="Waning! Flame is being detected";
    }
    else{
        root[id_string]["flameInfo"]={};
        root[id_string]["alertFlag"]=0;
        root[id_string]["numOfFlameRects"]=fire_num;
        root[id_string]["message"]="Safely! No Flame";
    }
}


int ji_init(int argc, char** argv) {
    fstream f;
    Json::CharReaderBuilder rbuilder;
    Json::Value root;
    JSONCPP_STRING errs;
    f.open ("pro.json",ios::in);
    if(!f.is_open()){
        cout<<"Open json file error!"<<endl;
    }
    bool parse_ok =Json::parseFromStream (rbuilder, f, &root, &errs);
    if (!parse_ok) {
        cout<<"Parse json file error!"<<endl;
    }else{
      draw_analysis_result = root["draw_analysis_result"].asString()=="1" ? true :false;
      show_video = root["show_video"].asString()=="1" ? true :false;
      show_roi = root["show_roi"].asString()=="1" ? true :false;
      thresh = stof(root["thresh"].asString());
      cur_gpu_id = atoi(root["gpu"].asString().c_str());
    }
    
	return 0;
}

void *ji_create_predictor() {
    
    std::string  cfg_file = "./backup/fire.cfg";
    std::string  weights_file = "./backup/fire_weight.weights";

    Detector *p;
    p=new Detector(cfg_file, weights_file,cur_gpu_id);
    
    return p;
}

void ji_destory_predictor(void* predictor) {
    Detector *detector=(Detector*)predictor;
    delete detector;
}

int ji_calc(void* predictor, const unsigned char* buffer, int length,
		const char* args, const char* outfn, char** json) {

    if( buffer == NULL) {
      return -1;
    }
    vector<unsigned char> decodebuf(buffer,buffer+length);
    cv::Mat img =cv::imdecode(cv::Mat(decodebuf), 1);
    if (img.empty()) {
      return -1;
    }
    Json::Value root;
    Detector *detector=(Detector*)predictor;
    
    std::vector<bbox_t> result_vec;
    if(args!=NULL){
      BoostInterface boostInterface;
      cv::Size cur_size=cv::Size(img.cols,img.rows);
      vector<cv::Point> points;
      boostInterface.parsePolygon(args,cur_size,points);
      //getpoint(args,cur_size,points);
      cv::Rect rec= boostInterface.ploygon2Rect(points);
      cv::Mat roi_mat=img(rec).clone();
      int low_lenth = rec.width > rec.height ? rec.width:rec.height;
      float f_rate = (WIDTH-1.0)/low_lenth;
      cv::Mat roi_img = getRoi(roi_mat,f_rate);
      result_vec= detector->detect(roi_img,thresh); 
      roi_img.release();
      goback(result_vec,rec,f_rate);
      if(outfn!=NULL){
        draw_boxes(img, result_vec,&rec);
        cv::imwrite(outfn,img);
    }
    }
    else{
      result_vec = detector->detect(img);
      if(outfn!=NULL){
        draw_boxes(img, result_vec);
        cv::imwrite(outfn,img);
    }
    }
        
    write_result(result_vec, root);
    
    char* data;
    string s1=root.toStyledString();
    int len = s1.length();
    data = (char *)malloc((len+1)*sizeof(char));
    s1.copy(data,len,0);
    data[len]='\0';
    *json=data; //转换为json格式并存到文件流
/*    if(show_video){
	cv::imshow("window",img); 
	cv::waitKey(0);
    }*/
    return 0;
        

}

int ji_calc_file(void* predictor, const char* infn, const char* args,
		const char* outfn, char** json) {
    Json::Value root;

    cv::Mat img = cv::imread(infn);

    if (img.empty()) {
      return -1;
    }
    Detector *detector=(Detector*)predictor;
    std::vector<bbox_t> result_vec;
    if(args!=NULL){
      BoostInterface boostInterface;
      cv::Size cur_size=cv::Size(img.cols,img.rows);
      vector<cv::Point> points;
      boostInterface.parsePolygon(args,cur_size,points);
      //getpoint(args,cur_size,points);
      cv::Rect rec= boostInterface.ploygon2Rect(points);
      cv::Mat roi_mat=img(rec).clone();
      int low_lenth = rec.width > rec.height ? rec.width:rec.height;
      float f_rate = (WIDTH-1.0)/low_lenth;
      cv::Mat roi_img = getRoi(roi_mat,f_rate);
      result_vec= detector->detect(roi_img,thresh); 
      roi_img.release();
      goback(result_vec,rec,f_rate);
      if(outfn!=NULL){
        draw_boxes(img, result_vec,&rec);
        cv::imwrite(outfn,img);
    }
    }
    else{
      result_vec = detector->detect(img,thresh);
      if(outfn!=NULL){
        draw_boxes(img, result_vec);
        cv::imwrite(outfn,img);
    }
    }
    write_result(result_vec, root);
    char* data;
    string s1=root.toStyledString();
    int len = s1.length();
    data = (char *)malloc((len+1)*sizeof(char));
    s1.copy(data,len,0);
    data[len]='\0';
    *json=data; //转换为json格式并存到文件流  
/*    if(show_video){
	cv::imshow("window",img); 
	waitKey(0);
    }  */  
	return 0;

}

int ji_calc_video_file(void* predictor, const char* infn, const char* args,
		const char* outfn, JI_EVENT* event) {
	    
            Detector *detector=(Detector*)predictor;
            Json::Value root;
	    cv::Mat cur_frame;
            int frame_id=0;
            bool stop=false;
            cv::VideoCapture cap(infn);
            double ttime;
	    
            std::string out_videofile;
	    bool save_output_videofile=false;
	    int const video_fps = cap.get(CV_CAP_PROP_FPS);
            cv::Size const frame_size(cap.get(cv::CAP_PROP_FRAME_WIDTH),cap.get(cv::CAP_PROP_FRAME_HEIGHT));
	    
	    
	    if (!cap.isOpened()) {
	      return -1;
	     }
	    
	    cv::VideoWriter output_video;
            if(outfn!=NULL){
		    out_videofile = outfn;
                save_output_videofile=true;
            }
            if (save_output_videofile){
                output_video.open(out_videofile, CV_FOURCC('D', 'I', 'V', 'X'), video_fps,frame_size, true);
	    }
	    while(!stop){
	      cap >> cur_frame;

	      if(cur_frame.empty()){
		break;
	      }
	      ttime=cap.get(CV_CAP_PROP_POS_MSEC);
	      std::vector<bbox_t> result_vec;
	      if(args!=NULL){
		BoostInterface boostInterface;
		cv::Size cur_size=cv::Size(cur_frame.cols,cur_frame.rows);
		vector<cv::Point> points;
		boostInterface.parsePolygon(args,cur_size,points);
		//getpoint(args,cur_size,points);
		cv::Rect rec= boostInterface.ploygon2Rect(points);
		cv::Mat roi_mat=cur_frame(rec).clone();
		int low_lenth = rec.width > rec.height ? rec.width:rec.height;
		float f_rate = (WIDTH-1.0)/low_lenth;
		cv::Mat roi_img = getRoi(roi_mat,f_rate);
		result_vec= detector->detect(roi_img,thresh); 
		roi_img.release();
		goback(result_vec,rec,f_rate);
		if(outfn!=NULL){
		  draw_boxes(cur_frame, result_vec,&rec);
	      }
	      }
	      else{
		result_vec = detector->detect(cur_frame,thresh);
		if(outfn!=NULL){
		  draw_boxes(cur_frame, result_vec);
	      }
	      }
	      //std::vector<bbox_t> result_vec=detector->detect(cur_frame);
	      write_video_result(result_vec,ttime,frame_id,root);
	      if (output_video.isOpened()){
		output_video << cur_frame;
	    }
	    if(show_video){
	      cv::imshow("window",cur_frame); 
	      if(waitKey(30) >=0) 
		break;
	    }
	    frame_id++;
	    }
        output_video.release();
        //json output

        char* data;
        string s1=root.toStyledString();
    
        int len = s1.length();
        data = (char *)malloc((len+1)*sizeof(char));
        s1.copy(data,len,0);
        data[len]='\0';
        event->json=data; //转换为json格式并存到文件流

        return 0;
}

int ji_calc_video_frame(void* predictor, JI_CV_FRAME* inframe, const char* args,
		JI_CV_FRAME* outframe, JI_EVENT* event) {
        

        Detector *detector=(Detector*)predictor;
        Json::Value root;
        cv::Mat mat_frame(inframe->rows,inframe->cols,inframe->type,(unsigned char*)inframe->data,inframe->step);
	if (mat_frame.empty()) {
	  return -1;
	}
	std::vector<bbox_t> result_vec;
	if(args!=NULL){
	  BoostInterface boostInterface;
	  cv::Size cur_size=cv::Size(mat_frame.cols,mat_frame.rows);
	  vector<cv::Point> points;
	  boostInterface.parsePolygon(args,cur_size,points);
	  //getpoint(args,cur_size,points);
	  cv::Rect rec= boostInterface.ploygon2Rect(points);
	  cv::Mat roi_mat=mat_frame(rec).clone();
	  int low_lenth = rec.width > rec.height ? rec.width:rec.height;
	  float f_rate = (WIDTH-1.0)/low_lenth;
	  cv::Mat roi_img = getRoi(roi_mat,f_rate);
	  result_vec= detector->detect(roi_img,thresh); 
	  roi_img.release();
	  goback(result_vec,rec,f_rate);
	  if(outframe!=NULL){
	    draw_boxes(mat_frame, result_vec,&rec);
	    }
	  }
      else{
	  result_vec = detector->detect(mat_frame,thresh);
	  if(outframe!=NULL){
	    draw_boxes(mat_frame, result_vec);

	}
      }
        write_frame_result(result_vec,0,root);
        if(outframe!=NULL){
            
            outframe->cols=(int) mat_frame.cols;
	        outframe->data=(void*)mat_frame.data;
	        outframe->rows=(int) mat_frame.rows;
	        outframe->step=mat_frame.step;
	        outframe->type=mat_frame.type();
        }

        char* data;
        string s1=root.toStyledString();
        int len = s1.length();
        data = (char *)malloc((len+1)*sizeof(char));
        s1.copy(data,len,0);
        data[len]='\0';
        event->json=data; //转换为json格式并存到文件流
/*	if(show_video){
	cv::imshow("window",mat_frame); 
	waitKey(0);
    }   */     
        //event=event_temp;
	return 0;
}
