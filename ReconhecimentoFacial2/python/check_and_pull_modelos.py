import os
import io
import datetime
import pytz
import csv
from google.oauth2 import service_account
from googleapiclient.discovery import build
from googleapiclient.http import MediaIoBaseDownload

# Caminho absoluto do arquivo de credenciais
SERVICE_ACCOUNT_FILE = '/home/pi/ReconhecimentoFacial2/python/credentials.json'
SCOPES = ['https://www.googleapis.com/auth/drive']
PASTA_MODELOS = 'models'  # Nome da pasta no Drive

# Caminho local dos arquivos
DESTINO = '/home/pi/ReconhecimentoFacial2/repositorio/Reconhecimento-Facial/models/'
ARQUIVOS_MODELOS = ['lbph_model.yml', 'labels.txt', 'timestamp.txt']

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

def obter_id_arquivo(drive_service, nome_arquivo, pasta_id):
    resultado = drive_service.files().list(
        q=f"name='{nome_arquivo}' and '{pasta_id}' in parents",
        spaces='drive',
        fields='files(id, name)',
    ).execute()
    arquivos = resultado.get('files', [])
    if not arquivos:
        print(f"[ERRO] Arquivo '{nome_arquivo}' não encontrado na pasta.")
        return None
    return arquivos[0]['id']

def baixar_arquivo(drive_service, file_id, caminho_saida):
    request = drive_service.files().get_media(fileId=file_id)
    with io.FileIO(caminho_saida, 'wb') as f:
        downloader = MediaIoBaseDownload(f, request)
        done = False
        while not done:
            status, done = downloader.next_chunk()
        print(f"[OK] Baixado: {caminho_saida}")

def ler_timestamp_local():
    try:
        with open(os.path.join(DESTINO, 'timestamp.txt'), 'r') as f:
            return f.read().strip()
    except FileNotFoundError:
        return None

def main():
    drive_service = autenticar()
    pasta_id = encontrar_pasta(drive_service, PASTA_MODELOS)
    if not pasta_id:
        return

    id_timestamp = obter_id_arquivo(drive_service, 'timestamp.txt', pasta_id)
    if not id_timestamp:
        return

    # Baixa o timestamp da nuvem temporariamente
    timestamp_nuvem_path = '/tmp/timestamp_remoto.txt'
    baixar_arquivo(drive_service, id_timestamp, timestamp_nuvem_path)

    # Compara timestamps
    with open(timestamp_nuvem_path, 'r') as f:
        timestamp_nuvem = f.read().strip()

    timestamp_local = ler_timestamp_local()

    if timestamp_local == timestamp_nuvem:
        print("[INFO] Modelos já estão atualizados.")
        return

    print("[SYNC] Modelos desatualizados. Baixando nova versão...")

    # Baixar todos os arquivos
    for nome in ARQUIVOS_MODELOS:
        file_id = obter_id_arquivo(drive_service, nome, pasta_id)
        if file_id:
            caminho_saida = os.path.join(DESTINO, nome)
            baixar_arquivo(drive_service, file_id, caminho_saida)

    print("[OK] Sincronização concluída.")

if __name__ == '__main__':
    main()
