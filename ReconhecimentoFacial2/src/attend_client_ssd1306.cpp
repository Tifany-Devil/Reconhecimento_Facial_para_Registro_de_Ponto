#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <ctime>
#include <sstream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <cstring>

// ===== DISPLAY SSD1306 =====
#define SSD1306_ADDR 0x3C
int fd;
uint8_t buffer[8][128];

void cmd(uint8_t c) { wiringPiI2CWriteReg8(fd, 0x00, c); }
void data(uint8_t d) { wiringPiI2CWriteReg8(fd, 0x40, d); }

void initDisplay() {
  cmd(0xAE); cmd(0x20); cmd(0x00); cmd(0xB0);
  cmd(0xC8); cmd(0x00); cmd(0x10); cmd(0x40);
  cmd(0x81); cmd(0x7F); cmd(0xA1); cmd(0xA6);
  cmd(0xA8); cmd(0x3F); cmd(0xA4); cmd(0xD3); cmd(0x00);
  cmd(0xD5); cmd(0x80); cmd(0xD9); cmd(0xF1);
  cmd(0xDA); cmd(0x12); cmd(0xDB); cmd(0x40);
  cmd(0x8D); cmd(0x14); cmd(0xAF);
}

void clearBuffer() { memset(buffer, 0, sizeof(buffer)); }

void updateDisplay() {
  for (int page = 0; page < 8; page++) {
    cmd(0xB0 + page); cmd(0x00); cmd(0x10);
    for (int col = 0; col < 128; col++) data(buffer[page][col]);
  }
}

void drawPixel(int x, int y) {
  if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
  buffer[y / 8][x] |= (1 << (y % 8));
}

#include "glcdfont.h"
void drawChar(int x, int y, char c) {
  if (c < 32 || c > 127) c = '?';
  for (int i = 0; i < 5; i++) {
    uint8_t line = glcdfont[(c - 32) * 5 + i];
    for (int j = 0; j < 8; j++)
      if (line & (1 << j)) drawPixel(x + i, y + j);
  }
}

void drawString(int x, int y, const char *s) {
  while (*s) {
    drawChar(x, y, *s++);
    x += 6;
    if (x + 6 > 128) { x = 0; y += 8; }
  }
}

void showMessage(const std::string &l1, const std::string &l2, const std::string &l3) {
  clearBuffer();
  drawString(0, 0, l1.c_str());
  drawString(0, 8, l2.c_str());
  drawString(0, 16, l3.c_str());
  updateDisplay();
}

using namespace std;
using namespace cv;
using namespace cv::face;

const double LIMIAR_CONFIANCA = 68.0;
const int CONFIRMACAO_N_FRAMES = 3;
const string ARQUIVO_CSV_py = "/home/pi/ReconhecimentoFacial2/python/registro.csv";
const string ARQUIVO_CSV_git = "/home/pi/ReconhecimentoFacial2/repositorio/Reconhecimento-Facial/registro.csv";
const int BUTTON_PIN = 17;
string nome_temp = "", nome_final = "";
int confirm = 0, cooldown = 0;
  
map<int, string> loadLabels(const string &filename) {
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

void salvar_csv(const string &caminho, const vector<vector<string>> &tabela) {
  ofstream arq_out(caminho, ios::trunc);
  for (const auto &row : tabela) {
    for (size_t i = 0; i < row.size(); i++) {
      arq_out << row[i];
      if (i != row.size() - 1) arq_out << ",";
    }
    arq_out << "\n";
  }
  arq_out.close();
}

bool registrar_ponto_csv(const string &nome) {
  vector<vector<string>> tabela;
  ifstream arq_in(ARQUIVO_CSV_py);
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
  bool encontrado = false, pode_registrar = true;

  for (auto &row : tabela) {
    if (!row.empty() && row[0] == nome) {
      encontrado = true;
      if (row.size() > 1) {
        struct tm tm_ultimo = {};
        strptime(row.back().c_str(), "%Y-%m-%d %H:%M:%S", &tm_ultimo);
        time_t t_ultimo = mktime(&tm_ultimo);
        double diff = difftime(time(nullptr), t_ultimo) / 60.0;

        if (diff < 30.0) {
          cout << "[NEGADO] Último ponto de " << nome << " foi há "
               << int(diff) << " min. Aguarde 30.\n";
          showMessage("Aguarde", to_string(int(30 - diff)) + " min", "para novo ponto");
          delay(3000);
           cooldown = 30;
          //clearBuffer();
         // delay(3000);
        //  updateDisplay();
         //delay(1000);
          nome_temp.clear();
          confirm = 0;
          cooldown = 30;
          return false;
        }
      }

      if (pode_registrar) {
        row.push_back(agora);
        cout << "[REGISTRADO] " << nome << " às " << agora << endl;
      }
      break;
    }
  }

  if (!encontrado && pode_registrar) {
    tabela.push_back({nome, agora});
    cout << "[REGISTRADO] Primeiro ponto: " << nome << " às " << agora << endl;
  }

  if (pode_registrar) {
    salvar_csv(ARQUIVO_CSV_py, tabela);
    salvar_csv(ARQUIVO_CSV_git, tabela);
    return true;
  }
  return false;
}

int main() {
  wiringPiSetupGpio();
  pinMode(BUTTON_PIN, INPUT);
  pullUpDnControl(BUTTON_PIN, PUD_UP);

  fd = wiringPiI2CSetup(SSD1306_ADDR);
  if (fd < 0) {
    cerr << "[ERRO] Falha no I2C.\n";
    return -1;
  }
  initDisplay();
  clearBuffer();
  updateDisplay();

  string model_path  = "/home/pi/ReconhecimentoFacial2/repositorio/Reconhecimento-Facial/models/lbph_model.yml";
  string labels_path = "/home/pi/ReconhecimentoFacial2/repositorio/Reconhecimento-Facial/models/labels.txt";
  string cascade_f   = "/home/pi/ReconhecimentoFacial2/repositorio/Reconhecimento-Facial/models/haarcascade_frontalface_default.xml";

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

  VideoCapture cap(
    "libcamerasrc ! video/x-raw,width=640,height=480,format=BGRx,framerate=30/1 "
    "! videoconvert ! video/x-raw,format=BGR ! appsink",
    CAP_GSTREAMER
  );
  if (!cap.isOpened()) {
    cerr << "Não foi possível abrir a câmera.\n";
    return -1;
  }

  namedWindow("Camera", WINDOW_AUTOSIZE);  // <=== adiciona janela da câmera



  while (true) {
    Mat frame;
    cap >> frame;
    if (frame.empty()) continue;

    imshow("Camera", frame);  // <=== mostra frame em janela

    Mat gray;
    cvtColor(frame, gray, COLOR_BGR2GRAY);

    vector<Rect> faces;
    face_cascade.detectMultiScale(gray, faces, 1.1, 5, 0, Size(100, 100));

    if (!faces.empty() && cooldown == 0) {
      Rect r = faces[0];
      Mat roi = gray(r).clone();
      resize(roi, roi, Size(200, 200));
      equalizeHist(roi, roi);
      
      
      int label;
      double conf;
      model->predict(roi, label, conf);
      string nome = labels.count(label) ? labels[label] : "(desconhecido)";
      cout << "[DEBUG] " << nome << " (" << conf << ")\n";

      if (conf < LIMIAR_CONFIANCA && labels.count(label)) {
        if (nome == nome_temp) confirm++;
        else { nome_temp = nome; confirm = 1; }

        if (confirm >= CONFIRMACAO_N_FRAMES) {
          showMessage("Ola, " + nome, "Pressione o botao", "p/ bater ponto");
          while (digitalRead(BUTTON_PIN) == HIGH) delay(10);
          if (registrar_ponto_csv(nome)) {
            showMessage("Ponto", "registrado", " ");
            delay(3000);
            cooldown = 30;
          //  clearBuffer();
          //  delay(3000);
           // updateDisplay();
            system("python3 /home/pi/ReconhecimentoFacial2/python/sync_csv_to_google.py");
            system("/home/pi/ReconhecimentoFacial2/src/sincronizacao/sync_registro_git");
          }
        }
      } else {
        nome_temp = "";
        confirm = 0;
      }
    }

    if (cooldown > 0) cooldown--;
    if ((char)waitKey(1) == 'q') break;
  }
  destroyAllWindows();  // <=== fecha janela ao sair
  return 0;
}
