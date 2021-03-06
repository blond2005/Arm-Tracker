////
////  main.cpp
////  Project_Vision: Arm-Tracker
////
//// By Alia Hassan & Shehab Mohamed
//// CS463: Fundamentals of Computer Vision
//// The American University in Cairo
////

#include <iostream>
#include <string>
#include <vector>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#include "opencv2/objdetect/objdetect.hpp"

using namespace std;
using namespace cv;


int main()
{
    VideoCapture cap(0);
    if(!cap.isOpened())
        cout<<"Webcam is not opened."<<endl;
    
    // Window 640x480
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    
    
    Mat frame, frame_HSV, frame_YCRCB;
    Mat Skin_Mask_YCRCB, Skin_Mask_YCRCB_ROI;
    vector<vector<Point>> paths;
    
    while(true)
    {
        cap.read(frame);
        cv::flip(frame, frame, 1);
        
        //Good Skin-Tone Range.
        auto lower_YCRCB = Scalar(0, 148, 88);
        auto upper_YCRCB = Scalar(255, 210, 150);
        
        
        cvtColor(frame, frame_YCRCB, CV_BGR2YCrCb);

        vector<Mat> Contours;
        vector<Vec4i> Hierarchy;
        
        
        // Region of Interest 2/3 Lower Image.
        Mat frame_YCRCB_ROI = frame_YCRCB(Range((int)frame_YCRCB.rows/3, frame_YCRCB.rows), Range(0, frame_YCRCB.cols));
        Mat frame_ROI = frame(Range((int)frame.rows/3, frame.rows), Range(0, frame.cols));
        inRange(frame_YCRCB_ROI, lower_YCRCB, upper_YCRCB, Skin_Mask_YCRCB_ROI);
        inRange(frame_YCRCB, lower_YCRCB, upper_YCRCB, Skin_Mask_YCRCB);
        
        medianBlur(Skin_Mask_YCRCB_ROI, Skin_Mask_YCRCB_ROI, 5);
        medianBlur(Skin_Mask_YCRCB, Skin_Mask_YCRCB, 5);
        
        // Applying Region of Interest
        findContours(Skin_Mask_YCRCB_ROI, Contours, Hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        vector<Mat> Better_Contours;
        vector<Point> largest (1);
        
        for(int i=0; i<Contours.size(); i++)
        {
            auto area = contourArea(Contours[i]);
            if(area > 1200 && area < 15000)
            {
                auto M = moments(Contours[i]);
                int cX = M.m10 / M.m00;
                int cY = M.m01 / M.m00;
                Better_Contours.push_back(Contours[i]);
                drawContours(frame_ROI, Contours, i, Scalar(0,255,0), 3);
                //Drawing Centroid on Contours.
                circle(frame_ROI, Point(cX, cY), 3, Scalar(255,0,0), -1);
            }
        }
        
        //Calculating Paths
        for( int a = 0; a < Better_Contours.size() ; a++)
        {
            auto M = moments(Better_Contours[a]);
            int cX = M.m10 / M.m00;
            int cY = M.m01 / M.m00;
            bool close = false;
            
            if ( cX > 0 && cY > 0)
            {
                for (int b = 0; b < paths.size(); b++)
                {
                    if ( abs(cX - paths[b].back().x) < 30 && abs(cY - paths[b].back().y) < 30)
                    {
                        paths[b].push_back(Point(cX, cY));
                        close = true;
                        break;
                    }
                    else
                        paths[b].push_back(paths[b].back());
                }
                
                if (close == false)
                {
                    vector<Point> temp;
                    temp.push_back(Point(cX, cY));
                    paths.push_back(temp);
                }
                cout << "coor " << cX << " " << cY << endl;
            }
            
        }
        
        //Path clean up - removing obsolete paths
        for (int d = 0; d < paths.size(); d++)
        {
            if (paths[d].size() > 5)
            {
                if( paths[d][paths[d].size()-1] == paths[d][paths[d].size()-2] && paths[d][paths[d].size()-2] == paths[d][paths[d].size()-3] && paths[d][paths[d].size()-3] == paths[d][paths[d].size()-4])
                {
                    paths.erase(paths.begin()+ d);
                }
            }
        }
        
        //Drawing paths
        for (int c = 0; c < paths.size(); c++)
        {
            if( paths[c].size() >  40)
            {
                for ( int j = (int)paths[c].size() - 40; j < paths[c].size(); j++)
                    line(frame_ROI, paths[c][j-1], paths[c][j], Scalar(0,0,0), 3);
            }
        }
        
        //////////////// Convex Hull ////////////////
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        
        vector<vector<Point>> hull(Better_Contours.size());
        
        //Calcuating Convex Hull.
        for(int i=0; i<Better_Contours.size(); i++)
            convexHull(Mat(Better_Contours[i]), hull[i], false);
        
        //Drawing Convex Hull.
        for(int i=0; i<Better_Contours.size(); i++)
        {
            auto area2 = contourArea(hull[i]);
            auto ratio = Contours[i].rows/area2;
            cout<<"Ratio: "<<ratio<<endl;
            drawContours( frame_ROI, hull, i, Scalar(0,0,255), 3, 8, vector<Vec4i>() );
        }
        
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        imshow("Arm Tracker", frame);
        //imshow("Masked Skin-Tone", Skin_Mask_YCRCB);
    }
    
    return 0;
}

// RGB Color Segmentation. Function not used.
//void RGB_Range(Mat& frame)
//{
//    for(int i=0; i<frame.rows; i++)
//    {
//        for(int j=0; j<frame.cols; j++)
//        {
//            int R = frame.at<Vec3b>(i, j)[0];
//            int G = frame.at<Vec3b>(i, j)[1];
//            int B = frame.at<Vec3b>(i, j)[2];
//
//            bool e1 = (R>95) && (G>40) && (B>20) && ((max(R,max(G,B)) - min(R, min(G,B)))>15) && (abs(R-G)>15) && (R>G) && (R>B);
//            bool e2 = (R>220) && (G>210) && (B>170) && (abs(R-G)<=15) && (R>B) && (G>B);
//            
//            if(e1 || e2)
//            {
//                frame.at<Vec3b>(i, j)[0] = 255;
//                frame.at<Vec3b>(i, j)[1] = 255;
//                frame.at<Vec3b>(i, j)[2] = 255;
//            }
//        }
//    }
//}