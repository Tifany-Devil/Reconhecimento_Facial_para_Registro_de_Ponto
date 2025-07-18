#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <iostream>
#include <fstream>
#include <map>

using namespace std;
using namespace cv;

// Função para carregar labels.txt
map<int, string> loadLabels(const string& filename) {
    map<int, string> labels;
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "[ERRO] Não foi possível abrir " << filename << endl;
        return labels;
    }
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

int main() {
    // Caminhos
    string model_path = "/home/pi/ReconhecimentoFacial2/models/lbph_model.yml";
    string labels_path = "/home/pi/ReconhecimentoFacial2/models/labels.txt";
    string imagem_path = "/home/pi/ReconhecimentoFacial2/data/Tifany/Tifany_20250606_152304.jpg"; // substitua pela imagem desejada

    // Carrega labels
    map<int, string> labels = loadLabels(labels_path);
    if (labels.empty()) {
        cerr << "[ERRO] labels.txt vazio ou inválido." << endl;
        return 1;
    }

    // Cria reconhecedor e carrega modelo
    Ptr<face::LBPHFaceRecognizer> model = face::LBPHFaceRecognizer::create();
    model->read(model_path);

    // Carrega imagem
    Mat img = imread(imagem_path, IMREAD_GRAYSCALE);
    if (img.empty()) {
        cerr << "[ERRO] Falha ao carregar imagem " << imagem_path << endl;
        return 1;
    }

    resize(img, img, Size(200, 200)); // garantir tamanho correto

    int label;
    double confidence;
    model->predict(img, label, confidence);

    cout << "[INFO] Label: " << label << " | Confiança: " << confidence << endl;

    if (confidence < 100.0 && labels.count(label)) {
        cout << "[RESULTADO] Reconhecido: " << labels[label] << endl;
    } else {
        cout << "[RESULTADO] Desconhecido" << endl;
    }

    return 0;
}
