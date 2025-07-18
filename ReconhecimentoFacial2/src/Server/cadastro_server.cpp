#include "mongoose.h"
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>

namespace fs = std::filesystem;

const std::string SAVE_DIR       = "/home/pi/ReconhecimentoFacial2/data";
const std::string SCRIPT_CHECK   = "python3 /home/pi/ReconhecimentoFacial2/python/check_and_pull_modelos.py";
const std::string SCRIPT_CHECK_GIT   = "/home/pi/ReconhecimentoFacial2/src/sincronizacao/check_and_pull_git";
const std::string SCRIPT_TRAIN   = "python3 /home/pi/ReconhecimentoFacial2/python/train.py";
const std::string SCRIPT_UPLOAD  = "python3 /home/pi/ReconhecimentoFacial2/python/upload_modelos.py";
const std::string SCRIPT_UPLOAD_GIT  = "/home/pi/ReconhecimentoFacial2/src/sincronizacao/upload_modelos_git";
const std::string CASCADE_PATH   = "/home/pi/ReconhecimentoFacial2/repositorio/Reconhecimento-Facial/models/haarcascade_frontalface_default.xml";

static cv::CascadeClassifier face_cascade;

void salvar_e_processar(const std::string& nome,
                       const std::string& filename,
                       const char* data,
                       size_t size) {
    // Decodifica JPEG em BGR
    cv::Mat buf(1, (int)size, CV_8UC1, (void*)data);
    cv::Mat img = cv::imdecode(buf, cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "[ERRO] Falha ao decodificar imagem " << filename << '\n';
        return;
    }

    // Converte para cinza
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

    // Detecta rosto
    std::vector<cv::Rect> faces;
    face_cascade.detectMultiScale(gray, faces, 1.1, 5, 0, cv::Size(100, 100));
    if (faces.empty()) {
        std::cerr << "[WARN] Nenhuma face detectada em " << filename << '\n';
        return;
    }

    // Extrai ROI, redimensiona e equaliza
    cv::Mat rosto = gray(faces[0]).clone();
    cv::resize(rosto, rosto, cv::Size(200, 200));
    cv::equalizeHist(rosto, rosto);

    // Salva imagem processada
    fs::create_directories(SAVE_DIR + "/" + nome);
    std::string outpath = SAVE_DIR + "/" + nome + "/" + filename;
    if (!cv::imwrite(outpath, rosto)) {
        std::cerr << "[ERRO] Falha ao salvar " << outpath << '\n';
    } else {
        std::cout << "[OK] Salvo e processado: " << outpath << '\n';
    }
}

static void handle(struct mg_connection* c,
                   int ev,
                   void* ev_data,
                   void* fn_data) {
    if (ev != MG_EV_HTTP_MSG) return;
    auto* hm = (struct mg_http_message*) ev_data;

    if (mg_http_match_uri(hm, "/receber")) {
        std::string nome;
        struct mg_http_part part;
        int ofs = 0;

        // Itera partes multipart
        while ((ofs = mg_http_next_multipart(hm->body, ofs, &part)) > 0) {
            std::string key(part.name.ptr, part.name.len);
            if (key == "nome") {
                nome.assign(part.body.ptr, part.body.len);
                for (auto& ch : nome) if (ch == ' ') ch = '_';
            } else if (key == "fotos" && part.filename.len > 0) {
                std::string filename(part.filename.ptr, part.filename.len);
                salvar_e_processar(nome, filename, part.body.ptr, part.body.len);
            }
        }

        if (nome.empty()) {
            mg_http_reply(c, 400,
                          "Content-Type: text/plain\r\n",
                          "[ERRO] Nome do funcionário ausente\n");
            return;
        }

        // Sincroniza modelos antes do treinamento
std::cout << "[SYNC] Verificando se modelos estão atualizados no Drive e no Git...\n";
int check_drive = system(SCRIPT_CHECK.c_str());
int check_git   = system(SCRIPT_CHECK_GIT.c_str());

if (check_drive != 0 || check_git != 0) {
    std::cerr << "[ERRO] Falha ao verificar modelos. Cancelando cadastro.\n";
    mg_http_reply(c, 500,
                  "Content-Type: text/plain\r\n",
                  "[ERRO] Falha ao verificar modelos (Drive ou Git)\n");
    return;
}

        // Treina e sobe novos modelos
std::cout << "[INFO] Chamando treinamento e upload...\n";
int code_train  = system(SCRIPT_TRAIN.c_str());
int code_drive  = system(SCRIPT_UPLOAD.c_str());
int code_git    = system(SCRIPT_UPLOAD_GIT.c_str());

std::cout << "[OK] Finalizado com códigos: train=" << code_train
          << ", upload_drive=" << code_drive
          << ", upload_git=" << code_git << "\n";

mg_http_reply(c, 200,
              "Content-Type: text/plain\r\n",
              "[OK] Fotos de %s salvas, treinadas e sincronizadas (Drive + Git)\n",
              nome.c_str());

    } else {
        mg_http_reply(c, 404, "", "Not Found\n");
    }
}

int main() {
    // Inicializa gerenciador Mongoose
    mg_mgr mgr;
    mg_mgr_init(&mgr);

    // Carrega cascade Haar
    if (!face_cascade.load(CASCADE_PATH)) {
        std::cerr << "[ERROR] Falha ao carregar cascade: " << CASCADE_PATH << '\n';
        return 1;
    }

    std::cout << "[INFO] Servidor HTTP rodando em http://0.0.0.0:5000/receber\n";
    mg_http_listen(&mgr, "http://0.0.0.0:5000/receber", handle, NULL);

    // Loop principal
    while (true) mg_mgr_poll(&mgr, 1000);

    mg_mgr_free(&mgr);
    return 0;
}
