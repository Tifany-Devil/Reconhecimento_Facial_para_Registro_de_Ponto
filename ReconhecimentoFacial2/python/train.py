# python/train.py

import cv2
import os
import sys
import numpy as np

def lista_subpastas(path):
    """
    Retorna uma lista com o nome de cada subpasta dentro de 'path'
    (ou seja, cada funcionário).
    """
    return [d for d in os.listdir(path) if os.path.isdir(os.path.join(path, d))]

def lista_arquivos_imagem(path):
    """
    Retorna todos os arquivos *.jpg e *.png dentro de 'path'.
    """
    imgs = []
    for f in os.listdir(path):
        nome_lower = f.lower()
        if nome_lower.endswith(".jpg") or nome_lower.endswith(".png"):
            imgs.append(os.path.join(path, f))
    return imgs

if __name__ == "__main__":
    # 1) Defina os caminhos
    data_dir   = os.path.expanduser("~/ReconhecimentoFacial2/data")
    models_dir = os.path.expanduser("~/ReconhecimentoFacial2/repositorio/Reconhecimento-Facial/models")

    # 2) Cria a pasta "models/" caso não exista
    os.makedirs(models_dir, exist_ok=True)

    # 3) Verifica se há subpastas em data/ (cada subpasta = funcionário)
    nomes_funcionarios = lista_subpastas(data_dir)
    if len(nomes_funcionarios) == 0:
        print("[ERROR] Não há subpastas em data/. Execute primeiro o enroll.py.")
        sys.exit(1)

    imagens = []
    rotulos = []
    label_to_name = {}

    # 4) Para cada funcionário (subpasta), associe um label inteiro
    for idx, nome in enumerate(nomes_funcionarios):
        label_to_name[idx] = nome
        pasta = os.path.join(data_dir, nome)
        arquivos = lista_arquivos_imagem(pasta)
        if len(arquivos) == 0:
            print(f"[WARN] Pasta '{pasta}' está vazia. Pule este funcionário.")
            continue

        # 5) Para cada imagem nessa subpasta, carregue em escala de cinza, redimensione e armazene
        for img_path in arquivos:
            img = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)
            if img is None:
                print(f"[WARN] Falha ao ler imagem: {img_path}")
                continue
            # Garante tamanho 200×200
            if img.shape != (200, 200):
                img = cv2.resize(img, (200, 200))
            imagens.append(img)
            rotulos.append(idx)

    if len(imagens) == 0:
        print("[ERROR] Nenhuma imagem válida foi encontrada em data/.")
        sys.exit(1)

    print(f"[INFO] {len(label_to_name)} funcionário(s) encontrado(s) em data/:")
    for lbl, nome in label_to_name.items():
        print(f"   {lbl} → {nome}")
    print(f"[INFO] Total de imagens para treino: {len(imagens)}")

    # 6) Cria e treina o reconhecedor LBPH
    model = cv2.face.LBPHFaceRecognizer_create()
    model.train(imagens, np.array(rotulos))

    # 7) Salva o modelo
    model_path = os.path.join(models_dir, "lbph_model.yml")
    model.write(model_path)
    print(f"[OK] Modelo LBPH salvo em: {model_path}")

    # 8) Salva labels.txt (formato: label;nome)
    labels_path = os.path.join(models_dir, "labels.txt")
    with open(labels_path, "w") as f:
        for lbl, nome in label_to_name.items():
            f.write(f"{lbl};{nome}\n")
    print(f"[OK] Labels salvos em: {labels_path}")
