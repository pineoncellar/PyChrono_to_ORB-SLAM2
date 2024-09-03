// 该文件将打开给定的视频文件，并将图像传递给ORB-SLAM2进行定位

// 需要opencv
#include <opencv2/opencv.hpp>

// ws库
#include "easywsclient.hpp"

#pragma comment( lib, "ws2_32" )
#include <WinSock2.h>

// ORB-SLAM的系统接口
#include "System.h"

#include <assert.h>
#include <stdio.h>
#include <string>
#include <chrono>   // for time stamp
#include <iostream>

using namespace std;
using namespace cv;

using easywsclient::WebSocket;
static WebSocket::pointer ws = NULL;

// 记录系统时间
auto start = chrono::system_clock::now();

// 参数文件与字典文件
string parameterFile = "./myvideo.yaml";
string vocFile = "./Vocabulary/ORBvoc.txt";

// 声明 ORB-SLAM3 系统
ORB_SLAM3::System SLAM(vocFile, parameterFile, ORB_SLAM3::System::MONOCULAR, true);


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


    cv::Mat frame;
    frame = image;   // 读取相机数据
    if (frame.data == nullptr)
        return;

    // rescale because image is too large
    cv::Mat frame_resized;
    cv::resize(frame, frame_resized, cv::Size(640, 480));

    auto now = chrono::system_clock::now();
    auto timestamp = chrono::duration_cast<chrono::milliseconds>(now - start);
    SLAM.TrackMonocular(frame_resized, double(timestamp.count()) / 1000.0);
        cv::waitKey(30);

    imshow("Received Image", image);
    waitKey(1);
}

int main(int argc, char** argv) {

    INT rc;
    WSADATA wsaData;

    rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc) {
        printf("WSAStartup Failed.\n");
        return 1;
    }

    static WebSocket::pointer ws = WebSocket::from_url("ws://127.0.0.1:29507");
    assert(ws);
    ws->send("{\"action\": \"start video stream\",\"data\" : {}}");
    while (ws->getReadyState() != WebSocket::CLOSED) {
        ws->poll();
        ws->dispatch(handle_message);
    }
    delete ws;
    WSACleanup();

    SLAM.Shutdown();
    return 0;
}
