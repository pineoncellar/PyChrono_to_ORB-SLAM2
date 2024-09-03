// 该文件将打开给定的视频文件，并将图像传递给ORB-SLAM2进行定位

// 需要opencv
#include <opencv2/opencv.hpp>

// ORB-SLAM的系统接口
#include "System.h"

// ws库
#include "easywsclient.hpp"

/*
#ifdef _WIN32
#pragma comment( lib, "ws2_32" )
#include <WinSock2.h>
#endif
*/

#include <assert.h>
#include <stdio.h>
#include <string>
#include <chrono>   // for time stamp
#include <iostream>

using namespace std;
using namespace cv;

using easywsclient::WebSocket;
static WebSocket::pointer ws = NULL;


void handle_message(const std::string& message)
{
    //printf(">>> %s\n", message.c_str());
    //if (message == "world") { ws->close(); }

    // 将接收到的字符串数据转换为OpenCV的Mat对象
    std::vector<uchar> data(message.begin(), message.end());
    Mat image = imdecode(data, IMREAD_COLOR);

    // 检查图像是否成功解码
    if (image.empty()) {
        std::cerr << "Failed to decode image data" << std::endl;
    }
    else
    {
        std::cout << "get img." << std::endl;
    }

    imshow("Received Image", image);
    waitKey(1);
}


// 参数文件与字典文件
string parameterFile = "./myvideo.yaml";
string vocFile = "./Vocabulary/ORBvoc.txt";

// 视频文件，修改的话需要和你的视频名字一起改
string videoFile = "./myvideo.mp4";

int main(int argc, char** argv) {

    // 声明 ORB-SLAM3 系统
    ORB_SLAM3::System SLAM(vocFile, parameterFile, ORB_SLAM3::System::MONOCULAR, true);

    // 获取视频图像
    cv::VideoCapture cap(videoFile);    // change to 1 if you want to use USB camera.

    // 记录系统时间
    auto start = chrono::system_clock::now();

    while (1) {
        cv::Mat frame;
        cap >> frame;   // 读取相机数据
        if (frame.data == nullptr)
            break;

        // rescale because image is too large
        cv::Mat frame_resized;
        cv::resize(frame, frame_resized, cv::Size(640, 480));

        auto now = chrono::system_clock::now();
        auto timestamp = chrono::duration_cast<chrono::milliseconds>(now - start);
        SLAM.TrackMonocular(frame_resized, double(timestamp.count()) / 1000.0);
        cv::waitKey(30);
    }

    SLAM.Shutdown();
    return 0;
}