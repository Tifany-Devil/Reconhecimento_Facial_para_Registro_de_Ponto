import csv
import os
from google.oauth2.service_account import Credentials
from googleapiclient.discovery import build

# Nome da planilha e aba
SPREADSHEET_ID = '1_z8DdDDd5Z5UhVL45jmaFzHC5P9m7T7ZWXC1Nhvgl4Q'
RANGE_NAME = 'Pontos!A1'

# Caminho do arquivo de credenciais
SERVICE_ACCOUNT_FILE = '/home/pi/ReconhecimentoFacial2/python/credentials.json'
SCOPES = ['https://www.googleapis.com/auth/spreadsheets']

def ler_csv(path='/home/pi/ReconhecimentoFacial2/python/registro.csv'):
    with open(path, 'r') as f:
        return list(csv.reader(f))

def atualizar_planilha(valores):
    creds = Credentials.from_service_account_file(
        SERVICE_ACCOUNT_FILE, scopes=SCOPES)

    service = build('sheets', 'v4', credentials=creds)
    sheet = service.spreadsheets()

    # Limpa conteúdo antigo
    sheet.values().clear(spreadsheetId=SPREADSHEET_ID, range=RANGE_NAME).execute()

    # Envia dados atualizados
    body = {'values': valores}
    sheet.values().update(
        spreadsheetId=SPREADSHEET_ID,
        range=RANGE_NAME,
        valueInputOption='RAW',
        body=body
    ).execute()
    print("[OK] Sincronização concluída com sucesso.")

if __name__ == '__main__':
    if not os.path.exists('/home/pi/ReconhecimentoFacial2/python/registro.csv'):
        print("[ERRO] Arquivo registro.csv não encontrado.")
    else:
        dados = ler_csv()
        atualizar_planilha(dados)
