#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <ctime>
#include <sstream>

using namespace std;
using namespace cv;

const double LIMIAR_CONFIANCA = 58.0;
const int CONFIRMACAO_N_FRAMES = 3;
const string ARQUIVO_CSV = "/home/pi/ReconhecimentoFacial2/python/registro.csv";

map<int, string> loadLabels(const string& filename) {
    map<int, string> labels;
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        size_t sep = line.find(';');
        if (sep != string::npos) {
            int id = stoi(line.substr(0, sep));
            string nome = line.substr(sep + 1);
            labels[id] = nome;
        }
    }
    return labels;
}

string timestamp() {
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return string(buf);
}

void registrar_ponto_csv(const string& nome) {
    vector<vector<string>> tabela;
    ifstream arq_in(ARQUIVO_CSV);
    string linha;

    while (getline(arq_in, linha)) {
        stringstream ss(linha);
        string cel;
        vector<string> row;
        while (getline(ss, cel, ',')) row.push_back(cel);
        tabela.push_back(row);
    }
    arq_in.close();

    string agora = timestamp();
    bool encontrado = false;
    bool pode_registrar = true;

    for (auto& row : tabela) {
        if (!row.empty() && row[0] == nome) {
            encontrado = true;
            if (row.size() > 1) {
                string ultimo_ponto = row.back();
                struct tm tm_ultimo = {};
                strptime(ultimo_ponto.c_str(), "%Y-%m-%d %H:%M:%S", &tm_ultimo);
                tm_ultimo.tm_isdst = -1;  // corrige horário de verão/local
                time_t t_ultimo = mktime(&tm_ultimo);

                time_t t_agora = time(nullptr);
                double diff_minutos = difftime(t_agora, t_ultimo) / 60.0;

                if (diff_minutos < 30.0) {
                    cout << "[NEGADO] Último ponto de " << nome << " foi há "
                         << static_cast<int>(diff_minutos)
                         << " minutos. Aguarde 30 minutos.\n";
                    pode_registrar = false;
                }
            }

            if (pode_registrar) {
                row.push_back(agora);
                cout << "[REGISTRADO] Ponto para " << nome << " às " << agora << endl;
            }
            break;
        }
    }

    if (!encontrado && pode_registrar) {
        tabela.push_back({nome, agora});
        cout << "[REGISTRADO] Primeiro ponto para " << nome << " às " << agora << endl;
    }

    if (pode_registrar) {
        ofstream arq_out(ARQUIVO_CSV, ios::trunc);
        for (const auto& row : tabela) {
            for (size_t i = 0; i < row.size(); i++) {
                arq_out << row[i];
                if (i != row.size() - 1) arq_out << ",";
            }
            arq_out << "\n";
        }
        arq_out.close();
    }
}

int main() {
    string model_path  = "/home/pi/ReconhecimentoFacial2/models/lbph_model.yml";
    string labels_path = "/home/pi/ReconhecimentoFacial2/models/labels.txt";
    string cascade_f   = "/home/pi/ReconhecimentoFacial2/models/haarcascade_frontalface_default.xml";

    CascadeClassifier face_cascade;
    if (!face_cascade.load(cascade_f)) {
        cerr << "Erro ao carregar cascade.\n";
        return -1;
    }

    Ptr<face::LBPHFaceRecognizer> model = face::LBPHFaceRecognizer::create();
    model->read(model_path);
    map<int, string> labels = loadLabels(labels_path);
    if (labels.empty()) {
        cerr << "Erro: labels.txt está vazio.\n";
        return -1;
    }

    string pipeline =
        "libcamerasrc ! video/x-raw,width=640,height=480,format=(string)BGRx,framerate=30/1 "
        "! videoconvert ! video/x-raw,format=(string)BGR ! appsink";
    VideoCapture cap(pipeline, CAP_GSTREAMER);
    if (!cap.isOpened()) {
        cerr << "Não foi possível abrir a câmera.\n";
        return -1;
    }

    namedWindow("Reconhecimento Facial", WINDOW_AUTOSIZE);
    cout << "[INFO] Pressione 'q' para sair.\n";

    string nome_exibido = "";
    string nome_temp = "";
    int contador_confirmado = 0;
    int cooldown = 0;
    const int COOLDOWN_MAX = 10;

    while (true) {
        Mat frame;
        cap >> frame;
        if (frame.empty()) break;

        Mat gray;
        cvtColor(frame, gray, COLOR_BGR2GRAY);

        vector<Rect> faces;
        face_cascade.detectMultiScale(gray, faces, 1.1, 5, 0, Size(100, 100));

        if (!faces.empty() && cooldown == 0) {
            Rect r = faces[0];
            Mat faceROI = gray(r).clone();
            resize(faceROI, faceROI, Size(200, 200));
            equalizeHist(faceROI, faceROI);

            int label;
            double confidence;
            model->predict(faceROI, label, confidence);

            string nome_prev = labels.count(label) ? labels[label] : "(desconhecido)";
            cout << "[DEBUG] Previsto: " << nome_prev
                 << " | Label: " << label
                 << " | Confiança: " << confidence << endl;

            if (confidence < LIMIAR_CONFIANCA && labels.count(label)) {
                if (nome_prev == nome_temp) {
                    contador_confirmado++;
                } else {
                    nome_temp = nome_prev;
                    contador_confirmado = 1;
                }

                if (contador_confirmado >= CONFIRMACAO_N_FRAMES) {
                    nome_exibido = nome_prev;
                    cout << "[✔] Reconhecido: " << nome_exibido << endl;

                    // Confirmação manual
                    cout << "Funcionário: " << nome_exibido << ". Deseja bater ponto? (s/N): ";
                    char resp;
                    cin >> resp;
                    if (resp == 's' || resp == 'S') {
                        registrar_ponto_csv(nome_exibido);
                    } else {
                        cout << "[CANCELADO] Ponto não registrado.\n";
                    }

                    cooldown = COOLDOWN_MAX;
                } else {
                    cout << "[...] Confirmando: " << nome_prev << " (" << contador_confirmado << "/" << CONFIRMACAO_N_FRAMES << ")\n";
                }
            } else {
                nome_temp = "";
                contador_confirmado = 0;
                cout << "[X] Confiança alta (" << confidence << ") → Desconhecido\n";
            }
        }

        if (cooldown > 0) cooldown--;

        if (!faces.empty() && !nome_exibido.empty()) {
            Rect r = faces[0];
            int font = FONT_HERSHEY_SIMPLEX;
            double fs = 0.7;
            int th = 2;
            string txt = nome_exibido;
            Size ts = getTextSize(txt, font, fs, th, nullptr);
            Point pt(r.x, r.y - 10);
            Mat overlay = frame.clone();
            rectangle(overlay, Rect(pt.x, pt.y - ts.height, ts.width+10, ts.height+5),
                      Scalar(255,255,255), FILLED);
            addWeighted(overlay, 0.4, frame, 0.6, 0, frame);
            putText(frame, txt, Point(pt.x+5, pt.y), font, fs, Scalar(0,0,0), th);
        }

        imshow("Reconhecimento Facial", frame);
        char key = (char)waitKey(1);
        if (key == 'g') {
    cout << "[SYNC] Enviando registro.csv para a planilha Google..." << endl;
    int status = system("python3 /home/pi/ReconhecimentoFacial2/python/sync_csv_to_google.py");
    if (status == 0) {
        cout << "[OK] Sincronização concluída." << endl;
    } else {
        cout << "[ERRO] Falha ao sincronizar (código " << status << ")." << endl;
    }
}
        if (key == 'q') break;
    }

    cap.release();
    destroyAllWindows();
    return 0;
}
