# 📸 Sistema de Registro de Ponto com Reconhecimento Facial Distribuído

Este projeto implementa um sistema autônomo e distribuído de **registro de ponto com reconhecimento facial**, embarcado em **Raspberry Pi 4**. É especialmente voltado para ambientes como **canteiros de obras**, onde há mobilidade entre unidades.

O sistema realiza o **cadastro facial via navegador em um servidor C++ (no PC)**, que envia imagens para a Raspberry Pi. Esta, por sua vez, processa, treina o modelo localmente e sincroniza os dados com um repositório **GitLab**. O reconhecimento embarcado ocorre em tempo real, com exibição no display OLED e confirmação por botão físico.

---

## 📁 Estrutura de diretórios

```

/home/pi/ReconhecimentoFacial2/
├── build/                  # Binários compilados
├── data/                   # Imagens por funcionário
├── logs/                   # Logs do sistema
├── python/                 # Scripts de treino
│   └── credentials.json    # Chave da conta de serviço Google
├── repositorio/            # Clonado do GitLab com modelos e CSV
├── src/
│   ├── Server/             # Servidor de recepção na RPi
│   ├── sincronizacao/      # check, upload e sync via Git
│   ├── attend\_client\_ssd1306.cpp  # Reconhecimento local
│   └── glcdfont.h
└── registro.csv            # Registro de ponto por funcionário

````

---

## ⚙️ Componentes

- **Raspberry Pi 4 Model B**
- **Pi Camera v2.1**
- **Display OLED SSD1306 (I²C)**
- **Botão push (GPIO)**
- **Servidor web (no PC com Mongoose + C++)**
- **Sincronização via GitLab (modelos + CSV)**

---

## 🧠 Funcionamento Geral

### 🔸 1. **Cadastro de Funcionário (PC)**

- O operador acessa a **página web** hospedada no PC.
- Captura as fotos pelo navegador e insere o nome.
- As fotos são enviadas para a Raspberry Pi via `curl`.

### 🔸 2. **Recepção e Treinamento (Raspberry Pi)**

- O servidor `cadastro_server.cpp` roda constantemente.
- Ao receber imagens:
  - Detecta a face (HaarCascade), converte e redimensiona.
  - Salva em `data/<nome>/`.
  - Executa `check_and_pull_git`, depois `train.py`.
  - Gera `lbph_model.yml` e `labels.txt`.
  - Executa `upload_modelos_git` (C++) → envia para GitLab.

### 🔸 3. **Reconhecimento Facial e Registro (RPi)**

- O `attend_client_ssd1306` executa:
  - `git pull` para baixar o modelo atualizado.
  - Ativa a câmera, detecta rosto, identifica via LBPH.
  - Exibe no OLED: “Funcionário: [nome] Deseja bater ponto?”
  - Se o botão for pressionado:
    - Atualiza `registro.csv`.
    - Executa `sync_registro_git` para enviar ao GitLab.
    - Aplica bloqueio de 30 min entre registros.

---

## 🧱 Como compilar

### 🖥️ No servidor da Raspberry Pi (cadastro)

```bash
cd src/Server
g++ cadastro_server.cpp mongoose.c -o cadastro_server -lpthread `pkg-config --cflags --libs opencv4`
````

### 📷 Cliente de reconhecimento facial (com display SSD1306)

```bash
cd src
g++ attend_client_ssd1306.cpp -o attend_ssd1306 `pkg-config --cflags --libs opencv4` -lwiringPi
```

---

## 🔧 Requisitos e dependências

### Instale no Raspberry Pi:

```bash
sudo apt update
sudo apt install g++ cmake python3-pip libopencv-dev wiringpi git -y
pip3 install opencv-contrib-python
```

---

## 🔐 Configuração do GitLab

* Clone o repositório remoto em:
  `/home/pi/ReconhecimentoFacial2/repositorio/`

* Esse repositório Git deve conter:

  * `models/lbph_model.yml`, `labels.txt`, `timestamp.txt`
  * `registro.csv`

> O `cadastro_server.cpp` e `attend_client_ssd1306.cpp` fazem commit e push automaticamente via terminal.

---

## 🛡️ Configuração do Google Sheets (opcional)

* Coloque o arquivo `credentials.json` na pasta:

  ```
  ReconhecimentoFacial2/python/credentials.json
  ```

* Esse arquivo é usado pelo script Python para sincronizar com uma planilha.

> ⚠️ Este arquivo **NÃO DEVE** ser enviado para o repositório. Adicione ao `.gitignore`.

---

## 🧪 Testes e Validação

* Testado com sucesso em rede local (servidor no PC e RPi conectada).
* Reconhecimento facial responsivo.
* Atualização automática dos modelos.
* Confirmação de ponto via botão físico.
* Sincronização confiável via GitLab.

---

## 📌 Comandos úteis

### Executar servidor de recepção (na RPi):

```bash
./cadastro_server
```

### Executar cliente de reconhecimento:

```bash
./attend_ssd1306
```

---

## 👩‍💻 Autora

**Tífany Severo**
Projeto Final – *Sistemas Operacionais Embarcados*
Universidade de Brasília – 2025/1

---

