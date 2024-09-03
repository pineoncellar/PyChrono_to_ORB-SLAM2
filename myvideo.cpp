// ���ļ����򿪸�������Ƶ�ļ�������ͼ�񴫵ݸ�ORB-SLAM2���ж�λ

// ��Ҫopencv
#include <opencv2/opencv.hpp>

// ORB-SLAM��ϵͳ�ӿ�
#include "System.h"

// ws��
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

    // �����յ����ַ�������ת��ΪOpenCV��Mat����
    std::vector<uchar> data(message.begin(), message.end());
    Mat image = imdecode(data, IMREAD_COLOR);

    // ���ͼ���Ƿ�ɹ�����
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


// �����ļ����ֵ��ļ�
string parameterFile = "./myvideo.yaml";
string vocFile = "./Vocabulary/ORBvoc.txt";

// ��Ƶ�ļ����޸ĵĻ���Ҫ�������Ƶ����һ���
string videoFile = "./myvideo.mp4";

int main(int argc, char** argv) {

    // ���� ORB-SLAM3 ϵͳ
    ORB_SLAM3::System SLAM(vocFile, parameterFile, ORB_SLAM3::System::MONOCULAR, true);

    // ��ȡ��Ƶͼ��
    cv::VideoCapture cap(videoFile);    // change to 1 if you want to use USB camera.

    // ��¼ϵͳʱ��
    auto start = chrono::system_clock::now();

    while (1) {
        cv::Mat frame;
        cap >> frame;   // ��ȡ�������
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