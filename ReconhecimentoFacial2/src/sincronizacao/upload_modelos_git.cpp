#include <cstdlib>
#include <iostream>
#include <ctime>

std::string timestamp() {
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}

int main() {
    const std::string repo = "/home/pi/ReconhecimentoFacial2/repositorio/Reconhecimento-Facial";
    const std::string modelos = "models/lbph_model.yml models/labels.txt models/timestamp.txt";
    std::string mensagem = "\"Atualiza modelos - " + timestamp() + "\"";

    std::cout << "[SYNC] Subindo modelos ao GitLab...\n";
    std::string cmd =
        "cd " + repo + " && "
        "git pull && "
        "git add " + modelos + " && "
        "git commit -m " + mensagem + " && "
        "git push";

    int ret = system(cmd.c_str());
    if (ret == 0) {
        std::cout << "[OK] Modelos enviados com sucesso.\n";
    } else {
        std::cerr << "[ERRO] Falha ao subir modelos para o GitLab.\n";
    }
    return ret;
}
