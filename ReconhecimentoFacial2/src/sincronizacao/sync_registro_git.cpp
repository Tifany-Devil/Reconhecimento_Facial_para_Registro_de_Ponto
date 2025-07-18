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
    const std::string repo = "~/ReconhecimentoFacial2/repositorio/Reconhecimento-Facial";
    const std::string registro = "registro.csv";
    std::string mensagem = "\"Atualiza registro - " + timestamp() + "\"";

    std::cout << "[SYNC] Enviando registro.csv ao GitLab...\n";
    std::string cmd =
        "cd " + repo + " && "
        "git pull && "
        "git add " + registro + " && "
        "git commit -m " + mensagem + " && "
        "git push";

    int ret = system(cmd.c_str());
    if (ret == 0) {
        std::cout << "[OK] registro.csv sincronizado com sucesso.\n";
    } else {
        std::cerr << "[ERRO] Falha ao sincronizar registro.csv.\n";
    }
    return ret;
}
