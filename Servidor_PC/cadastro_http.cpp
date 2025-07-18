#define MG_ENABLE_HTTP 1

#include "mongoose.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <curl/curl.h>

namespace fs = std::filesystem;

// Prefixo para arquivos temporários
const std::string TEMP_IMG = "upload";
// URL da sua RPi
//const std::string RPI_URL  = "http://172.20.10.2:5000/receber";
const std::string RPI_URL  = "http://192.168.0.26:5000/receber";

// Página HTML + JS que captura até 10 fotos da webcam do computador,
// ajusta o canvas à resolução real do vídeo e envia tudo num POST
const std::string html_form = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Cadastro de Funcionário</title>
</head>
<body>
  <h2>Cadastro de Funcionário</h2>
  <input type="text" id="nome" placeholder="Nome" required><br><br>
  <video id="video" width="320" height="240" autoplay></video><br>
  <button id="capture">Capturar Foto</button>
  <button id="submit">Enviar Tudo</button>
  <div id="previews" style="margin-top:10px;"></div>

  <canvas id="canvas" style="display:none;"></canvas>

  <script>
    const video   = document.getElementById('video');
    const canvas  = document.getElementById('canvas');
    const ctx     = canvas.getContext('2d');
    const previews= document.getElementById('previews');
    const captures= [];
    const nomeInput = document.getElementById('nome');

    // Peça resolução ideal de 640×480
    const constraints = {
      video: { width: { ideal: 640 }, height: { ideal: 480 } }
    };

    // Abre a webcam e ajusta o canvas à resolução real
    navigator.mediaDevices.getUserMedia(constraints)
      .then(stream => {
        video.srcObject = stream;
        video.addEventListener('loadedmetadata', () => {
          canvas.width  = video.videoWidth;
          canvas.height = video.videoHeight;
        });
      })
      .catch(err => alert('Erro ao abrir câmera: ' + err));

    document.getElementById('capture').onclick = () => {
      if (captures.length >= 10) return alert('Máximo de 10 fotos');
      // Desenha frame inteiro no tamanho do canvas
      ctx.drawImage(video, 0, 0, canvas.width, canvas.height);
      canvas.toBlob(blob => {
        captures.push(blob);
        const img = document.createElement('img');
        img.src = URL.createObjectURL(blob);
        img.style.width = '120px';
        img.style.margin = '4px';
        previews.appendChild(img);
      }, 'image/jpeg');
    };

    document.getElementById('submit').onclick = () => {
      const nome = nomeInput.value.trim();
      if (!nome)            return alert('Informe o nome');
      if (captures.length===0) return alert('Capture ao menos uma foto');

      const form = new FormData();
      form.append('nome', nome);
      captures.forEach((blob, i) =>
        form.append('fotos', blob, `foto${i}.jpg`)
      );

      fetch('/', { method: 'POST', body: form })
        .then(res => res.text())
        .then(txt => alert(txt))
        .catch(err => alert('Erro no envio: ' + err));
    };
  </script>
</body>
</html>
)";

// Envia nome + várias imagens para a RPi via libcurl
void enviar_para_rpi(const std::string& nome, const std::vector<std::string>& imagens) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    std::cerr << "[ERRO] libcurl não inicializada.\n";
    return;
  }

  curl_mime* form = curl_mime_init(curl);

  // Campo 'nome'
  curl_mimepart* part = curl_mime_addpart(form);
  curl_mime_name(part, "nome");
  curl_mime_data(part, nome.c_str(), CURL_ZERO_TERMINATED);

  // Campo 'fotos' para cada arquivo
  for (const auto& img : imagens) {
    part = curl_mime_addpart(form);
    curl_mime_name(part, "fotos");
    curl_mime_filedata(part, img.c_str());
  }

  curl_easy_setopt(curl, CURLOPT_URL, RPI_URL.c_str());
  curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

  CURLcode res = curl_easy_perform(curl);
  if (res == CURLE_OK) {
    std::cout << "[OK] Enviado para RPi: " << nome
              << " (" << imagens.size() << " fotos)\n";
  } else {
    std::cerr << "[ERRO] libcurl: " << curl_easy_strerror(res) << "\n";
  }

  curl_mime_free(form);
  curl_easy_cleanup(curl);
}

// Handler HTTP do Mongoose
static void handle(struct mg_connection* c, int ev, void* ev_data) {
  if (ev != MG_EV_HTTP_MSG) return;
  auto* hm = (struct mg_http_message*) ev_data;

  std::string uri    (hm->uri.buf,    hm->uri.len);
  std::string method (hm->method.buf, hm->method.len);

  if (uri == "/" && method == "GET") {
    mg_http_reply(c, 200,
                  "Content-Type: text/html\r\n",
                  "%s", html_form.c_str());
  }
  else if (uri == "/" && method == "POST") {
    std::vector<std::string> imagens;
    std::string nome;
    int ofs = 0;
    struct mg_http_part part;

    // Itera cada parte multipart
    while ((ofs = mg_http_next_multipart(hm->body, ofs, &part)) > 0) {
      std::string var(part.name.buf, part.name.len);
      if (var == "nome") {
        nome.assign(part.body.buf, part.body.len);
        // substitui espaços e cria pasta
        for (auto& c : nome) if (c == ' ') c = '_';
        fs::create_directory(nome);
      }
      else if (var == "fotos") {
        // salva dentro de ./<nome>/
        std::string fn = nome + "/" + TEMP_IMG + "_" +
                         std::to_string(imagens.size()) + ".jpg";
        std::ofstream out(fn, std::ios::binary);
        out.write(part.body.buf, part.body.len);
        out.close();
        imagens.push_back(fn);
      }
    }

    if (nome.empty() || imagens.empty()) {
      mg_http_reply(c, 400,
                    "Content-Type: text/plain\r\n",
                    "Erro: faltando nome ou fotos\n");
    } else {
      enviar_para_rpi(nome, imagens);
      mg_http_reply(c, 200,
                    "Content-Type: text/plain\r\n",
                    "[OK] %d fotos enviadas para RPi\n",
                    (int)imagens.size());
    }
  }
  else {
    mg_http_reply(c, 404, "", "Not Found");
  }
}

int main() {
  mg_mgr mgr;
  mg_mgr_init(&mgr);
  std::cout << "[INFO] Servidor rodando em http://localhost:8080\n";
  mg_http_listen(&mgr, "http://0.0.0.0:8080", handle, nullptr);
  for (;;) mg_mgr_poll(&mgr, 1000);
  mg_mgr_free(&mgr);
  return 0;
}
