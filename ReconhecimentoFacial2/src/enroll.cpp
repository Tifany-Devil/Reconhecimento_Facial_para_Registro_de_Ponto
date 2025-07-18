#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>

using namespace cv;
using namespace std;
using namespace std::chrono;

static string timestamp() {
    auto now = system_clock::now();
    auto in_time_t = system_clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&in_time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

bool createDirectory(const string& path) {
    if (mkdir(path.c_str(), 0755) == 0) return true;
    if (errno == EEXIST) {
        struct stat st;
        return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
    }
    return false;
}

int main() {
    destroyAllWindows();
    string cascade_path = "/home/pi/ReconhecimentoFacial2/models/haarcascade_frontalface_default.xml";
    CascadeClassifier face_cascade;
    if (!face_cascade.load(cascade_path)) {
        cerr << "[ERROR] Falha ao carregar cascade." << endl;
        return -1;
    }
    
        std::cout << "[SYNC] Verificando se modelos estão atualizados no Drive...\n";
    int check_status = system("python3 /home/pi/ReconhecimentoFacial2/python/check_and_pull_modelos.py");
    if (check_status != 0) {
        std::cerr << "[ERRO] Falha ao verificar modelos. Cancelando cadastro.\n";
        return 1;
    }

    cout << "=============== CADASTRO DE FUNCIONARIO ===============" << endl;
    cout << "Digite o nome do funcionário (sem espaços; use underline): ";
    string nome;
    getline(cin, nome);
    if (nome.empty()) return -1;

    string data_dir = "/home/pi/ReconhecimentoFacial2/data";
    string out_dir = data_dir + "/" + nome;
    if (!createDirectory(data_dir) || !createDirectory(out_dir)) {
        cerr << "[ERROR] Falha ao criar diretórios." << endl;
        return -1;
    }

    const string pipeline =
        "libcamerasrc ! video/x-raw,width=320,height=240,format=(string)BGRx,framerate=30/1 "
        "! videoconvert ! video/x-raw,format=(string)BGR ! appsink";

    VideoCapture cap(pipeline, CAP_GSTREAMER);
    if (!cap.isOpened()) {
        cerr << "[ERROR] Falha ao abrir a PiCam." << endl;
        return -1;
    }

    namedWindow("Cadastro", WINDOW_AUTOSIZE);

    int contador = 0;
    const int max_capturas = 20;
    auto last_capture = steady_clock::now();

    while (contador < max_capturas) {
        Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        Mat gray;
        cvtColor(frame, gray, COLOR_BGR2GRAY);

        vector<Rect> faces;
        face_cascade.detectMultiScale(gray, faces, 1.1, 5, 0, Size(100, 100));

        if (!faces.empty()) {
            rectangle(frame, faces[0], Scalar(0, 255, 0), 2);

            auto now = steady_clock::now();
            auto elapsed = duration_cast<milliseconds>(now - last_capture);
            if (elapsed.count() >= 1000) {
                Mat rosto = gray(faces[0]).clone();
                if (!rosto.empty()) {
                    resize(rosto, rosto, Size(200, 200));
                    equalizeHist(rosto, rosto);

                    string fname = out_dir + "/" + nome + "_" + timestamp() + ".jpg";
                    if (imwrite(fname, rosto)) {
                        cout << "[OK] (" << contador + 1 << "/" << max_capturas << ") " << fname << endl;
                        contador++;
                        last_capture = now;
                    } else {
                        cerr << "[ERRO] Falha ao salvar " << fname << endl;
                    }
                }
            }
        } else {
            cout << "[INFO] Nenhuma face detectada." << endl;
        }

        imshow("Cadastro", frame);
        char key = (char)waitKey(1);
        if (key == 'q') {
            cout << "[INFO] Cancelado pelo usuário." << endl;
            break;
        }
    }

    cap.release();
    destroyAllWindows();
    cout << "[FINALIZADO] Total de imagens salvas: " << contador << endl;
    

cout << "[INFO] Iniciando treinamento do modelo..." << endl;
int status = system("python3 /home/pi/ReconhecimentoFacial2/python/train.py");
if (status == 0) {
    cout << "[OK] Treinamento finalizado com sucesso." << endl;
} else {
    cerr << "[ERRO] Falha ao executar train.py (status " << status << ")." << endl;
}

    std::cout << "[SYNC] Enviando modelos atualizados para o Google Drive...\n";
    int upload_status = system("python3 /home/pi/ReconhecimentoFacial2/python/upload_modelos.py");
    if (upload_status != 0) {
        std::cerr << "[ERRO] Falha ao enviar os modelos para o Drive.\n";
    } else {
        std::cout << "[OK] Modelos sincronizados com sucesso.\n";
    }

return 0;
}
