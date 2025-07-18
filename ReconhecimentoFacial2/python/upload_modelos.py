import os
import io
import datetime
from google.oauth2 import service_account
from googleapiclient.discovery import build
from googleapiclient.http import MediaFileUpload

# Caminhos
SERVICE_ACCOUNT_FILE = '/home/pi/ReconhecimentoFacial2/python/credentials.json'
SCOPES = ['https://www.googleapis.com/auth/drive']
PASTA_MODELOS = 'models'
ARQUIVOS_MODELOS = ['lbph_model.yml', 'labels.txt']
CAMINHO_LOCAL = '/home/pi/ReconhecimentoFacial2/repositorio/Reconhecimento-Facial/models/'

def autenticar():
    creds = service_account.Credentials.from_service_account_file(
        SERVICE_ACCOUNT_FILE, scopes=SCOPES)
    return build('drive', 'v3', credentials=creds)

def encontrar_pasta(drive_service, nome_pasta):
    resultado = drive_service.files().list(
        q=f"name='{nome_pasta}' and mimeType='application/vnd.google-apps.folder'",
        spaces='drive',
        fields='files(id, name)',
    ).execute()
    arquivos = resultado.get('files', [])
    if not arquivos:
        print(f"[ERRO] Pasta '{nome_pasta}' não encontrada no Drive.")
        return None
    return arquivos[0]['id']

def encontrar_id_arquivo(drive_service, nome_arquivo, pasta_id):
    resultado = drive_service.files().list(
        q=f"name='{nome_arquivo}' and '{pasta_id}' in parents",
        spaces='drive',
        fields='files(id, name)',
    ).execute()
    arquivos = resultado.get('files', [])
    return arquivos[0]['id'] if arquivos else None

def upload_ou_atualizar(drive_service, arquivo, pasta_id):
    caminho_arquivo = os.path.join(CAMINHO_LOCAL, arquivo)
    file_metadata = {
        'name': arquivo,
        'parents': [pasta_id]
    }
    media = MediaFileUpload(caminho_arquivo, resumable=True)

    existente_id = encontrar_id_arquivo(drive_service, arquivo, pasta_id)
    if existente_id:
        atualizado = drive_service.files().update(
            fileId=existente_id,
            media_body=media,
        ).execute()
        print(f"[OK] Atualizado: {arquivo}")
    else:
        criado = drive_service.files().create(
            body=file_metadata,
            media_body=media,
            fields='id'
        ).execute()
        print(f"[OK] Enviado: {arquivo}")

def gerar_timestamp_txt():
    agora = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    caminho = os.path.join(CAMINHO_LOCAL, 'timestamp.txt')
    with open(caminho, 'w') as f:
        f.write(agora)
    return 'timestamp.txt'

def main():
    drive_service = autenticar()
    pasta_id = encontrar_pasta(drive_service, PASTA_MODELOS)
    if not pasta_id:
        return

    # Envia arquivos do modelo
    for arquivo in ARQUIVOS_MODELOS:
        upload_ou_atualizar(drive_service, arquivo, pasta_id)

    # Gera e envia novo timestamp
    novo_timestamp = gerar_timestamp_txt()
    upload_ou_atualizar(drive_service, novo_timestamp, pasta_id)

    print("[OK] Upload completo.")

if __name__ == '__main__':
    main()
