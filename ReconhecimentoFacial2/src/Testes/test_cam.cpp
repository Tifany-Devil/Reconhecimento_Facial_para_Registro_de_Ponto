// src/test_cam.cpp

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main() {
    // O mesmo pipeline GStreamer que funcionou no seu enroll.cpp:
    const string pipeline =
        "libcamerasrc ! video/x-raw,width=640,height=480,format=(string)BGRx,framerate=30/1 "
        "! videoconvert ! video/x-raw,format=(string)BGR ! appsink";

    // Tenta abrir a PiCam via GStreamer:
    VideoCapture cap(pipeline, CAP_GSTREAMER);
    if (!cap.isOpened()) {
        cerr << "[ERRO] Não foi possível abrir a câmera via GStreamer/libcamera." << endl;
        return -1;
    }
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);

    cout << ">>> Conseguiu abrir a PiCam! Pressione 'q' para sair <<<" << endl;

    Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        imshow("Feed PiCam (aperte 'q' para sair)", frame);
        if (waitKey(1) == 'q') break;
    }

    cap.release();
    destroyAllWindows();
    return 0;
}
