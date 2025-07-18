#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <fstream>

// Executa um comando e retorna seu stdout
std::string exec_and_capture(const std::string &cmd) {
    std::array<char,128> buf;
    std::string out;
    std::unique_ptr<FILE,decltype(&pclose)> pipe(popen(cmd.c_str(),"r"),pclose);
    if (!pipe) throw std::runtime_error("popen falhou");
    while (fgets(buf.data(), buf.size(), pipe.get())) out += buf.data();
    return out;
}
std::string trim(const std::string &s) {
    auto b = s.find_first_not_of("\r\n ");
    auto e = s.find_last_not_of("\r\n ");
    return b==std::string::npos ? "" : s.substr(b,e-b+1);
}

int main() {
    const std::string repo          = "/home/pi/ReconhecimentoFacial2/repositorio/Reconhecimento-Facial";
    const std::string local_ts_file = repo + "/models/timestamp.txt";

    try {
        std::cout << "[SYNC] Buscando informações do GitLab...\n";
        exec_and_capture("cd " + repo + " && git fetch");

        // Pega só a primeira linha do timestamp remoto
        std::string remote_ts = exec_and_capture(
            "cd " + repo +
            " && git show origin/master:models/timestamp.txt | head -n1"
        );
        remote_ts = trim(remote_ts);
        std::cout << "[DEBUG] Timestamp remoto: " << remote_ts << "\n";

        // Lê o timestamp local (primeira linha)
        std::ifstream fin(local_ts_file);
        std::string local_ts;
        if (fin.is_open()) {
            std::getline(fin, local_ts);
            fin.close();
            local_ts = trim(local_ts);
        } else {
            std::cout << "[INFO] timestamp.txt local não encontrado.\n";
        }
        std::cout << "[DEBUG] Timestamp local:  " << local_ts << "\n";

        if (remote_ts == local_ts) {
            std::cout << "[OK] Modelos já estão atualizados.\n";
            return 0;
        }

        std::cout << "[SYNC] Timestamp diferente → sincronizando...\n";

        int ret = system(
            ("cd " + repo + 
             " && git reset --hard origin/master").c_str()
        );
        if (ret == 0) {
            std::cout << "[OK] Repositório alinhado a origin/master.\n";
        } else {
            std::cerr << "[ERRO] Falha no git reset --hard.\n";
        }
        return ret;

    } catch (const std::exception &e) {
        std::cerr << "[ERRO] Exceção: " << e.what() << "\n";
        return 1;
    }
}
